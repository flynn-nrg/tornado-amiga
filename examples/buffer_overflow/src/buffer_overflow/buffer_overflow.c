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
