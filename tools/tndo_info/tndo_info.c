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

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "tndo.h"

static TndoHeader *readHeader(FILE *fd) {
  TndoHeader *th = calloc(sizeof(TndoHeader), 1);
  if (!th) {
    return 0;
  }

  int read = fread(th, sizeof(TndoHeader), 1, fd);
  if (read != 1) {
    printf("FATAL - Cannot read TNDO header.\n");
    return 0;
  }

  // Check for TNDO signature.
  if (ntohl(th->magic) != TNDO_MAGIC_INT) {
    return 0;
  }

  // Return pointer to TNDO struct.
  return th;
}

static void printHeader(TndoHeader *th) {

  printf("File type: ");

  switch (ntohl(th->type)) {
  case TNDO_TYPE_GENERIC:
    printf("Generic.\n");
    break;
  case TNDO_TYPE_GFX:
    printf("Graphics data.\n");
    break;
  case TNDO_TYPE_AUDIO:
    printf("Audio.\n");
    break;
  default:
    printf("Uknown.\n");
    break;
  }

  printf("Compression: ");
  switch (ntohl(th->compression)) {
  case TNDO_COMPRESSION_NONE:
    printf("None.\n");
    break;
  case TNDO_COMPRESSION_DLTA:
    printf("Audio DLTA (lossless).\n");
    break;
  case TNDO_COMPRESSION_ZLIB:
    printf("Zlib.\n");
    break;
  case TNDO_COMPRESSION_ADPCM:
    printf("ADPCM.\n");
    break;
  case TNDO_COMPRESSION_DLTA_14:
    printf("Audio DLTA (lossy).\n");
    break;
  case TNDO_COMPRESSION_CAPS:
    printf("Audio CAPS.\n");
    break;
  case TNDO_COMPRESSION_LZH:
    printf("TNDO LZSS/LZW hybrid.\n");
    break;
  case TNDO_COMPRESSION_LZSS:
    printf("TNDO LZSS.\n");
    break;
  case TNDO_COMPRESSION_LZW:
    printf("TNDO LZW.\n");
    break;
  default:
    printf("Uknown.\n");
    break;
  }

  printf("Compressed size: %u bytes\n", ntohl(th->compressed_size));
  printf("Uncompressed size: %u bytes\n", ntohl(th->uncompressed_size));

  if (ntohl(th->type) == TNDO_TYPE_AUDIO) {
    printf("Sample rate: %u\n", ntohl(th->sampleRate));
    printf("Bit depth: %u\n", ntohl(th->bitsPerSample));
    printf("Number of channels: %u\n", ntohl(th->numChannels));
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s file\n", argv[0]);
    exit(0);
  }

  FILE *fd = fopen(argv[1], "r");
  if (!fd) {
    fprintf(stderr, "FATAL - Could not open %s for reading. Exiting.\n",
            argv[1]);
    exit(EXIT_FAILURE);
  }

  TndoHeader *th = readHeader(fd);
  if (!th) {
    fprintf(stderr, "%s does not look like a TNDO file. Exiting.\n", argv[1]);
    exit(0);
  }

  printf("File name: %s\n", argv[1]);
  printHeader(th);
  fclose(fd);
  free(th);
  exit(EXIT_SUCCESS);
}
