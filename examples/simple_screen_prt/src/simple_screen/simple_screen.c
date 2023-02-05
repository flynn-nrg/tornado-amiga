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

#include "simple_screen.h"

static TornadoAsset assetList[] = {
    {
        .Name = (uint8_t *)"data/Capsule_logo.tndo", // Compressed raw pixel
                                                     // data and palette.
    },
};

static int displayInstance;
static char *background;
static uint32_t pal[256];

void flipSimpleScreen(int requires_forefront) {
  if (requires_forefront) {
    display_forefront(displayInstance);
  }

  display_flip(displayInstance);
}

void initSimpleScreen(unsigned int tornadoOptions, tornadoEffect *effect) {

  static int init = 0;
  if (init)
    return;
  init = 1;

  effect->numAssets = sizeof(assetList) / sizeof(TornadoAsset);
  effect->Assets = assetList;
  if (!loadAssets(assetList, effect->numAssets, tornadoOptions, 0)) {
    tndo_memory_shutdown(tornadoOptions);
    if (tornadoOptions & LOGGING) {
      printf("FATAL - Asset loading failed!\n");
    }
    exit(1);
  }

  // Palette is stored in LoadRGB32 format so we need to convert it first.
  loadRGB32toRGB((uint32_t *)effect->Assets[0].Data, pal);

  // 320x256 8 bitplanes. No sprites and no padding.
  displayInstance = display_init(pal, tornadoOptions, SCR_NORMAL, 0, 0, 0);

  // The first 3080 bytes are the palette in LoadRGB32 format.
  background = (char *)effect->Assets[0].Data + 3080;
}

t_canvas *renderSimpleScreen(int frame) {
  t_canvas *c = display_get(displayInstance);
  unsigned char *chunky = c->p.pix8;

  // Trivial copy...
  memcpy(chunky, background, 320 * 256);

  return c;
}

void freeSimpleScreen(tornadoEffect *effect) {}
