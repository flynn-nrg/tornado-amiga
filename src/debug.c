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
