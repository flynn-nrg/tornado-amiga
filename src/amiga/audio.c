/*
Copyright (c) 2019, Miguel Mendez. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio.h"
#include "memory.h"
#include "mod_replay.h"
#include "paula_output.h"

// WARNING: This code only works on big endian CPUs!!!
WavHeader *parseWavHeader(void *data) {
  int *iPtr = (int *)data;
  char *cPtr = (char *)data;

  // RIFF?
  if (iPtr[0] != 0x52494646) {
    printf("FATAL - This is not a WAV file!\n");
    return 0;
  }

  // WAVE?
  if (iPtr[2] != 0x57415645) {
    printf("FATAL - This is not a WAV file!\n");
    return 0;
  }

  // Search for "fmt " subchunk
  int delta = 12;
  int skip = 0;
  int foundFmt = 0;
  unsigned int *p;

  cPtr += delta;

  do {
    p = (unsigned int *)cPtr;
    if (p[0] == 0x666d7420) {
      foundFmt = 1;
      break;
    }
    // Skip to the next subchunk.
    skip = ((p[1] & 0xff) << 24) | ((p[1] & 0xff00) << 8) |
           ((p[1] & 0xff0000) >> 8) | ((p[1] & 0xff000000) >> 24);
    delta += skip + 8;
    cPtr += skip + 8;

    // Give up after 1024 bytes.
  } while (delta < 1024);

  if (!foundFmt) {
    printf("FATAL - fmt subchunk not found!\n");
    return 0;
  }

  iPtr = (int *)cPtr;
  int audioFormat = ((iPtr[2] & 0xff000000) >> 24);
  if (audioFormat != WAV_PCM) {
    printf("FATAL - Only PCM is supported!\n");
    return 0;
  }

  int numChannels = (iPtr[2] & 0xff00) >> 8;
  int sampleRate = ((iPtr[3] & 0xff) << 24) | ((iPtr[3] & 0xff00) << 8) |
                   ((iPtr[3] & 0xff0000) >> 8) | ((iPtr[3] & 0xff000000) >> 24);
  int bitsPerSample = (iPtr[5] & 0xff00) >> 8;

  // "data"
  if (iPtr[6] != 0x64617461) {
    printf("FATAL - data subchunk not found %x!\n", iPtr[6]);
    return 0;
  }

  WavHeader *wh = tndo_malloc(sizeof(WavHeader), 0);
  wh->format = audioFormat;
  wh->numChannels = numChannels;
  wh->sampleRate = sampleRate;
  wh->bitsPerSample = bitsPerSample;
  wh->data = (char *)(iPtr + 8);
  wh->dataLen = ((iPtr[7] & 0xff) << 24) | ((iPtr[7] & 0xff00) << 8) |
                ((iPtr[7] & 0xff0000) >> 8) | ((iPtr[7] & 0xff000000) >> 24);

  return wh;
}

static int audioMode = 0;

// If your demo uses PCM audio and then a mod, you have to switch modes or bad
// things will happen.
void setAudioMode(int mode) {
  switch (mode) {
  case PCM_REPLAY_MODE:
    if (audioMode == P61_REPLAY_MODE) {
#ifdef TORNADO_P61
      p61End();
#endif
    }
    PaulaOutput_Start();
    audioMode = PCM_REPLAY_MODE;
    break;
  case P61_REPLAY_MODE:
    if (audioMode == PCM_REPLAY_MODE) {
      PaulaOutput_ShutDown();
    }
    audioMode = P61_REPLAY_MODE;
    break;
  }
}

int getAudioMode() { return audioMode; }

// Pretracker doesn't clean up after itself.
void prtEnd() { PaulaOutput_ShutDown(); }

// Cinter doesn't clean up after itself.
void CinterEnd() { PaulaOutput_ShutDown(); }

void fixSamples(char *data, int dataLen, int bitsPerSample) {
  switch (bitsPerSample) {
  case 8:
    for (int i = 0; i < dataLen; i++) {
      data[i] -= 128;
    }
    break;
  case 16:
    break;
  default:
    printf("Warning: Called fixSamples with unsupported setting: %i\n",
           bitsPerSample);
    break;
  }
}

int calcPeriod(int freq) {
  int pal = 3546895;
  return pal / freq;
}
