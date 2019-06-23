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

static uint8_t consume8(uint8_t *data, lzssCtx *ctx) {
  uint8_t byte = data[ctx->consumed++];
  return byte;
}

static uint16_t consume16(uint8_t *data, lzssCtx *ctx) {
  uint16_t word;
  word = (uint16_t)(data[ctx->consumed++]) << 8;
  word |= (uint16_t)data[ctx->consumed++];
  return word;
}

static uint32_t consume12(uint8_t *data, lzssCtx *ctx) {
  uint32_t d;
  d = ((uint32_t)data[ctx->consumed++]) << 8;
  d |= (uint32_t)data[ctx->consumed];

  if (!ctx->nibbleFlag) {
    d = d >> 4;
    ctx->nibbleFlag = 1;
  } else {
    d = d & 0xfff;
    ctx->nibbleFlag = 0;
    ctx->consumed++;
  }

  return d;
}

static void flush8(uint8_t *data, uint8_t byte, lzssCtx *ctx) {
  data[ctx->usedBytes++] = byte;
}

static void flush16(uint8_t *data, uint16_t bytes, lzssCtx *ctx) {
  data[ctx->usedBytes++] = bytes >> 8;
  data[ctx->usedBytes++] = bytes & 0xff;
}

static void flush24(uint8_t *data, uint16_t byteshi, uint16_t byteslo,
                    lzssCtx *ctx) {
  uint32_t bytes = (byteshi << 12) | (byteslo & 0xfff);
  data[ctx->usedBytes++] = (bytes >> 16) & 0xff;
  data[ctx->usedBytes++] = (bytes >> 8) & 0xff;
  data[ctx->usedBytes++] = bytes & 0xff;
}

static void flush32(uint8_t *data, uint32_t bytes, lzssCtx *ctx) {
  data[ctx->usedBytes++] = bytes >> 24;
  data[ctx->usedBytes++] = (bytes >> 16) & 0xff;
  data[ctx->usedBytes++] = (bytes >> 8) & 0xff;
  data[ctx->usedBytes++] = bytes & 0xff;
}

static void flush(uint8_t *data, uint8_t *string, uint32_t size, lzssCtx *ctx) {
  uint8_t *dst = &data[ctx->usedBytes];
  memcpy(dst, string, size);
  ctx->usedBytes += size;
}

// Returns the position of the first byte that's different.
static int fastCompare(uint8_t *s1, uint8_t *s2, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    if (s1[i] != s2[i]) {
      return i;
    }
  }

  return len;
}

static void joinLiterals8(uint8_t *s1, uint8_t *s2, uint32_t sizeLimit,
                          lzssCtx *ctx) {
  uint32_t consumedTmp = ctx->consumed;
  uint32_t numTotal = 0;
  uint32_t totalRun = 0;
  uint8_t curOffset = LZSS_LITERAL;
  uint8_t curRun;
  uint32_t oldTotal = 0;

  consumedTmp -= 2;

  for (;;) {
    curOffset = s1[consumedTmp++];
    curRun = s1[consumedTmp++];
    consumedTmp += curRun;
    if (curOffset != LZSS_LITERAL)
      break;
    oldTotal = totalRun;
    totalRun += curRun;
    if (totalRun > 255) {
      totalRun = oldTotal;
      break;
    }
    numTotal++;
    if (consumedTmp == sizeLimit)
      break;
  }

  flush8(s2, LZSS_LITERAL, ctx);
  flush8(s2, (uint8_t)totalRun, ctx);

  ctx->consumed -= 2;
  for (int i = 0; i < numTotal; i++) {
    ctx->consumed++;
    uint8_t r = s1[ctx->consumed++];
    flush(s2, &s1[ctx->consumed], r, ctx);
    ctx->consumed += r;
  }
}

static void joinLiterals12_4(uint8_t *s1, uint8_t *s2, uint32_t sizeLimit,
                             lzssCtx *ctx) {
  uint32_t consumedTmp = ctx->consumed;
  uint32_t numTotal = 0;
  uint32_t totalRun = 0;
  uint16_t curOffset = LZSS_LITERAL;
  uint16_t curRun;
  uint32_t oldTotal = 0;

  consumedTmp -= 2;

  for (;;) {
    curOffset = (uint16_t)(s1[consumedTmp++] << 8);
    curOffset |= (uint16_t)s1[consumedTmp++];
    curRun = curOffset & 0xf;
    curOffset = curOffset >> 4;
    consumedTmp += curRun;
    if (curOffset != LZSS_LITERAL)
      break;
    oldTotal = totalRun;
    totalRun += curRun;
    if (totalRun > 15) {
      totalRun = oldTotal;
      break;
    }
    numTotal++;
    if (consumedTmp == sizeLimit)
      break;
  }

  flush16(s2, (uint16_t)totalRun, ctx);

  ctx->consumed -= 2;
  for (int i = 0; i < numTotal; i++) {
    ctx->consumed++;
    uint16_t r = (uint16_t)(s1[ctx->consumed++]);
    r = r & 0xf;
    flush(s2, &s1[ctx->consumed], r, ctx);
    ctx->consumed += r;
  }
}

