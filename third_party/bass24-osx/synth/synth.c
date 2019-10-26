/*
	BASS simple synth
	Copyright (c) 2001-2017 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include <stdio.h>
#include <math.h>
#include "bass.h"

WindowPtr win;

BASS_INFO info;
HSTREAM stream;	// the stream

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define KEYS 20
const CGKeyCode keys[KEYS]={
	kVK_ANSI_Q,kVK_ANSI_2,kVK_ANSI_W,kVK_ANSI_3,kVK_ANSI_E,kVK_ANSI_R,kVK_ANSI_5,kVK_ANSI_T,kVK_ANSI_6,kVK_ANSI_Y,kVK_ANSI_7,kVK_ANSI_U,
	kVK_ANSI_I,kVK_ANSI_9,kVK_ANSI_O,kVK_ANSI_0,kVK_ANSI_P,kVK_ANSI_LeftBracket,kVK_ANSI_Equal,kVK_ANSI_RightBracket
};
#define MAXVOL 0.22
#define DECAY (MAXVOL/4000)
float vol[KEYS]={0},pos[KEYS]; // keys' volume and pos

const DWORD fxtype[5]={BASS_FX_DX8_CHORUS,BASS_FX_DX8_DISTORTION,BASS_FX_DX8_ECHO,BASS_FX_DX8_FLANGER,BASS_FX_DX8_REVERB};
HFX fx[5]={0}; // effect handles

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

DWORD CALLBACK WriteStream(HSTREAM handle, float *buffer, DWORD length, void *user)
{
	int k,c;
	float omega,s;
	memset(buffer,0,length);
	for (k=0;k<KEYS;k++) {
		if (!vol[k]) continue;
		omega=2*M_PI*pow(2.0,(k+3)/12.0)*440.0/info.freq;
		for (c=0;c<length/sizeof(float);c+=2) {
			buffer[c]+=sin(pos[k])*vol[k];
			buffer[c+1]=buffer[c]; // left and right channels are the same
			pos[k]+=omega;
			if (vol[k]<MAXVOL) {
				vol[k]-=DECAY;
				if (vol[k]<=0) { // faded-out
					vol[k]=0;
					break;
				}
			}
		}
		pos[k]=fmod(pos[k],2*M_PI);
	}
	return length;
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

pascal OSStatus FXEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	DWORD on=GetControl32BitValue(inUserData);
	ControlID cid;
	GetControlID(inUserData,&cid);
	if (!on) {
		BASS_ChannelRemoveFX(stream,fx[cid.id-10]);
		fx[cid.id-10]=0;
	} else
		fx[cid.id-10]=BASS_ChannelSetFX(stream,fxtype[cid.id-10],0);
	return noErr;
}

static OSStatus KeyEventHandler(EventHandlerCallRef inCaller, EventRef inEvent, void *inUserData)
{
	UInt32 kc=0;
	GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &kc);
	int key;
	for (key=0;key<KEYS;key++)
		if (kc==keys[key]) {
			bool down=GetEventKind(inEvent)==kEventRawKeyDown;
			if (down && vol[key]<MAXVOL) {
				pos[key]=0;
				vol[key]=MAXVOL+DECAY/2; // start key (setting "vol" slightly higher than MAXVOL to cover any rounding-down)
			} else if (!down && vol[key])
				vol[key]-=DECAY; // trigger key fadeout
			break;
		}
	return 0;
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

	// initialize default output device
	if (!BASS_Init(-1,44100,0,0,NULL))
		Error("Can't initialize device");

	// Create Window and stuff
	err = CreateNibReference(CFSTR("synth"), &nibRef);
	if (err) return err;
	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);
	
	BASS_GetInfo(&info);
	stream=BASS_StreamCreate(info.freq,2,BASS_SAMPLE_FLOAT,(STREAMPROC*)WriteStream,0); // create a stream (stereo for effects)
	BASS_ChannelSetAttribute(stream,BASS_ATTRIB_BUFFER,0); // no buffering for minimum latency
	BASS_ChannelPlay(stream,FALSE); // start it

	SetupControlHandler(10,kEventControlHit,FXEventHandler);
	SetupControlHandler(11,kEventControlHit,FXEventHandler);
	SetupControlHandler(12,kEventControlHit,FXEventHandler);
	SetupControlHandler(13,kEventControlHit,FXEventHandler);
	SetupControlHandler(14,kEventControlHit,FXEventHandler);
	
	EventTypeSpec events[]={
		{kEventClassKeyboard,kEventRawKeyUp},
		{kEventClassKeyboard,kEventRawKeyDown},
	};
	EventHandlerRef eh;
    InstallApplicationEventHandler(KeyEventHandler,GetEventTypeCount(events),events,0,&eh);

	ControlFontStyleRec fsr={kControlUseFontMask|kControlUseSizeMask|kControlUseJustMask,ATSFontFamilyFindFromName(CFSTR("Courier"),0),20,0,0,teCenter};
	SetControlFontStyle(GetControl(21),&fsr);
	
	ShowWindow(win);
	RunApplicationEventLoop();
	
	BASS_Free();
	
    return 0; 
}
