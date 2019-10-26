/*
	BASS simple playback test
	Copyright (c) 1999-2012 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include "bass.h"

WindowPtr win=NULL;

HSTREAM *strs=NULL;
int strc=0;
HMUSIC *mods=NULL;
int modc=0;
HSAMPLE *sams=NULL;
int samc=0;

char **strlist=NULL;
char **modlist=NULL;
char **samlist=NULL;

int GetStream(const char *entry)
{
	int a;
	for (a=0;a<strc;a++)
		if (entry==strlist[a]) return a;
	return 0;
}

int GetMusic(const char *entry)
{
	int a;
	for (a=0;a<modc;a++)
		if (entry==modlist[a]) return a;
	return 0;
}

int GetSample(const char *entry)
{
	int a;
	for (a=0;a<samc;a++)
		if (entry==samlist[a]) return a;
	return 0;
}

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
				ControlRef list=GetControl(cid.id/10*10);
				switch (cid.id) {
					case 14: // add a stream
						{
							HSTREAM chan;
							if (chan=BASS_StreamCreateFile(FALSE,file,0,0,0)) {
								strc++;
								strs=(HSTREAM*)realloc((void*)strs,strc*sizeof(*strs));
								strs[strc-1]=chan;
								strlist=realloc(strlist,strc*sizeof(*strlist));
								strlist[strc-1]=strdup(strrchr(file,'/')+1);
								AddDataBrowserItems(list,kDataBrowserNoItem,1,(DataBrowserItemID*)&strlist[strc-1],kDataBrowserItemNoProperty);
								DrawOneControl(list);
							} else
								Error("Can't open stream");
						}
						break;
					case 24: // add a music
						{
							HMUSIC chan;
							if (chan=BASS_MusicLoad(FALSE,file,0,0,BASS_MUSIC_RAMP,1)) {
								modc++;
								mods=realloc((void*)mods,modc*sizeof(*mods));
								mods[modc-1]=chan;
								modlist=realloc(modlist,modc*sizeof(*modlist));
								modlist[modc-1]=strdup(strrchr(file,'/')+1);
								AddDataBrowserItems(list,kDataBrowserNoItem,1,(DataBrowserItemID*)&modlist[modc-1],kDataBrowserItemNoProperty);
								DrawOneControl(list);
							} else
								Error("Can't load music");
						}
						break;
					case 32: // add a sample
						{
							HSAMPLE sam;
							/* load a sample from "file" and give it a max of 3 simultaneous
								playings using playback position as override decider */
							if (sam=BASS_SampleLoad(FALSE,file,0,0,3,BASS_SAMPLE_OVER_POS)) {
								samc++;
								sams=realloc((void*)sams,samc*sizeof(*sams));
								sams[samc-1]=sam;
								samlist=realloc(samlist,samc*sizeof(*samlist));
								samlist[samc-1]=strdup(strrchr(file,'/')+1);
								AddDataBrowserItems(list,kDataBrowserNoItem,1,(DataBrowserItemID*)&samlist[samc-1],kDataBrowserItemNoProperty);
								DrawOneControl(list);
							} else
								Error("Can't load sample");
						}
						break;
				}
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
	return noErr;
}

