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

#ifndef TNDO_FILE_H
#define TNDO_FILE_H

#include <stdint.h>
#include <stdio.h>

#define TNDO_FILE_OK 0
#define TNDO_FILE_EOPEN 1
#define TNDO_FILE_IO 2
#define TNDO_FILE_EFORMAT 3
#define TNDO_FILE_EINVAL 4
#define TNDO_FILE_ECHUNK -1

#define TNDO_VFS_MAX_PATH_LEN 256
#define TNDO_VFS_MAGIC "TVFS"
#define TNDO_VFS_MAGIC_INT 0x54564653

typedef struct {
  uint32_t magic;
  uint32_t numChunks;
} tndo_vfs_header;

typedef struct {
  uint8_t fileName[TNDO_VFS_MAX_PATH_LEN];
  uint32_t size;
  uint32_t offset;
} tndo_chunk;

int tndo_vfs_init(const char *path, int tornadoOptions,
                  void (*callback)(int, int));
int tndo_vfs_end(void);
FILE *tndo_fopen(const char *path, const char *mode);
int tndo_fclose(FILE *stream);
long tndo_ftell(FILE *stream);
size_t tndo_fread(void *ptr, size_t size, size_t nitems, FILE *stream);
void tndo_rewind(FILE *stream);
int tndo_fseek(FILE *stream, long offset, int whence);
int tndo_file_vfs_init(const char *path);
int tndo_ferror(FILE *stream);

#endif
