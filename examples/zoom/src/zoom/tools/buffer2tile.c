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
