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

#ifndef __AMIGA__
#include <arpa/inet.h>
#endif

#include "ddpcm.h"
#include "ddpcm_loader.h"
#include "memory.h"
#include "ptr_bridges.h"
#include "tndo.h"
#include "tndo_file.h"
#include "tornado_settings.h"
#include <tndo_assert.h>

static ddpcmHeader *ddpcmData;
static uint8_t *buffer;

// Load a ddpcm file.
ddpcmHeader *ddpcmLoadFile(FILE *fd, int tornadoOptions) {
  ddpcmHeader *ddpcmData = ddpcmLoadHeader(fd, tornadoOptions);

  ddpcmData->left = (uint8_t *)tndo_malloc(
      ddpcmData->numFrames * DDPCM_COMPRESSED_FRAME_SIZE, 0);
  ddpcmData->right = (uint8_t *)tndo_malloc(
      ddpcmData->numFrames * DDPCM_COMPRESSED_FRAME_SIZE, 0);
  tndo_fread(ddpcmData->left, ddpcmData->numFrames, DDPCM_COMPRESSED_FRAME_SIZE,
             fd);
  tndo_fread(ddpcmData->right, ddpcmData->numFrames,
             DDPCM_COMPRESSED_FRAME_SIZE, fd);

  return ddpcmData;
}

ddpcmDecodedData *ddpcmLoadAndUnpackFile(FILE *fd, int tornadoOptions) {
  ddpcmHeader *ddpcmData = ddpcmLoadHeader(fd, tornadoOptions);
  return decodeDDPCMStream(ddpcmData, fd, tornadoOptions);
}

// Load the header incliding the q_tables and the scales.
ddpcmHeader *ddpcmLoadHeader(FILE *fd, int tornadoOptions) {
  uint32_t size;
  uint32_t numSamples;
  uint32_t numFrames;
  uint32_t numQTables;
  uint32_t framesPerQTable;

  tndo_fseek(fd, 0, SEEK_END);
  size = tndo_ftell(fd);
  tndo_fseek(fd, sizeof(TndoHeader), SEEK_SET);
  size -= sizeof(TndoHeader);

  ddpcmData = (ddpcmHeader *)tndo_malloc(sizeof(ddpcmHeader), 0);

  // Preamble, in network order:
  // uint32_t numSamples;
  // uint32_t numFrames;
  // uint32_t numQTables;
  // uint32_t framesPerQTable;

  tndo_fread(&numSamples, 1, sizeof(uint32_t), fd);
  tndo_fread(&numFrames, 1, sizeof(uint32_t), fd);
  tndo_fread(&numQTables, 1, sizeof(uint32_t), fd);
  tndo_fread(&framesPerQTable, 1, sizeof(uint32_t), fd);

  size -= 4 * sizeof(uint32_t);

  ddpcmData->numSamples = ENDI4(numSamples);
  ddpcmData->numFrames = ENDI4(numFrames);
  ddpcmData->numQTables = ENDI4(numQTables);
  ddpcmData->framesPerQTable = ENDI4(framesPerQTable);

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Number of samples: %u\n", ddpcmData->numSamples);
    printf("DEBUG - Number of frames: %u\n", ddpcmData->numFrames);
    printf("DEBUG - Number of q_tables: %u\n", ddpcmData->numQTables);
    printf("DEBUG - Frames per q_table: %u\n", ddpcmData->framesPerQTable);
  }

  ddpcmData->qtablesLeft =
      (int16_t **)tndo_malloc(ddpcmData->numQTables * sizeof(int16_t *), 0);
  ddpcmData->qtablesRight =
      (int16_t **)tndo_malloc(ddpcmData->numQTables * sizeof(int16_t *), 0);

  uint8_t *buffer = tndo_malloc(size * sizeof(uint8_t), 0);

  // Two qtables and two scales.
  uint32_t to_read =
      (2 * ddpcmData->numQTables * DDPCM_QTABLE_ENTRIES * sizeof(int16_t)) +
      (2 * ddpcmData->numFrames);
  tndo_fread(buffer, to_read, sizeof(uint8_t), fd);
  size -= to_read;

  int16_t *qtablesLeft;
  int16_t *qtablesRight;

  qtablesLeft = (int16_t *)buffer;
  qtablesRight = (int16_t *)buffer;
  qtablesRight += ddpcmData->numQTables * DDPCM_QTABLE_ENTRIES;

  for (uint32_t i = 0; i < ddpcmData->numQTables; i++) {
    ddpcmData->qtablesLeft[i] = qtablesLeft;
    ddpcmData->qtablesRight[i] = qtablesRight;
    qtablesLeft += DDPCM_QTABLE_ENTRIES;
    qtablesRight += DDPCM_QTABLE_ENTRIES;
  }

#ifndef __AMIGA__
  // q_table data is stored in network order.
  for (uint32_t i = 0; i < ddpcmData->numQTables; i++) {
    int16_t *ql = ddpcmData->qtablesLeft[i];
    int16_t *qr = ddpcmData->qtablesRight[i];
    for (uint32_t j = 0; j < DDPCM_QTABLE_ENTRIES; j++) {
      ql[j] = ntohs(ql[j]);
      qr[j] = ntohs(qr[j]);
    }
  }
#endif

  uint8_t *scales = (uint8_t *)buffer;
  scales += ddpcmData->numQTables * DDPCM_QTABLE_ENTRIES * sizeof(int16_t) * 2;
  ddpcmData->scalesLeft = scales;
  ddpcmData->scalesRight = scales + ddpcmData->numFrames;

  return ddpcmData;
}
