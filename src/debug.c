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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "system.h"

void serialLog(const char *data) {
  unsigned char c;

  c = *data++;

  while (c != 0) {
    serialPutc(c);
    c = *data++;
  }
}

memoryLog *memLogInit(int size) {
  memoryLog *log = (memoryLog *)calloc(1, sizeof(memoryLog));
  log->logDataOrig = (unsigned char *)calloc(1, size);
  log->logData = log->logDataOrig;
  log->maxSize = size;
  log->avail = size;
  return log;
}

void memLog(const char *data, int dataSize, memoryLog *log) {
  if (dataSize > log->avail)
    return;
  memcpy(log->logData, data, dataSize);
  log->avail -= dataSize;
  log->curSize += dataSize;
  log->logData += dataSize;
}

int memLogSave(const char *fileName, memoryLog *log) {
  FILE *fd;
  size_t written;

  fd = fopen(fileName, "w");
  if (!fd)
    return 0;

  written = fwrite(log->logDataOrig, log->curSize, 1, fd);
  if (!written)
    return 0;

  fclose(fd);
  return 1;
}

void memLogFree(memoryLog *log) {
  free(log->logDataOrig);
  free(log);
}

#ifdef __AMIGA__
int tndo_fprintf(FILE *restrict stream, const char *restrict format, ...) {
#ifdef __DEBUG_CODE
  va_list(args);
  va_start(args, format);
  return vfprintf(stream, format, args);
#else
  return 0;
#endif
}
#endif
