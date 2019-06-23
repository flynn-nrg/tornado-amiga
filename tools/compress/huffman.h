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

#ifndef HUFFMAN_H
#define HUFFMAN_H

typedef struct treeElement {
  uint32_t weight;
  uint32_t type;
  uint32_t symbol;
  struct treeElement *left;
  struct treeElement *right;
  struct treeElement *parent;
} treeElement;

typedef struct dictEntry {
  uint32_t symbol;
  uint32_t code;
} dictEntry;

typedef struct dictionary {
  uint32_t numEntries;
  uint32_t size;
  struct dictEntry **entries;
} dictionary;

typedef struct stack {
  uint32_t pos;
  uint32_t size;
  void **elements;
} stack;

typedef struct compressedData {
  uint32_t size;
  void *data;
} compressedData;

#define TE_LEAF 0
#define TE_INTERNAL 1

#define TE_LEFT 0
#define TE_RIGHT 1

#define DICT_SIZE 256

compressedData *h_compress(void *data, uint32_t size);
void *h_uncompress(void *data, uint32_t size);

#endif
