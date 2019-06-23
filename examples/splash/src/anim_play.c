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
#include <string.h>

#include <ptr_bridges.h>

#include "anim_play.h"

#define FRAME_SIZE 320 * 180
#define FRAME_ROWS 180
#define FRAME_X 320
#define BITMAP_LEN 32
#define SEGMENT_LEN 10

static unsigned char *payload;
static int num_frames;
static int last_decoded;

// Init decoder.
// Data format:
// 0 - double word with the number of frames.
// 4 - frame 0.
// FRAME_X * FRAME_ROWS - double word bitmap followed by segments.
int decoder_init(unsigned char *data) {
  last_decoded = 0;
  int *num_frames_ptr = (int *)data;
  num_frames = num_frames_ptr[0];
  payload = data + sizeof(uint32_t);
  return ENDI4(num_frames);
}

void decode_frame(unsigned char *output_buffer) {
  uint32_t bitmap;
  uint32_t *bptr;

  // First frame is stored in full.
  if (last_decoded == 0) {
    memcpy(output_buffer, payload, FRAME_SIZE);
    payload += FRAME_SIZE;
    last_decoded++;
  } else {
    for (int j = 0; j < FRAME_ROWS; j++) {
      bptr = (uint32_t *)payload;
      bitmap = *bptr;
      bitmap = ENDI4(bitmap);
      payload += sizeof(uint32_t);
      uint32_t mask = 1 << 31;
      for (int i = 0; i < BITMAP_LEN; i++) {
        if (bitmap & mask) {
          memcpy(&output_buffer[(j * FRAME_X) + (i * SEGMENT_LEN)], payload,
                 SEGMENT_LEN);
          payload += SEGMENT_LEN;
        }
        mask = mask >> 1;
      }
    }
  }
}
