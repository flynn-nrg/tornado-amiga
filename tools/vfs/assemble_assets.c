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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tndo_file.h"

static void usage(char *progname) {
  printf("\nUsage: %s -o <file> file1 file2 file3...\n", progname);
  exit(0);
}

int main(int argc, char **argv) {
  int ch;
  char *output;
  FILE *fd;

  printf("Tornado Demo System Asset Manager\n");
  printf("(C) 2017-2019 Miguel Mendez\n");

  if (argc < 2)
    usage(argv[0]);

  while ((ch = getopt(argc, argv, "o:")) != -1) {
    switch (ch) {
    case 'o':
      output = optarg;
      break;
    default:
      usage(argv[0]);
    }
  }

  argc -= optind;
  argv += optind;

  printf("\nAssembling assets to %s.\n", output);

  fd = fopen(output, "w");
  if (!fd) {
    printf("FATAL - Could not open %s for writing. Exiting.\n", output);
    exit(EXIT_FAILURE);
  }

  // TNDO CONTAINER format:
  // uint32_t magic
  // uint32_t numChunks
  // for each chunck...
  // uint8_t fileName[256]
  // uint32_t size
  // uint32_t offset (absolute)
  // file data...

  // Write magic identifier...
  if (!fwrite(TNDO_VFS_MAGIC, sizeof(TNDO_VFS_MAGIC) - 1, 1, fd)) {
    fprintf(stderr, "FATAL - Could not write header. Exiting.\n");
    fclose(fd);
    exit(EXIT_FAILURE);
  }

  // Write number of chunks in this container...
  uint32_t numChunks = htonl(argc);
  if (!fwrite(&numChunks, sizeof(uint32_t), 1, fd)) {
    fprintf(stderr, "Failed to write number of chunks field. Exiting.\n");
    fclose(fd);
    exit(1);
  }

  uint8_t *fileNameBuffer = calloc(TNDO_VFS_MAX_PATH_LEN, 1);

  uint32_t offset = sizeof(tndo_vfs_header) + (argc * sizeof(tndo_chunk));

  // Generate the chunk headers for every file...
  for (int i = 0; i < argc; i++) {
    FILE *asset = fopen(argv[i], "r");
    if (!asset) {
      fprintf(stderr, "FATAL - Could not open %s for reading. Exiting.\n",
              argv[i]);
      fclose(fd);
      exit(EXIT_FAILURE);
    }
    fseek(asset, 0, SEEK_END);
    long a_size = ftell(asset);
    fclose(asset);

    // File name
    memset(fileNameBuffer, 0, TNDO_VFS_MAX_PATH_LEN);
    assert(strnlen(argv[i], TNDO_VFS_MAX_PATH_LEN) < TNDO_VFS_MAX_PATH_LEN);
    memcpy(fileNameBuffer, argv[i],
           strnlen(argv[i], TNDO_VFS_MAX_PATH_LEN - 1));
    if (!fwrite(fileNameBuffer, TNDO_VFS_MAX_PATH_LEN, 1, fd)) {
      fprintf(stderr, "FATAL - Could not write chunk field. Exiting.\n");
      fclose(fd);
      exit(EXIT_FAILURE);
    }

    // File size
    uint32_t size = htonl(a_size);
    if (!fwrite(&size, sizeof(uint32_t), 1, fd)) {
      fprintf(stderr, "FATAL - Could not write chunk field. Exiting.\n");
      fclose(fd);
      exit(EXIT_FAILURE);
    }

    uint32_t currentOffset = htonl(offset);
    if (!fwrite(&currentOffset, sizeof(uint32_t), 1, fd)) {
      fprintf(stderr, "FATAL - Could not write chunk field. Exiting.\n");
      fclose(fd);
      exit(EXIT_FAILURE);
    }

    offset += a_size;
  }

  free(fileNameBuffer);

  printf("Adding files: ");

  for (int i = 0; i < argc; i++) {
    printf("%s ", argv[i]);
    FILE *asset = fopen(argv[i], "r");
    if (!asset) {
      fprintf(stderr, "Failed to open %s for reading. Aborting.\n", argv[i]);
      fclose(fd);
      exit(1);
    }
    fseek(asset, 0, SEEK_END);
    long a_size = ftell(asset);
    fseek(asset, 0, SEEK_SET);
    void *buffer = calloc(a_size, 1);
    if (!fread(buffer, a_size, 1, asset)) {
      fprintf(stderr, "FATAL - Failed to read %s. Aborting.\n", argv[i]);
      fclose(asset);
      fclose(fd);
      exit(EXIT_FAILURE);
    }

    if (!fwrite(buffer, a_size, 1, fd)) {
      fprintf(stderr,
              "FATAL - Failed to write asset to container file. Aborting.\n");
      fclose(asset);
      fclose(fd);
      exit(EXIT_FAILURE);
    }

    fclose(asset);
    free(buffer);
  }

  printf("\n");

  printf("Done.\n");
  fclose(fd);
  exit(EXIT_SUCCESS);
}
