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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "lzw.h"

// Initialises the dictionary:
// 0 - 255 stores the 1 byte long strings.
// 256 and 257 store the clear and stop codes.
// 258 - dictSize reserved for longer strings.
static dictionary *initDict(uint32_t dictSize) {
  dictionary *d = calloc(1, sizeof(dictionary));
  d->size = dictSize;
  d->entries = calloc(dictSize, sizeof(dictEntry *));

  // Populate all strings of length 1.
  for (int i = 0; i < 256; i++) {
    d->entries[i] = calloc(1, sizeof(dictEntry));
    d->entries[i]->code = i;
    d->entries[i]->symbol = calloc(1, 1);
    d->entries[i]->symbol[0] = (uint8_t)i;
    d->entries[i]->symbolSize = 1;
  }

  // Clear and stop codes.
  d->entries[LZW_CLEAR] = calloc(1, sizeof(dictEntry));
  d->entries[LZW_STOP] = calloc(1, sizeof(dictEntry));

  d->entries[LZW_CLEAR]->code = LZW_CLEAR;
  d->entries[LZW_STOP]->code = LZW_STOP;

  d->numEntries = LZW_STOP + 1;

  // Fill-in the codes for the rest.
  // All these codes have a symbol of length 0 for now.
  for (int i = (LZW_STOP + 1); i < dictSize; i++) {
    d->entries[i] = calloc(1, sizeof(dictEntry));
    d->entries[i]->code = i;
  }

  return d;
}

static uint32_t usedBytes;
static uint32_t usedBits;
static uint16_t bitBuffer;

#define BIT_BUFFER_SIZE 16

static void flush(uint8_t *data) {
  data[usedBytes++] = (uint8_t)(bitBuffer >> 8);
  data[usedBytes++] = (uint8_t)(bitBuffer & 0xff);
  usedBits = 0;
  bitBuffer = 0;
}

static void flush8(uint8_t *data, uint8_t c) {
  data[usedBytes++] = c;
  usedBits = 0;
}

static void append(uint8_t *data, uint32_t code, uint32_t codeLen) {
  uint32_t remaining = BIT_BUFFER_SIZE - usedBits;

  if (codeLen > remaining) {
    bitBuffer = ((bitBuffer << (remaining)) | (code >> (codeLen - remaining)));
    flush(data);
    bitBuffer = code;
    usedBits = (codeLen - remaining);
  } else {
    bitBuffer = (bitBuffer << codeLen) | code;
    usedBits += codeLen;
  }

  if (usedBits == BIT_BUFFER_SIZE) {
    flush(data);
  }
}

static int dictEntryCmpFunc(const void *a, const void *b) {
  dictEntry **da = (dictEntry **)a;
  dictEntry **db = (dictEntry **)b;
  if ((*da)->score > (*db)->score) {
    return -1;
  } else if ((*da)->score < (*db)->score) {
    return 1;
  }
  return 0;
}

static int fastCompare(uint8_t *s1, uint8_t *s2, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    if (s1[i] != s2[i]) {
      return 1;
    }
  }

  return 0;
}

// Hash function for strings
static uint16_t strHash(uint8_t *s, uint32_t len) {
  uint16_t hash = 0;
  for (int i = 0; i < len; i++) {
    hash += (uint16_t)s[i];
  }

  return hash;
}

// Returns a pointer to a dictionary entry
// if the string is found. NULL otherwise.
static dictEntry *findStringInDict(dictEntry **dht, uint8_t *s, uint32_t len) {
  uint16_t hash = strHash(s, len);
  dictEntry *de = dht[hash];

  while (de) {
    if (de->symbolSize == len) {
      if (!fastCompare(s, de->symbol, len)) {
        return de;
      }
    }
    de = de->next;
  }

  return 0;
}

static void appendToDictHash(dictEntry **dht, dictEntry *de) {
  uint16_t hash = strHash(de->symbol, de->symbolSize);
  dictEntry **dh = &dht[hash];
  while (*dh) {
    dh = &(*dh)->next;
  }
  *dh = de;
}

// Builds a hash table with all the dictionary entries.
static dictEntry **buildDictHash(dictionary *d) {
  dictEntry **dht = calloc(LZW_HASHTABLE_SIZE, sizeof(dictEntry *));

  for (int i = 0; i < d->numEntries; i++) {
    appendToDictHash(dht, d->entries[i]);
  }

  return dht;
}