static void joinLiterals12(uint8_t *s1, uint8_t *s2, uint32_t sizeLimit,
                           lzssCtx *ctx) {
  uint32_t consumedTmp = ctx->consumed;
  uint32_t numTotal = 0;
  uint32_t totalRun = 0;
  uint16_t curOffset = LZSS_LITERAL;
  uint16_t curRun;
  uint32_t oldTotal = 0;

  consumedTmp -= 3;

  for (;;) {
    curOffset = (uint16_t)(s1[consumedTmp++] << 8);
    curOffset |= (uint16_t)s1[consumedTmp];
    curOffset = curOffset >> 4;
    curRun = (uint16_t)(s1[consumedTmp++] << 8);
    curRun |= (uint16_t)s1[consumedTmp++];
    curRun = curRun & 0xfff;
    consumedTmp += curRun;
    if (curOffset != LZSS_LITERAL)
      break;
    oldTotal = totalRun;
    totalRun += curRun;
    if (totalRun > 4095) {
      totalRun = oldTotal;
      break;
    }
    numTotal++;
    if (consumedTmp == sizeLimit)
      break;
  }

  flush24(s2, LZSS_LITERAL, totalRun, ctx);

  ctx->consumed -= 3;
  for (int i = 0; i < numTotal; i++) {
    ctx->consumed++;
    uint16_t r = (uint16_t)(s1[ctx->consumed++] << 8);
    r |= (uint16_t)s1[ctx->consumed++];
    r = r & 0xfff;
    flush(s2, &s1[ctx->consumed], r, ctx);
    ctx->consumed += r;
  }
}

static void joinLiterals16(uint8_t *s1, uint8_t *s2, uint32_t sizeLimit,
                           lzssCtx *ctx) {
  uint32_t consumedTmp = ctx->consumed;
  uint32_t numTotal = 0;
  uint32_t totalRun = 0;
  uint16_t curOffset = LZSS_LITERAL;
  uint16_t curRun;
  uint32_t oldTotal = 0;

  consumedTmp -= 4;

  for (;;) {
    curOffset = (uint16_t)(s1[consumedTmp++] << 8);
    curOffset |= (uint16_t)s1[consumedTmp++];
    curRun = (uint16_t)(s1[consumedTmp++] << 8);
    curRun |= (uint16_t)s1[consumedTmp++];
    consumedTmp += curRun;
    if (curOffset != LZSS_LITERAL)
      break;
    oldTotal = totalRun;
    totalRun += curRun;
    if (totalRun > 65535) {
      totalRun = oldTotal;
      break;
    }
    numTotal++;
    if (consumedTmp == sizeLimit)
      break;
  }

  flush16(s2, LZSS_LITERAL, ctx);
  flush16(s2, (uint16_t)totalRun, ctx);

  ctx->consumed -= 4;
  for (int i = 0; i < numTotal; i++) {
    ctx->consumed++;
    ctx->consumed++;
    uint16_t r = (uint16_t)(s1[ctx->consumed++] << 8);
    r |= (uint16_t)s1[ctx->consumed++];
    flush(s2, &s1[ctx->consumed], r, ctx);
    ctx->consumed += r;
  }
}

