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
