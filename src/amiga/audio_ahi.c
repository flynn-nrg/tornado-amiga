/*
Copyright (c) 2019 Miguel Mendez

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software in a
product, an acknowledgment in the product documentation would be appreciated but
is not required.

    2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <devices/ahi.h>
#include <exec/exec.h>
#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "audio_ahi.h"
#include "memory.h"
#include "paula_output.h"
#include "tndo_assert.h"
#include "tornado_settings.h"

// Call the player function 50 times per second.
#define PLAYER_FREQ 50

#define DEFAULT_SAMPLE_FREQ 22050

struct Library *AHIBase;
static struct MsgPort *AHI_mp = 0;
static struct AHIRequest *AHI_io = 0;
static LONG AHI_device = -1;
static struct AHIAudioCtrl *actrl = 0;
static struct IORequest timereq;
static LONG mixfreq = 0;

static char drivername[256];

static struct Hook PlayerHook = {
    0,    0,
    0, //(ULONG (* )()) PlayerFunc,
    NULL, NULL,
};

// Double buffer
struct AHISampleInfo Sample0 = {
    AHIST_S16S,
    0,
    0,
};

struct AHISampleInfo Sample1 = {
    AHIST_S16S,
    0,
    0,
};

// Returns 0 if ok, 1 if an error occurred.
uint32_t audioAhiInit(demoParams *dp) {

  int sampleFreq = DEFAULT_SAMPLE_FREQ;

  switch (dp->audioPeriod) {
  case REPLAY_PERIOD_11025:
    sampleFreq = 11025;
    break;
  case REPLAY_PERIOD_22050:
    sampleFreq = 22050;
    break;
  }

  AHI_mp = CreateMsgPort();
  if (!AHI_mp) {
    fprintf(stderr, "FATAL - Failed to create message port\n");
    return 1;
  }

  AHI_io =
      (struct AHIRequest *)CreateIORequest(AHI_mp, sizeof(struct AHIRequest));
  if (!AHI_io) {
    fprintf(stderr, "FATAL - Failed to create i/o request\n");
    return 1;
  }

  AHI_io->ahir_Version = 4;

  AHI_device = OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *)AHI_io, 0);
  if (AHI_device) {
    fprintf(stderr, "FATAL - Failed to open device %s\n", AHINAME);
    return 1;
  }

  AHIBase = (struct Library *)AHI_io->ahir_Std.io_Device;

  struct AHIAudioModeRequester *req = AHI_AllocAudioRequest(
      AHIR_PubScreenName, NULL, AHIR_TitleText, "Select a mode and rate",
      AHIR_InitialMixFreq, sampleFreq, AHIR_DoMixFreq, TRUE, TAG_DONE);

  if (!req) {
    fprintf(stderr, "FATAL - AHI_AllocAudioRequest() failed\n");
    return 1;
  }

  if (!(AHI_AudioRequest(req, TAG_DONE))) {
    fprintf(stderr, "FATAL - AHI_AudioRequest() failed\n");
    return 1;
  }

  actrl = AHI_AllocAudio(
      AHIA_AudioID, req->ahiam_AudioID, AHIA_MixFreq, req->ahiam_MixFreq,
      AHIA_Channels, 1, AHIA_Sounds, 2, AHIA_PlayerFunc, &PlayerHook,
      AHIA_PlayerFreq, PLAYER_FREQ << 16, AHIA_MinPlayerFreq, PLAYER_FREQ << 16,
      AHIA_MaxPlayerFreq, PLAYER_FREQ << 16, AHIA_UserData, FindTask(NULL),
      TAG_DONE);

  if (!actrl) {
    fprintf(stderr, "FATAL - AHI_AllocAudio() failed\n");
    return 1;
  }

  AHI_ControlAudio(actrl, AHIC_MixFreq_Query, &mixfreq, TAG_DONE);
  AHI_FreeAudioRequest(req);

  int samples = 0;
  if (!AHI_GetAudioAttrs(AHI_INVALID_ID, actrl, AHIDB_Driver, drivername,
                         AHIDB_BufferLen, sizeof(drivername),
                         AHIDB_MaxPlaySamples, &samples, TAG_DONE)) {
    fprintf(stderr, "FATAL - AHI_GetAudioAttrs() failed\n");
    return 1;
  }

  int bufferSize = samples * sampleFreq / mixfreq;

  if (dp->tornadoOptions & VERBOSE_DEBUGGING) {
    fprintf(stderr, "DEBUG - AHI driver: %s, buffer: %i samples\n",
            drivername, samples);
  }

  Sample0.ahisi_Length = bufferSize;
  Sample1.ahisi_Length = bufferSize;
  Sample0.ahisi_Address = tndo_malloc(bufferSize * 2, 0);
  Sample1.ahisi_Address = tndo_malloc(bufferSize * 2, 0);

  tndo_assert(Sample0.ahisi_Address);
  tndo_assert(Sample1.ahisi_Address);

  if (AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, &Sample0, actrl)) {
    fprintf(stderr, "FATAL - AHI_LoadSound() failed\n");
    return 1;
  }

  if (AHI_LoadSound(1, AHIST_DYNAMICSAMPLE, &Sample1, actrl)) {
    fprintf(stderr, "FATAL - AHI_LoadSound() failed\n");
    return 1;
  }

  return 0;
}

void audioAHIEnd() {
  AHI_ControlAudio(actrl, AHIC_Play, FALSE, TAG_DONE);
  AHI_FreeAudio(actrl);
  CloseDevice((struct IORequest *)AHI_io);
  DeleteIORequest((struct IORequest *)AHI_io);
  DeleteMsgPort(AHI_mp);
}
