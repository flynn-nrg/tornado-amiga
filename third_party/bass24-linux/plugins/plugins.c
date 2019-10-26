/*
	BASS plugin test
	Copyright (c) 2005-2009 Un4seen Developments Ltd.
*/

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <glob.h>
#include "bass.h"

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

// translate a CTYPE value to text
const char *GetCTypeString(DWORD ctype, HPLUGIN plugin)
{
	if (plugin) { // using a plugin
		const BASS_PLUGININFO *pinfo=BASS_PluginGetInfo(plugin); // get plugin info
		int a;
		for (a=0;a<pinfo->formatc;a++) {
			if (pinfo->formats[a].ctype==ctype) // found a "ctype" match...
				return pinfo->formats[a].name; // return it's name
		}
	}
	// check built-in stream formats...
	if (ctype==BASS_CTYPE_STREAM_OGG) return "Ogg Vorbis";
	if (ctype==BASS_CTYPE_STREAM_MP1) return "MPEG layer 1";
	if (ctype==BASS_CTYPE_STREAM_MP2) return "MPEG layer 2";
	if (ctype==BASS_CTYPE_STREAM_MP3) return "MPEG layer 3";
	if (ctype==BASS_CTYPE_STREAM_AIFF) return "Audio IFF";
	if (ctype==BASS_CTYPE_STREAM_WAV_PCM) return "PCM WAVE";
	if (ctype==BASS_CTYPE_STREAM_WAV_FLOAT) return "Floating-point WAVE";
	if (ctype&BASS_CTYPE_STREAM_WAV) return "WAVE";
	return "?";
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
		BASS_StreamFree(chan); // free old stream before opening new
		if (!(chan=BASS_StreamCreateFile(FALSE,file,0,0,BASS_SAMPLE_LOOP))) {
			gtk_button_set_label(obj,"click here to open a file...");
			gtk_label_set_text(GTK_LABEL(GetWidget("info")),"");
			gtk_range_set_range(GTK_RANGE(GetWidget("position")),0,0);
			Error("Can't play the file");
		} else {
			gtk_button_set_label(obj,file);
			{ // display the file type and length
				char text[100];
				QWORD bytes=BASS_ChannelGetLength(chan,BASS_POS_BYTE);
				DWORD time=BASS_ChannelBytes2Seconds(chan,bytes);
				BASS_CHANNELINFO info;
				BASS_ChannelGetInfo(chan,&info);
				sprintf(text,"channel type = %x (%s)\nlength = %llu (%u:%02u)",
					info.ctype,GetCTypeString(info.ctype,info.plugin),bytes,time/60,time%60);
				gtk_label_set_text(GTK_LABEL(GetWidget("info")),text);
				gtk_range_set_range(GTK_RANGE(GetWidget("position")),0,time); // update scroller range
			}
			BASS_ChannelPlay(chan,FALSE);
		}
		g_free(file);
	}
}

gboolean PositionChange(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer data)
{
	BASS_ChannelSetPosition(chan,BASS_ChannelSeconds2Bytes(chan,value),BASS_POS_BYTE);
	return FALSE;
}

gboolean TimerProc(gpointer data)
{
	gtk_range_set_value(GTK_RANGE(GetWidget("position")),BASS_ChannelBytes2Seconds(chan,BASS_ChannelGetPosition(chan,BASS_POS_BYTE))); // update position
	return TRUE;
}

int main(int argc, char* argv[])
{
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
	glade=glade_xml_new(GLADE_PATH"plugins.glade",NULL,NULL);
	if (!glade) return 0;
	win=GetWidget("window1");
	if (!win) return 0;
	glade_xml_signal_autoconnect(glade);

	{ // setup plugin list and file selector
		GtkTreeView *list=GTK_TREE_VIEW(GetWidget("plugins"));
		GtkTreeViewColumn *col=gtk_tree_view_column_new();
		gtk_tree_view_append_column(list,col);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(col,renderer,TRUE);
		gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
		GtkListStore *liststore=gtk_list_store_new(1,G_TYPE_STRING);
		gtk_tree_view_set_model(list,GTK_TREE_MODEL(liststore));
		g_object_unref(liststore);
		gtk_tree_selection_set_mode(gtk_tree_view_get_selection(list),GTK_SELECTION_NONE);

		GtkFileFilter *filter;
		regex_t *fregex;
		filesel=gtk_file_chooser_dialog_new("Open File",GTK_WINDOW(win),GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OPEN,GTK_RESPONSE_ACCEPT,NULL);
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"BASS built-in (*.wav;*.aif;*.mp3;*.mp2;*.mp1;*.ogg)");
		fregex=malloc(sizeof(*fregex));
		regcomp(fregex,"\\.(mp[1-3]|ogg|wav|aif)$",REG_ICASE|REG_NOSUB|REG_EXTENDED);
		gtk_file_filter_add_custom(filter,GTK_FILE_FILTER_FILENAME,FileExtensionFilter,fregex,NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
		{ // look for plugins (in the executable's directory)
			glob_t g;
			char path[300];
			if (readlink("/proc/self/exe",path,300)<=0) {
				Error("Can't locate executable");
				return 0;
			}
			strcpy(strrchr(path,'/')+1,"libbass*.so");
			if (!glob(path,0,0,&g)) {
				int a;
				for (a=0;a<g.gl_pathc;a++) {
					HPLUGIN plug;
					if (plug=BASS_PluginLoad(g.gl_pathv[a],0)) { // plugin loaded...
						// add it to the list
						char *file=strrchr(g.gl_pathv[a],'/')+1;
						GtkTreeIter it;
						gtk_list_store_append(liststore,&it);
						gtk_list_store_set(liststore,&it,0,file,-1);
						// get plugin info to add to the file selector filter
						const BASS_PLUGININFO *pinfo=BASS_PluginGetInfo(plug);
						int b;
						for (b=0;b<pinfo->formatc;b++) {
							char buf[300], *p;
							filter=gtk_file_filter_new();
							sprintf(buf,"%s (%s) - %s",pinfo->formats[b].name,pinfo->formats[b].exts,file);
							gtk_file_filter_set_name(filter,buf);
							// build filter regex
							sprintf(buf,"\\.(%s)$",pinfo->formats[b].exts);
							while (p=strchr(buf,'*')) { // find an extension
								if (p[-1]==';') // not the first...
									p[-1]='|'; // add an alternation
								memmove(p,p+2,strlen(p+2)+1); // remove the "*."
							}
							fregex=malloc(sizeof(*fregex));
							regcomp(fregex,buf,REG_ICASE|REG_NOSUB|REG_EXTENDED);
							gtk_file_filter_add_custom(filter,GTK_FILE_FILTER_FILENAME,FileExtensionFilter,fregex,NULL);
							gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
						}
					}
				}
				globfree(&g);
			}
			{
				GtkTreeIter it;
				if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore),&it)) { // no plugins...
					gtk_list_store_append(liststore,&it);
					gtk_list_store_set(liststore,&it,0,"no plugins - visit the BASS webpage to get some",-1);
				}
			}
		}
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"All files");
		gtk_file_filter_add_pattern(filter,"*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
	}

	g_timeout_add(500,TimerProc,NULL);

	gtk_main();

	gtk_widget_destroy(filesel);

	// "free" the output device and all plugins
	BASS_Free();
	BASS_PluginFree(0);

    return 0;
}
