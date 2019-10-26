/*
	BASS DX8 effects test
	Copyright (c) 2001-2017 Un4seen Developments Ltd.
*/

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <math.h>
#include "bass.h"

// path to glade file
#ifndef GLADE_PATH
#define GLADE_PATH ""
#endif

GladeXML *glade;
GtkWidget *win=0;
GtkWidget *filesel;

DWORD floatable=BASS_SAMPLE_FLOAT;	// floating-point channel support?

DWORD chan;			// channel handle
DWORD fxchan=0;		// output stream handle
HFX fx[4];			// 3 eq bands + reverb

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

void UpdateFX(const char *id)
{
	GtkRange *obj=GTK_RANGE(GetWidget(id));
	double v=gtk_range_get_value(obj);
	int b=atoi(id+2)-1;
	if (b<3) {
		BASS_DX8_PARAMEQ p;
		BASS_FXGetParameters(fx[b],&p);
		p.fGain=v;
		BASS_FXSetParameters(fx[b],&p);
	} else {
		BASS_DX8_REVERB p;
		BASS_FXGetParameters(fx[3],&p);
		p.fReverbMix=(v?log(v/20.0)*20:-96);
		BASS_FXSetParameters(fx[3],&p);
	}
}

void SetupFX()
{
	// setup the effects
	BASS_DX8_PARAMEQ p;
	DWORD ch=fxchan?fxchan:chan; // set on output stream if enabled, else file stream
	fx[0]=BASS_ChannelSetFX(ch,BASS_FX_DX8_PARAMEQ,0);
	fx[1]=BASS_ChannelSetFX(ch,BASS_FX_DX8_PARAMEQ,0);
	fx[2]=BASS_ChannelSetFX(ch,BASS_FX_DX8_PARAMEQ,0);
	fx[3]=BASS_ChannelSetFX(ch,BASS_FX_DX8_REVERB,0);
	p.fGain=0;
	p.fBandwidth=18;
	p.fCenter=125;
	BASS_FXSetParameters(fx[0],&p);
	p.fCenter=1000;
	BASS_FXSetParameters(fx[1],&p);
	p.fCenter=8000;
	BASS_FXSetParameters(fx[2],&p);
	UpdateFX("fx1");
	UpdateFX("fx2");
	UpdateFX("fx3");
	UpdateFX("fx4");
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
		if (!(chan=BASS_StreamCreateFile(FALSE,file,0,0,BASS_SAMPLE_LOOP|floatable))
			&& !(chan=BASS_MusicLoad(FALSE,file,0,0,BASS_MUSIC_RAMPS|BASS_SAMPLE_LOOP|floatable,1))) {
			// whatever it is, it ain't playable
			gtk_button_set_label(obj,"click here to open a file...");
			Error("Can't play the file");
		} else {
			gtk_button_set_label(obj,file);
			if (!fxchan) SetupFX(); // set effects on file if not using output stream
			BASS_ChannelPlay(chan,FALSE);
		}
		g_free(file);
	}
}

gboolean FXChange(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer data)
{
	UpdateFX(gtk_widget_get_name(GTK_WIDGET(range)));
	return FALSE;
}

void OutputToggled(GtkToggleButton *obj, gpointer data)
{
	// remove current effects
	DWORD ch=fxchan?fxchan:chan;
	BASS_ChannelRemoveFX(ch,fx[0]);
	BASS_ChannelRemoveFX(ch,fx[1]);
	BASS_ChannelRemoveFX(ch,fx[2]);
	BASS_ChannelRemoveFX(ch,fx[3]);
	if (obj->active)
		fxchan=BASS_StreamCreate(0,0,0,STREAMPROC_DEVICE,0); // get device output stream
	else
		fxchan=0; // stop using device output stream
	SetupFX();
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
		// no floating-point channel support
		floatable=0;
		// enable floating-point (8.24 fixed-point in this case) DSP instead
		BASS_SetConfig(BASS_CONFIG_FLOATDSP,TRUE);
	}

	// initialize default device
	if (!BASS_Init(-1,44100,0,NULL,NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	// initialize GUI
	glade=glade_xml_new(GLADE_PATH"fxtest.glade",NULL,NULL);
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
