/*
	BASS spectrum analyser example
	Copyright (c) 2002-2012 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include <stdio.h>
#include <math.h>
#include "bass.h"

#define SPECWIDTH 368	// display width
#define SPECHEIGHT 127	// height (changing requires palette adjustments too)

#pragma pack(1)
typedef struct {
	BYTE rgbRed,rgbGreen,rgbBlue,Aplha;
} RGBQUAD;
#pragma pack()

WindowPtr win;

DWORD chan;

CGContextRef specdc;
DWORD specbuf[SPECWIDTH*SPECHEIGHT];
DWORD palette[256];

int specmode=0,specpos=0; // spectrum mode (and marker pos for 2nd mode)

// display error messages
void Error(const char *es)
{
	short i;
	char mes[200];
	sprintf(mes,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	CFStringRef ces=CFStringCreateWithCString(0,mes,0);
	DialogRef alert;
	CreateStandardAlert(0,CFSTR("Error"),ces,NULL,&alert);
	RunStandardAlert(alert,NULL,&i);
	CFRelease(ces);
}

// select a file to play, and play it
BOOL PlayFile()
{
	BOOL ret=FALSE;
	NavDialogRef fileDialog;
	NavDialogCreationOptions fo;
	NavGetDefaultDialogCreationOptions(&fo);
	fo.optionFlags=0;
	fo.parentWindow=win;
	NavCreateChooseFileDialog(&fo,NULL,NULL,NULL,NULL,NULL,&fileDialog);
	if (!NavDialogRun(fileDialog)) {
		NavReplyRecord r;
		if (!NavDialogGetReply(fileDialog,&r)) {
			AEKeyword k;
			FSRef fr;
			if (!AEGetNthPtr(&r.selection,1,typeFSRef,&k,NULL,&fr,sizeof(fr),NULL)) {
				char file[256];
				FSRefMakePath(&fr,(BYTE*)file,sizeof(file));
				if (!(chan=BASS_StreamCreateFile(FALSE,file,0,0,BASS_SAMPLE_LOOP))
					&& !(chan=BASS_MusicLoad(FALSE,file,0,0,BASS_MUSIC_RAMP|BASS_SAMPLE_LOOP,1))) {
					Error("Can't play file");
				} else {
					BASS_ChannelPlay(chan,FALSE);
					ret=TRUE;
				}
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
	return ret;
}

// update the spectrum display - the interesting bit :)
pascal void UpdateSpectrum(EventLoopTimerRef inTimer, void *inUserData)
{
	int x,y,y1;

	if (specmode==3) { // waveform
		int c;
		float *buf;
		BASS_CHANNELINFO ci;
		memset(specbuf,0,sizeof(specbuf));
		BASS_ChannelGetInfo(chan,&ci); // get number of channels
		buf=alloca(ci.chans*SPECWIDTH*sizeof(float)); // allocate buffer for data
		BASS_ChannelGetData(chan,buf,(ci.chans*SPECWIDTH*sizeof(float))|BASS_DATA_FLOAT); // get the sample data (floating-point to avoid 8 & 16 bit processing)
		for (c=0;c<ci.chans;c++) {
			for (x=0;x<SPECWIDTH;x++) {
				int v=(1-buf[x*ci.chans+c])*SPECHEIGHT/2; // invert and scale to fit display
				if (v<0) v=0;
				else if (v>=SPECHEIGHT) v=SPECHEIGHT-1;
				if (!x) y=v;
				do { // draw line from previous sample...
					if (y<v) y++;
					else if (y>v) y--;
					specbuf[y*SPECWIDTH+x]=palette[c&1?127:1]; // left=green, right=red (could add more colours to palette for more chans)
				} while (y!=v);
			}
		}
	} else {
		float fft[1024]; // get the FFT data
		BASS_ChannelGetData(chan,fft,BASS_DATA_FFT2048);

		if (!specmode) { // "normal" FFT
			memset(specbuf,0,sizeof(specbuf));
			for (x=0;x<SPECWIDTH/2;x++) {
#if 1
				y=sqrt(fft[x+1])*3*SPECHEIGHT-4; // scale it (sqrt to make low values more visible)
#else
				y=fft[x+1]*10*SPECHEIGHT; // scale it (linearly)
#endif
				if (y>SPECHEIGHT) y=SPECHEIGHT; // cap it
				if (x && (y1=(y+y1)/2)) // interpolate from previous to make the display smoother
					while (--y1>=0) specbuf[(SPECHEIGHT-1-y1)*SPECWIDTH+x*2-1]=palette[y1+1];
				y1=y;
				while (--y>=0) specbuf[(SPECHEIGHT-1-y)*SPECWIDTH+x*2]=palette[y+1]; // draw level
			}
		} else if (specmode==1) { // logarithmic, acumulate & average bins
			int b0=0;
			memset(specbuf,0,sizeof(specbuf));
#define BANDS 28
			for (x=0;x<BANDS;x++) {
				float peak=0;
				int b1=pow(2,x*10.0/(BANDS-1));
				if (b1>1023) b1=1023;
				if (b1<=b0) b1=b0+1; // make sure it uses at least 1 FFT bin
				for (;b0<b1;b0++)
					if (peak<fft[1+b0]) peak=fft[1+b0];
				y=sqrt(peak)*3*SPECHEIGHT-4; // scale it (sqrt to make low values more visible)
				if (y>SPECHEIGHT) y=SPECHEIGHT; // cap it
				while (--y>=0)
					for (y1=0;y1<SPECWIDTH/BANDS-2;y1++)
						specbuf[(SPECHEIGHT-1-y)*SPECWIDTH+x*(SPECWIDTH/BANDS)+y1]=palette[y+1]; // draw bar
			}
		} else { // "3D"
			for (x=0;x<SPECHEIGHT;x++) {
				y=sqrt(fft[x+1])*3*127; // scale it (sqrt to make low values more visible)
				if (y>127) y=127; // cap it
				specbuf[(SPECHEIGHT-1-x)*SPECWIDTH+specpos]=palette[128+y]; // plot it
			}
			// move marker onto next position
			specpos=(specpos+1)%SPECWIDTH;
			for (x=0;x<SPECHEIGHT;x++) specbuf[x*SPECWIDTH+specpos]=palette[255];
		}
	}

	// update the display
	static const Rect r={0,0,SPECHEIGHT,SPECWIDTH};
	InvalWindowRect(win,&r);
}

static pascal OSStatus EventWindowDrawContent(EventHandlerCallRef callRef, EventRef event, void *userData)
{ // this stuff requires OSX 10.4
	CGContextRef cgc;
    QDBeginCGContext(GetWindowPort(win),&cgc);
	CGImageRef cgi=CGBitmapContextCreateImage(specdc);
    CGRect cr=CGRectMake(0,0,SPECWIDTH,SPECHEIGHT);
	CGContextDrawImage(cgc,cr,cgi);
    CGImageRelease(cgi);
    QDEndCGContext(GetWindowPort(win),&cgc);
	return noErr;
}

static pascal OSStatus EventWindowClick(EventHandlerCallRef callRef, EventRef event, void *userData)
{
	specmode=(specmode+1)%4; // swap spectrum mode
	memset(specbuf,0,sizeof(specbuf));	// clear display
	return noErr;
}

int main(int argc, char* argv[])
{
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
	Rect wr;
	wr.left=200;
	wr.right=wr.left+SPECWIDTH;
	wr.top=200;
	wr.bottom=wr.top+SPECHEIGHT;
	CreateNewWindow(kMovableModalWindowClass,kWindowStandardHandlerAttribute,&wr,&win);
	SetWindowTitleWithCFString(win,CFSTR("BASS spectrum example (click to toggle mode)"));
	ChangeWindowAttributes(win,kWindowAsyncDragAttribute,kWindowNoAttributes);
	{
		EventTypeSpec etype={kEventClassWindow,kEventWindowDrawContent};
		InstallWindowEventHandler(win,NewEventHandlerUPP(EventWindowDrawContent),1,&etype,0,NULL);
		etype.eventKind=kEventWindowHandleContentClick;
		InstallWindowEventHandler(win,NewEventHandlerUPP(EventWindowClick),1,&etype,0,NULL);
	}

	// create the bitmap
	CGColorSpaceRef colorSpace=CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	specdc=CGBitmapContextCreate(specbuf,SPECWIDTH,SPECHEIGHT,8,SPECWIDTH*4,colorSpace,kCGImageAlphaNoneSkipLast);
	CGColorSpaceRelease(colorSpace);

	{ // setup palette
		RGBQUAD *pal=(RGBQUAD*)palette;
		int a;
		memset(palette,0,sizeof(palette));
		for (a=1;a<128;a++) {
			pal[a].rgbGreen=256-2*a;
			pal[a].rgbRed=2*a;
		}
		for (a=0;a<32;a++) {
			pal[128+a].rgbBlue=8*a;
			pal[128+32+a].rgbBlue=255;
			pal[128+32+a].rgbRed=8*a;
			pal[128+64+a].rgbRed=255;
			pal[128+64+a].rgbBlue=8*(31-a);
			pal[128+64+a].rgbGreen=8*a;
			pal[128+96+a].rgbRed=255;
			pal[128+96+a].rgbGreen=255;
			pal[128+96+a].rgbBlue=8*a;
		}
	}

	ShowWindow(win);
	
	// setup update timer (40hz)
	EventLoopTimerRef timer;
	InstallEventLoopTimer(GetCurrentEventLoop(),kEventDurationNoWait,kEventDurationSecond/40,NewEventLoopTimerUPP(UpdateSpectrum),0,&timer);

	RunApplicationEventLoop();

    CGContextRelease(specdc);
	DisposeWindow(win);
	BASS_Free();
	return 0;	
}
