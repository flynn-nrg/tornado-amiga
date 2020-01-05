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
#include <unistd.h>

#define BUFFER_SIZE 320 * 180
#define TILE_SIZE 256 * 256
#define TILE_NUM_COLS 16.0f

static void resample(uint8_t *src, uint8_t *dst) {
  float delta = 320.0f / 256.0f;

  for (uint32_t y = 0; y < 180; y++) {
    float xorig = 0.0f;
    for (uint32_t x = 0; x < 255; x++) {
      dst[x + (y * 256)] = src[(int)xorig + (y * 320)];
      xorig += delta;
    }
  }
}

static float compute_color_scale(uint8_t *buffer) {
  uint32_t max_value = 0;
  for (uint32_t i = 0; i < TILE_SIZE; i++) {
    if (((uint32_t)buffer[i]) > max_value) {
      max_value = (uint32_t)buffer[i];
    }
  }

  float scale = TILE_NUM_COLS / (float)max_value;

  return scale;
}

static void downscale_colour(uint8_t *buffer, float scale) {
  for (uint32_t i = 0; i < TILE_SIZE; i++) {
    buffer[i] = (uint8_t)((float)buffer[i] * scale);
  }
}

int main(int argc, char **argv) {

  if (argc != 3) {
    fprintf(stderr, "Usage: %s buffer.raw tile.raw\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  FILE *in = fopen(argv[1], "r");
  if (!in) {
    fprintf(stderr, "FATAL - Could not open <%s> for reading.\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  FILE *out = fopen(argv[2], "w");
  if (!out) {
    fprintf(stderr, "FATAL - Could not open <%s> for writing.\n", argv[2]);
    exit(EXIT_FAILURE);
  }

  uint8_t *src = calloc(BUFFER_SIZE, 1);
  uint8_t *dst = calloc(TILE_SIZE, 1);

  size_t read = fread(src, BUFFER_SIZE, 1, in);

  if (!read) {
    fprintf(stderr, "FATAL - Could no read data from <%s>.\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  fclose(in);

  resample(src, dst);

  float scale = compute_color_scale(dst);
  downscale_colour(dst, scale);

  size_t written = fwrite(dst, TILE_SIZE, 1, out);

  if (!written) {
    fprintf(stderr, "FATAL - Could no write data to <%s>.\n", argv[2]);
    exit(EXIT_FAILURE);
  }

  fclose(out);

  free(src);
  free(dst);

  exit(EXIT_SUCCESS);
}
