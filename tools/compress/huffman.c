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

static void printBin(uint32_t d) {
  for (int i = 7; i >= 0; i -= 1) {
    if (d & (1 << i)) {
      printf("1");
    } else {
      printf("0");
    }
  }
}

static stack *newStack(size) {
  stack *s = calloc(1, sizeof(stack));
  s->elements = calloc(size, sizeof(void *));
  s->size = size;
  s->pos = size;
  return s;
}

static void *popStack(stack *s) {
  if (s->pos == s->size) {
    return 0;
  }
  void *e = s->elements[s->pos];
  treeElement *t = (treeElement *)e;
  s->pos += 1;
  return e;
}

static int pushStack(stack *s, void *e) {
  treeElement *t = (treeElement *)e;
  if (s->pos == 0) {
    return 1;
  }
  s->pos -= 1;
  s->elements[s->pos] = e;
  return 0;
}

static int isStackEmpty(stack *s) {
  if (s->pos == s->size) {
    return 1;
  }

  return 0;
}

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

static void addToDict(dictionary *d, treeElement *e) {
  if (e->type == TE_LEAF) {
    d->entries[d->numEntries]->symbol = e->symbol;
    d->numEntries++;
  }
}

static void populateDict(dictionary *d) {
  uint32_t *codes = generateHuffmanCodes(d->numEntries);
  for (int i = 0; i < d->numEntries; i++) {
    d->entries[i]->code = codes[i];
  }
}

// Build sorted dictionary from the tree.
// In-order LNR binary tree traversal.
static dictionary *buildDict(treeElement *tree) {
  dictionary *d = calloc(1, sizeof(dictionary));
  d->entries = calloc(DICT_SIZE, sizeof(dictEntry *));
  for (int i = 0; i < DICT_SIZE; i++) {
    d->entries[i] = calloc(1, sizeof(dictEntry));
  }

  // Traverse tree and build dictionary
  stack *s = newStack(DICT_SIZE);
  treeElement *node = tree;

  while (!isStackEmpty(s) || node != NULL) {
    if (node != NULL) {
      if (pushStack(s, (void *)node)) {
        fprintf(stderr, "FATAL - Out of stack. Aborting.");
        abort();
      }
      node = node->left;
    } else {
      node = popStack(s);
      if (!node) {
        fprintf(stderr, "FATAL - Pop failed. Aborting.");
        abort();
      }
      addToDict(d, (treeElement *)node);
      node = node->right;
    }
  }

  populateDict(d);

  return d;
}

// Build symbol tree from the source data.
static treeElement *buildTree(void *data, uint32_t size) {
  uint32_t numLeaves = 0;

  treeElement **tree = calloc(DICT_SIZE, sizeof(treeElement *));
  treeElement **queue1 = calloc(DICT_SIZE, sizeof(treeElement *));
  treeElement **queue2 = calloc(DICT_SIZE, sizeof(treeElement *));
  treeElement **sourceQueue = queue1;
  treeElement **destinationQueue = queue2;

  // Build leaves for each symbol.
  unsigned char *payload = (unsigned char *)data;

  for (int i = 0; i < DICT_SIZE; i++) {
    tree[i] = calloc(1, sizeof(treeElement));
    tree[i]->symbol = i;
  }

  for (int i = 0; i < size; i++) {
    unsigned char c = payload[i];
    tree[c]->weight += 1;
  }

  // Build the priority queue.
  int k = 0;
  for (int i = 0; i < DICT_SIZE; i++) {
    if (tree[i]->weight > 0) {
      queue1[k] = tree[i];
      k++;
      numLeaves++;
    }
  }

  // Sort the priority queue by reverse probability.
  if (radixsort((const unsigned char **)queue1, numLeaves, NULL, 0)) {
    fprintf(stderr, "FATAL - radixsort returned non 0. Aborting.");
    exit(EXIT_FAILURE);
  }

  // Combine the 2 highest priority leaves into an internal node
  // and insert it in order. Decrease numLeaves. Once finished,
  // the tree has been generated.
  while (numLeaves > 1) {
    treeElement *leaf1 = sourceQueue[0];
    treeElement *leaf2 = sourceQueue[1];
    treeElement *internal = calloc(1, sizeof(treeElement));
    internal->type = TE_INTERNAL;
    leaf1->parent = internal;
    leaf2->parent = internal;
    internal->weight = leaf1->weight + leaf2->weight;
    internal->left = leaf1;
    internal->right = leaf2;
    if (numLeaves == 2) {
      destinationQueue[0] = internal;
      break;
    }

    int j = 2;
    int inserted = 0;
    int i;
    for (i = 0; i < (numLeaves - 2); i++) {
      if (sourceQueue[j]->weight < internal->weight) {
        destinationQueue[i] = sourceQueue[j];
        j++;
      } else {
        if (!inserted) {
          destinationQueue[i] = internal;
          inserted = 1;
        } else {
          destinationQueue[i] = sourceQueue[j];
          j++;
        }
      }
    }

    if (j < numLeaves) {
      if (sourceQueue[j]->weight < internal->weight) {
        destinationQueue[i] = sourceQueue[j];
        j++;
      } else {
        if (!inserted) {
          destinationQueue[i] = internal;
          inserted = 1;
        } else {
          destinationQueue[i] = sourceQueue[j];
          j++;
        }
      }
    } else {
      if (!inserted) {
        destinationQueue[i] = internal;
        inserted = 1;
      }
    }

    numLeaves--;
    treeElement **tempQueue = sourceQueue;
    sourceQueue = destinationQueue;
    destinationQueue = tempQueue;
  }

  return destinationQueue[0];
}

