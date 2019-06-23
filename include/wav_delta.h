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

#ifndef WAV_DELTA_H
#define WAV_DELTA_H

#define WAV_PCM 1

#define MAX_SEGMENT_SIZE 2048
#define MIN_SEGMENT_SIZE 64

#define MAX_BITS 16
#define MIN_BITS 8

#define DLTA_MAGIC "DLTA"

typedef struct {
  int format;
  int numChannels;
  int sampleRate;
  int bitsPerSample;
  int *data;
  int dataLen;
} WavHeader;

typedef struct encodingSettings {
  uint32_t segmentSize;
  uint32_t maxBits;
  uint32_t fileSize;
} encodingSettings;

WavHeader *parseWavHeader(void *);
uint16_t computeMaximumDelta(uint16_t *, uint32_t);
void twosComplement(int16_t *, uint32_t);
void byteSwap(uint16_t *, uint32_t);
void dither14bit(uint16_t *, uint32_t);
void trim14bit(uint8_t *, uint32_t);
void downSample(int16_t *, uint32_t, uint32_t);
void deltaEncode(uint16_t *, uint16_t *, uint32_t, uint32_t);
void deltaDecode(uint16_t *, uint16_t *, uint32_t, uint32_t);
uint32_t isCompressable(uint16_t *, uint32_t, uint32_t);
encodingSettings *findBestSettings(uint16_t *, uint32_t);

#endif
