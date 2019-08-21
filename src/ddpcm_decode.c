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

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __AMIGA__
#include <arpa/inet.h>
#endif

#include "memory.h"
#include "ptr_bridges.h"

#include "ddpcm.h"
#include "ddpcm_decode.h"

#ifdef __AMIGA__
#include "ddpcm_lowlevel.h"
#endif

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
  int16_t y1, y2, p;

  int16_t *first = (int16_t *)src;

#ifdef __AMIGA__
  dst[0] = (int16_t)first[0];
  dst[1] = (int16_t)first[1];
#else
  dst[0] = ntohs((int16_t)first[0]);
  dst[1] = ntohs((int16_t)first[1]);
#endif

#ifdef __AMIGA__
  unpack6to8_asm(&src[4], unpacked);
#else
  unpack6to8(&src[4], unpacked);
  unpack6to8(&src[10], &unpacked[8]);
  unpack6to8(&src[16], &unpacked[16]);
  unpack6to8(&src[22], &unpacked[24]);
  unpack6to8(&src[28], &unpacked[32]);
  unpack6to8(&src[34], &unpacked[40]);
#endif

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

ddpcmDecodedData *decodeDDPCMStream(ddpcmHeader *ddpcmh) {
  ddpcmDecodedData *ddpcmdd =
      (ddpcmDecodedData *)tndo_malloc(sizeof(ddpcmDecodedData), 0);
  ddpcmdd->numSamples = ddpcmh->numSamples;
  ddpcmdd->left =
      (int16_t *)tndo_malloc(ddpcmh->numSamples * sizeof(int16_t), 0);
  ddpcmdd->right =
      (int16_t *)tndo_malloc(ddpcmh->numSamples * sizeof(int16_t), 0);

  // Left channel...
  uint8_t *uleftSrc = ddpcmh->left;
  uint32_t ulOffset = 0;
  uint32_t ulDone = 0;
  int16_t *ulDest = ddpcmdd->left;
  uint32_t ulDstOffset = 0;
  uint32_t ulFrame = 0;
  for (uint32_t i = 0; i < ddpcmh->numQTables; i++) {
    for (uint32_t j = 0; j < ddpcmh->framesPerQTable; j++) {
      decodeFrame(&uleftSrc[ulOffset], &ulDest[ulDstOffset],
                  ddpcmh->qtablesLeft[i], ddpcmh->scalesLeft[ulFrame++]);
      ulOffset += DDPCM_COMPRESSED_FRAME_SIZE;
      ulDstOffset += DDPCM_FRAME_NUMSAMPLES;
    }
  }

  // Right channel...
  uint8_t *urightSrc = ddpcmh->right;
  uint32_t urOffset = 0;
  uint32_t urDone = 0;
  int16_t *urDest = ddpcmdd->right;
  uint32_t urDstOffset = 0;
  uint32_t urFrame = 0;
  for (uint32_t i = 0; i < ddpcmh->numQTables; i++) {
    for (uint32_t j = 0; j < ddpcmh->framesPerQTable; j++) {
      decodeFrame(&urightSrc[urOffset], &urDest[urDstOffset],
                  ddpcmh->qtablesRight[i], ddpcmh->scalesRight[urFrame++]);
      urOffset += DDPCM_COMPRESSED_FRAME_SIZE;
      urDstOffset += DDPCM_FRAME_NUMSAMPLES;
    }
  }

  return ddpcmdd;
}
