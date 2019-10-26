/*
	BASS simple DSP test
	Copyright (c) 2000-2012 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include <stdio.h>
#include <math.h>
#include "bass.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

WindowPtr win;

DWORD chan;	// the channel... HMUSIC or HSTREAM

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


// "rotate"
HDSP rotdsp=0;	// DSP handle
float rotpos;	// cur.pos
void CALLBACK Rotate(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
fprintf(stderr,"dsp: %d\n",length);
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

ControlRef GetControl(int id)
{
	ControlRef cref;
	ControlID cid={0,id};
	GetControlByID(win,&cid,&cref);
	return cref;
}

void SetupControlHandler(int id, DWORD event, EventHandlerProcPtr proc)
{
	EventTypeSpec etype={kEventClassControl,event};
	ControlRef cref=GetControl(id);
	InstallControlEventHandler(cref,NewEventHandlerUPP(proc),1,&etype,cref,NULL);
}
    
pascal OSStatus DSPEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	DWORD on=GetControl32BitValue(inUserData);
	ControlID cid;
	GetControlID(inUserData,&cid);
	switch (cid.id) {
		case 11: // toggle "rotate"
			if (on) {
				rotpos=M_PI/4;
				rotdsp=BASS_ChannelSetDSP(chan,&Rotate,0,2);
			} else
				BASS_ChannelRemoveDSP(chan,rotdsp);
			break;
		case 12: // toggle "echo"
			if (on) {
				memset(echbuf,0,sizeof(echbuf));
				echpos=0;
				echdsp=BASS_ChannelSetDSP(chan,&Echo,0,1);
			} else
				BASS_ChannelRemoveDSP(chan,echdsp);
			break;
		case 13: // toggle "flanger"
			if (on) {
				memset(flabuf,0,sizeof(flabuf));
				flapos=0;
			    flas=FLABUFLEN/2;
			    flasinc=0.002f;
				fladsp=BASS_ChannelSetDSP(chan,&Flange,0,0);
			} else
				BASS_ChannelRemoveDSP(chan,fladsp);
			break;
	}
	return noErr;
}

pascal OSStatus OpenEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
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
				// free both MOD and stream, it must be one of them! :)
				BASS_MusicFree(chan);
				BASS_StreamFree(chan);
				if (!(chan=BASS_StreamCreateFile(FALSE,file,0,0,BASS_SAMPLE_LOOP|BASS_SAMPLE_FLOAT))
					&& !(chan=BASS_MusicLoad(FALSE,file,0,0,BASS_MUSIC_RAMPS|BASS_SAMPLE_LOOP|BASS_SAMPLE_FLOAT,1))) {
					// whatever it is, it ain't playable
					SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
					Error("Can't play the file");
				} else {
					BASS_CHANNELINFO info;
					BASS_ChannelGetInfo(chan,&info);
					if (info.chans!=2) { // only stereo is allowed
						SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
						BASS_MusicFree(chan);
						BASS_StreamFree(chan);
						Error("only stereo sources are supported");
					} else {
						CFStringRef cs=CFStringCreateWithCString(0,file,kCFStringEncodingUTF8);
						SetControlTitleWithCFString(inUserData,cs);
						CFRelease(cs);
						// setup DSPs on new channel and play it
						DSPEventHandler(0,0,GetControl(11));
						DSPEventHandler(0,0,GetControl(12));
						DSPEventHandler(0,0,GetControl(13));
						BASS_ChannelPlay(chan,FALSE);
					}
				}
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
    return noErr;
}

int main(int argc, char* argv[])
{
	IBNibRef 		nibRef;
	OSStatus		err;
    
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	// enable floating-point DSP
	BASS_SetConfig(BASS_CONFIG_FLOATDSP,TRUE);

	// initialize default device
	if (!BASS_Init(-1,44100,0,NULL,NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	// Create Window and stuff
	err = CreateNibReference(CFSTR("dsptest"), &nibRef);
	if (err) return err;
	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);

	SetupControlHandler(10,kEventControlHit,OpenEventHandler);
	SetupControlHandler(11,kEventControlHit,DSPEventHandler);
	SetupControlHandler(12,kEventControlHit,DSPEventHandler);
	SetupControlHandler(13,kEventControlHit,DSPEventHandler);

	ShowWindow(win);
	RunApplicationEventLoop();

	BASS_Free();

    return 0; 
}
