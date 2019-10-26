/*
	BASS "live" spectrum analyser example
	Copyright (c) 2002-2010 Un4seen Developments Ltd.
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

// update the spectrum display - the interesting bit :)
pascal void UpdateSpectrum(EventLoopTimerRef inTimer, void *inUserData)
{
	int x,y,y1;

	if (specmode==3) { // waveform
		short buf[SPECWIDTH];
		memset(specbuf,0,sizeof(specbuf));
		BASS_ChannelGetData(chan,buf,SPECWIDTH*sizeof(short)); // get the sample data
		for (x=0;x<SPECWIDTH;x++) {
			int v=(32767-buf[x])*SPECHEIGHT/65536; // invert and scale to fit display
			if (!x) y=v;
			do { // draw line from previous sample...
				if (y<v) y++;
				else if (y>v) y--;
				specbuf[y*SPECWIDTH+x]=palette[abs(y-SPECHEIGHT/2)*2+1];
			} while (y!=v);
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

// Recording callback - not doing anything with the data
BOOL CALLBACK DuffRecording(HRECORD handle, const void *buffer, DWORD length, void *user)
{
	return TRUE; // continue recording
}

static pascal OSStatus EventWindowDrawContent(EventHandlerCallRef callRef, EventRef event, void *userData)
{ // this stuff requires OSX 10.4
	static DWORD quietcount=0;
	CGContextRef cgc;
    QDBeginCGContext(GetWindowPort(win),&cgc);
	CGImageRef cgi=CGBitmapContextCreateImage(specdc);
    CGRect cr=CGRectMake(0,0,SPECWIDTH,SPECHEIGHT);
	CGContextDrawImage(cgc,cr,cgi);
    CGImageRelease(cgi);
	if (LOWORD(BASS_ChannelGetLevel(chan))<500) { // check if it's quiet
		quietcount++;
		if (quietcount>40 && (quietcount&16)) { // it's been quiet for over a second
			CGContextSetRGBFillColor(cgc,1,1,1,1);
			Point p;
			short b;
			GetThemeTextDimensions(CFSTR("make some noise!"),kThemeSystemFont,0,FALSE,&p,&b);
			Rect r={SPECHEIGHT/3,0,SPECHEIGHT,SPECWIDTH};
			DrawThemeTextBox(CFSTR("make some noise!"),kThemeSystemFont,0,FALSE,&r,teJustCenter,cgc);
		}
	} else
		quietcount=0; // not quiet
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

	// initialize BASS recording (default device)
	if (!BASS_RecordInit(-1)) {
		Error("Can't initialize device");
		return 0;
	}
	// start recording (44100hz mono 16-bit)
	if (!(chan=BASS_RecordStart(44100,1,0,DuffRecording,0))) {
		Error("Can't start recording");
		BASS_RecordFree();
		return 0;
	}

	// create the window
	Rect wr;
	wr.left=200;
	wr.right=wr.left+SPECWIDTH;
	wr.top=200;
	wr.bottom=wr.top+SPECHEIGHT;
	CreateNewWindow(kMovableModalWindowClass,kWindowStandardHandlerAttribute,&wr,&win);
	SetWindowTitleWithCFString(win,CFSTR("BASS \"live\" spectrum (click to toggle mode)"));
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
	BASS_RecordFree();
	return 0;	
}
