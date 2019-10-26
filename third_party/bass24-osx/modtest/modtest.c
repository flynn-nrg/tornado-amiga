/*
	BASS MOD music test
	Copyright (c) 1999-2014 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include <stdio.h>
#include "bass.h"

WindowPtr win;

DWORD music;	// the HMUSIC channel

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
    
void SetStaticText(int id, const char *text)
{
	ControlRef cref=GetControl(id);
	SetControlData(cref,kControlNoPart,kControlStaticTextTextTag,strlen(text),text);
	DrawOneControl(cref);
}

DWORD GetFlags()
{
	DWORD flags=BASS_MUSIC_POSRESET; // stop notes when seeking
	switch (GetControl32BitValue(GetControl(21))-1) {
		case 0:
			flags|=BASS_MUSIC_NONINTER; // no interpolation
			break;
		case 2:
			flags|=BASS_MUSIC_SINCINTER; // sinc interpolation
			break;
	}
	switch (GetControl32BitValue(GetControl(22))-1) {
		case 1:
			flags|=BASS_MUSIC_RAMP; // ramping
			break;
		case 2:
			flags|=BASS_MUSIC_RAMPS; // "sensitive" ramping
			break;
	}
	switch (GetControl32BitValue(GetControl(23))-1) {
		case 1:
			flags|=BASS_MUSIC_SURROUND; // surround
			break;
		case 2:
			flags|=BASS_MUSIC_SURROUND2; // "mode2"
			break;
	}
	return flags;
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
				BASS_MusicFree(music); // free the current music
				music=BASS_MusicLoad(FALSE,file,0,0,GetFlags(),1); // load the new music
				if (music) { // success
					DWORD length=BASS_ChannelGetLength(music,BASS_POS_MUSIC_ORDER); // get the order length
					CFStringRef cs=CFStringCreateWithCString(0,file,kCFStringEncodingUTF8);
					SetControlTitleWithCFString(inUserData,cs);
					CFRelease(cs);
					{
						char text[100],*ctype="";
						BASS_CHANNELINFO info;
						int channels=0;
						while (BASS_ChannelGetAttributeEx(music,BASS_ATTRIB_MUSIC_VOL_CHAN+channels,0,0)) channels++; // count channels
						BASS_ChannelGetInfo(music,&info);
						switch (info.ctype&~BASS_CTYPE_MUSIC_MO3) {
							case BASS_CTYPE_MUSIC_MOD:
								ctype="MOD";
								break;
							case BASS_CTYPE_MUSIC_MTM:
								ctype="MTM";
								break;
							case BASS_CTYPE_MUSIC_S3M:
								ctype="S3M";
								break;
							case BASS_CTYPE_MUSIC_XM:
								ctype="XM";
								break;
							case BASS_CTYPE_MUSIC_IT:
								ctype="IT";
								break;
						}
						snprintf(text,sizeof(text),"name: %s, format: %dch %s%s",BASS_ChannelGetTags(music,BASS_TAG_MUSIC_NAME),channels,ctype,info.ctype&BASS_CTYPE_MUSIC_MO3?" (MO3)":"");
						SetStaticText(11,text);
					}
					SetControl32BitMaximum(GetControl(20),length-1); // update scroller range
					BASS_ChannelPlay(music,FALSE); // start it
				} else { // failed
					SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
					SetStaticText(11,"");
					SetStaticText(15,"");
					Error("Can't play the file");
				}
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
    return noErr;
}

pascal OSStatus PlayEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	if (BASS_ChannelIsActive(music)==BASS_ACTIVE_PLAYING)
		BASS_ChannelPause(music);
	else
		BASS_ChannelPlay(music,FALSE);
    return noErr;
}

pascal void PosEventHandler(ControlHandle control, SInt16 part)
{
	DWORD pos=GetControl32BitValue(control);
	BASS_ChannelSetPosition(music,pos,BASS_POS_MUSIC_ORDER); // set the position
}

pascal OSStatus FlagEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	BASS_ChannelFlags(music,GetFlags(),-1); // update flags
	return noErr;
}

pascal void TimerProc(EventLoopTimerRef inTimer, void *inUserData)
{ // update display
	char text[16];
	QWORD pos=BASS_ChannelGetPosition(music,BASS_POS_MUSIC_ORDER);
	if (pos!=(QWORD)-1) {
		SetControl32BitValue(GetControl(20),LOWORD(pos)); // update position
		sprintf(text,"%03d.%03d",LOWORD(pos),HIWORD(pos));
		SetStaticText(15,text);
	}
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
	if (!BASS_Init(-1,44100,0,NULL,NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	// Create Window and stuff
	err = CreateNibReference(CFSTR("modtest"), &nibRef);
	if (err) return err;
	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);

	SetupControlHandler(10,kEventControlHit,OpenEventHandler);
	SetupControlHandler(12,kEventControlHit,PlayEventHandler);
	SetControlAction(GetControl(20),NewControlActionUPP(PosEventHandler));
	SetupControlHandler(21,kEventControlValueFieldChanged,FlagEventHandler);
	SetupControlHandler(22,kEventControlValueFieldChanged,FlagEventHandler);
	SetupControlHandler(23,kEventControlValueFieldChanged,FlagEventHandler);

	EventLoopTimerRef timer;
	InstallEventLoopTimer(GetCurrentEventLoop(),kEventDurationNoWait,kEventDurationSecond/10,NewEventLoopTimerUPP(TimerProc),0,&timer);

	ShowWindow(win);
	RunApplicationEventLoop();

	BASS_Free();

    return 0; 
}
