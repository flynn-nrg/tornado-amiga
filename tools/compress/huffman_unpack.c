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

#include "huffman.h"

static uint32_t *generateHuffmanCodes(uint32_t amount) {
  uint32_t *codes = calloc(amount, sizeof(uint32_t));
  codes[0] = 0;
  codes[1] = 1;

  uint32_t code = 0;
  uint32_t mask = 0;

  for (int i = 2; i < amount - 1; i++) {
    code = 0;
    mask = 1 << 1;
    for (int j = 0; j < i - 1; j++) {
      code |= mask;
      mask = mask << 1;
    }
    codes[i] = code;
  }

  codes[amount - 1] = codes[amount - 2] | 1;

  return codes;
}

static uint32_t consumedBytes;
static uint32_t consumedBits;
static uint32_t producedBytes;
static uint16_t bitBuffer;

static void flush8(unsigned char *data, unsigned char c) {
  data[producedBytes++] = c;
}

static void consume32(unsigned char *data, uint32_t *d) {
  uint8_t c;
  uint32_t temp;
  c = data[consumedBytes++];
  temp = ((uint32_t)c) << 24;
  c = data[consumedBytes++];
  temp |= ((uint32_t)c) << 16;
  c = data[consumedBytes++];
  temp |= ((uint32_t)c) << 8;
  c = data[consumedBytes++];
  temp |= (uint32_t)c;
  *d = temp;
}

static uint32_t consume1(unsigned char *data, uint32_t i) {
  uint32_t remaining = 8 - consumedBits;
  uint8_t b;
  if (!remaining) {
    consumedBytes++;
    consumedBits = 0;
  }

  i = i << 1;
  b = data[consumedBytes];
  b = b << consumedBits;
  b = b >> 7;
  i |= b;
  consumedBits++;
  return i;
}

static uint32_t consume2(unsigned char *data, uint32_t i) {
  uint32_t remaining = 8 - consumedBits;
  uint32_t j;
  uint8_t b;
  if (remaining > 1) {
    i = i << 2;
    b = data[consumedBytes];
    b = b << consumedBits;
    b = b >> 6;
    i |= b;
    consumedBits += 2;
    return i;
  } else {
    j = consume1(data, i);
    return consume1(data, j);
  }
}

void *h_uncompress(void *data, uint32_t size) {
  uint32_t uSize, maxCodeLen, dictLen, *codes;
  uint8_t *dict;
  uint8_t *uData;
  uint32_t buffer;
  uint32_t currentLen;
  uint32_t found;

  consumedBytes = 0;
  consumedBits = 0;
  producedBytes = 0;
  bitBuffer = 0;

  // Header (in network order!):
  // uint32_t uncompressed data size
  // uint32_t maximum code length
  // uint32_t dictionary length
  // uint8_t dictionary entries

  unsigned char *payload = (unsigned char *)data;

  consume32(payload, &uSize);
  consume32(payload, &maxCodeLen);
  consume32(payload, &dictLen);

  // Assign dictionary
  dict = &payload[consumedBytes];
  consumedBytes += dictLen;

  codes = generateHuffmanCodes(dictLen);

  uData = malloc(uSize);

  buffer = 0;

  while (producedBytes < uSize) {
    found = 0;
    buffer = consume2(data, buffer);
    if (buffer < 3) {
      uData[producedBytes++] = dict[buffer];
      buffer = 0;
      found = 1;
    } else {
      currentLen = 2;
      while (currentLen < maxCodeLen) {
        buffer = consume1(data, buffer);
        for (int i = 3; i < dictLen; i++) {
          if (codes[i] == buffer) {
            uData[producedBytes++] = dict[i];
            buffer = 0;
            found = 1;
            break;
          }
        }

        if (found)
          break;
        currentLen++;
      }
    }
  }

  printf("Uncompressed: %i\n", uSize);
  printf("Dictionary size: %i\n", dictLen);
  printf("Maximum code length: %i\n", maxCodeLen);
  printf("Uncompressed sequence: ");
  for (int i = 0; i < uSize; i++) {
    printf("%c", uData[i]);
  }

  printf("\n");

  free(codes);

  return uData;
}