pascal OSStatus RemEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	DWORD sel;
	ControlID cid;
	GetControlID(inUserData,&cid);
	ControlRef list=GetControl(cid.id/10*10);
	if (!GetDataBrowserSelectionAnchor(list,(DataBrowserItemID*)&sel,(DataBrowserItemID*)&sel) && sel) {
		switch (cid.id) {
			case 15: // remove stream
				RemoveDataBrowserItems(list,kDataBrowserNoItem,1,(DataBrowserItemID*)&sel,kDataBrowserItemNoProperty);
				DrawOneControl(list);
				sel=GetStream((char*)sel);
				BASS_StreamFree(strs[sel]);
				free(strlist[sel]);
				strc--;
				memmove(strs+sel,strs+sel+1,(strc-sel)*sizeof(*strs));
				memmove(strlist+sel,strlist+sel+1,(strc-sel)*sizeof(*strlist));
				break;
			case 25: // remove music
				RemoveDataBrowserItems(list,kDataBrowserNoItem,1,(DataBrowserItemID*)&sel,kDataBrowserItemNoProperty);
				DrawOneControl(list);
				sel=GetMusic((char*)sel);
				BASS_MusicFree(mods[sel]);
				free(modlist[sel]);
				modc--;
				memmove(mods+sel,mods+sel+1,(modc-sel)*sizeof(*mods));
				memmove(modlist+sel,modlist+sel+1,(modc-sel)*sizeof(*modlist));
				break;
			case 33: // remove sample
				RemoveDataBrowserItems(list,kDataBrowserNoItem,1,(DataBrowserItemID*)&sel,kDataBrowserItemNoProperty);
				DrawOneControl(list);
				sel=GetSample((char*)sel);
				BASS_SampleFree(sams[sel]);
				free(samlist[sel]);
				samc--;
				memmove(sams+sel,sams+sel+1,(samc-sel)*sizeof(*sams));
				memmove(samlist+sel,samlist+sel+1,(samc-sel)*sizeof(*samlist));
				break;
		}
	}
	return noErr;
}

pascal OSStatus PlayEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	DWORD sel;
	ControlID cid;
	GetControlID(inUserData,&cid);
	if (!GetDataBrowserSelectionAnchor(GetControl(cid.id/10*10),(DataBrowserItemID*)&sel,(DataBrowserItemID*)&sel) && sel) {
		DWORD chan;
		BOOL restart=FALSE;
		switch (cid.id) {
			case 13: // restart stream
				restart=TRUE;
			case 11: // play stream
				chan=strs[GetStream((char*)sel)];
				break;
			case 23: // restart music
				restart=TRUE;
			case 21: // play music
				chan=mods[GetMusic((char*)sel)];
				break;
			case 31: // play sample (at default rate, volume=50%, random pan position)
				chan=BASS_SampleGetChannel(sams[GetSample((char*)sel)],FALSE);
				BASS_ChannelSetAttribute(chan,BASS_ATTRIB_VOL,0.5f);
				BASS_ChannelSetAttribute(chan,BASS_ATTRIB_PAN,((rand()%201)-100)/100.f);
				break;
		}
		if (!BASS_ChannelPlay(chan,restart))
			Error("Can't play channel");
	}
	return noErr;
}

pascal OSStatus StopEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	DWORD sel;
	ControlID cid;
	GetControlID(inUserData,&cid);
	if (!GetDataBrowserSelectionAnchor(GetControl(cid.id/10*10),(DataBrowserItemID*)&sel,(DataBrowserItemID*)&sel) && sel) {
		DWORD chan;
		switch (cid.id) {
			case 12: // stop stream
				chan=strs[GetStream((char*)sel)];
				break;
			case 22: // stop music
				chan=mods[GetMusic((char*)sel)];
				break;
		}
		BASS_ChannelStop(chan);
	}
	return noErr;
}

pascal OSStatus StopAllEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	BASS_Pause(); // pause output
	return noErr;
}

pascal OSStatus ResumeEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	BASS_Start(); // resume output
	return noErr;
}

pascal OSStatus UThreadsEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	BASS_SetConfig(BASS_CONFIG_UPDATETHREADS,GetControl32BitValue(inUserData)?2:1); // set 1 or 2 update threads
	return noErr;
}

pascal void VolumeEventHandler(ControlHandle control, SInt16 part)
{
	DWORD p=GetControl32BitValue(control);
	ControlID cid;
	GetControlID(control,&cid);
	switch (cid.id) {
		case 16:
			BASS_SetConfig(BASS_CONFIG_GVOL_STREAM,p*100); // global stream volume (0-10000)
			break;
		case 26:
			BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC,p*100); // global MOD volume (0-10000)
			break;
		case 34:
			BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE,p*100); // global sample volume (0-10000)
			break;
		case 43:
			BASS_SetVolume(p/100.f); // output volume (0-1)
			break;
	}
}


