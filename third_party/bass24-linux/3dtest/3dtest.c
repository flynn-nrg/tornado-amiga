/*
	BASS 3D test
	Copyright (c) 1999-2012 Un4seen Developments Ltd.
*/

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <regex.h>
#include "bass.h"

// path to glade file
#ifndef GLADE_PATH
#define GLADE_PATH ""
#endif

GladeXML *glade;
GtkWidget *win=0;
GtkWidget *filesel;

// channel (sample/music) info structure
typedef struct {
	DWORD channel;			// the channel
	BASS_3DVECTOR pos,vel;	// position,velocity
} Channel;

Channel *chans=NULL;		// the channels
int chanc=0,chan=-1;		// number of channels, current channel

#define TIMERPERIOD	50		// timer period (ms)
#define MAXDIST		50		// maximum distance of the channels (m)
#define SPEED		12		// speed of the channels' movement (m/s)

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

// Update the button states
void UpdateButtons()
{
	gtk_widget_set_sensitive(GetWidget("remove"),chan==-1?FALSE:TRUE);
	gtk_widget_set_sensitive(GetWidget("play"),chan==-1?FALSE:TRUE);
	gtk_widget_set_sensitive(GetWidget("stop"),chan==-1?FALSE:TRUE);
	gtk_widget_set_sensitive(GetWidget("movex"),chan==-1?FALSE:TRUE);
	gtk_widget_set_sensitive(GetWidget("movez"),chan==-1?FALSE:TRUE);
	gtk_widget_set_sensitive(GetWidget("movereset"),chan==-1?FALSE:TRUE);
	if (chan!=-1) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(GetWidget("movex")),fabs(chans[chan].vel.x));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(GetWidget("movez")),fabs(chans[chan].vel.z));
	}
}

gboolean ListSelectionChange(GtkTreeView *treeview, gpointer data)
{ // the selected channel has (probably) changed
	GtkTreeSelection *ts;
	GtkTreeModel *tm;
	GtkTreeIter it;
	ts=gtk_tree_view_get_selection(GTK_TREE_VIEW(GetWidget("channels")));
	if (gtk_tree_selection_get_selected(ts,&tm,&it)) {
		char *rows=gtk_tree_model_get_string_from_iter(tm,&it);
		chan=atoi(rows);
		g_free(rows);
	} else
		chan=-1;
	UpdateButtons();
	return TRUE;
}

gboolean FileExtensionFilter(const GtkFileFilterInfo *info, gpointer data)
{
	return !regexec((regex_t*)data,info->filename,0,NULL,0);
}

void AddClicked(GtkButton *obj, gpointer data)
{ // add a channel
	int resp=gtk_dialog_run(GTK_DIALOG(filesel));
	gtk_widget_hide(filesel);
	if (resp==GTK_RESPONSE_ACCEPT) {
		char *file=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
		DWORD newchan;
		// Load a music or sample from "file"
		if ((newchan=BASS_MusicLoad(FALSE,file,0,0,BASS_MUSIC_RAMP|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D,1))
			|| (newchan=BASS_SampleLoad(FALSE,file,0,0,1,BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_MONO))) {
			Channel *c;
			chanc++;
			chans=(Channel*)realloc((void*)chans,chanc*sizeof(Channel));
			c=chans+chanc-1;
			memset(c,0,sizeof(Channel));
			c->channel=newchan;
			BASS_SampleGetChannel(newchan,FALSE); // initialize sample channel
			{ // add it to the list
				GtkTreeView *tree=GTK_TREE_VIEW(GetWidget("channels"));
				GtkListStore *tm=GTK_LIST_STORE(gtk_tree_view_get_model(tree));
				GtkTreeIter it;
				gtk_list_store_append(tm,&it);
				gtk_list_store_set(tm,&it,0,strrchr(file,'/')+1,-1);
			}
		} else
			Error("Can't load file (note samples must be mono)");
		g_free(file);
	}
}

