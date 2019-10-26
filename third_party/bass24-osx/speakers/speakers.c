/*
	BASS multi-speaker example
	Copyright (c) 2003-2008 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include <stdio.h>
#include "bass.h"

WindowPtr win;

DWORD flags[4]={BASS_SPEAKER_FRONT,BASS_SPEAKER_REAR,BASS_SPEAKER_CENLFE,BASS_SPEAKER_REAR2};
HSTREAM chan[4];

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
				ControlID cid;
				GetControlID(inUserData,&cid);
				int speaker=cid.id-10;
				BASS_StreamFree(chan[speaker]); // free old stream before opening new
				if (!(chan[speaker]=BASS_StreamCreateFile(FALSE,file,0,0,flags[speaker]|BASS_SAMPLE_LOOP))) {
					SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
					Error("Can't play the file");
				} else {
					CFStringRef cs=CFStringCreateWithCString(0,file,kCFStringEncodingUTF8);
					SetControlTitleWithCFString(inUserData,cs);
					CFRelease(cs);
					BASS_ChannelPlay(chan[speaker],FALSE);
				}
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
    return noErr;
}

pascal OSStatus SwapEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	ControlID cid;
	GetControlID(inUserData,&cid);
	int speaker=cid.id-20;
	BASS_CHANNELINFO i;
	{ // swap handles
		HSTREAM temp=chan[speaker];
		chan[speaker]=chan[speaker+1];
		chan[speaker+1]=temp;
	}
	{ // swap text
		CFStringRef temp1,temp2;
		CopyControlTitleAsCFString(GetControl(10+speaker),&temp1);
		CopyControlTitleAsCFString(GetControl(10+speaker+1),&temp2);
		SetControlTitleWithCFString(GetControl(10+speaker),temp2);
		SetControlTitleWithCFString(GetControl(10+speaker+1),temp1);
		CFRelease(temp1);
		CFRelease(temp2);
	}
	// update speaker flags
	BASS_ChannelFlags(chan[speaker],flags[speaker],BASS_SPEAKER_FRONT);
	BASS_ChannelFlags(chan[speaker+1],flags[speaker+1],BASS_SPEAKER_FRONT);
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

	// initialize default device
	if (!BASS_Init(-1,44100,0,NULL,NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	// Create Window and stuff
	err = CreateNibReference(CFSTR("speakers"), &nibRef);
	if (err) return err;
	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);

	{ // check how many speakers the device supports
		BASS_INFO i;
		BASS_GetInfo(&i);
		if (i.speakers<8) {
			DeactivateControl(GetControl(13));
			DeactivateControl(GetControl(22));
		}
		if (i.speakers<6) {
			DeactivateControl(GetControl(12));
			DeactivateControl(GetControl(21));
		}
		if (i.speakers<4) {
			DeactivateControl(GetControl(11));
			DeactivateControl(GetControl(20));
		}
	}

	SetupControlHandler(10,kEventControlHit,OpenEventHandler);
	SetupControlHandler(11,kEventControlHit,OpenEventHandler);
	SetupControlHandler(12,kEventControlHit,OpenEventHandler);
	SetupControlHandler(13,kEventControlHit,OpenEventHandler);
	SetupControlHandler(20,kEventControlHit,SwapEventHandler);
	SetupControlHandler(21,kEventControlHit,SwapEventHandler);
	SetupControlHandler(22,kEventControlHit,SwapEventHandler);
	SetupControlHandler(23,kEventControlHit,SwapEventHandler);

	ShowWindow(win);
	RunApplicationEventLoop();

	BASS_Free();

    return 0; 
}