// Returns 1 if dictionary is full.
static uint32_t addToDict(dictionary *d, dictEntry **dht, uint8_t *string,
                          uint32_t len) {
  if (d->numEntries < d->size) {
    // Symbol lengths are stored using uint8_t.
    if (len < LZW_MAX_SYMBOL_LEN) {
      d->entries[d->numEntries]->symbolSize = len;
      d->entries[d->numEntries]->symbol = string;
      appendToDictHash(dht, d->entries[d->numEntries]);
      d->numEntries++;
    }
    return 0;
  }

  return 1;
}

compressedData *lzw_compress(void *data, uint32_t size, uint32_t dictSize,
                             uint32_t codeLen, uint32_t optimumDict,
                             uint32_t verbose) {
  compressedData *c = calloc(1, sizeof(compressedData));

  // Overallocate work output buffer.
  uint8_t *workBuffer = calloc(2 * size, sizeof(uint8_t));

  uint8_t *payload = (uint8_t *)data;

  dictionary *dict = initDict(dictSize * optimumDict);

  uint32_t found = 0;
  uint32_t foundCode = 0;
  uint32_t consumed = 0;
  uint32_t done = 0;
  uint32_t dictFull = 0;
  uint32_t maxLenUsed = 0;

  if (verbose) {
    printf("--------------------------\n");
    printf("Step 1 - Build dictionary.\n");
    printf("--------------------------\n");
  }
  dictEntry **dht = buildDictHash(dict);

  while ((consumed < size) && !dictFull) {
    uint8_t *substring = &payload[consumed];
    if (verbose)
      printf("Step 1... %.1f%%\r",
             ((float)dict->numEntries / (float)(dictSize * optimumDict)) *
                 100.0f);
    uint32_t maxLen = 255;
    if (maxLen > (size - consumed))
      maxLen = size - consumed;
    for (int sLen = 1; sLen <= maxLen; sLen++) {
      found = 0;
      dictEntry *de = findStringInDict(dht, substring, sLen);
      if (de) {
        found = 1;
        // Emit the last code
        if ((size - consumed) == sLen) {
          consumed += sLen;
          done = 1;
        }
      }

      if (!found && !done) {
        // Add sLen to dictionary.
        consumed += sLen - 1;
        dictFull = addToDict(dict, dht, substring, sLen);
        if (sLen > maxLenUsed) {
          maxLenUsed = sLen;
        }
        break;
      }
    }
  }

  if (verbose) {
    printf("Number of codes used: %i\n", dict->numEntries);
    printf("Maximum symbol length: %i\n", maxLenUsed);
  }

  // Second pass...
  usedBytes = 0;
  usedBits = 0;
  bitBuffer = 0;

  consumed = 0;
  done = 0;

  if (verbose) {
    printf("--------------------------------------------\n");
    printf("Step 2 - Compress with pre-built dictionary.\n");
    printf("--------------------------------------------\n");
  }

  while (consumed < size) {
    uint8_t *substring = &payload[consumed];
    if (verbose)
      printf("Step 2... %.1f%%\r", ((float)consumed / (float)size) * 100.0f);
    uint32_t maxLen =
        (size - consumed) > maxLenUsed ? maxLenUsed : (size - consumed);
    for (int sLen = maxLen; sLen >= 1; sLen--) {
      dictEntry *de = findStringInDict(dht, substring, sLen);
      if (de) {
        foundCode = de->code;
        // Emit found code
        de->used++;
        consumed += sLen;
        append(workBuffer, foundCode, codeLen);
        break;
      }
    }
  }

  // Add stop symbol
  append(workBuffer, (uint32_t)LZW_STOP, codeLen);
  if (usedBits) {
    // Make sure the final flush is properly aligned.
    uint32_t remaining = BIT_BUFFER_SIZE - usedBits;
    bitBuffer = bitBuffer << remaining;
    flush(workBuffer);
  }

  uint32_t compressedDataSize = usedBytes;

  if (verbose) {
    printf("Uncompressed: %i, compressed: %i (%.1f%%)\n", size, usedBytes,
           ((float)usedBytes / (float)size) * 100.0f);
    printf("Maximum symbol length: %i\n", maxLenUsed);
  }

  if (optimumDict == LZW_OPTIMUM_DICTIOMARY_ENABLE) {
    if (verbose) {
      printf("---------------------------------------\n");
      printf("Step 3 - Generate optimum dictionary.\n");
      printf("---------------------------------------\n");
      printf("Finding optimum codes...");
    }

    for (int i = LZW_STOP + 1; i < dict->numEntries; i++) {
      dict->entries[i]->score = dict->entries[i]->used;
    }

    qsort(&dict->entries[LZW_STOP + 1], dict->numEntries - (LZW_STOP + 1),
          sizeof(dictEntry *), dictEntryCmpFunc);

    // Remap codes.
    if (dict->numEntries > dictSize) {
      dict->numEntries = dictSize;
    }

    for (int i = 0; i < dict->numEntries; i++) {
      dict->entries[i]->code = i;
      dict->entries[i]->used = 0;
      dict->entries[i]->next = 0;
    }

    if (verbose) {
      printf("done.\n");

      printf("------------------------------------------\n");
      printf("Step 4 - Compress with optimum dictionary.\n");
      printf("------------------------------------------\n");
    }

    // Rebuild the hash table;
    dht = buildDictHash(dict);

    usedBytes = 0;
    usedBits = 0;
    bitBuffer = 0;
    consumed = 0;
    done = 0;

    while (consumed < size) {
      uint8_t *substring = &payload[consumed];
      if (verbose)
        printf("Step 4... %.1f%%\r", ((float)consumed / (float)size) * 100.0f);
      uint32_t maxLen =
          (size - consumed) > maxLenUsed ? maxLenUsed : (size - consumed);
      for (int sLen = maxLen; sLen >= 1; sLen--) {
        dictEntry *de = findStringInDict(dht, substring, sLen);
        if (de) {
          foundCode = de->code;
          // Emit found code
          de->used++;
          consumed += sLen;
          append(workBuffer, foundCode, codeLen);
          break;
        }
      }
    }

    // Add stop symbol
    append(workBuffer, (uint32_t)LZW_STOP, codeLen);
    if (usedBits) {
      // Make sure the final flush is properly aligned.
      uint32_t remaining = BIT_BUFFER_SIZE - usedBits;
      bitBuffer = bitBuffer << remaining;
      flush(workBuffer);
    }

    compressedDataSize = usedBytes;

    if (verbose) {
      printf("Uncompressed: %i, compressed: %i (%.1f%%)\n", size, usedBytes,
             ((float)usedBytes / (float)size) * 100.0f);
      printf("Maximum symbol length: %i\n", maxLenUsed);

      printf("------------------------------\n");
      printf("Step 5 - Save compressed file.\n");
      printf("------------------------------\n");
    }

  } else {
    if (verbose) {
      printf("------------------------------\n");
      printf("Step 3 - Save compressed file.\n");
      printf("------------------------------\n");
    }
  }

  // Dictionary overhead...
  uint32_t overHead = 0;
  uint32_t numUsed = 0;
  for (int i = LZW_STOP + 1; i < dict->numEntries; i++) {
    if (dict->entries[i]->used) {
      numUsed++;
      overHead += dict->entries[i]->symbolSize;
    }
    // Symbol length.
    overHead++;
  }

  // Header + dictionary + compressed payload.
  c->size = compressedDataSize + overHead + (LZW_HEADER_SIZE);
  c->data = calloc(c->size, sizeof(uint8_t));

  // Header (in network order!):
  // uint32_t uncompressed data size
  // uint32_t clear code
  // uint32_t stop code
  // uint32_t dictionary size
  // uint32_t dictinary payload size
  // uint32_t code length

  usedBytes = 0;
  bitBuffer = size >> 16;
  flush(c->data);
  bitBuffer = size & 0xffff;
  flush(c->data);

  bitBuffer = LZW_CLEAR >> 16;
  flush(c->data);
  bitBuffer = LZW_CLEAR & 0xffff;
  flush(c->data);

  bitBuffer = LZW_STOP >> 16;
  flush(c->data);
  bitBuffer = LZW_STOP & 0xffff;
  flush(c->data);

  bitBuffer = dict->numEntries >> 16;
  flush(c->data);
  bitBuffer = dict->numEntries & 0xffff;
  flush(c->data);

  bitBuffer = overHead >> 16;
  flush(c->data);
  bitBuffer = overHead & 0xffff;
  flush(c->data);

  bitBuffer = codeLen >> 16;
  flush(c->data);
  bitBuffer = codeLen & 0xffff;
  flush(c->data);

  usedBits = 0;
  bitBuffer = 0;

  // Save dictionary.
  for (int i = LZW_STOP + 1; i < dict->numEntries; i++) {
    if (dict->entries[i]->used) {
      flush8(c->data, (uint8_t)dict->entries[i]->symbolSize);
      memcpy(&c->data[usedBytes], dict->entries[i]->symbol,
             dict->entries[i]->symbolSize);
      usedBytes += dict->entries[i]->symbolSize;
    } else {
      flush8(c->data, 0);
    }
  }

  // Save payload.
  memcpy(&c->data[usedBytes], workBuffer, compressedDataSize);

  if (verbose) {
    printf("Dictionary: %i symbols, %i bytes\n", numUsed, overHead);
    printf("Total compressed size: %i (%.1f%%)\n", c->size,
           ((float)(c->size) / (float)size) * 100.0f);
  }

  return c;
}
