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