pascal void TimerProc(EventLoopTimerRef inTimer, void *inUserData)
{	// update the CPU usage % display
	char text[10];
	sprintf(text,"%.2f",BASS_GetCPU());
	{
		ControlRef cref=GetControl(40);
		SetControlData(cref,kControlNoPart,kControlStaticTextTextTag,strlen(text),text);
		DrawOneControl(cref);
	}
}

OSStatus MyDataBrowserItemDataCallback(ControlRef browser, DataBrowserItemID item, DataBrowserPropertyID property, DataBrowserItemDataRef itemData, Boolean setValue)
{
	if (!setValue && property=='blah') {
		CFStringRef cs=CFStringCreateWithCString(0,(char*)item,kCFStringEncodingUTF8);
		SetDataBrowserItemDataText(itemData,cs);
		CFRelease(cs);
	}
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

	// initialize default output device
	if (!BASS_Init(-1,44100,0,win,NULL)) {
		Error("Can't initialize device");
		return 0;
	}

	// create window and stuff
	err = CreateNibReference(CFSTR("basstest"), &nibRef);
	if (err) return err;
	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);

	DataBrowserCallbacks dbc;
	dbc.version=kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks(&dbc);
	dbc.u.v1.itemDataCallback=MyDataBrowserItemDataCallback;
	SetDataBrowserCallbacks(GetControl(10),&dbc);
	SetDataBrowserCallbacks(GetControl(20),&dbc);
	SetDataBrowserCallbacks(GetControl(30),&dbc);

	SetupControlHandler(11,kEventControlHit,PlayEventHandler);
	SetupControlHandler(12,kEventControlHit,StopEventHandler);
	SetupControlHandler(13,kEventControlHit,PlayEventHandler);
	SetupControlHandler(14,kEventControlHit,OpenEventHandler);
	SetupControlHandler(15,kEventControlHit,RemEventHandler);
	SetupControlHandler(21,kEventControlHit,PlayEventHandler);
	SetupControlHandler(22,kEventControlHit,StopEventHandler);
	SetupControlHandler(23,kEventControlHit,PlayEventHandler);
	SetupControlHandler(24,kEventControlHit,OpenEventHandler);
	SetupControlHandler(25,kEventControlHit,RemEventHandler);
	SetupControlHandler(31,kEventControlHit,PlayEventHandler);
	SetupControlHandler(32,kEventControlHit,OpenEventHandler);
	SetupControlHandler(33,kEventControlHit,RemEventHandler);
	SetupControlHandler(41,kEventControlHit,StopAllEventHandler);
	SetupControlHandler(42,kEventControlHit,ResumeEventHandler);
	SetupControlHandler(44,kEventControlHit,UThreadsEventHandler);

	// initialize volume sliders
	SetControl32BitValue(GetControl(16),100);
	SetControl32BitValue(GetControl(26),100);
	SetControl32BitValue(GetControl(34),100);
	SetControl32BitValue(GetControl(43),BASS_GetVolume()*100);
	ControlActionUPP VolumeAUPP=NewControlActionUPP(VolumeEventHandler);
	SetControlAction(GetControl(16),VolumeAUPP);
	SetControlAction(GetControl(26),VolumeAUPP);
	SetControlAction(GetControl(34),VolumeAUPP);
	SetControlAction(GetControl(43),VolumeAUPP);

	EventLoopTimerRef timer;
	InstallEventLoopTimer(GetCurrentEventLoop(),kEventDurationNoWait,kEventDurationSecond/2,NewEventLoopTimerUPP(TimerProc),0,&timer);

	ShowWindow(win);
	RunApplicationEventLoop();

	BASS_Free(); // close output

	return 0; 
}
