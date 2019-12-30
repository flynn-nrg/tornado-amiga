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

#include "memory.h"
#include "tndo_file.h"

#include "lzcommon.h"
#include "lzw_unpack_stream.h"

static uint32_t consumed = 0;

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
void lzw_uncompress_stream(FILE *source, uint8_t *dst, uint32_t fileSize) {
  uint32_t produced = 0;

  consumed = 0;

  uint8_t header[LZW_HEADER_SIZE];

  tndo_fread(header, LZW_HEADER_SIZE, 1, source);

  // Header (in network order!):
  // uint32_t uncompressed data size
  // uint32_t clear code
  // uint32_t stop code
  // uint32_t dictionary size
  // uint32_t dictinary payload size
  // uint32_t code length

  uint32_t uSize = consume32(header);
  uint32_t clearCode = consume32(header);
  uint32_t stopCode = consume32(header);
  uint32_t dictSize = consume32(header);
  uint32_t dictPayload = consume32(header);
  uint32_t codeLen = consume32(header);

  uint8_t *workBuffers = (uint8_t *)tndo_get_packed_data_buffer(
      dictPayload + LZW_STREAM_BUFFER + (dictSize * sizeof(uint8_t *)) +
      (dictSize * sizeof(uint8_t)));

  uint8_t **symbols = (uint8_t **)workBuffers;
  workBuffers += (dictSize * sizeof(uint8_t *));

  uint8_t *lengths = workBuffers;
  workBuffers += (dictSize * sizeof(uint8_t));

  uint8_t *buffer = workBuffers;
  workBuffers += LZW_STREAM_BUFFER;

  uint8_t *dict = workBuffers;

  // Read dictionary into memory.
  uint32_t read = tndo_fread(dict, dictPayload, 1, source);
  if (!read) {
    fprintf(stderr, "FATAL - Failed to LZW dictionary. Aborting.\n");
    abort();
  }

  consumed = 0;

  // Strings of length 1 are implicit. We already have stop and clear so we
  // don't need them either.
  for (int i = LZW_STOP + 1; i < (int)dictSize; i++) {
    uint8_t d = consume8(dict);
    if (d) {
      symbols[i] = &dict[consumed];
      lengths[i] = d;
      consumed += d;
    }
  }

  uint32_t left = fileSize - (dictPayload + LZW_HEADER_SIZE);
  uint32_t effective;

  switch (codeLen) {
  case 12:
    // 2 codes take 3 bytes. Load an even amount of codes.
    effective = LZW_STREAM_BUFFER - 4;
    break;
  case 16:
    effective = LZW_STREAM_BUFFER;
    break;
  }

  while (left > 0) {
    uint32_t toRead = effective;

    if (left < effective) {
      toRead = left;
    }

    uint32_t read = tndo_fread(buffer, toRead, 1, source);
    if (!read) {
      fprintf(stderr,
              "FATAL - Failed to read LZW compressed data. Aborting.\n");
      abort();
    }

    left -= toRead;

    // Decode payload.
    nibbleFlag = 0;
    consumed = 0;
    uint32_t code = 0;
    uint8_t *dstBuffer = (uint8_t *)dst;

    for (;;) {
      switch (codeLen) {
      case 12:
        code = consume12(buffer);
        break;
      case 16:
        code = consume16(buffer);
        break;
      }

      if (code == LZW_STOP)
        break;

      if (code < LZW_CLEAR) {
        dstBuffer[produced++] = (uint8_t)code;
      } else {
        memcpy(&dstBuffer[produced], symbols[code], lengths[code]);
        produced += lengths[code];
      }

      if (consumed == effective)
        break;
    }
  }
}
