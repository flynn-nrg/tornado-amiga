/*
	BASS 3D test
	Copyright (c) 1999-2012 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include <math.h>
#include "bass.h"

WindowPtr win;

// channel (sample/music) info structure
typedef struct {
	DWORD channel;			// the channel
	BASS_3DVECTOR pos,vel;	// position,velocity
} Channel;

Channel *chans=NULL;		// the channels
int chanc=0,chan=-1;		// number of channels, current channel

char **filelist=NULL;

int GetChannel(const char *entry)
{
	int a;
	for (a=0;a<chanc;a++)
		if (entry==filelist[a]) return a;
	return -1;
}

#define TIMERPERIOD	50		// timer period (ms)
#define MAXDIST		50		// maximum distance of the channels (m)
#define SPEED		12		// speed of the channels' movement (m/s)

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

// Update the button states
void UpdateButtons()
{
	int a;
	for (a=12;a<=17;a++) {
		if (chan==-1)
			DeactivateControl(GetControl(a));
		else
			ActivateControl(GetControl(a));
	}
	if (chan!=-1) {
		char t[20];
		sprintf(t,"%d",abs((int)chans[chan].vel.x));
		SetControlData(GetControl(15),0,kControlEditTextTextTag,strlen(t),t);
		sprintf(t,"%d",abs((int)chans[chan].vel.z));
		SetControlData(GetControl(16),0,kControlEditTextTextTag,strlen(t),t);
	}
}

OSStatus ChannelsItemDataCallback(ControlRef browser, DataBrowserItemID item, DataBrowserPropertyID property, DataBrowserItemDataRef itemData, Boolean setValue)
{
	if (!setValue && property=='blah') {
		CFStringRef cs=CFStringCreateWithCString(0,(char*)item,kCFStringEncodingUTF8);
		SetDataBrowserItemDataText(itemData,cs);
		CFRelease(cs);
	}
	return noErr;
}

void ChannelItemNotificationCallback(ControlRef browser, DataBrowserItemID item, DataBrowserItemNotification message)
{
	if (message==kDataBrowserItemSelected) {
		chan=GetChannel((char*)item);
		UpdateButtons();
	}
}

pascal OSStatus AddEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
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
				DWORD newchan;
				char file[256];
				FSRefMakePath(&fr,(BYTE*)file,sizeof(file));
				if ((newchan=BASS_MusicLoad(FALSE,file,0,0,BASS_MUSIC_RAMP|BASS_MUSIC_LOOP|BASS_MUSIC_3D,1))
					|| (newchan=BASS_SampleLoad(FALSE,file,0,0,1,BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_MONO|BASS_SAMPLE_MUTEMAX))) {
					chanc++;
					chans=(Channel*)realloc((void*)chans,chanc*sizeof(Channel));
					memset(&chans[chanc-1],0,sizeof(Channel));
					chans[chanc-1].channel=newchan;
					BASS_SampleGetChannel(newchan,FALSE); // initialize sample channel
					filelist=realloc(filelist,chanc*sizeof(*filelist));
					filelist[chanc-1]=strdup(strrchr(file,'/')+1);
					AddDataBrowserItems(GetControl(10),kDataBrowserNoItem,1,(DataBrowserItemID*)&filelist[chanc-1],kDataBrowserItemNoProperty);
				} else
					Error("Can't load file (note samples must be mono)");
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
    return noErr;
}

pascal OSStatus RemEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	RemoveDataBrowserItems(GetControl(10),kDataBrowserNoItem,1,(DataBrowserItemID*)&filelist[chan],kDataBrowserItemNoProperty);

	BASS_SampleFree(chans[chan].channel);
	BASS_MusicFree(chans[chan].channel);
	free(filelist[chan]);
	chanc--;
	memcpy(chans+chan,chans+chan+1,(chanc-chan)*sizeof(*chans));
	memcpy(filelist+chan,filelist+chan+1,(chanc-chan)*sizeof(*filelist));
	chan=-1;
	UpdateButtons();
    return noErr;
}

pascal OSStatus PlayEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	BASS_ChannelPlay(chans[chan].channel,FALSE);
    return noErr;
}

pascal OSStatus StopEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	BASS_ChannelPause(chans[chan].channel);
    return noErr;
}

pascal OSStatus ResetEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{ // reset the position and velocity to 0
	memset(&chans[chan].pos,0,sizeof(chans[chan].pos));
	memset(&chans[chan].vel,0,sizeof(chans[chan].vel));
	UpdateButtons();
    return noErr;
}

pascal OSStatus VelEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
	char t[10]="";
	int a;
	GetControlData(inUserData,0,kControlEditTextTextTag,sizeof(t)-1,t,(Size*)&a);
	a=atoi(t);
	if (a>99) a=99;
	sprintf(t,"%d",a);
	SetControlData(inUserData,0,kControlEditTextTextTag,strlen(t),t);
	{ // update channel velocity...
		ControlID cid;
		GetControlID(inUserData,&cid);
		if (cid.id==15) { // X
			if (abs((int)chans[chan].vel.x)!=a) chans[chan].vel.x=a;
		} else { // Z
			if (abs((int)chans[chan].vel.z)!=a) chans[chan].vel.z=a;
		}
	}
    return noErr;
}

pascal void FactorEventHandler(ControlHandle control, SInt16 part)
{
	int pos=GetControl32BitValue(control);
	ControlID cid;
	GetControlID(control,&cid);
	if (cid.id==20) // change the rolloff factor
		BASS_Set3DFactors(-1,pow(2,(pos-10)/5.0),-1);
	else // change the doppler factor
		BASS_Set3DFactors(-1,-1,pow(2,(pos-10)/5.0));
}

pascal OSStatus DrawEventHandler(EventHandlerCallRef callRef, EventRef inEvent, void *inUserData)
{
	CGContextRef context;
	CGRect bounds;
	int c,cx,cy;

	GetEventParameter(inEvent, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(context), NULL, &context);

	HIViewGetBounds(inUserData,&bounds);
	cx=bounds.size.width/2;
	cy=bounds.size.height/2;
	
	CGContextSetRGBFillColor(context,1,1,1,1);
	CGContextFillRect(context,bounds);
	CGContextSetRGBStrokeColor(context,0,0,0,1);

	{
		CGRect r={{cx-4,cy-4},{8,8}};
		CGContextSetRGBFillColor(context,0.5,0.5,0.5,1);
		CGContextFillEllipseInRect(context,r);
		CGContextStrokeEllipseInRect(context,r);
	}

	for (c=0;c<chanc;c++) {
		if (BASS_ChannelIsActive(chans[c].channel)==BASS_ACTIVE_PLAYING) {
			if (chans[c].pos.z>=MAXDIST || chans[c].pos.z<=-MAXDIST)
				chans[c].vel.z=-chans[c].vel.z;
			if (chans[c].pos.x>=MAXDIST || chans[c].pos.x<=-MAXDIST)
				chans[c].vel.x=-chans[c].vel.x;
			chans[c].pos.z+=chans[c].vel.z*TIMERPERIOD/1000;
			chans[c].pos.x+=chans[c].vel.x*TIMERPERIOD/1000;
			BASS_ChannelSet3DPosition(chans[c].channel,&chans[c].pos,NULL,&chans[c].vel);
		}
		CGRect r={{cx+(int)((cx-10)*chans[c].pos.x/MAXDIST)-4,cy-(int)((cy-10)*chans[c].pos.z/MAXDIST)-4},{8,8}};
		if (chan==c)
			CGContextSetRGBFillColor(context,1,0,0,1);
		else
			CGContextSetRGBFillColor(context,1,1,1,1);
		CGContextFillEllipseInRect(context,r);
		CGContextStrokeEllipseInRect(context,r);
	}
	BASS_Apply3D();
	return noErr;
}

pascal void Update(EventLoopTimerRef inTimer, void *inUserData)
{
	HIViewSetNeedsDisplay(GetControl(30),true); // update the display
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

	err = CreateNibReference(CFSTR("3dtest"), &nibRef);
	if (err) return err;

	// Initialize the output device with 3D support
	if (!BASS_Init(-1,44100,BASS_DEVICE_3D,NULL,NULL)) {
		Error("Can't initialize output device");
		return 0;
	}

	{
		BASS_INFO bi;
		BASS_GetInfo(&bi);
		if (bi.speakers>2) {
			short i;
			DialogRef alert;
			AlertStdCFStringAlertParamRec alertparam;
			GetStandardAlertDefaultParams(&alertparam,kStdCFStringAlertVersionOne);
			alertparam.cancelButton=kAlertStdAlertCancelButton;
			alertparam.defaultText=CFSTR("Yes");
			alertparam.cancelText=CFSTR("No");
			CreateStandardAlert(0,CFSTR("Multiple speakers were detected."),CFSTR("Would you like to use them?"),&alertparam,&alert);
			RunStandardAlert(alert,NULL,&i);
			if (i==kStdCancelItemIndex) BASS_SetConfig(BASS_CONFIG_3DALGORITHM,BASS_3DALG_OFF);
		}
	}
	
	// Use meters as distance unit, real world rolloff, real doppler effect
	BASS_Set3DFactors(1,1,1);
	
	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);

	DataBrowserCallbacks dbc;
	dbc.version=kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks(&dbc);
	dbc.u.v1.itemDataCallback=ChannelsItemDataCallback;
	dbc.u.v1.itemNotificationCallback=ChannelItemNotificationCallback;
	SetDataBrowserCallbacks(GetControl(10),&dbc);

	SetupControlHandler(11,kEventControlHit,AddEventHandler);
	SetupControlHandler(12,kEventControlHit,RemEventHandler);
	SetupControlHandler(13,kEventControlHit,PlayEventHandler);
	SetupControlHandler(14,kEventControlHit,StopEventHandler);
	SetupControlHandler(17,kEventControlHit,ResetEventHandler);
	SetupControlHandler(30,kEventControlDraw,DrawEventHandler);

	SetControlAction(GetControl(20),NewControlActionUPP(FactorEventHandler));
	SetControlAction(GetControl(21),NewControlActionUPP(FactorEventHandler));

	EventTypeSpec etype={kEventClassTextField,kEventTextAccepted};
	ControlRef cref=GetControl(15);
	InstallControlEventHandler(cref,NewEventHandlerUPP(VelEventHandler),1,&etype,cref,NULL);
	cref=GetControl(16);
	InstallControlEventHandler(cref,NewEventHandlerUPP(VelEventHandler),1,&etype,cref,NULL);

	UpdateButtons();

	// setup update timer
	EventLoopTimerRef timer;
	InstallEventLoopTimer(GetCurrentEventLoop(),kEventDurationNoWait,kEventDurationSecond*TIMERPERIOD/1000,NewEventLoopTimerUPP(Update),0,&timer);

	ShowWindow(win);
	RunApplicationEventLoop();

	BASS_Free();

    return 0; 
}
