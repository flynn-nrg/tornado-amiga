/*
	BASS simple DSP test
	Copyright (c) 2000-2017 Un4seen Developments Ltd.
*/

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include <regex.h>
#include <math.h>
#include "bass.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// path to glade file
#ifndef GLADE_PATH
#define GLADE_PATH ""
#endif

GladeXML *glade;
GtkWidget *win=0;
GtkWidget *filesel;

DWORD chan;	// the channel... HMUSIC or HSTREAM

// display error messages
void Error(const char *es)
{
	GtkWidget *dialog=gtk_message_dialog_new(GTK_WINDOW(win),GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

// "rotate"
HDSP rotdsp=0;	// DSP handle
float rotpos;	// cur.pos
void CALLBACK Rotate(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
	float *d=(float*)buffer;
	DWORD a;

	for (a=0;a<length/4;a+=2) {
		d[a]*=fabs(sin(rotpos));
		d[a+1]*=fabs(cos(rotpos));
		rotpos+=0.00003;
	}
	rotpos=fmod(rotpos,2*M_PI);
}

// "echo"
HDSP echdsp=0;	// DSP handle
#define ECHBUFLEN 1200	// buffer length
float echbuf[ECHBUFLEN][2];	// buffer
int echpos;	// cur.pos
void CALLBACK Echo(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
	float *d=(float*)buffer;
	DWORD a;

	for (a=0;a<length/4;a+=2) {
		float l=d[a]+(echbuf[echpos][1]/2);
		float r=d[a+1]+(echbuf[echpos][0]/2);
#if 1 // 0=echo, 1=basic "bathroom" reverb
		echbuf[echpos][0]=d[a]=l;
		echbuf[echpos][1]=d[a+1]=r;
#else
		echbuf[echpos][0]=d[a];
		echbuf[echpos][1]=d[a+1];
		d[a]=l;
		d[a+1]=r;
#endif
		echpos++;
		if (echpos==ECHBUFLEN) echpos=0;
	}
}

// "flanger"
HDSP fladsp=0;	// DSP handle
#define FLABUFLEN 350	// buffer length
float flabuf[FLABUFLEN][2];	// buffer
int flapos;	// cur.pos
float flas,flasinc;	// sweep pos/increment
void CALLBACK Flange(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
	float *d=(float*)buffer;
	DWORD a;

	for (a=0;a<length/4;a+=2) {
		int p1=(flapos+(int)flas)%FLABUFLEN;
		int p2=(p1+1)%FLABUFLEN;
		float f=flas-(int)flas;
		float s;

		s=(d[a]+((flabuf[p1][0]*(1-f))+(flabuf[p2][0]*f)))*0.7;
		flabuf[flapos][0]=d[a];
		d[a]=s;

		s=(d[a+1]+((flabuf[p1][1]*(1-f))+(flabuf[p2][1]*f)))*0.7;
		flabuf[flapos][1]=d[a+1];
		d[a+1]=s;

		flapos++;
		if (flapos==FLABUFLEN) flapos=0;
		flas+=flasinc;
		if (flas<0 || flas>FLABUFLEN-1) {
			flasinc=-flasinc;
			flas+=flasinc;
		}
	}
}

#define GetWidget(id) glade_xml_get_widget(glade,id)

void WindowDestroy(GtkObject *obj, gpointer data)
{
	gtk_main_quit();
}

void RotateToggled(GtkToggleButton *obj, gpointer data)
{ // toggle "rotate"
	if (obj->active) {
		rotpos=M_PI/4;
		rotdsp=BASS_ChannelSetDSP(chan,&Rotate,0,2);
	} else
		BASS_ChannelRemoveDSP(chan,rotdsp);
}

void EchoToggled(GtkToggleButton *obj, gpointer data)
{ // toggle "echo"
	if (obj->active) {
		memset(echbuf,0,sizeof(echbuf));
		echpos=0;
		echdsp=BASS_ChannelSetDSP(chan,&Echo,0,1);
	} else
		BASS_ChannelRemoveDSP(chan,echdsp);
}

void FlangerToggled(GtkToggleButton *obj, gpointer data)
{ // toggle "flanger"
	if (obj->active) {
		memset(flabuf,0,sizeof(flabuf));
		flapos=0;
		flas=FLABUFLEN/2;
		flasinc=0.002f;
		fladsp=BASS_ChannelSetDSP(chan,&Flange,0,0);
	} else
		BASS_ChannelRemoveDSP(chan,fladsp);
}

gboolean FileExtensionFilter(const GtkFileFilterInfo *info, gpointer data)
{
	return !regexec((regex_t*)data,info->filename,0,NULL,0);
}

void OpenClicked(GtkButton *obj, gpointer data)
{
	int resp=gtk_dialog_run(GTK_DIALOG(filesel));
	gtk_widget_hide(filesel);
	if (resp==GTK_RESPONSE_ACCEPT) {
		char *file=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
		// free both MOD and stream, it must be one of them! :)
		BASS_MusicFree(chan);
		BASS_StreamFree(chan);
		if (!(chan=BASS_StreamCreateFile(FALSE,file,0,0,BASS_SAMPLE_LOOP|BASS_SAMPLE_FLOAT))
			&& !(chan=BASS_MusicLoad(FALSE,file,0,0,BASS_MUSIC_RAMPS|BASS_SAMPLE_LOOP|BASS_SAMPLE_FLOAT,1))) {
			// whatever it is, it ain't playable
			gtk_button_set_label(obj,"click here to open a file...");
			Error("Can't play the file");
		} else {
			BASS_CHANNELINFO info;
			BASS_ChannelGetInfo(chan,&info);
			if (info.chans!=2) { // the DSP expects stereo
				gtk_button_set_label(obj,"click here to open a file...");
				BASS_MusicFree(chan);
				BASS_StreamFree(chan);
				Error("only stereo sources are supported");
			} else {
				gtk_button_set_label(obj,file);
				// setup DSPs on new channel and play it
				RotateToggled(GTK_TOGGLE_BUTTON(GetWidget("rotate")),0);
				EchoToggled(GTK_TOGGLE_BUTTON(GetWidget("echo")),0);
				FlangerToggled(GTK_TOGGLE_BUTTON(GetWidget("flanger")),0);
				BASS_ChannelPlay(chan,FALSE);
			}
		}
		g_free(file);
	}
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

	// check for floating-point capability
	if (!BASS_GetConfig(BASS_CONFIG_FLOAT)) {
		Error("This example requires floating-point support");
		return 0;
	}

	// enable floating-point DSP (not really necessary as the channels will be floating-point anyway)
	BASS_SetConfig(BASS_CONFIG_FLOATDSP,TRUE);

	// initialize default device
	if (!BASS_Init(-1,44100,0,NULL,NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	// initialize GUI
	glade=glade_xml_new(GLADE_PATH"dsptest.glade",NULL,NULL);
	if (!glade) return 0;
	win=GetWidget("window1");
	if (!win) return 0;
	glade_xml_signal_autoconnect(glade);

	{ // initialize file selector
		GtkFileFilter *filter;
		filesel=gtk_file_chooser_dialog_new("Open File",GTK_WINDOW(win),GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OPEN,GTK_RESPONSE_ACCEPT,NULL);
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"Playable files");
		regcomp(&fregex,"\\.(mo3|xm|mod|s3m|it|umx|mp[1-3]|ogg|wav|aif)$",REG_ICASE|REG_NOSUB|REG_EXTENDED);
		gtk_file_filter_add_custom(filter,GTK_FILE_FILTER_FILENAME,FileExtensionFilter,&fregex,NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"All files");
		gtk_file_filter_add_pattern(filter,"*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
	}

	gtk_main();

	gtk_widget_destroy(filesel);
	regfree(&fregex);

	BASS_Free();

    return 0;
}
