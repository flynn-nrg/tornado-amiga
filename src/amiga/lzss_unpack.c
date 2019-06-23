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
#include <string.h>

#include "lzss.h"

static uint32_t consumed;

static uint32_t consume32(uint8_t *data) {
  uint32_t d;

  d = ((uint32_t)data[consumed++]) << 24;
  d |= ((uint32_t)data[consumed++]) << 16;
  d |= ((uint32_t)data[consumed++]) << 8;
  d |= ((uint32_t)data[consumed++]);

  return d;
}

static uint8_t consume8(uint8_t *data) {
  uint8_t d;
  d = data[consumed++];
  return d;
}

static uint32_t nibbleFlag;

static uint32_t consume12(uint8_t *data) {
  uint32_t d;
  d = ((uint32_t)data[consumed++]) << 8;
  d |= (uint32_t)data[consumed];

  if (!nibbleFlag) {
    d = d >> 4;
    nibbleFlag = 1;
  } else {
    d = d & 0xfff;
    nibbleFlag = 0;
    consumed++;
  }

  return d;
}

static uint32_t consume16(uint8_t *data) {
  uint32_t d;

  d = ((uint32_t)data[consumed++]) << 8;
  d |= (uint32_t)data[consumed++];

  return d;
}

// Destination buffer must be preallocated!
void lzss_uncompress(uint8_t *data, uint8_t *dst, uint32_t size) {
  uint32_t produced = 0;

  consumed = 0;

  // Header (in network order!):
  // uint32_t uncompressed data size
  // uint32_t code length (8, 12 or 16 bits)

  uint32_t uSize = consume32(data);
  uint32_t codeLen = consume32(data);

  // Decode payload.
  nibbleFlag = 0;
  uint32_t offset, runSize;
  uint8_t *dstBuffer = (uint8_t *)dst;

  while (consumed < size) {
    switch (codeLen) {
    case LZSS_CODELEN_8_8:
      offset = (uint32_t)consume8(data);
      runSize = (uint32_t)consume8(data);
      break;
    case LZSS_CODELEN_12_4:
      offset = (uint32_t)consume16(data);
      runSize = offset & 0xf;
      offset = offset >> 4;
      break;
    case LZSS_CODELEN_12_12:
      offset = (uint32_t)consume12(data);
      runSize = (uint32_t)consume12(data);
      break;
    case LZSS_CODELEN_16_16:
      offset = (uint32_t)consume16(data);
      runSize = (uint32_t)consume16(data);
      break;
    }

    if (offset == LZSS_LITERAL) {
      memcpy(&dstBuffer[produced], &data[consumed], runSize);
      produced += runSize;
      consumed += runSize;
    } else {
      memcpy(&dstBuffer[produced], &dstBuffer[produced - offset], runSize);
      produced += runSize;
    }
  }
}
