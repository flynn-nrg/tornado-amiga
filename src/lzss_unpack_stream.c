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
      fprintf(stderr, "FATAL - Failed to read LZSS compressed data. Aborting.\n");
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
