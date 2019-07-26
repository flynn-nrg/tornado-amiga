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

ddpcmHeader *ddpcmLoadFile(FILE *fd, int tornadoOptions) {
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

  uint8_t *buffer = calloc(size, sizeof(uint8_t));
  tndo_fread(buffer, size, sizeof(uint8_t), fd);

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

  uint8_t *scales = (uint8_t *)buffer;
  *scales += ddpcmData->numQTables * DDPCM_QTABLE_ENTRIES * sizeof(int16_t) * 2;
  ddpcmData->scalesLeft = scales;
  ddpcmData->scalesRight = scales + ddpcmData->numFrames;

  ddpcmData->left = ddpcmData->scalesRight + ddpcmData->numFrames;
  ddpcmData->right = ddpcmData->left + ddpcmData->numSamples * sizeof(int16_t);

  return ddpcmData;
}
