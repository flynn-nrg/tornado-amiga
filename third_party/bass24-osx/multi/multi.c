/*
	BASS multiple output example
	Copyright (c) 2001-2008 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include <stdio.h>
#include "bass.h"

WindowPtr win;

DWORD outdev[2];	// output devices
DWORD latency[2];	// latencies
HSTREAM chan[2];	// the streams

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
				int devn=cid.id-10;
				BASS_StreamFree(chan[devn]); // free old stream before opening new
				BASS_SetDevice(outdev[devn]); // set the device to create stream on
				if (!(chan[devn]=BASS_StreamCreateFile(FALSE,file,0,0,BASS_SAMPLE_LOOP))) {
					SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
					Error("Can't play the file");
				} else {
					CFStringRef cs=CFStringCreateWithCString(0,file,kCFStringEncodingUTF8);
					SetControlTitleWithCFString(inUserData,cs);
					CFRelease(cs);
					BASS_ChannelPlay(chan[devn],FALSE);
				}
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
	return noErr;
}

// Cloning DSP function
void CALLBACK CloneDSP(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
	BASS_StreamPutData((HSTREAM)user,buffer,length); // user = clone
}

pascal OSStatus CloneEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	ControlID cid;
	GetControlID(inUserData,&cid);
	int devn=cid.id-15;
	BASS_CHANNELINFO i;
	if (!BASS_ChannelGetInfo(chan[devn^1],&i)) {
		Error("Nothing to clone");
	} else {
		BASS_StreamFree(chan[devn]); // free old stream
		BASS_SetDevice(outdev[devn]); // set the device to create stream on
		if (!(chan[devn]=BASS_StreamCreate(i.freq,i.chans,i.flags,STREAMPROC_PUSH,0))) { // create a "push" stream
			SetControlTitleWithCFString(GetControl(10+devn),CFSTR("click here to open a file..."));
			Error("Can't create clone");
		} else {
			BASS_ChannelLock(chan[devn^1],TRUE); // lock source stream to synchonise buffer contents
			BASS_ChannelSetDSP(chan[devn^1],CloneDSP,(void*)chan[devn],0); // set DSP to feed data to clone
			{ // copy buffered data to clone
				DWORD d=BASS_ChannelSeconds2Bytes(chan[devn],latency[devn]/1000.f); // playback delay
				DWORD c=BASS_ChannelGetData(chan[devn^1],0,BASS_DATA_AVAILABLE);
				BYTE *buf=(BYTE*)malloc(c);
				c=BASS_ChannelGetData(chan[devn^1],buf,c);
				if (c>d) BASS_StreamPutData(chan[devn],buf+d,c-d);
				free(buf);
			}
			BASS_ChannelLock(chan[devn^1],FALSE); // unlock source stream
			BASS_ChannelPlay(chan[devn],FALSE); // play clone
			SetControlTitleWithCFString(GetControl(10+devn),CFSTR("clone"));
		}
	}
	return noErr;
}

pascal OSStatus SwapEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	{ // swap handles
		HSTREAM temp=chan[0];
		chan[0]=chan[1];
		chan[1]=temp;
	}
	{ // swap text
		CFStringRef cs[2];
		CopyControlTitleAsCFString(GetControl(10),&cs[0]);
		CopyControlTitleAsCFString(GetControl(11),&cs[1]);
		SetControlTitleWithCFString(GetControl(10),cs[1]);
		SetControlTitleWithCFString(GetControl(11),cs[0]);
		CFRelease(cs[0]);
		CFRelease(cs[1]);
	}
	// update the channel devices
	BASS_ChannelSetDevice(chan[0],outdev[0]);
	BASS_ChannelSetDevice(chan[1],outdev[1]);
	return noErr;
}

// Simple device selector dialog stuff begins here
pascal OSStatus SelectorEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	HICommand command;
	GetEventParameter(inEvent,kEventParamDirectObject,typeHICommand,NULL,sizeof (HICommand),NULL,&command);
	if (command.commandID=='ok  ')
		QuitAppModalLoopForWindow(inUserData);
	return noErr;
}

OSStatus SelectorItemDataCallback(ControlRef browser, DataBrowserItemID item, DataBrowserPropertyID property, DataBrowserItemDataRef itemData, Boolean setValue)
{
	if (!setValue && property=='blah') {
		BASS_DEVICEINFO i;
		BASS_GetDeviceInfo(item,&i);
		CFStringRef cs=CFStringCreateWithCString(0,i.name,0);
		SetDataBrowserItemDataText(itemData,cs);
		CFRelease(cs);
	}
	return noErr;
}

int SelectDevice(IBNibRef nibRef, CFStringRef title)
{
	WindowRef win;
    
	CreateWindowFromNib(nibRef, CFSTR("Selector"), &win);

	SetWindowTitleWithCFString(win,title);

	ControlRef db;
	ControlID cid={0,10};
	GetControlByID(win,&cid,&db);

	DataBrowserCallbacks dbc;
	dbc.version=kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks(&dbc);
	dbc.u.v1.itemDataCallback=SelectorItemDataCallback;
	SetDataBrowserCallbacks(db,&dbc);

	{
		BASS_DEVICEINFO i;
		unsigned long c;
		for (c=1;BASS_GetDeviceInfo(c,&i);c++) { // device 1 = 1st real device
			if (i.flags&BASS_DEVICE_ENABLED) // enabled, so add it...
				AddDataBrowserItems(db,kDataBrowserNoItem,1,&c,kDataBrowserItemNoProperty);
		}
	}

	EventTypeSpec etype={kEventClassCommand, kEventCommandProcess};
	InstallWindowEventHandler(win, NewEventHandlerUPP(SelectorEventHandler), 1, &etype, win, NULL);

	ShowWindow(win);
	RunAppModalLoopForWindow(win);

	DWORD sel;
	GetDataBrowserSelectionAnchor(db,(DataBrowserItemID*)&sel,(DataBrowserItemID*)&sel);

	DisposeWindow(win);
	
	return sel;
}
// Simple device selector dialog stuff ends here

int main(int argc, char* argv[])
{
	IBNibRef nibRef;
	OSStatus err;
    
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) {
		Error("An incorrect version of BASS was loaded");
		return 0;
	}

	err = CreateNibReference(CFSTR("multi"), &nibRef);
	if (err) return err;

	// Let the user choose the output devices
	outdev[0]=SelectDevice(nibRef,CFSTR("Select output device #1"));
	outdev[1]=SelectDevice(nibRef,CFSTR("Select output device #2"));

	{ // setup output devices
		BASS_INFO info;
		if (!BASS_Init(outdev[0],44100,0,NULL,NULL)) {
			Error("Can't initialize device 1");
			return 0;
		}
		BASS_GetInfo(&info);
		latency[0]=info.latency;
		if (!BASS_Init(outdev[1],44100,0,NULL,NULL)) {
			Error("Can't initialize device 2");
			return 0;
		}
		BASS_GetInfo(&info);
		latency[1]=info.latency;
	}
	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);

	{
		BASS_DEVICEINFO i;
		BASS_GetDeviceInfo(outdev[0],&i);
		SetControlText(20,i.name);
		BASS_GetDeviceInfo(outdev[1],&i);
		SetControlText(21,i.name);
	}

	SetupControlHandler(10,kEventControlHit,OpenEventHandler);
	SetupControlHandler(11,kEventControlHit,OpenEventHandler);
	SetupControlHandler(15,kEventControlHit,CloneEventHandler);
	SetupControlHandler(16,kEventControlHit,CloneEventHandler);
	SetupControlHandler(30,kEventControlHit,SwapEventHandler);

	ShowWindow(win);
	RunApplicationEventLoop();

	// release both devices
	BASS_SetDevice(outdev[0]);
	BASS_Free();
	BASS_SetDevice(outdev[1]);
	BASS_Free();

	return 0; 
}
