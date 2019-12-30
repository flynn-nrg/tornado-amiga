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
