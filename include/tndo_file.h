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
