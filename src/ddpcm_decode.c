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

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __AMIGA__
#include <arpa/inet.h>
#endif

#include "ddpcm.h"
#include "ddpcm_decode.h"
#include "memory.h"
#include "ptr_bridges.h"
#include "tndo_file.h"

#ifdef __AMIGA__
#include "ddpcm_lowlevel.h"
#endif

#include "tornado_settings.h"
#include <tndo_assert.h>

// Linear extrapolation predictor.
// y = y1 + (y2 - y1) * t.
static inline int16_t predict(int16_t y1, int16_t y2) {
  int32_t t = 2;
  int32_t y132 = (int32_t)y1;
  int32_t y232 = (int32_t)y2;

  int32_t p = y132 + ((y232 - y132) * t);
  if (p > INT16_MAX)
    return INT16_MAX / 2;
  if (p < INT16_MIN)
    return INT16_MIN / 2;
  return (int16_t)p;
}

static inline void unpack6to8(uint8_t *src, uint8_t *dst) {
  uint32_t unpacked1 = 0;
  uint32_t unpacked2 = 0;

  unpacked1 = (((uint32_t)*src) >> 2) << 24;
  unpacked1 |= (((uint32_t)*src++) & 0x3) << 20;
  unpacked1 |= (((uint32_t)*src) >> 4) << 16;
  unpacked1 |= (((uint32_t)*src++) & 0xf) << 10;
  unpacked1 |= (((uint32_t)*src) >> 6) << 8;
  unpacked1 |= (((uint32_t)*src++) & 0x3f);

  unpacked2 = (((uint32_t)*src) >> 2) << 24;
  unpacked2 |= (((uint32_t)*src++) & 0x3) << 20;
  unpacked2 |= (((uint32_t)*src) >> 4) << 16;
  unpacked2 |= (((uint32_t)*src++) & 0xf) << 10;
  unpacked2 |= (((uint32_t)*src) >> 6) << 8;
  unpacked2 |= (((uint32_t)*src++) & 0x3f);

  *dst++ = (uint8_t)((unpacked1 >> 24) & 0xff);
  *dst++ = (uint8_t)((unpacked1 >> 16) & 0xff);
  *dst++ = (uint8_t)((unpacked1 >> 8) & 0xff);
  *dst++ = (uint8_t)(unpacked1 & 0xff);

  *dst++ = (uint8_t)((unpacked2 >> 24) & 0xff);
  *dst++ = (uint8_t)((unpacked2 >> 16) & 0xff);
  *dst++ = (uint8_t)((unpacked2 >> 8) & 0xff);
  *dst = (uint8_t)(unpacked2 & 0xff);
}

// Decodes a single frame from src to dst using the provided q_table.
void decodeFrame(uint8_t *src, int16_t *dst, int16_t *q_table, uint8_t scale) {
  uint8_t unpacked[DDPCM_FRAME_NUMSAMPLES - 2];
  int16_t y1, y2, p, p2;

  int16_t *first = (int16_t *)src;

#ifdef __AMIGA__
  dst[0] = (int16_t)first[0];
  dst[1] = (int16_t)first[1];
#else
  dst[0] = ntohs((int16_t)first[0]);
  dst[1] = ntohs((int16_t)first[1]);
#endif

  unpack6to8(&src[4], unpacked);
  unpack6to8(&src[10], &unpacked[8]);
  unpack6to8(&src[16], &unpacked[16]);
  unpack6to8(&src[22], &unpacked[24]);
  unpack6to8(&src[28], &unpacked[32]);
  unpack6to8(&src[34], &unpacked[40]);

  float inverseScale = 1.0f / (float)scale;
  float fdelta;

  for (uint32_t i = 2; i < DDPCM_FRAME_NUMSAMPLES; i++) {
    y1 = dst[i - 2];
    y2 = dst[i - 1];
    p = predict(y1, y2);

    fdelta = ((float)q_table[unpacked[i - 2]] * inverseScale);
    dst[i] = p + (int16_t)floorf(fdelta);
  }
}

ddpcmDecodedData *decodeDDPCMStream(ddpcmHeader *ddpcmh, FILE *fd,
                                    int tornadoOptions) {
  ddpcmDecodedData *ddpcmdd =
      (ddpcmDecodedData *)tndo_malloc(sizeof(ddpcmDecodedData), 0);
  ddpcmdd->numSamples = ddpcmh->numSamples;
  ddpcmdd->left =
      (int16_t *)tndo_malloc(ddpcmh->numSamples * sizeof(int16_t), 0);
  ddpcmdd->right =
      (int16_t *)tndo_malloc(ddpcmh->numSamples * sizeof(int16_t), 0);

  ddpcmdd->bitsPerSample = 16;

  uint8_t *buffer =
      tndo_malloc(ddpcmh->framesPerQTable * DDPCM_COMPRESSED_FRAME_SIZE, 0);

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Unpacking left channel (%u samples)\n",
           ddpcmdd->numSamples);
  }

  // Left channel...
  uint32_t ulDone = 0;
  int16_t *ulDest = ddpcmdd->left;
  uint32_t ulDstOffset = 0;
  uint32_t ulFrame = 0;
  for (uint32_t i = 0; i < ddpcmh->numQTables; i++) {
    tndo_fread(buffer, ddpcmh->framesPerQTable, DDPCM_COMPRESSED_FRAME_SIZE,
               fd);
    uint8_t *uleftSrc = buffer;
    uint32_t ulOffset = 0;
    for (uint32_t j = 0; j < ddpcmh->framesPerQTable; j++) {
#ifdef __AMIGA__
      decodeFrame_asm(&uleftSrc[ulOffset], &ulDest[ulDstOffset],
                      ddpcmh->qtablesLeft[i], ddpcmh->scalesLeft[ulFrame++]);
#else
      decodeFrame(&uleftSrc[ulOffset], &ulDest[ulDstOffset],
                  ddpcmh->qtablesLeft[i], ddpcmh->scalesLeft[ulFrame++]);
#endif
      ulOffset += DDPCM_COMPRESSED_FRAME_SIZE;
      ulDstOffset += DDPCM_FRAME_NUMSAMPLES;
    }
  }

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Unpacking right channel (%u samples)\n",
           ddpcmdd->numSamples);
  }

  // Right channel...
  uint32_t urDone = 0;
  int16_t *urDest = ddpcmdd->right;
  uint32_t urDstOffset = 0;
  uint32_t urFrame = 0;
  for (uint32_t i = 0; i < ddpcmh->numQTables; i++) {
    tndo_fread(buffer, ddpcmh->framesPerQTable, DDPCM_COMPRESSED_FRAME_SIZE,
               fd);
    uint8_t *urightSrc = buffer;
    uint32_t urOffset = 0;
    for (uint32_t j = 0; j < ddpcmh->framesPerQTable; j++) {
#ifdef __AMIGA__
      decodeFrame_asm(&urightSrc[urOffset], &urDest[urDstOffset],
                      ddpcmh->qtablesRight[i], ddpcmh->scalesRight[urFrame++]);
#else
      decodeFrame(&urightSrc[urOffset], &urDest[urDstOffset],
                  ddpcmh->qtablesRight[i], ddpcmh->scalesRight[urFrame++]);

#endif
      urOffset += DDPCM_COMPRESSED_FRAME_SIZE;
      urDstOffset += DDPCM_FRAME_NUMSAMPLES;
    }
  }

  return ddpcmdd;
}
