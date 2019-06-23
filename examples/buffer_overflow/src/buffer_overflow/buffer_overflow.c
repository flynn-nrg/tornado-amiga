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

#include <math.h>

#include <assets.h>
#include <canvas.h>
#include <chrono.h>
#include <debug.h>
#include <display.h>
#include <imgui_overlay.h>
#include <memory.h>
#include <prof.h>
#include <telemetry.h>
#include <tndo_assert.h>
#include <tornado_settings.h>

#include <demo.h>

#include "buffer_overflow.h"

static int displayInstance;
static uint32_t pal[256];

void flipBufferOverflow(int requires_forefront) {
  if (requires_forefront) {
    display_forefront(displayInstance);
  }

#ifdef __DEBUG_CODE
  prof_reset_chrono();
#endif

  display_flip(displayInstance);

#ifdef __DEBUG_CODE
  prof_get_time("c2p", 320 * 256);
#endif
}

void initBufferOverflow(unsigned int tornadoOptions, tornadoEffect *effect) {

  static int init = 0;
  if (init)
    return;
  init = 1;

  // Generate palette.
  for (int i = 0; i < 255; i++) {
    float f0 = i;
    float f1 = f0 / 255;
    float f2 = pow(f1, 0.4545);
    int v = 255.0 * f2;
    pal[i] = (v) | (v << 8) | (v << 16);
  }

  // 320x256 8 bitplanes. No sprites and no padding.
  displayInstance = display_init(pal, tornadoOptions, SCR_NORMAL, 0, 0, 0);
}

t_canvas *renderBufferOverflow(int frame) {
  t_canvas *c = display_get(displayInstance);
  unsigned char *chunky = c->p.pix8;

#ifdef __DEBUG_CODE
  prof_reset_chrono();
#endif

  // There's a bug in this code. This will be caught by the sanitizer on the
  // posix target.
  for (int y = 0; y < 320; y++) {
    for (int x = 0; x < 320; x++) {
      chunky[(y * 320) + x] = (unsigned char)y;
    }
  }

#ifdef __DEBUG_CODE
  prof_get_time("copy", c->w * c->h);
#endif

  return c;
}

void freeBufferOverflow(tornadoEffect *effect) {}
