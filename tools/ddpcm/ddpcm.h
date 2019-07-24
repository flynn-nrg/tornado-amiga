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

#ifndef DDPCM_H
#define DDPCM_H

#define DDPCM_COMPRESSED_FRAME_SIZE 40
#define DDPCM_FRAME_NUMSAMPLES 50
#define DDPCM_MAX_TABLES 1024
#define DDPCM_MIN_TABLES 128
#define DDPCM_MAX_SCALING 64

typedef struct {
  uint32_t numSamples;
  uint32_t numFrames;
  uint32_t numQTables;
  uint32_t framesPerQTable;
  int16_t **qtablesLeft;
  int16_t **qtablesRight;
  uint8_t *scalesLeft;
  uint8_t *scalesRight;
  uint8_t *left;
  uint8_t *right;
} ddpcmHeader;

uint8_t encodeFrame(int16_t *src, uint8_t *dst, int16_t *q_table);
void decodeFrame(uint8_t *src, int16_t *dst, int16_t *q_table, uint8_t scale);
uint32_t framesPerQtable(uint32_t numFrames, uint32_t maxTables,
                         uint32_t minTables);

#endif