void RemoveClicked(GtkButton *obj, gpointer data)
{ // remove a channel
	GtkTreeModel *tm=gtk_tree_view_get_model(GTK_TREE_VIEW(GetWidget("channels")));
	GtkTreeIter it;
	if (gtk_tree_model_iter_nth_child(tm,&it,NULL,chan)) {
		Channel *c=chans+chan;
		// free both MOD and stream, it must be one of them! :)
		BASS_SampleFree(c->channel);
		BASS_MusicFree(c->channel);
		chanc--;
		memmove(c,c+1,(chanc-chan)*sizeof(Channel));
		gtk_list_store_remove(GTK_LIST_STORE(tm),&it);
	}
}

void PlayClicked(GtkButton *obj, gpointer data)
{
	BASS_ChannelPlay(chans[chan].channel,FALSE);
}

void StopClicked(GtkButton *obj, gpointer data)
{
	BASS_ChannelPause(chans[chan].channel);
}

void MoveXChanged(GtkSpinButton *spinbutton, gpointer data)
{ // X velocity
	float value=gtk_spin_button_get_value(GTK_SPIN_BUTTON(GetWidget("movex")));
	if (fabs(chans[chan].vel.x)!=value) chans[chan].vel.x=value;
}

void MoveZChanged(GtkSpinButton *spinbutton, gpointer data)
{ // Y velocity
	float value=gtk_spin_button_get_value(GTK_SPIN_BUTTON(GetWidget("movez")));
	if (fabs(chans[chan].vel.z)!=value) chans[chan].vel.z=value;
}

void MoveResetClicked(GtkButton *obj, gpointer data)
{ // reset the position and velocity to 0
	memset(&chans[chan].pos,0,sizeof(chans[chan].pos));
	memset(&chans[chan].vel,0,sizeof(chans[chan].vel));
	UpdateButtons();
}

void RolloffChanged(GtkRange *range, gpointer data)
{
	// change the rolloff factor
	double value=gtk_range_get_value(range);
	BASS_Set3DFactors(-1,pow(2,(value-10)/5.0),-1);
}

void DopplerChanged(GtkRange *range, gpointer data)
{
	// change the doppler factor
	double value=gtk_range_get_value(range);
	BASS_Set3DFactors(-1,-1,pow(2,(value-10)/5.0));
}

gboolean Update(gpointer data)
{
	int c,x,y,cx,cy;
	GtkWidget *dc=GetWidget("drawingarea1");
	GdkGC *gc=dc->style->fg_gc[GTK_WIDGET_STATE(dc)];
	GdkGCValues gcsave;

	gdk_gc_get_values(gc,&gcsave);

	cx=dc->allocation.width/2;
	cy=dc->allocation.height/2;

	{ // clear the display
		GdkColor c={0,0xffff,0xffff,0xffff};
		gdk_gc_set_rgb_fg_color(gc,&c);
		gdk_draw_rectangle(dc->window,gc,TRUE,0,0,dc->allocation.width,dc->allocation.height);
	}

	{ // Draw the listener
		GdkColor c={0,0x8000,0x8000,0x8000};
		gdk_gc_set_rgb_fg_color(gc,&c);
		gdk_draw_arc(dc->window,gc,TRUE,cx-4,cy-4,8,8,0,360*64);
	}

	for (c=0;c<chanc;c++) {
		// If the channel's playing then update it's position
		if (BASS_ChannelIsActive(chans[c].channel)==BASS_ACTIVE_PLAYING) {
			// Check if channel has reached the max distance
			if (chans[c].pos.z>=MAXDIST || chans[c].pos.z<=-MAXDIST)
				chans[c].vel.z=-chans[c].vel.z;
			if (chans[c].pos.x>=MAXDIST || chans[c].pos.x<=-MAXDIST)
				chans[c].vel.x=-chans[c].vel.x;
			// Update channel position
			chans[c].pos.z+=chans[c].vel.z*TIMERPERIOD/1000;
			chans[c].pos.x+=chans[c].vel.x*TIMERPERIOD/1000;
			BASS_ChannelSet3DPosition(chans[c].channel,&chans[c].pos,NULL,&chans[c].vel);
		}
		{ // Draw the channel position indicator
			static GdkColor cols[2]={{0,0xffff,0xc000,0xc000},{0,0xffff,0,0}};
			gdk_gc_set_rgb_fg_color(gc,&cols[chan==c]);
			x=cx+(int)((cx-10)*chans[c].pos.x/MAXDIST);
			y=cy-(int)((cy-10)*chans[c].pos.z/MAXDIST);
			gdk_draw_arc(dc->window,gc,TRUE,x-4,y-4,8,8,0,360*64);
		}
	}
	// Apply the 3D changes
	BASS_Apply3D();

	gdk_gc_set_values(gc,&gcsave,GDK_GC_FOREGROUND);

	return TRUE;
}

