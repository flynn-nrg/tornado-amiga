/*
	BASS custom looping example
	Copyright (c) 2004-2014 Un4seen Developments Ltd.
*/

#include <gtk/gtk.h>
#include <glib/gthread.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <math.h>
#include "bass.h"

#define WIDTH 600	// display width
#define HEIGHT 201	// height (odd number for centre line)

#pragma pack(1)
typedef struct {
	BYTE rgbRed,rgbGreen,rgbBlue;
} RGB;
#pragma pack()

GtkWidget *win=0;
GThread *scanthread=0;
BOOL killscan=FALSE;

DWORD chan;
DWORD bpp;			// bytes per pixel
QWORD loop[2]={0};	// loop start & end
HSYNC lsync;		// looping sync

GtkWidget *waveda;
GdkPixbuf *wavepb;
RGB palette[HEIGHT/2+1];

// display error messages
void Error(const char *es)
{
	GtkWidget *dialog=gtk_message_dialog_new(GTK_WINDOW(win),GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void WindowDestroy(GtkObject *obj, gpointer data)
{
	gtk_main_quit();
}

void CALLBACK LoopSyncProc(HSYNC handle,DWORD channel,DWORD data,void *user)
{
	if (!BASS_ChannelSetPosition(channel,loop[0],BASS_POS_BYTE)) // try seeking to loop start
		BASS_ChannelSetPosition(channel,0,BASS_POS_BYTE); // failed, go to start of file instead
}

void SetLoopStart(QWORD pos)
{
	loop[0]=pos;
}

void SetLoopEnd(QWORD pos)
{
	loop[1]=pos;
	BASS_ChannelRemoveSync(chan,lsync); // remove old sync
	lsync=BASS_ChannelSetSync(chan,BASS_SYNC_POS|BASS_SYNC_MIXTIME,loop[1],LoopSyncProc,0); // set new sync
}

// scan the peaks
void *ScanPeaks(void *p)
{
	DWORD decoder=(uintptr_t)p;
	RGB *wavebuf=(RGB*)gdk_pixbuf_get_pixels(wavepb);
	DWORD pos=0;
	float spp=BASS_ChannelBytes2Seconds(decoder,bpp); // seconds per pixel
	while (!killscan) {
		float peak[2];
		if (spp>1) { // more than 1 second per pixel, break it down...
			float todo=spp;
			peak[1]=peak[0]=0;
			do {
				float level[2],step=(todo<1?todo:1);
				BASS_ChannelGetLevelEx(decoder,level,step,BASS_LEVEL_STEREO); // scan peaks
				if (peak[0]<level[0]) peak[0]=level[0];
				if (peak[1]<level[1]) peak[1]=level[1];
				todo-=step;
			} while (todo>0);
		} else
			BASS_ChannelGetLevelEx(decoder,peak,spp,BASS_LEVEL_STEREO); // scan peaks
		{
			DWORD a;
			for (a=0;a<peak[0]*(HEIGHT/2);a++)
				wavebuf[(HEIGHT/2-1-a)*WIDTH+pos]=palette[1+a]; // draw left peak
			for (a=0;a<peak[1]*(HEIGHT/2);a++)
				wavebuf[(HEIGHT/2+1+a)*WIDTH+pos]=palette[1+a]; // draw right peak
		}
		pos++;
		if (pos>=WIDTH) break; // reached end of display
		if (!BASS_ChannelIsActive(decoder)) break; // reached end of channel
	}
	if (!killscan) {
		DWORD size;
		BASS_ChannelSetPosition(decoder,(QWORD)-1,BASS_POS_BYTE|BASS_POS_SCAN); // build seek table (scan to end)
		size=BASS_ChannelGetAttributeEx(decoder,BASS_ATTRIB_SCANINFO,0,0); // get seek table size
		if (size) { // got it
			void *info=malloc(size); // allocate a buffer
			BASS_ChannelGetAttributeEx(decoder,BASS_ATTRIB_SCANINFO,info,size); // get the seek table
			BASS_ChannelSetAttributeEx(chan,BASS_ATTRIB_SCANINFO,info,size); // apply it to the playback channel
			free(info);
		}
	}
	BASS_StreamFree(decoder); // free the decoder
	return NULL;
}

gboolean FileExtensionFilter(const GtkFileFilterInfo *info, gpointer data)
{
	return !regexec((regex_t*)data,info->filename,0,NULL,0);
}

// select a file to play, and start scanning it
BOOL PlayFile()
{
	BOOL ret=FALSE;
	regex_t fregex;
	GtkWidget *filesel; // file selector
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
	if (gtk_dialog_run(GTK_DIALOG(filesel))==GTK_RESPONSE_ACCEPT) {
		char *file=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));
		gtk_widget_hide(filesel);
		if (!(chan=BASS_StreamCreateFile(FALSE,file,0,0,0))
			&& !(chan=BASS_MusicLoad(FALSE,file,0,0,BASS_MUSIC_RAMP|BASS_MUSIC_POSRESET|BASS_MUSIC_PRESCAN,1))) {
			Error("Can't play file");
		} else {
			// create the bitmap
			wavepb=gdk_pixbuf_new(GDK_COLORSPACE_RGB,FALSE,8,WIDTH,HEIGHT);
			{ // setup palette
				RGB *pal=(RGB*)palette;
				int a;
				memset(palette,0,sizeof(palette));
				for (a=1;a<=HEIGHT/2;a++) {
					pal[a].rgbRed=(255*a)/(HEIGHT/2);
					pal[a].rgbGreen=255-pal[a].rgbRed;
				}
			}
			bpp=BASS_ChannelGetLength(chan,BASS_POS_BYTE)/WIDTH; // bytes per pixel
			{
				DWORD bpp1=BASS_ChannelSeconds2Bytes(chan,0.001); // minimum 1ms per pixel
				if (bpp<bpp1) bpp=bpp1;
			}
			BASS_ChannelSetSync(chan,BASS_SYNC_END|BASS_SYNC_MIXTIME,0,LoopSyncProc,0); // set sync to loop at end
			BASS_ChannelPlay(chan,FALSE); // start playing
			{ // create another channel to scan
				DWORD chan2=BASS_StreamCreateFile(FALSE,file,0,0,BASS_STREAM_DECODE);
				if (!chan2) chan2=BASS_MusicLoad(FALSE,file,0,0,BASS_MUSIC_DECODE,1);
#if GLIB_CHECK_VERSION(2,32,0)
				scanthread=g_thread_new(NULL,ScanPeaks,(void*)(uintptr_t)chan2); // start scanning in a new thread
#else
				scanthread=g_thread_create(ScanPeaks,(void*)(uintptr_t)chan2,TRUE,NULL); // start scanning in a new thread
#endif
			}
			ret=TRUE;
		}
		g_free(file);
	}
	gtk_widget_destroy(filesel);
	return ret;
}

