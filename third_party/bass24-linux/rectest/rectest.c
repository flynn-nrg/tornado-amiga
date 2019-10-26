/*
	BASS recording example
	Copyright (c) 2002-2009 Un4seen Developments Ltd.
*/

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include <malloc.h>
#include <regex.h>
#include "bass.h"

// path to glade file
#ifndef GLADE_PATH
#define GLADE_PATH ""
#endif

GladeXML *glade;
GtkWidget *win=0;
GtkWidget *filesel; // file selector

#define BUFSTEP 200000	// memory allocation unit

int input;				// current input source
char *recbuf=NULL;		// recording buffer
DWORD reclen;			// recording length

HRECORD rchan=0;		// recording channel
HSTREAM chan=0;			// playback channel

// display error messages
void Error(const char *es)
{
	GtkWidget *dialog=gtk_message_dialog_new(GTK_WINDOW(win),GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

#define GetWidget(id) glade_xml_get_widget(glade,id)

void WindowDestroy(GtkObject *obj, gpointer data)
{
	gtk_main_quit();
}

// buffer the recorded data
BOOL CALLBACK RecordingCallback(HRECORD handle, const void *buffer, DWORD length, void *user)
{
	// increase buffer size if needed
	if ((reclen%BUFSTEP)+length>=BUFSTEP) {
		recbuf=realloc(recbuf,((reclen+length)/BUFSTEP+1)*BUFSTEP);
		if (!recbuf) {
			rchan=0;
			Error("Out of memory!");
			gtk_button_set_label(GTK_BUTTON(GetWidget("record")),"Record");
			return FALSE; // stop recording
		}
	}
	// buffer the data
	memcpy(recbuf+reclen,buffer,length);
	reclen+=length;
	return TRUE; // continue recording
}

void StartRecording()
{
	WAVEFORMATEX *wf;
	if (recbuf) { // free old recording
		BASS_StreamFree(chan);
		chan=0;
		free(recbuf);
		recbuf=NULL;
		gtk_widget_set_sensitive(GetWidget("play"),FALSE);
		gtk_widget_set_sensitive(GetWidget("save"),FALSE);
		// close output device before recording incase of half-duplex device
		BASS_Free();
	}
	// allocate initial buffer and make space for WAVE header
	recbuf=malloc(BUFSTEP);
	reclen=44;
	// fill the WAVE header
	memcpy(recbuf,"RIFF\0\0\0\0WAVEfmt \20\0\0\0",20);
	memcpy(recbuf+36,"data\0\0\0\0",8);
	wf=(WAVEFORMATEX*)(recbuf+20);
	wf->wFormatTag=1;
	wf->nChannels=2;
	wf->wBitsPerSample=16;
	wf->nSamplesPerSec=44100;
	wf->nBlockAlign=wf->nChannels*wf->wBitsPerSample/8;
	wf->nAvgBytesPerSec=wf->nSamplesPerSec*wf->nBlockAlign;
	// start recording @ 44100hz 16-bit stereo
	if (!(rchan=BASS_RecordStart(44100,2,0,&RecordingCallback,0))) {
		Error("Couldn't start recording");
		free(recbuf);
		recbuf=0;
		return;
	}
	gtk_button_set_label(GTK_BUTTON(GetWidget("record")),"Stop");
}

void StopRecording()
{
	BASS_ChannelStop(rchan);
	rchan=0;
	gtk_button_set_label(GTK_BUTTON(GetWidget("record")),"Record");
	// complete the WAVE header
	*(DWORD*)(recbuf+4)=reclen-8;
	*(DWORD*)(recbuf+40)=reclen-44;
	// enable "save" button
	gtk_widget_set_sensitive(GetWidget("save"),TRUE);
	// setup output device (using default device)
	if (!BASS_Init(-1,44100,0,NULL,NULL)) {
		Error("Can't initialize output device");
		return;
	}
	// create a stream from the recording
	if (chan=BASS_StreamCreateFile(TRUE,recbuf,0,reclen,0))
		gtk_widget_set_sensitive(GetWidget("play"),TRUE); // enable "play" button
	else
		BASS_Free();
}

gboolean FileExtensionFilter(const GtkFileFilterInfo *info, gpointer data)
{
	return !regexec((regex_t*)data,info->filename,0,NULL,0);
}

// write the recorded data to disk
void WriteToDisk()
{
	int resp=gtk_dialog_run(GTK_DIALOG(filesel));
	gtk_widget_hide(filesel);
	if (resp==GTK_RESPONSE_ACCEPT) {
		char *file=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
		FILE *fp=fopen(file,"wb");
		if (fp) {
			fwrite(recbuf,reclen,1,fp);
			fclose(fp);
		} else
			Error("Can't create the file");
		g_free(file);
	}
}

void UpdateInputInfo()
{
	float level;
	int r=BASS_RecordGetInput(input,&level); // get info on the input
	if (r==-1 || level<0) { // failed
		BASS_RecordGetInput(-1,&level); // try master input instead
		if (level<0) level=1; // that failed too, just display 100%
	}
	gtk_range_set_value(GTK_RANGE(GetWidget("level")),level*100); // set the level slider
}

void RecordClicked(GtkButton *obj, gpointer data)
{
	if (!rchan)
		StartRecording();
	else
		StopRecording();
}

void PlayClicked(GtkButton *obj, gpointer data)
{
	BASS_ChannelPlay(chan,TRUE); // play the recorded data
}

void SaveClicked(GtkButton *obj, gpointer data)
{
	WriteToDisk();
}

void InputChanged(GtkComboBox *obj, gpointer data)
{
	int i;
	input=gtk_combo_box_get_active(obj); // get the selection
	// enable the selected input
	for (i=0;BASS_RecordSetInput(i,BASS_INPUT_OFF,-1);i++) ; // 1st disable all inputs, then...
	BASS_RecordSetInput(input,BASS_INPUT_ON,-1); // enable the selected
	UpdateInputInfo(); // update info
}

void LevelChanged(GtkRange *range, gpointer data)
{
	double level=gtk_range_get_value(range)/100;
	if (!BASS_RecordSetInput(input,0,level)) // failed to set input level
		BASS_RecordSetInput(-1,0,level); // try master level instead
}

gboolean TimerProc(gpointer data)
{ // update the recording/playback counter
	char text[30]="";
	if (rchan) // recording
		sprintf(text,"%llu",BASS_ChannelGetPosition(rchan,BASS_POS_BYTE));
	else if (chan) {
		if (BASS_ChannelIsActive(chan)) // playing
			sprintf(text,"%llu / %llu",BASS_ChannelGetPosition(chan,BASS_POS_BYTE),BASS_ChannelGetLength(chan,BASS_POS_BYTE));
		else
			sprintf(text,"%llu",BASS_ChannelGetLength(chan,BASS_POS_BYTE));
	}
	gtk_label_set(GTK_LABEL(GetWidget("status")),text);
	return TRUE;
}

int main(int argc, char* argv[])
{
	regex_t fregex;

	gtk_init(&argc,&argv);

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	// initialize default recording device
	if (!BASS_RecordInit(-1)) {
		Error("Can't initialize recording device");
		return 0;
	}

	// initialize GUI
	glade=glade_xml_new(GLADE_PATH"rectest.glade",NULL,NULL);
	if (!glade) return 0;
	win=GetWidget("window1");
	if (!win) return 0;
	glade_xml_signal_autoconnect(glade);

	{ // get list of inputs
		int c;
		const char *i;
		GtkComboBox *list=GTK_COMBO_BOX(GetWidget("input"));
		for (c=0;i=BASS_RecordGetInputName(c);c++) {
			gtk_combo_box_append_text(list,i);
			if (!(BASS_RecordGetInput(c,NULL)&BASS_INPUT_OFF)) { // this 1 is currently "on"
				input=c;
				gtk_combo_box_set_active(list,input);
				UpdateInputInfo(); // display info
			}
		}
	}

	{ // initialize file selector
		GtkFileFilter *filter;
		filesel=gtk_file_chooser_dialog_new("Save File",GTK_WINDOW(win),GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_SAVE,GTK_RESPONSE_ACCEPT,NULL);
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(filesel),TRUE);
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"WAV files");
		regcomp(&fregex,"\\.wav$",REG_ICASE|REG_NOSUB|REG_EXTENDED);
		gtk_file_filter_add_custom(filter,GTK_FILE_FILTER_FILENAME,FileExtensionFilter,&fregex,NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"All files");
		gtk_file_filter_add_pattern(filter,"*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
	}

	g_timeout_add(200,TimerProc,NULL);

	gtk_main();

	gtk_widget_destroy(filesel);
	regfree(&fregex);

	// release all BASS stuff
	BASS_RecordFree();
	BASS_Free();

    return 0;
}
