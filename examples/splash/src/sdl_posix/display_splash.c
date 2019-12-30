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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include <assets.h>
#include <canvas.h>
#include <dev_window.h>
#include <keyboard.h>
#include <memory.h>
#include <ptr_bridges.h>
#include <tndo_assert.h>

#include "display_splash.h"

#define SCREEN_W (320)
#define SCREEN_H (180)

#define SCREEN_PADDING (100)

// Rocket control uses rocketTime and not the timer, so
// we don't need the delay. This is the default for normal
// operation.
static int flip_delay = 25;

// --------------------------------------------------
// Splash screen support
// --------------------------------------------------
static t_canvas _fb_splash = {.w = 1, .h = 1, .bypp = 1};
static unsigned int _pal256_splash[256];
static void **splashAssets = 0;
static int *splashSizes = 0;
static int splashHires = 0;

void display_set_palette_splash(void *pal) {
  uint32_t *source = (uint32_t *)pal;
  int numCols;
  int startCols;

  numCols = (ENDI4(source[0])) >> 16;
  int k = 0;
  source++;
  for (int i = 0; i < numCols; i++) {
    unsigned int r = source[k++];
    unsigned int g = source[k++];
    unsigned int b = source[k++];
    _pal256_splash[i] = ((ENDI4(r)) & 0xff000000) >> 8;
    _pal256_splash[i] |= ((ENDI4(g)) & 0xff000000) >> 16;
    _pal256_splash[i] |= ((ENDI4(b)) & 0xff000000) >> 24;
  }
}

void display_init_splash(const char *const *splashFiles, int numFiles,
                         int sizeX, int sizeY, int offset, int depth,
                         int tornadoOptions) {
  _fb_splash.w = sizeX;
  _fb_splash.h = sizeY;
  _fb_splash.bypp = 1;
  _fb_splash.p.pixels =
      tndo_malloc(_fb_splash.w * _fb_splash.h * _fb_splash.bypp, 0);

  if (numFiles > 0) {
    splashAssets = (void **)tndo_malloc(sizeof(void *) * numFiles, 0);
    splashSizes = (int *)tndo_malloc(sizeof(int) * numFiles, 0);
    if (!loadAssets(&splashAssets[0], &splashFiles[0], &splashSizes[0],
                    numFiles, tornadoOptions, 0)) {
      fprintf(stderr, "Failed to load splash asserts. Aborting");
      abort();
    }
  }

  if (sizeX > 320 || sizeY > 180) {
    splashHires = 1;
  }

  if (numFiles > 0) {
    display_set_palette_splash(splashAssets[0]);
  }

  dev_window_output_init(flip_delay, 0);
}

void display_waitvbl_splash(int num_frames) {}

int display_get_palette_skip_splash(void *pal) {
  uint32_t *source = (uint32_t *)pal;
  int numCols = (ENDI4(source[0])) >> 16;
  return ((numCols * 3 * sizeof(uint32_t)) + (2 * sizeof(uint32_t)));
}

t_canvas *display_get_splash() { return &_fb_splash; }

void display_forefront_splash() {}

void display_flip_splash() {
  int x, y;
  int y_delta;

  t_canvas *out = dev_window_get_canvas();
  tndo_assert((_fb_splash.w) <= out->w);
  tndo_assert((_fb_splash.h) <= out->h);

  if (splashHires) {
    for (y = 0; y < _fb_splash.h; y++) {
      unsigned int *dst_a = out->p.pix32 + y * out->w;

      for (x = 0; x < _fb_splash.w; x++) {
        unsigned int og = _fb_splash.p.pix8[y * _fb_splash.w + x];
        unsigned int c = _pal256_splash[og];

        dst_a[x] = c;
      }
    }

  } else {

    // This splash screen is 16:9. Centre it.
    y_delta = ((out->h / 2) - SCREEN_H) / 2;

    for (y = 0; y < _fb_splash.h; y++) {
      unsigned int *dst_a = out->p.pix32 + (y + y_delta) * 2 * out->w;
      unsigned int *dst_b = out->p.pix32 + (((y + y_delta) * 2) + 1) * out->w;
      for (x = 0; x < _fb_splash.w; x++) {
        unsigned int og = _fb_splash.p.pix8[y * _fb_splash.w + x];
        unsigned int c = _pal256_splash[og];

        dst_a[x * 2] = c;
        dst_a[x * 2 + 1] = c;
        dst_b[x * 2] = c;
        dst_b[x * 2 + 1] = c;
      }
    }
  }

  dev_window_output_flip();
}

void display_show_splash() {
  unsigned char *source = (unsigned char *)splashAssets[0];
  // Skip palette
  source += display_get_palette_skip_splash(splashAssets[0]);

  display_set_palette_splash(splashAssets[0]);

  memcpy(_fb_splash.p.pix8, source, _fb_splash.w * _fb_splash.h);
  display_flip_splash();
}

void display_end_splash(uint32_t *pal) {}
