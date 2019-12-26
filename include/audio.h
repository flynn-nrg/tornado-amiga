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

#ifndef INCLUDE_AUDIO_H
#define INCLUDE_AUDIO_H

#define WAV_PCM 1
#define PCM_REPLAY_MODE 1
#define P61_REPLAY_MODE (1 << 1)
#define PRT_REPLAY_MODE (1 << 2)

typedef struct {
  int format;
  int numChannels;
  int sampleRate;
  int bitsPerSample;
  char *data;
  int dataLen;
} WavHeader;

WavHeader *parseWavHeader(void *data);
void setAudioMode(int);
int getAudioMode(void);
void fixSamples(char *, int, int);

#endif
