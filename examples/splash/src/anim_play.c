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
