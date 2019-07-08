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

#include "lzw.h"
#include "lzw_unpack_inner.h"
#include "memory.h"

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

// Destination buffer must be preallocated!
void lzw_uncompress(uint8_t *data, uint8_t *dst, uint32_t size) {
  uint32_t produced = 0;

  consumed = 0;

  // Header (in network order!):
  // uint32_t uncompressed data size
  // uint32_t clear code
  // uint32_t stop code
  // uint32_t dictionary size
  // uint32_t dictinary payload size
  // uint32_t code length

  uint32_t uSize = consume32(data);
  uint32_t clearCode = consume32(data);
  uint32_t stopCode = consume32(data);
  uint32_t dictSize = consume32(data);
  uint32_t dictPayload = consume32(header);
  uint32_t codeLen = consume32(data);

  uint8_t **symbols =
      tndo_malloc(dictSize * sizeof(uint8_t *), TNDO_REUSABLE_MEM);
  uint8_t *lengths = tndo_malloc(dictSize * sizeof(uint8_t), TNDO_REUSABLE_MEM);

  // Strings of length 1 are implicit. We already have stop and clear so we
  // don't need them either.
  for (int i = LZW_STOP + 1; i < dictSize; i++) {
    uint8_t d = consume8(data);
    if (d) {
      symbols[i] = &data[consumed];
      lengths[i] = d;
      consumed += d;
    }
  }

  // Decode payload.
  nibbleFlag = 0;
  uint32_t code;
  uint8_t *dstBuffer = (uint8_t *)dst;

  switch (codeLen) {
  case 12:
    lzw_unpack_inner_12(&data[consumed], dstBuffer, symbols, lengths, clearCode,
                        stopCode);
    break;
  case 16:
    lzw_unpack_inner_16(&data[consumed], dstBuffer, symbols, lengths, clearCode,
                        stopCode);
    break;
  }
}
