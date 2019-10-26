/*
	BASS recording example
	Copyright (c) 2002-2009 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include "bass.h"

WindowPtr win;

#define BUFSTEP 200000	// memory allocation unit

int input;				// current input source
char *recbuf=NULL;		// recording buffer
DWORD reclen;			// recording length

HRECORD rchan=0;		// recording channel
HSTREAM chan=0;			// playback channel

#ifdef _BIG_ENDIAN
inline DWORD le_32(DWORD v)
{
	return (v>>24)|((v>>8)&0xff00)|((v&0xff00)<<8)|(v<<24);
}
inline WORD le_16(WORD v)
{
	return (v>>8)|(v<<8);
}
#else
#define le_32(v) (v)
#define le_16(v) (v)
#endif

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

void SetControlText(int id, const char *text)
{
	CFStringRef cs=CFStringCreateWithCString(0,text,0);
	SetControlTitleWithCFString(GetControl(id),cs);
	CFRelease(cs);
}

// buffer the recorded data
BOOL CALLBACK RecordingCallback(HRECORD handle, const void *buffer, DWORD length, void *user)
{
	// increase buffer size if needed
	if ((reclen%BUFSTEP)+length>=BUFSTEP) {
		recbuf=realloc(recbuf,((reclen+length)/BUFSTEP+1)*BUFSTEP);
		if (!recbuf) {
			rchan=0;
//			Error("Out of memory!");
			SetControlText(10,"Record");
			return FALSE; // stop recording
		}
	}
	{ // buffer the data (WAVE is little-endian so swap order)
		short *buf=(short*)(recbuf+reclen);
		int a;
		for (a=0;a<length/2;a++) buf[a]=le_16(((short*)buffer)[a]);
		reclen+=length;
	}
	return TRUE; // continue recording
}

void StartRecording()
{
	WAVEFORMATEX *wf;
	if (recbuf) { // free old recording
		BASS_StreamFree(chan);
		chan=0;
		free(recbuf);
		recbuf=NULL;
		DisableControl(GetControl(11));
		DisableControl(GetControl(12));
	}
	// allocate initial buffer and make space for WAVE header
	recbuf=malloc(BUFSTEP);
	reclen=44;
	// fill the WAVE header
	memcpy(recbuf,"RIFF\0\0\0\0WAVEfmt \20\0\0\0",20);
	memcpy(recbuf+36,"data\0\0\0\0",8);
	wf=(WAVEFORMATEX*)(recbuf+20);
	wf->wFormatTag=le_16(1);
	wf->nChannels=le_16(2);
	wf->wBitsPerSample=le_16(16);
	wf->nSamplesPerSec=le_32(44100);
	wf->nBlockAlign=le_16(4);
	wf->nAvgBytesPerSec=le_32(176400);
	// start recording @ 44100hz 16-bit stereo
	if (!(rchan=BASS_RecordStart(44100,2,0,&RecordingCallback,0))) {
		Error("Couldn't start recording");
		free(recbuf);
		recbuf=0;
		return;
	}
	SetControlText(10,"Stop");
}

void StopRecording()
{
	BASS_ChannelStop(rchan);
	rchan=0;
	SetControlText(10,"Record");
	// complete the WAVE header
	*(DWORD*)(recbuf+4)=le_32(reclen-8);
	*(DWORD*)(recbuf+40)=le_32(reclen-44);
	// create a stream from the recording
	if (chan=BASS_StreamCreateFile(TRUE,recbuf,0,reclen,0)) {
		// enable "play" & "save" buttons
		EnableControl(GetControl(11));
		EnableControl(GetControl(12));
	} else
		Error("Can't stream recorded data");
}

// write the recorded data to disk
void WriteToDisk()
{
	NavDialogRef fileDialog;
	NavDialogCreationOptions fo;
	NavGetDefaultDialogCreationOptions(&fo);
	fo.optionFlags=0;
	fo.parentWindow=win;
	fo.saveFileName=CFSTR("bass.wav");
	NavCreatePutFileDialog(&fo,0,0,NULL,NULL,&fileDialog);
	if (!NavDialogRun(fileDialog)) {
		NavReplyRecord r;
		if (!NavDialogGetReply(fileDialog,&r)) {
			AEKeyword k;
			FSRef fr;
			if (!AEGetNthPtr(&r.selection,1,typeFSRef,&k,NULL,&fr,sizeof(fr),NULL)) {
				FILE *fp;
				char file[256],*fe;
				FSRefMakePath(&fr,(BYTE*)file,sizeof(file));
				fe=strchr(file,0);
				*fe++='/';
				CFStringGetCString(r.saveFileName,fe,file+sizeof(file)-fe,kCFStringEncodingUTF8);
				if (fp=fopen(file,"wb")) {
					fwrite(recbuf,reclen,1,fp);
					fclose(fp);
				} else 
					Error("Can't create the file");
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
}

void UpdateInputInfo()
{
	float level;
	int it=BASS_RecordGetInput(input,&level); // get info on the input
	if (it==-1 || level<0) { // failed to get level
		level=1; // just display 100%
		DisableControl(GetControl(14));
	} else
		EnableControl(GetControl(14));
	SetControl32BitValue(GetControl(14),level*100); // set the level slider
}

pascal OSStatus RecordEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	if (!rchan)
		StartRecording();
	else
		StopRecording();
	return noErr;
}

pascal OSStatus PlayEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	BASS_ChannelPlay(chan,TRUE); // play the recorded data
	return noErr;
}

pascal OSStatus SaveEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	WriteToDisk();
	return noErr;
}

pascal OSStatus DeviceEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	input=GetControl32BitValue(inUserData)-1; // get the selection
	BASS_RecordSetInput(input,BASS_INPUT_ON,-1); // enable the selected input
	UpdateInputInfo(); // update info
	return noErr;
}

pascal void VolumeEventHandler(ControlHandle control, SInt16 part)
{ // set input source level
	float level=GetControl32BitValue(control)/100.f;
	BASS_RecordSetInput(input,0,level);
}

pascal void TimerProc(EventLoopTimerRef inTimer, void *inUserData)
{ // update the recording/playback counter
	char text[30]="";
	if (rchan) // recording
		sprintf(text,"%llu",BASS_ChannelGetPosition(rchan,BASS_POS_BYTE));
	else if (chan) {
		if (BASS_ChannelIsActive(chan)) // playing
			sprintf(text,"%llu / %llu",BASS_ChannelGetPosition(chan,BASS_POS_BYTE),BASS_ChannelGetLength(chan,BASS_POS_BYTE));
		else
			sprintf(text,"%llu",BASS_ChannelGetLength(chan,BASS_POS_BYTE));
	}
	{
		ControlRef cref=GetControl(20);
		SetControlData(cref,kControlNoPart,kControlStaticTextTextTag,strlen(text),text);
		DrawOneControl(cref);
	}
}

int main(int argc, char* argv[])
{
	IBNibRef nibRef;
	OSStatus err;
    
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	// setup recording and output devices (using default devices)
	if (!BASS_RecordInit(-1) || !BASS_Init(-1,44100,0,win,NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	// Create Window and stuff
	err = CreateNibReference(CFSTR("rectest"), &nibRef);
	if (err) return err;
	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);

	{ // get list of inputs
		int c;
		const char *i;
		ControlRef cref=GetControl(13);
		MenuRef menu=GetControlPopupMenuHandle(cref);
		DeleteMenuItem(menu,1); // remove placeholder
		for (c=0;i=BASS_RecordGetInputName(c);c++) {
			CFStringRef cs=CFStringCreateWithCString(0,i,0);
			AppendMenuItemTextWithCFString(menu,cs,0,0,0);
			CFRelease(cs);
			if (!(BASS_RecordGetInput(c,NULL)&BASS_INPUT_OFF)) // this 1 is currently "on"
				input=c;
		}
		if (!c) { // no input controls
			AppendMenuItemTextWithCFString(menu,CFSTR("no controls"),0,0,0);
			DisableControl(cref);
		} else {
			SetControl32BitMaximum(cref,c);
			SetControl32BitValue(cref,input+1);
			UpdateInputInfo(); // display info
		}
	}

	SetupControlHandler(10,kEventControlHit,RecordEventHandler);
	SetupControlHandler(11,kEventControlHit,PlayEventHandler);
	SetupControlHandler(12,kEventControlHit,SaveEventHandler);
	SetupControlHandler(13,kEventControlValueFieldChanged,DeviceEventHandler);
	SetControlAction(GetControl(14),NewControlActionUPP(VolumeEventHandler));

	EventLoopTimerRef timer;
	InstallEventLoopTimer(GetCurrentEventLoop(),kEventDurationNoWait,kEventDurationSecond/5,NewEventLoopTimerUPP(TimerProc),0,&timer);

	ShowWindow(win);
	RunApplicationEventLoop();

	// release all BASS stuff
	BASS_RecordFree();
	BASS_Free();

    return 0; 
}