compressedData *lzss_compress_internal(uint8_t *payload, uint32_t size,
                                       uint32_t windowSize, uint32_t verbose) {
  uint32_t width;
  uint32_t maxRun;
  uint32_t minWorth;

  compressedData *c = calloc(1, sizeof(compressedData));

  // Overallocate work output buffers.
  uint8_t *workBuffer1 = calloc(2 * size, sizeof(uint8_t));
  uint8_t *workBuffer2 = calloc(2 * size, sizeof(uint8_t));

  lzssCtx *ctx = calloc(1, sizeof(lzssCtx));

  // Header in network order:
  // uint32_t uncompressed data size
  // uint32_t code length (8, 12 or 16 bits)

  flush32(workBuffer1, size, ctx);

  switch (windowSize) {
  case LZSS_WINDOW_255:
    flush32(workBuffer1, LZSS_CODELEN_8_8, ctx);
    width = 255;
    maxRun = 255;
    minWorth = 4;
    flush8(workBuffer1, LZSS_LITERAL, ctx);
    flush8(workBuffer1, minWorth, ctx);
    flush(workBuffer1, payload, minWorth, ctx);
    ctx->consumed = minWorth;
    break;
  case LZSS_WINDOW_4095_4:
    flush32(workBuffer1, LZSS_CODELEN_12_4, ctx);
    width = 4095;
    maxRun = 15;
    minWorth = 6;
    flush16(workBuffer1, minWorth, ctx);
    flush(workBuffer1, payload, minWorth, ctx);
    ctx->consumed = minWorth;
    break;
  case LZSS_WINDOW_4095_12:
    flush32(workBuffer1, LZSS_CODELEN_12_12, ctx);
    width = 4095;
    maxRun = 4095;
    minWorth = 6;
    flush24(workBuffer1, LZSS_LITERAL, minWorth, ctx);
    flush(workBuffer1, payload, minWorth, ctx);
    ctx->consumed = minWorth;
    break;
  case LZSS_WINDOW_65535:
    flush32(workBuffer1, LZSS_CODELEN_16_16, ctx);
    width = 65535;
    maxRun = 65535;
    minWorth = 8;
    flush16(workBuffer1, LZSS_LITERAL, ctx);
    flush16(workBuffer1, minWorth, ctx);
    flush(workBuffer1, payload, minWorth, ctx);
    ctx->consumed = minWorth;
    break;
  default:
    fprintf(stderr, "FATAL - Unimplemented window size: %i. Aborting.\n",
            windowSize);
    abort();
  }

  uint32_t found = 0;
  uint32_t numLiterals = 0;

  if (verbose) {
    printf("------------------\n");
    printf("Step 1 - Compress.\n");
    printf("------------------\n");
  }

  while (ctx->consumed < size) {
    if (verbose)
      printf("Compressing... %.1f%%\r",
             ((float)ctx->consumed / (float)size) * 100.0f);
    uint32_t maxLen = width;
    uint32_t bestRun = minWorth;
    uint32_t bestOffset = 0;

    if (maxLen >= (size - ctx->consumed))
      maxLen = size - ctx->consumed;
    if (maxLen > ctx->consumed)
      maxLen = ctx->consumed;
    if (maxLen > minWorth) {
      for (uint32_t offset = maxLen; offset > (minWorth - 1); offset--) {
        uint8_t *s1 = payload;
        s1 += ctx->consumed;
        uint8_t *s2 = s1;
        s1 -= offset;
        uint32_t newRun =
            fastCompare(s1, s2, offset > maxRun ? maxRun : offset);
        if (newRun > bestRun) {
          bestRun = newRun;
          bestOffset = offset;
          found = 1;
        }
      }
    }
    if (found) {
      switch (windowSize) {
      case LZSS_WINDOW_255:
        flush8(workBuffer1, bestOffset, ctx);
        flush8(workBuffer1, bestRun, ctx);
        break;
      case LZSS_WINDOW_4095_4:
        flush16(workBuffer1, (bestOffset << 4) | (bestRun & 0xf), ctx);
        break;
      case LZSS_WINDOW_4095_12:
        flush24(workBuffer1, bestOffset, bestRun, ctx);
        break;
      case LZSS_WINDOW_65535:
        flush16(workBuffer1, bestOffset, ctx);
        flush16(workBuffer1, bestRun, ctx);
        break;
      }
      found = 0;
      ctx->consumed += bestRun;

    } else {
      uint32_t flushSize = minWorth;
      if ((size - ctx->consumed) < minWorth)
        flushSize = size - ctx->consumed;
      switch (windowSize) {
      case LZSS_WINDOW_255:
        flush8(workBuffer1, LZSS_LITERAL, ctx);
        flush8(workBuffer1, flushSize, ctx);
        break;
      case LZSS_WINDOW_4095_4:
        // LZSS_LITERAL is 0, so we just flush the literal run size.
        flush16(workBuffer1, flushSize, ctx);
        break;
      case LZSS_WINDOW_4095_12:
        flush24(workBuffer1, LZSS_LITERAL, flushSize, ctx);
        break;
      case LZSS_WINDOW_65535:
        flush16(workBuffer1, LZSS_LITERAL, ctx);
        flush16(workBuffer1, flushSize, ctx);
        break;
      }
      flush(workBuffer1, &payload[ctx->consumed], flushSize, ctx);
      ctx->consumed += flushSize;
      numLiterals++;
    }
  }

  if (verbose) {
    printf("Uncompressed: %i, compressed: %i (%.1f%%)\n", size, ctx->usedBytes,
           ((float)ctx->usedBytes / (float)size) * 100.0f);
    printf("Number of literal runs: %i\n", numLiterals);

    printf("-------------------------------\n");
    printf("Step 2 - Optimise literal runs.\n");
    printf("-------------------------------\n");
  }

  uint32_t cSize = ctx->usedBytes;
  ctx->consumed = 0;
  ctx->usedBytes = 0;
  ctx->nibbleFlag = 0;

  // Header in network order:
  // uint32_t uncompressed data size
  // uint32_t code length (8 or 16 bits)

  flush32(workBuffer2, size, ctx);

  switch (windowSize) {
  case LZSS_WINDOW_255:
    flush32(workBuffer2, LZSS_CODELEN_8_8, ctx);
    ctx->consumed = ctx->usedBytes;
    break;
  case LZSS_WINDOW_4095_4:
    flush32(workBuffer2, LZSS_CODELEN_12_4, ctx);
    ctx->consumed = ctx->usedBytes;
    break;
  case LZSS_WINDOW_4095_12:
    flush32(workBuffer2, LZSS_CODELEN_12_12, ctx);
    ctx->consumed = ctx->usedBytes;
    break;
  case LZSS_WINDOW_65535:
    flush32(workBuffer2, LZSS_CODELEN_16_16, ctx);
    ctx->consumed = ctx->usedBytes;
    break;
  default:
    fprintf(stderr, "FATAL - Unimplemented window size: %i. Aborting.\n",
            windowSize);
    abort();
  }

  found = 0;
  numLiterals = 0;

  uint8_t offset8, run8;
  uint16_t offset12, run12;
  uint16_t offset12_4, run12_4;
  uint16_t offset16, run16;

  while (ctx->consumed < cSize) {
    if (verbose)
      printf("Processing... %.1f%%\r",
             ((float)ctx->consumed / (float)cSize) * 100.0f);
    switch (windowSize) {
    case LZSS_WINDOW_255:
      offset8 = consume8(workBuffer1, ctx);
      run8 = consume8(workBuffer1, ctx);
      if (offset8 == LZSS_LITERAL) {
        joinLiterals8(workBuffer1, workBuffer2, cSize, ctx);
        numLiterals++;
      } else {
        flush8(workBuffer2, offset8, ctx);
        flush8(workBuffer2, run8, ctx);
      }
      break;
    case LZSS_WINDOW_4095_4:
      offset12_4 = consume16(workBuffer1, ctx);
      run12_4 = offset12_4 & 0xf;
      offset12_4 = offset12_4 >> 4;
      if (offset12_4 == LZSS_LITERAL) {
        joinLiterals12_4(workBuffer1, workBuffer2, cSize, ctx);
        numLiterals++;
      } else {
        flush16(workBuffer2, (offset12_4 << 4) | (run12_4), ctx);
      }
      break;
    case LZSS_WINDOW_4095_12:
      offset12 = consume12(workBuffer1, ctx);
      run12 = consume12(workBuffer1, ctx);
      if (offset12 == LZSS_LITERAL) {
        joinLiterals12(workBuffer1, workBuffer2, cSize, ctx);
        numLiterals++;
      } else {
        flush24(workBuffer2, offset12, run12, ctx);
      }
      break;
    case LZSS_WINDOW_65535:
      offset16 = consume16(workBuffer1, ctx);
      run16 = consume16(workBuffer1, ctx);
      if (offset16 == LZSS_LITERAL) {
        joinLiterals16(workBuffer1, workBuffer2, cSize, ctx);
        numLiterals++;
      } else {
        flush16(workBuffer2, offset16, ctx);
        flush16(workBuffer2, run16, ctx);
      }
      break;
    }
  }

  if (verbose) {
    printf("Uncompressed: %i, compressed: %i (%.1f%%)\n", size, ctx->usedBytes,
           ((float)ctx->usedBytes / (float)size) * 100.0f);
    printf("Number of literal runs: %i\n", numLiterals);
  }

  c->data = workBuffer2;
  c->size = ctx->usedBytes;

  return c;
}

compressedData *chooseBest(compressedData *c8, compressedData *c12,
                           compressedData *c16) {
  compressedData *c;
  if (c8->size < c12->size) {
    c = c8;
  } else {
    c = c12;
  }

  if (c16->size < c->size) {
    c = c16;
  }

  return c;
}

compressedData *lzss_compress(void *data, uint32_t size, uint32_t windowSize,
                              uint32_t verbose) {

  uint8_t *payload = (uint8_t *)data;

  if (windowSize) {
    compressedData *c =
        lzss_compress_internal(payload, size, windowSize, verbose);
    return c;
  }

  // If windowSize is set to 0 we try all window sizes and return the best.
  // This is mainly used by the hybrid compressor, which uses LZSS to compress
  // the payload before doing an LZW pass.
  compressedData *c8 =
      lzss_compress_internal(payload, size, LZSS_WINDOW_255, verbose);
  compressedData *c12 =
      lzss_compress_internal(payload, size, LZSS_WINDOW_4095_12, verbose);
  compressedData *c16 =
      lzss_compress_internal(payload, size, LZSS_WINDOW_65535, verbose);

  compressedData *c = chooseBest(c8, c12, c16);
  return c;
}
