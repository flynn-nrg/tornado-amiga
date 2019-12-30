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

#include "memory.h"
#include "tndo.h"
#include "tornado_wave_delta.h"
#include "wav_delta.h"

// Load and decompress a delta encoded file.
// Returns a null pointer on error.
uint16_t *loadDeltaFile(char *fileName) {
  FILE *fd = fopen(fileName, "r");
  if (!fd) {
    printf("FATAL - Cannot open file <%s> for reading.\n", fileName);
    return 0;
  }

  // Read the header. Remember that the header is stored in Network order!
  TndoHeader *th = (TndoHeader *)tndo_malloc(sizeof(TndoHeader), 0);
  int read = fread(th, sizeof(TndoHeader), 1, fd);
  if (read != 1) {
    printf("FATAL - Cannot read TNDO header from file <%s>.\n", fileName);
    fclose(fd);
    return 0;
  }

  // Check for TNDO signature.
  if (th->magic != TNDO_MAGIC_INT) {
    printf("FATAL - <%s> is not a TNDO file!\n", fileName);
    fclose(fd);
    return 0;
  }

  // Check for audio settings.
  if (th->type != TNDO_TYPE_AUDIO) {
    printf("FATAL - <%s> is not an audio file!\n", fileName);
    fclose(fd);
    return 0;
  }

  // Check compression settings.
  if (th->compression != TNDO_COMPRESSION_DLTA) {
    printf("FATAL - <%s> is not DLTA compressed!\n", fileName);
    fclose(fd);
    return 0;
  }

  printf("DLTA file:\nMagic: %x\nSample Rate: %u\nSegment Size: %u\nNumber of "
         "Segments: %u\nSample depth:%u\n",
         th->magic, th->sampleRate, th->segmentSize, th->numSegments,
         th->maxBits);

  // Consume compressable bitmap.
  uint8_t *cBitmap =
      (uint8_t *)tndo_malloc(th->numSegments * sizeof(uint8_t), 0);
  read = fread(cBitmap, sizeof(uint8_t), th->numSegments, fd);
  if (read != (int)th->numSegments) {
    printf("FATAL - Cannot read compressable bitmap from file <%s>.\n",
           fileName);
    fclose(fd);
    return 0;
  }

  fclose(fd);

  return 0;
}