int main(int argc, char* argv[])
{
	regex_t fregex[2];

	gtk_init(&argc,&argv);

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	// Initialize the output device with 3D support
	if (!BASS_Init(-1,44100,BASS_DEVICE_3D,NULL,NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	{
		BASS_INFO i;
		BASS_GetInfo(&i);
		if (i.speakers>2) {
			GtkWidget *dialog=gtk_message_dialog_new(NULL,0,
				GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,"Multiple speakers were detected. Would you like to use them?");
			if (gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_NO)
				BASS_SetConfig(BASS_CONFIG_3DALGORITHM,BASS_3DALG_OFF);
			gtk_widget_destroy(dialog);
		}
	}

	// Use meters as distance unit, real world rolloff, real doppler effect
	BASS_Set3DFactors(1,1,1);

	// initialize GUI
	glade=glade_xml_new(GLADE_PATH"3dtest.glade",NULL,NULL);
	if (!glade) return 0;
	win=GetWidget("window1");
	if (!win) return 0;
	glade_xml_signal_autoconnect(glade);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(GetWidget("channels"))),"changed",G_CALLBACK(ListSelectionChange),NULL);

	{ // setup list
		GtkTreeView *list=GTK_TREE_VIEW(GetWidget("channels"));
		GtkTreeViewColumn *col=gtk_tree_view_column_new();
		gtk_tree_view_append_column(list,col);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(col,renderer,TRUE);
		gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
		GtkListStore *liststore=gtk_list_store_new(1,G_TYPE_STRING);
		gtk_tree_view_set_model(list,GTK_TREE_MODEL(liststore));
		g_object_unref(liststore);
	}

	{ // initialize file selector
		GtkFileFilter *filter;
		filesel=gtk_file_chooser_dialog_new("Open File",GTK_WINDOW(win),GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,GTK_STOCK_OPEN,GTK_RESPONSE_ACCEPT,NULL);
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"Streamable files (wav/aif/mp3/mp2/mp1/ogg)");
		regcomp(&fregex[0],"\\.(mp[1-3]|ogg|wav|aif)$",REG_ICASE|REG_NOSUB|REG_EXTENDED);
		gtk_file_filter_add_custom(filter,GTK_FILE_FILTER_FILENAME,FileExtensionFilter,&fregex[0],NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"MOD music files (mo3/xm/mod/s3m/it/mtm/umx)");
		regcomp(&fregex[1],"\\.(mo3|xm|mod|s3m|it|umx)$",REG_ICASE|REG_NOSUB|REG_EXTENDED);
		gtk_file_filter_add_custom(filter,GTK_FILE_FILTER_FILENAME,FileExtensionFilter,&fregex[1],NULL);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
		filter=gtk_file_filter_new();
		gtk_file_filter_set_name(filter,"All files");
		gtk_file_filter_add_pattern(filter,"*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filesel),filter);
	}

	g_timeout_add(TIMERPERIOD,Update,NULL);

	UpdateButtons();

	gtk_main();

	gtk_widget_destroy(filesel);
	regfree(&fregex[0]);
	regfree(&fregex[1]);

	BASS_Free(); // close output

    return 0;
}
