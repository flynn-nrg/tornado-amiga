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

#ifndef LZCOMMON_H
#define LZCOMMON_H

#define BIT_BUFFER_SIZE 16

#define LZW_CLEAR 256
#define LZW_STOP 257

#define LZW_DICT_SIZE_SMALL 4096
#define LZW_CODE_LEN_SMALL 12
#define LZW_DICT_SIZE_BIG 65536
#define LZW_CODE_LEN_BIG 16

#define LZW_MAX_SYMBOL_LEN 255

#define LZW_OPTIMUM_DICTIOMARY_DISABLE 1
#define LZW_OPTIMUM_DICTIOMARY_ENABLE 4

#define LZW_HASHTABLE_SIZE 65536

#define LZW_STREAM_BUFFER 65536
#define LZSS_STREAM_BUFFER 65536

#define LZW_HEADER_SIZE 6 * sizeof(uint32_t)

typedef struct compressedData {
  uint32_t size;
  uint8_t *data;
} compressedData;

typedef struct dictEntry {
  uint8_t *symbol;
  uint32_t symbolSize;
  uint32_t code;
  uint32_t used;
  uint32_t score;
  struct dictEntry *next;
} dictEntry;

typedef struct dictionary {
  uint32_t numEntries;
  uint32_t size;
  struct dictEntry **entries;
} dictionary;

static void printBin16(uint16_t d) {
  for (int i = 15; i >= 0; i -= 1) {
    if (d & (1 << i)) {
      printf("1");
    } else {
      printf("0");
    }
  }
}

static void dumpStr(uint8_t *string, uint32_t len) {
  for (int i = 0; i < (int)len; i++) {
    printf("%c", string[i]);
  }
}

static void printBin12(uint32_t d) {
  for (int i = 11; i >= 0; i -= 1) {
    if (d & (1 << i)) {
      printf("1");
    } else {
      printf("0");
    }
  }
}

static uint32_t nonzeroBits(uint32_t code) {
  uint32_t mask = 0;
  for (int i = 31; i > 1; i -= 1) {
    mask = 1 << i;
    if (code & mask) {
      return i + 1;
    }
  }

  return 2;
}

#endif
