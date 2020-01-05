/*
Copyright (c) 2019, Miguel Mendez. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ptr_bridges.h"
#include "tndo_assert.h"
#include "tndo_file.h"
#include "tornado_settings.h"

typedef struct {
  uint32_t id;
  uint32_t startOffset;
  uint32_t endOffset;
  uint32_t pos;
} tndo_file_descriptor;

static uint32_t vfs_mode_enabled = 0;
static FILE *vfs_container_fd;
static tndo_vfs_header tvfsh;
static tndo_chunk *chunks;
static uint32_t loadedChunks;

static void (*vfsLoaderCallback)(int loaded, int total) = 0;

static int fastCompare(const char *src, const char *dst, int len) {
  for (int i = 0; i < len; i++) {
    if (src[i] != dst[i]) {
      return 0;
    }
  }

  return 1;
}

static int findChunk(const char *path) {
  tndo_assert(strlen(path) < TNDO_VFS_MAX_PATH_LEN);
  int len = strlen(path);
  for (uint32_t i = 0; i < tvfsh.numChunks; i++) {
    if (fastCompare(path, (const char *)chunks[i].fileName, len)) {
      return i;
    }
  }

  fprintf(stderr, "FATAL - Chunk <%s> not found.\n", path);
  return TNDO_FILE_ECHUNK;
}

int tndo_vfs_init(const char *path, int tornadoOptions,
                  void (*callback)(int, int)) {

  vfs_container_fd = fopen(path, "r");
  if (!vfs_container_fd) {
    fprintf(stderr, "FATAL - Could not open %s for reading.\n", path);
    return TNDO_FILE_EOPEN;
  }

  if (!fread(&tvfsh, sizeof(tndo_vfs_header), 1, vfs_container_fd)) {
    fclose(vfs_container_fd);
    fprintf(stderr, "FATAL - Could not read header.\n");
    return TNDO_FILE_IO;
  }

  tvfsh.magic = ENDI4(tvfsh.magic);
  tvfsh.numChunks = ENDI4(tvfsh.numChunks);

  if (tvfsh.magic != TNDO_VFS_MAGIC_INT) {
    fclose(vfs_container_fd);
    fprintf(stderr, "FATAL - %s is not a TNDO VFS container.\n", path);
    return TNDO_FILE_EFORMAT;
  }

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - VFS container <%s> opened with %u chunks.\n", path,
           tvfsh.numChunks);
  }

  chunks = (tndo_chunk *)calloc(tvfsh.numChunks, sizeof(tndo_chunk));

  if (!fread(chunks, sizeof(tndo_chunk), tvfsh.numChunks, vfs_container_fd)) {
    fclose(vfs_container_fd);
    fprintf(stderr, "FATAL - Could not read chunk metadata.\n");
    return TNDO_FILE_IO;
  }

  for (uint32_t i = 0; i < tvfsh.numChunks; i++) {
    chunks[i].size = ENDI4(chunks[i].size);
    chunks[i].offset = ENDI4(chunks[i].offset);
  }

  loadedChunks = 0;
  vfsLoaderCallback = callback;
  vfs_mode_enabled = 1;
  return TNDO_FILE_OK;
}

int tndo_vfs_end() {
  fclose(vfs_container_fd);
  free(chunks);
  return TNDO_FILE_OK;
}

static tndo_file_descriptor fd;

FILE *tndo_fopen(const char *path, const char *mode) {
  if (vfs_mode_enabled) {
    int chunkId = findChunk(path);
    if (chunkId == TNDO_FILE_ECHUNK) {
      return 0;
    }
    fd.id = chunkId;
    fd.startOffset = chunks[chunkId].offset;
    fd.pos = chunks[chunkId].offset;
    fd.endOffset = chunks[chunkId].offset + chunks[chunkId].size;
    fseek(vfs_container_fd, fd.startOffset, SEEK_SET);
    return ((FILE *)&fd);
  } else {
    return fopen(path, mode);
  }
}

int tndo_fclose(FILE *stream) {
  if (vfs_mode_enabled) {
    loadedChunks++;
    if (vfsLoaderCallback) {
      vfsLoaderCallback(loadedChunks, tvfsh.numChunks);
    }
    return 0;
  } else {
    return fclose(stream);
  }
}

size_t tndo_fread(void *ptr, size_t size, size_t nitems, FILE *stream) {
  if (vfs_mode_enabled) {
    tndo_file_descriptor *fd = (tndo_file_descriptor *)stream;
    if (fd->pos == fd->endOffset)
      return 0; // EOF
    uint32_t toRead = (uint32_t)size * (uint32_t)nitems;
    int read = 0;
    if (fd->pos + toRead > fd->endOffset) {
      toRead = fd->endOffset - fd->pos;
      read = fread(ptr, 1, toRead, vfs_container_fd);
    } else {

      read = fread(ptr, size, nitems, vfs_container_fd);
    }
    fd->pos += toRead;
    return read;
  } else {
    return fread(ptr, size, nitems, stream);
  }
}

long tndo_ftell(FILE *stream) {
  if (vfs_mode_enabled) {
    tndo_file_descriptor *fd = (tndo_file_descriptor *)stream;
    return fd->pos - fd->startOffset;
  } else {
    return ftell(stream);
  }
}

void tndo_rewind(FILE *stream) {
  if (vfs_mode_enabled) {
    tndo_file_descriptor *fd = (tndo_file_descriptor *)stream;
    fd->pos = fd->startOffset;
  } else {
    rewind(stream);
  }
}

int tndo_fseek(FILE *stream, long offset, int whence) {
  if (vfs_mode_enabled) {
    tndo_file_descriptor *fd = (tndo_file_descriptor *)stream;
    switch (whence) {
    case SEEK_SET:
      fd->pos = fd->startOffset + offset;
      return fseek(vfs_container_fd, fd->startOffset + offset, SEEK_SET);
      break; // not reached.
    case SEEK_END:
      fd->pos = fd->endOffset;
      return fseek(vfs_container_fd, fd->endOffset, SEEK_SET);
      break; // not reached.
    case SEEK_CUR:
      fd->pos += offset;
      return fseek(vfs_container_fd, fd->pos + offset, SEEK_SET);
      break; // not reached.
    default:
      fprintf(stderr, "FATAL - fseek() whence value %i not implemented.\n",
              whence);
      break;
    }

    return 0;

  } else {
    return fseek(stream, offset, whence);
  }
}

int tndo_ferror(FILE *stream) {
  if (vfs_mode_enabled) {
    return ferror(vfs_container_fd);
  } else {
    return ferror(stream);
  }
}
