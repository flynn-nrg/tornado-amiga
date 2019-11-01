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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include <assets.h>
#include <canvas.h>
#include <dev_window.h>
#include <display.h>
#include <keyboard.h>
#include <memory.h>
#include <ptr_bridges.h>
#include <tndo_assert.h>

// Rocket control uses rocketTime and not the timer, so
// we don't need the delay. This is the default for normal
// operation.
static int flip_delay = 25;

void sdl_display_set_delay(int delay) { flip_delay = delay; }

static unsigned int BlendARGB(unsigned int pix0, unsigned int pix1,
                              int alphafx8) {
  unsigned int ag0 = (pix0 & 0xff00ff00) >> 8;
  unsigned int rb0 = pix0 & 0x00ff00ff;
  unsigned int ag1 = (pix1 & 0xff00ff00) >> 8;
  unsigned int rb1 = pix1 & 0x00ff00ff;

  unsigned int ag =
      ((ag0 * alphafx8 + ag1 * (0x100 - alphafx8)) >> 8) & 0xff00ff;
  unsigned int rb =
      ((rb0 * alphafx8 + rb1 * (0x100 - alphafx8)) >> 8) & 0xff00ff;

  return ((ag << 8) | rb);
}

void display_null_init() {}

// ---------------------------------------------------
// Unified display.
// ---------------------------------------------------

static display_instance *di;
static int maxDisplayInstances;
static int display_last_instance = 0;
static int display_1x2 = 0;
static int rocketControl = 0;

void display_subsystem_init(int numInstances) {
  di = tndo_malloc(numInstances * sizeof(display_instance), 0);
  maxDisplayInstances = numInstances;
}

void display_subsystem_end() {
#ifdef __DEBUG_CODE
  printf("DEBUG - display_subsystem_end(): %u/%u display instances used.\n",
         display_last_instance, maxDisplayInstances);
#endif
}

int display_init(unsigned int *pal, unsigned int options, int mode,
                 unsigned int padding_top, unsigned int padding_bottom,
                 sprite_options *so) {

  tndo_assert(display_last_instance < maxDisplayInstances);

  int scale_x = 0;
  int scale_y = 0;
  int width = 0;
  int height = 0;
  int bypp = 1;
  int is16_9 = 0;

  switch (mode) {
  case SCR_NORMAL:
  case SCR_NORMAL_6BPL:
    scale_x = 2;
    scale_y = 2;
    width = 320;
    height = 256;
    break;
  case SCR_16_9_HL_8BPL:
    scale_x = 1;
    scale_y = 1;
    width = 640;
    height = 360;
    is16_9 = 1;
    break;
  case SCR_16_9:
  case SCR_16_9_6BPL:
  case SCR_16_9_6_4_BPL:
  case SCR_16_9_5BPL:
  case SCR_16_9_4BPL:
    scale_x = 2;
    scale_y = 2;
    width = 320;
    height = 180;
    is16_9 = 1;
    break;
  }

  di[display_last_instance].fb.w = width;
  di[display_last_instance].fb.h = height;
  di[display_last_instance].fb.sx = scale_x;
  di[display_last_instance].fb.sy = scale_y;
  di[display_last_instance].fb.bypp = bypp;
  di[display_last_instance].is16_9 = is16_9;
  di[display_last_instance].fb.p.pixels = tndo_malloc(
      di[display_last_instance].fb.w *
          (di[display_last_instance].fb.h + padding_top + padding_bottom) *
          di[display_last_instance].fb.bypp,
      0);

  di[display_last_instance].paddingTop = padding_top;
  di[display_last_instance].paddingBottom = padding_bottom;

  memset(di[display_last_instance].fb.p.pixels, 0,
         di[display_last_instance].fb.w * di[display_last_instance].fb.h *
             di[display_last_instance].fb.bypp);

  for (int i = 0; i < 256; i++) {
    di[display_last_instance].pal256[i] = pal[i];
  }

  if (options & ENABLE_ROCKET) {
    dev_window_output_init(flip_delay, DEV_WINDOW_ROCKET);
    rocketControl = 1;
  } else {
    dev_window_output_init(flip_delay, DEV_WINDOW_NORMAL);
  }

  int current_instance = display_last_instance;
  display_last_instance++;
  return current_instance;
}

t_canvas *display_get(int instance) { return &di[instance].fb; }

void display_set_sprites_pos(int instance, sprite_options *so) {}

void display_flip(int instance) {
  int x, y;
  int y_delta;
  int modulo = 0;

  t_canvas *out = dev_window_get_canvas();
  tndo_assert((di[instance].fb.w * di[instance].fb.sx) <= out->w);
  tndo_assert((di[instance].fb.h * di[instance].fb.sy) <= out->h);

  if (di[instance].is16_9) {
    y_delta = ((out->h / 2) - (di[instance].fb.h)) / 2;
  } else {
    y_delta = 0;
  }

  if (rocketControl) {
    modulo = 640;
  }

#if 0
  for (y = 0; y < di[instance].fb.h; y++) {
    unsigned int *dst_a = out->p.pix32 + (y + y_delta) * 2 * (out->w + modulo);
    unsigned int *dst_b =
        out->p.pix32 + (((y + y_delta) * 2) + 1) * (out->w + modulo);
    for (x = 0; x < di[instance].fb.w; x++) {
      int newy = display_1x2 ? (y >> 1) : y;
      unsigned int og =
          di[instance]
              .fb.p
              .pix8[(newy + di[instance].paddingTop) * di[instance].fb.w + x];
      unsigned int c = di[instance].pal256[og];

      dst_a[x * 2] = c;
      dst_a[x * 2 + 1] = c;
      dst_b[x * 2] = c;
      dst_b[x * 2 + 1] = c;
    }
  }
#else
  int yStart = 0;
  int yStop = out->h;
  int yOffset = 0;
  if (di[instance].is16_9) {
      yStop = yStop * 9 / 16;
      yOffset += (out->h - yStop) / 2;
  }
  for (y = yStart ; y < yStop ; y++) {
      int ySrc = y / di[instance].fb.sy;
      tndo_assert(ySrc < di[instance].fb.h);
      unsigned int *dst = out->p.pix32 + (yOffset + y) * (out->w + modulo);
      uint8_t      *src = di[instance].fb.p.pix8 + ySrc * di[instance].fb.w;
      for (x = 0 ; x < out->w ; x++) {
          int xSrc = x / di[instance].fb.sx;
          tndo_assert(xSrc < di[instance].fb.w);
          unsigned int og = src[xSrc];
          dst[x] = di[instance].pal256[og];
      }
  }
#endif

  dev_window_output_flip();
}

void display_forefront(int instance) {}

void display_end(int instance) { dev_window_output_close(); }

void display_1x2_set(int enable) { display_1x2 = enable; }

void display_copper(int instance) {}

void display_set_fade_black(int a, int b) {}

void display_set_fade_white(int a, int b) {}