static uint32_t usedBytes;
static uint32_t usedBits;
static uint16_t bitBuffer;

#define BIT_BUFFER_SIZE 16

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

static void flush(unsigned char *data) {
  data[usedBytes++] = (unsigned char)(bitBuffer >> 8);
  data[usedBytes++] = (unsigned char)(bitBuffer & 0xff);
  usedBits = 0;
  bitBuffer = 0;
}

static void flush8(unsigned char *data, unsigned char c) {
  data[usedBytes++] = c;
}

static void append(unsigned char *data, uint32_t code) {
  uint32_t codeLen;
  if (code <= 2) {
    codeLen = 2;
  } else {
    codeLen = nonzeroBits(code);
  }

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

compressedData *h_compress(void *data, uint32_t size) {
  compressedData *c = calloc(1, sizeof(compressedData));
  // Overallocate output buffer.
  c->data = calloc(size, sizeof(unsigned char));

  unsigned char *payload = (unsigned char *)data;

  treeElement *tree = buildTree(data, size);
  dictionary *dict = buildDict(tree);

  // Header (in network order!):
  // uint32_t uncompressed data size
  // uint32_t maximum code length
  // uint32_t dictionary length
  // uint8_t dictionary entries

  usedBytes = 0;
  bitBuffer = size >> 16;
  flush(c->data);
  bitBuffer = size & 0xffff;
  flush(c->data);

  uint32_t maxCodeLen = nonzeroBits(dict->entries[dict->numEntries - 1]->code);
  bitBuffer = maxCodeLen >> 16;
  flush(c->data);
  bitBuffer = maxCodeLen & 0xffff;
  flush(c->data);

  bitBuffer = (dict->numEntries) >> 16;
  flush(c->data);
  bitBuffer = (dict->numEntries) & 0xffff;
  flush(c->data);

  // Store dictionary
  for (int i = 0; i < dict->numEntries; i++) {
    unsigned char s = (unsigned char)dict->entries[i]->symbol;
    flush8(data, s);
  }

  usedBits = 0;
  bitBuffer = 0;

  for (int i = 0; i < size; i++) {
    uint32_t ic = (uint32_t)payload[i];
    for (int j = 0; j < dict->numEntries; j++) {
      if (ic == dict->entries[j]->symbol) {
        append(c->data, dict->entries[j]->code);
        break;
      }
    }
  }

  if (usedBits) {
    flush(c->data);
  }

  c->size = usedBytes;

  printf("Uncompressed: %i, compressed: %i (%.1f%%)\n", size, usedBytes,
         ((float)usedBytes / (float)size) * 100.0f);
  printf("Dictionary size: %i\n", dict->numEntries);
  printf("Maximum code length: %i\n", maxCodeLen);
  printf("Compressed sequence: ");

  unsigned char *cc = c->data;
  for (int i = 0; i < usedBytes; i++) {
    printBin((uint32_t)cc[i]);
  }
  printf("\n");
  return c;
}
