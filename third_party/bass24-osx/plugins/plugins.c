/*
	BASS plugin test
	Copyright (c) 2005-2009 Un4seen Developments Ltd.
*/

#include <Carbon/Carbon.h>
#include <mach-o/dyld.h>
#include <glob.h>
#include "bass.h"

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

// translate a CTYPE value to text
const char *GetCTypeString(DWORD ctype, HPLUGIN plugin)
{
	if (plugin) { // using a plugin
		const BASS_PLUGININFO *pinfo=BASS_PluginGetInfo(plugin); // get plugin info
		int a;
		for (a=0;a<pinfo->formatc;a++) {
			if (pinfo->formats[a].ctype==ctype) // found a "ctype" match...
				return pinfo->formats[a].name; // return it's name
		}
	}
	// check built-in stream formats...
	if (ctype==BASS_CTYPE_STREAM_OGG) return "Ogg Vorbis";
	if (ctype==BASS_CTYPE_STREAM_MP1) return "MPEG layer 1";
	if (ctype==BASS_CTYPE_STREAM_MP2) return "MPEG layer 2";
	if (ctype==BASS_CTYPE_STREAM_MP3) return "MPEG layer 3";
	if (ctype==BASS_CTYPE_STREAM_AIFF) return "Audio IFF";
	if (ctype==BASS_CTYPE_STREAM_WAV_PCM) return "PCM WAVE";
	if (ctype==BASS_CTYPE_STREAM_WAV_FLOAT) return "Floating-point WAVE";
	if (ctype&BASS_CTYPE_STREAM_WAV) return "WAVE";
	if (ctype==BASS_CTYPE_STREAM_CA) { // CoreAudio codec
		static char buf[100];
		const TAG_CA_CODEC *codec=(TAG_CA_CODEC*)BASS_ChannelGetTags(chan,BASS_TAG_CA_CODEC); // get codec info
		snprintf(buf,sizeof(buf),"CoreAudio: %s",codec->name);
		return buf;
	}
	return "?";
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
// if someone wants to somehow get the file selector to filter like in the Windows example, that'd be nice ;)
	if (!NavDialogRun(fileDialog)) {
		NavReplyRecord r;
		if (!NavDialogGetReply(fileDialog,&r)) {
			AEKeyword k;
			FSRef fr;
			if (!AEGetNthPtr(&r.selection,1,typeFSRef,&k,NULL,&fr,sizeof(fr),NULL)) {
				char file[256];
				FSRefMakePath(&fr,(BYTE*)file,sizeof(file));
				BASS_StreamFree(chan); // free old stream before opening new
				if (!(chan=BASS_StreamCreateFile(FALSE,file,0,0,BASS_SAMPLE_LOOP|BASS_SAMPLE_FLOAT))) {
					SetControlTitleWithCFString(inUserData,CFSTR("click here to open a file..."));
					{
						ControlRef cref=GetControl(11);
						SetControlData(cref,kControlNoPart,kControlStaticTextTextTag,0,"");
						DrawOneControl(cref);
					}
					SetControl32BitMaximum(GetControl(12),0);
					Error("Can't play the file");
				} else {
					CFStringRef cs=CFStringCreateWithCString(0,file,kCFStringEncodingUTF8);
					SetControlTitleWithCFString(inUserData,cs);
					CFRelease(cs);
					{ // display the file type and length
						QWORD bytes=BASS_ChannelGetLength(chan,BASS_POS_BYTE);
						DWORD time=BASS_ChannelBytes2Seconds(chan,bytes);
						BASS_CHANNELINFO info;
						BASS_ChannelGetInfo(chan,&info);
						sprintf(file,"channel type = %x (%s)\nlength = %llu (%u:%02u)",
							info.ctype,GetCTypeString(info.ctype,info.plugin),bytes,time/60,time%60);
						{
							ControlRef cref=GetControl(11);
							SetControlData(cref,kControlNoPart,kControlStaticTextTextTag,strlen(file),file);
							DrawOneControl(cref);
						}
						SetControl32BitMaximum(GetControl(12),time); // update scroller range
					}
					BASS_ChannelPlay(chan,FALSE);
				}
			}
			NavDisposeReply(&r);
		}
	}
	NavDialogDispose(fileDialog);
    return noErr;
}

pascal void PosEventHandler(ControlHandle control, SInt16 part)
{
	DWORD p=GetControl32BitValue(control);
	BASS_ChannelSetPosition(chan,BASS_ChannelSeconds2Bytes(chan,p),BASS_POS_BYTE);
}

pascal void TimerProc(EventLoopTimerRef inTimer, void *inUserData)
{
	SetControl32BitValue(GetControl(12),(DWORD)BASS_ChannelBytes2Seconds(chan,BASS_ChannelGetPosition(chan,BASS_POS_BYTE))); // update position
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
	IBNibRef nibRef;
	OSStatus err;
	glob_t g;
    
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
	err = CreateNibReference(CFSTR("plugins"), &nibRef);
	if (err) return err;
	err = CreateWindowFromNib(nibRef, CFSTR("Window"), &win);
	if (err) return err;
	DisposeNibReference(nibRef);

	DataBrowserCallbacks dbc;
	dbc.version=kDataBrowserLatestCallbacks;
	InitDataBrowserCallbacks(&dbc);
	dbc.u.v1.itemDataCallback=MyDataBrowserItemDataCallback;
	ControlRef list=GetControl(20);
	SetDataBrowserCallbacks(list,&dbc);

	{ // look for plugins (in the executable directory)
		char path[300];
		DWORD l=sizeof(path);
		_NSGetExecutablePath(path,&l);
		strcpy(strrchr(path,'/')+1,"libbass*.dylib");
		if (!glob(path,0,0,&g)) {
			int a;
			for (a=0;a<g.gl_pathc;a++) {
				if (BASS_PluginLoad(g.gl_pathv[a],0)) { // plugin loaded,  add it to the list...
					char *p=strrchr(g.gl_pathv[a],'/')+1;
					AddDataBrowserItems(list,kDataBrowserNoItem,1,(DataBrowserItemID*)&p,kDataBrowserItemNoProperty);
				}
			}
		}
		DWORD c;
		GetDataBrowserItemCount(list,kDataBrowserNoItem,FALSE,kDataBrowserItemNoState,(DataBrowserItemState*)&c);
		if (!c) { // no plugins...
			static const char *noplugins="no plugins - visit the BASS webpage to get some";
			AddDataBrowserItems(list,kDataBrowserNoItem,1,(DataBrowserItemID*)&noplugins,kDataBrowserItemNoProperty);
		}
	}

	SetupControlHandler(10,kEventControlHit,OpenEventHandler);
	SetControlAction(GetControl(12),NewControlActionUPP(PosEventHandler));

	EventLoopTimerRef timer;
	InstallEventLoopTimer(GetCurrentEventLoop(),kEventDurationNoWait,kEventDurationSecond/2,NewEventLoopTimerUPP(TimerProc),0,&timer);

	ShowWindow(win);
	RunApplicationEventLoop();

	globfree(&g);

	// "free" the output device and all plugins
	BASS_Free();
	BASS_PluginFree(0);

    return 0; 
}