void WindowButtonRelease(GtkWidget *obj, GdkEventButton *event, gpointer data)
{
	if (event->type==GDK_BUTTON_RELEASE) {
		if (event->button==1) // set loop start
			SetLoopStart(event->x*bpp);
		else if (event->button==3) // set loop end
			SetLoopEnd(event->x*bpp);
	}
}

void DrawTimeLine(QWORD pos, DWORD col, DWORD y)
{
	GdkGC *gc=waveda->style->fg_gc[GTK_WIDGET_STATE(waveda)];
	DWORD wpos=pos/bpp;
	GdkColor c={0,(col&0xff)<<8,col&0xff00,(col>>8)&0xff00};
	gdk_gc_set_rgb_fg_color(gc,&c);
	gdk_draw_line(waveda->window,gc,wpos,0,wpos,HEIGHT);
}

gboolean AreaExpose(GtkWidget *widget, GdkEventExpose *event, gpointer user)
{
	GdkGC *gc=widget->style->fg_gc[GTK_WIDGET_STATE(waveda)];
	GdkGCValues gcsave;
	gdk_gc_get_values(gc,&gcsave);
	gdk_draw_pixbuf(widget->window,gc,wavepb,0,0,0,0,-1,-1,GDK_RGB_DITHER_NONE,0,0);
	DrawTimeLine(loop[0],0xffff00,12); // loop start
	DrawTimeLine(loop[1],0x00ffff,24); // loop end
	DrawTimeLine(BASS_ChannelGetPosition(chan,BASS_POS_BYTE),0xffffff,0); // current pos
	gdk_gc_set_values(gc,&gcsave,GDK_GC_FOREGROUND);
	return FALSE;
}

gboolean TimerProc(gpointer data)
{
	// refresh window
	gtk_widget_queue_draw(waveda);
	return TRUE;
}

int main(int argc, char* argv[])
{
#if !GLIB_CHECK_VERSION(2,32,0)
	g_thread_init(NULL);
#endif
	gtk_init(&argc,&argv);

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	// initialize BASS
	if (!BASS_Init(-1,44100,0,win,NULL)) {
		Error("Can't initialize device");
		return 0;
	}
	if (!PlayFile()) { // start a file playing
		BASS_Free();
		return 0;
	}

	// create the window
	win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(win),GTK_WIN_POS_CENTER);
	gtk_window_set_resizable(GTK_WINDOW(win),FALSE);
	gtk_window_set_title(GTK_WINDOW(win),"BASS custom looping example (left-click to set loop start, right-click to set end)");
	g_signal_connect(win,"destroy",GTK_SIGNAL_FUNC(WindowDestroy),NULL);

	GtkWidget *ebox=gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(win),ebox);
	g_signal_connect(ebox,"button-release-event",GTK_SIGNAL_FUNC(WindowButtonRelease),NULL);

	waveda=gtk_drawing_area_new();
	gtk_widget_set_size_request(waveda,WIDTH,HEIGHT);
	gtk_container_add(GTK_CONTAINER(ebox),waveda);
	g_signal_connect(waveda,"expose-event",GTK_SIGNAL_FUNC(AreaExpose),NULL);

	// setup update timer (10hz)
	g_timeout_add(100,TimerProc,NULL);

	gtk_widget_show_all(win);
	gtk_main();

	killscan=TRUE;
	g_thread_join(scanthread); // wait for the scan thread

	g_object_unref(wavepb);

	BASS_Free();
	return 0;
}
