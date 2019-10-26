/*
	BASS MOD music test
	Copyright (c) 1999-2014 Un4seen Developments Ltd.
*/

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include <regex.h>
#include "bass.h"

// path to glade file
#ifndef GLADE_PATH
#define GLADE_PATH ""
#endif

GladeXML *glade;
GtkWidget *win=0;
GtkWidget *filesel;

DWORD music;	// the HMUSIC channel

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

gboolean FileExtensionFilter(const GtkFileFilterInfo *info, gpointer data)
{
	return !regexec((regex_t*)data,info->filename,0,NULL,0);
}

DWORD GetFlags()
{
	DWORD flags=BASS_MUSIC_POSRESET; // stop notes when seeking
	switch (gtk_combo_box_get_active(GTK_COMBO_BOX(GetWidget("interpolation")))) {
		case 0:
			flags|=BASS_MUSIC_NONINTER; // no interpolation
			break;
		case 2:
			flags|=BASS_MUSIC_SINCINTER; // sinc interpolation
			break;
	}
	switch (gtk_combo_box_get_active(GTK_COMBO_BOX(GetWidget("ramping")))) {
		case 1:
			flags|=BASS_MUSIC_RAMP; // ramping
			break;
		case 2:
			flags|=BASS_MUSIC_RAMPS; // "sensitive" ramping
			break;
	}
	switch (gtk_combo_box_get_active(GTK_COMBO_BOX(GetWidget("surround")))) {
		case 1:
			flags|=BASS_MUSIC_SURROUND; // surround
			break;
		case 2:
			flags|=BASS_MUSIC_SURROUND2; // "mode2"
			break;
	}
	return flags;
}

void OpenClicked(GtkButton *obj, gpointer data)
{
	int resp=gtk_dialog_run(GTK_DIALOG(filesel));
	gtk_widget_hide(filesel);
	if (resp==GTK_RESPONSE_ACCEPT) {
		char *file=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
		BASS_MusicFree(music); // free the current music
		music=BASS_MusicLoad(FALSE,file,0,0,GetFlags(),1); // load the new music
		if (music) { // success
			DWORD length=BASS_ChannelGetLength(music,BASS_POS_MUSIC_ORDER); // get the order length
			gtk_button_set_label(obj,file);
			{
				char text[100],*ctype="";
				BASS_CHANNELINFO info;
				int channels=0;
				while (BASS_ChannelGetAttributeEx(music,BASS_ATTRIB_MUSIC_VOL_CHAN+channels,0,0)) channels++; // count channels
				BASS_ChannelGetInfo(music,&info);
				switch (info.ctype&~BASS_CTYPE_MUSIC_MO3) {
					case BASS_CTYPE_MUSIC_MOD:
						ctype="MOD";
						break;
					case BASS_CTYPE_MUSIC_MTM:
						ctype="MTM";
						break;
					case BASS_CTYPE_MUSIC_S3M:
						ctype="S3M";
						break;
					case BASS_CTYPE_MUSIC_XM:
						ctype="XM";
						break;
					case BASS_CTYPE_MUSIC_IT:
						ctype="IT";
						break;
				}
				snprintf(text,sizeof(text),"name: %s, format: %dch %s%s",BASS_ChannelGetTags(music,BASS_TAG_MUSIC_NAME),channels,ctype,info.ctype&BASS_CTYPE_MUSIC_MO3?" (MO3)":"");
				gtk_label_set_text(GTK_LABEL(GetWidget("info")),text);
			}
			gtk_range_set_range(GTK_RANGE(GetWidget("position")),0,length-1); // update scroller range
			BASS_ChannelPlay(music,FALSE); // start it
		} else { // failed
			gtk_button_set_label(obj,"click here to open a file...");
			gtk_label_set_text(GTK_LABEL(GetWidget("info")),"");
			gtk_label_set_text(GTK_LABEL(GetWidget("posdisplay")),"");
			Error("Can't play the file");
		}
		g_free(file);
	}
}

void PlayClicked(GtkButton *obj, gpointer data)
{
	if (BASS_ChannelIsActive(music)==BASS_ACTIVE_PLAYING)
		BASS_ChannelPause(music);
	else
		BASS_ChannelPlay(music,FALSE);
}

gboolean PositionChange(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer data)
{
	BASS_ChannelSetPosition(music,value,BASS_POS_MUSIC_ORDER); // set the position
	return FALSE;
}

void FlagChanged(GtkComboBox *obj, gpointer data)
{
	BASS_ChannelFlags(music,GetFlags(),-1); // update flags
}

gboolean TimerProc(gpointer data)
{
	char text[16];
	QWORD pos=BASS_ChannelGetPosition(music,BASS_POS_MUSIC_ORDER);
	if (pos!=(QWORD)-1) {
		gtk_range_set_value(GTK_RANGE(GetWidget("position")),LOWORD(pos));
		sprintf(text,"%03d.%03d",LOWORD(pos),HIWORD(pos));
		gtk_label_set_text(GTK_LABEL(GetWidget("posdisplay")),text);
	}
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

	// initialize default output device
	if (!BASS_Init(-1,44100,0,NULL,NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	// initialize GUI
	glade=glade_xml_new(GLADE_PATH"modtest.glade",NULL,NULL);
	if (!glade) return 0;
	win=GetWidget("window1");
	if (!win) return 0;
	glade_xml_signal_autoconnect(glade);

	gtk_combo_box_set_active(GTK_COMBO_BOX(GetWidget("interpolation")),1);
	gtk_combo_box_set_active(GTK_COMBO_BOX(GetWidget("ramping")),2);
	gtk_combo_box_set_active(GTK_COMBO_BOX(GetWidget("surround")),0);

	{ // initialize file selector
		GtkFileFilter *filter;
		filesel=gtk_file_chooser_dialog_new("Open File",GTK_WINDOW(win),GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OPEN,GTK_RESPONSE_ACCEPT,NULL);
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"mo3/it/xm/s3m/mtm/mod/umx");
		regcomp(&fregex,"\\.(mo3|it|xm|s3m|mtm|mod|umx)$",REG_ICASE|REG_NOSUB|REG_EXTENDED);
		gtk_file_filter_add_custom(filter,GTK_FILE_FILTER_FILENAME,FileExtensionFilter,&fregex,NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"All files");
		gtk_file_filter_add_pattern(filter,"*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
	}

	g_timeout_add(100,TimerProc,NULL);

	gtk_main();

	gtk_widget_destroy(filesel);
	regfree(&fregex);

	BASS_Free();

    return 0;
}
