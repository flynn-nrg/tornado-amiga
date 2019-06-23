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
#include <unistd.h>

#include "lzss.h"

void usage() {
  fprintf(
      stderr,
      "Usage: compress -v -c <compression level> -i <infile> -o <outfile>\n");
  exit(EXIT_SUCCESS);
};

int main(int argc, char **argv) {
  FILE *inFile;
  FILE *outFile;
  uint32_t size;
  uint32_t read, written;
  uint32_t ch;
  uint32_t cLevel;
  uint32_t windowSize;
  uint32_t verbose = 0;

  cLevel = 1;

  if (argc < 3)
    usage();

  while ((ch = getopt(argc, argv, "vi:o:c:")) != -1) {
    switch (ch) {
    case 'i':
      inFile = fopen(optarg, "r");
      if (!inFile)
        exit(EXIT_FAILURE);
      break;
    case 'o':
      outFile = fopen(optarg, "w");
      if (!outFile)
        exit(EXIT_FAILURE);
      break;
    case 'c':
      cLevel = atoi(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    case '?':
    default:
      usage();
    }
  }

  argc -= optind;
  argv += optind;

  switch (cLevel) {
  case 1:
    windowSize = LZSS_WINDOW_255;
    break;
  case 2:
    windowSize = LZSS_WINDOW_4095_4;
    break;
  case 3:
    windowSize = LZSS_WINDOW_4095_12;
    break;
  case 4:
    windowSize = LZSS_WINDOW_65535;
    break;
  default:
    fprintf(stderr, "Invalid compression level <%i>. Exiting.\n", cLevel);
    exit(EXIT_FAILURE);
  }

  fseek(inFile, 0, SEEK_END);
  size = ftell(inFile);
  fseek(inFile, 0, SEEK_SET);

  void *data = malloc(size);
  if (!data)
    exit(EXIT_FAILURE);

  void *data2 = malloc(size);
  if (!data2)
    exit(EXIT_FAILURE);

  read = fread(data, size, 1, inFile);
  if (!read)
    exit(EXIT_FAILURE);

  fclose(inFile);

  compressedData *c = lzss_compress(data, size, windowSize, verbose);

  if (c->data) {
    lzss_uncompress((uint8_t *)c->data, (uint8_t *)data2, c->size);
    if (memcmp(data, data2, size)) {
      fprintf(stderr, "FATAL - Validation failed. Exiting.\n");
      exit(EXIT_FAILURE);
    }
    written = fwrite(c->data, c->size, 1, outFile);
    if (!written)
      exit(EXIT_FAILURE);
  }

  fclose(outFile);

  exit(EXIT_SUCCESS);
}
