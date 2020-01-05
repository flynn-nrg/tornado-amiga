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

#include "lzcommon.h"
#include "lzss.h"
#include "lzss_unpack_stream.h"
#include "memory.h"
#include "tndo_file.h"

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
void lzss_uncompress_stream(FILE *src, uint8_t *dst, uint32_t fileSize) {

  uint32_t produced = 0;

  consumed = 0;

  uint8_t header[LZSS_HEADER_SIZE];

  uint32_t read = tndo_fread(header, LZSS_HEADER_SIZE, 1, src);
  if (!read) {
    fprintf(stderr, "FATAL - Failed to read LZSS header. Aborting.\n");
    abort();
  }

  // Header (in network order!):
  // uint32_t uncompressed data size
  // uint32_t code length (8, 12 or 16 bits)

  uint32_t uSize = consume32(header);
  uint32_t codeLen = consume32(header);
  uint32_t codeSize = 0;

  switch (codeLen) {
  case LZSS_CODELEN_8_8:
    codeSize = LZSS_CODELEN_8_8_SIZE;
    break;
  case LZSS_CODELEN_12_4:
    codeSize = LZSS_CODELEN_12_4_SIZE;
    break;
  case LZSS_CODELEN_12_12:
    codeSize = LZSS_CODELEN_12_12_SIZE;
    break;
  case LZSS_CODELEN_16_16:
    codeSize = LZSS_CODELEN_16_16_SIZE;
    break;
  }

  // We request space for 1 chunk + the longest possible literal run if we were
  // to encounter a symbol near the end of the chunk.
  uint8_t *data = (uint8_t *)tndo_get_packed_data_buffer(LZSS_STREAM_BUFFER);

  uint32_t left = fileSize - LZSS_HEADER_SIZE;
  uint32_t effective = LZSS_STREAM_CHUNK;

  nibbleFlag = 0;
  uint8_t *dstBuffer = (uint8_t *)dst;

  while (left > 0) {
    uint32_t toRead = effective;

    if (left < effective) {
      toRead = left;
    }

    read = tndo_fread(data, toRead, 1, src);
    if (!read) {
      fprintf(stderr,
              "FATAL - Failed to read LZSS compressed data. Aborting.\n");
      abort();
    }

    left -= toRead;

    // Decode payload.
    uint32_t offset = 0, runSize = 0;
    uint32_t overflow = 0;
    uint32_t deficit = 0;

    consumed = 0;

    while (consumed < toRead) {

      if ((toRead - consumed) < codeSize) {
        overflow = codeSize - (toRead - consumed);
        read = tndo_fread(&data[LZSS_STREAM_CHUNK], overflow, 1, src);
        if (!read) {
          fprintf(stderr,
                  "FATAL - Failed to read LZSS compressed data. Aborting.\n");
          abort();
        }
        left -= overflow;
      }

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
        if (runSize > (toRead - consumed) || overflow) {
          if (overflow) {
            deficit = runSize;
          } else {
            deficit = runSize - (toRead - consumed);
          }

          read =
              tndo_fread(&data[LZSS_STREAM_CHUNK + overflow], deficit, 1, src);
          if (!read) {
            fprintf(stderr,
                    "FATAL - Failed to read LZSS compressed data. Aborting.\n");
            abort();
          }
          left -= deficit;
        }
        memcpy(&dstBuffer[produced], &data[consumed], runSize);
        produced += runSize;
        consumed += runSize;
      } else {
        memcpy(&dstBuffer[produced], &dstBuffer[produced - offset], runSize);
        produced += runSize;
      }
    }
  }
}
