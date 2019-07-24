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

#ifndef WAV_H
#define WAV_H

#define WAVE_HEADER_SIZE 44

typedef struct {
  int format;
  int numChannels;
  int sampleRate;
  int bitsPerSample;
  int *data;
  int dataLen;
} WavHeader;

typedef struct {
  int numChannels;
  int sampleRate;
  int bitsPerSample;
  int numSamples;
  int sampleSize;
  void *left;
  void *right;
} AudioData;

#define WAV_PCM 1

WavHeader *parseWavHeader(void *data);

AudioData *splitChannels(WavHeader *wh);
WavHeader *joinChannels(AudioData *ad);

int writeWAV(FILE *output, WavHeader *wh);

#endif
