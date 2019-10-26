/*
	BASS multiple output example
	Copyright (c) 2001-2008 Un4seen Developments Ltd.
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

DWORD outdev[2];	// output devices
DWORD latency[2];	// latencies
HSTREAM chan[2];	// the streams

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

void OpenClicked(GtkButton *obj, gpointer data)
{
	int resp=gtk_dialog_run(GTK_DIALOG(filesel));
	gtk_widget_hide(filesel);
	if (resp==GTK_RESPONSE_ACCEPT) {
		char *file=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
		const gchar *objname=gtk_widget_get_name(GTK_WIDGET(obj));
		int devn=atoi(objname+4)-1; // get device number from button name ("openX")
		BASS_StreamFree(chan[devn]); // free old stream before opening new
		BASS_SetDevice(outdev[devn]); // set the device to create stream on
		if (!(chan[devn]=BASS_StreamCreateFile(FALSE,file,0,0,BASS_SAMPLE_LOOP))) {
			gtk_button_set_label(obj,"click here to open a file...");
			Error("Can't play the file");
		} else {
			gtk_button_set_label(obj,file);
			BASS_ChannelPlay(chan[devn],FALSE);
		}
		g_free(file);
	}
}

// Cloning DSP function
void CALLBACK CloneDSP(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
	BASS_StreamPutData((uintptr_t)user,buffer,length); // user = clone
}

void CloneClicked(GtkButton *obj, gpointer data)
{
	const gchar *objname=gtk_widget_get_name(GTK_WIDGET(obj));
	int devn=atoi(objname+5)-1; // get device number from button name ("cloneX")
	BASS_CHANNELINFO i;
	if (!BASS_ChannelGetInfo(chan[devn^1],&i)) {
		Error("Nothing to clone");
	} else {
		BASS_StreamFree(chan[devn]); // free old stream
		BASS_SetDevice(outdev[devn]); // set the device to create stream on
		if (!(chan[devn]=BASS_StreamCreate(i.freq,i.chans,i.flags,STREAMPROC_PUSH,0))) { // create a "push" stream
			char oname[10];
			sprintf(oname,"open%d",devn+1);
			gtk_button_set_label(GTK_BUTTON(GetWidget(oname)),"click here to open a file...");
			Error("Can't create clone");
		} else {
			BASS_ChannelLock(chan[devn^1],TRUE); // lock source stream to synchonise buffer contents
			BASS_ChannelSetDSP(chan[devn^1],CloneDSP,(void*)(uintptr_t)chan[devn],0); // set DSP to feed data to clone
			{ // copy buffered data to clone
				DWORD d=BASS_ChannelSeconds2Bytes(chan[devn],latency[devn]/1000.f); // playback delay
				DWORD c=BASS_ChannelGetData(chan[devn^1],0,BASS_DATA_AVAILABLE);
				BYTE *buf=(BYTE*)malloc(c);
				c=BASS_ChannelGetData(chan[devn^1],buf,c);
				if (c>d) BASS_StreamPutData(chan[devn],buf+d,c-d);
				free(buf);
			}
			BASS_ChannelLock(chan[devn^1],FALSE); // unlock source stream
			BASS_ChannelPlay(chan[devn],FALSE); // play clone
			{
				char oname[10];
				sprintf(oname,"open%d",devn+1);
				gtk_button_set_label(GTK_BUTTON(GetWidget(oname)),"clone");
			}
		}
	}
}

void SwapClicked(GtkButton *obj, gpointer data)
{
	{ // swap handles
		HSTREAM temp=chan[0];
		chan[0]=chan[1];
		chan[1]=temp;
	}
	{ // swap text
		GtkButton *open1=GTK_BUTTON(GetWidget("open1")),*open2=GTK_BUTTON(GetWidget("open2"));
		char *temp=strdup(gtk_button_get_label(open1));
		gtk_button_set_label(open1,gtk_button_get_label(open2));
		gtk_button_set_label(open2,temp);
		free(temp);
	}
	// update the channel devices
	BASS_ChannelSetDevice(chan[0],outdev[0]);
	BASS_ChannelSetDevice(chan[1],outdev[1]);
}

// Simple device selector dialog stuff begins here
void ListRowActivated(GtkTreeView *obj, GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
	int sel=1+*gtk_tree_path_get_indices(path); // get selection
	gtk_dialog_response(GTK_DIALOG(GetWidget("dialog1")),sel);
}

int SelectDevice(const char *title)
{
	int sel=0;
	GtkWidget *win=GetWidget("dialog1");
	gtk_window_set_title(GTK_WINDOW(win),title);

	GtkTreeView *list=GTK_TREE_VIEW(GetWidget("list"));
	if (!gtk_tree_view_get_column(list,0)) { // no column yet, so add one...
		GtkTreeViewColumn *col=gtk_tree_view_column_new();
		gtk_tree_view_append_column(list,col);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(col,renderer,TRUE);
		gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	}

	GtkListStore *liststore=gtk_list_store_new(1,G_TYPE_STRING);
	{
		BASS_DEVICEINFO i;
		int c;
		for (c=1;BASS_GetDeviceInfo(c,&i);c++) { // device 1 = 1st real device
			if (i.flags&BASS_DEVICE_ENABLED) { // enabled, so add it...
				GtkTreeIter it;
				gtk_list_store_append(liststore,&it);
				gtk_list_store_set(liststore,&it,0,i.name,-1);
			}
		}
	}
	gtk_tree_view_set_model(list,GTK_TREE_MODEL(liststore));
	g_object_unref(liststore);
	{ // pre-select 1st entry
		GtkTreeSelection *tsel=gtk_tree_view_get_selection(list);
		GtkTreePath *tpath=gtk_tree_path_new_first();
		gtk_tree_selection_select_path(tsel,tpath);
		gtk_tree_path_free(tpath);
	}

	sel=gtk_dialog_run(GTK_DIALOG(win));
	gtk_widget_hide(win);

	return sel;
}
// Simple device selector dialog stuff ends here

int main(int argc, char* argv[])
{
	regex_t fregex;

	gtk_init(&argc,&argv);

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	// initialize GUI
	glade=glade_xml_new(GLADE_PATH"multi.glade",NULL,NULL);
	if (!glade) return 0;
	win=GetWidget("window1");
	if (!win) return 0;
	glade_xml_signal_autoconnect(glade);

	// Let the user choose the output devices
	outdev[0]=SelectDevice("Select output device #1");
	outdev[1]=SelectDevice("Select output device #2");

	{ // setup output devices
		BASS_INFO info;
		if (!BASS_Init(outdev[0],44100,0,NULL,NULL)) {
			Error("Can't initialize device 1");
			return 0;
		}
		BASS_GetInfo(&info);
		latency[0]=info.latency;
		if (!BASS_Init(outdev[1],44100,0,NULL,NULL)) {
			Error("Can't initialize device 2");
			return 0;
		}
		BASS_GetInfo(&info);
		latency[1]=info.latency;
	}
	{
		BASS_DEVICEINFO i;
		BASS_GetDeviceInfo(outdev[0],&i);
		gtk_frame_set_label(GTK_FRAME(GetWidget("device1")),i.name);
		BASS_GetDeviceInfo(outdev[1],&i);
		gtk_frame_set_label(GTK_FRAME(GetWidget("device2")),i.name);
	}

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

	gtk_widget_show(win);
	gtk_main();

	gtk_widget_destroy(filesel);
	regfree(&fregex);

	// release both devices
	BASS_SetDevice(outdev[0]);
	BASS_Free();
	BASS_SetDevice(outdev[1]);
	BASS_Free();

    return 0;
}
