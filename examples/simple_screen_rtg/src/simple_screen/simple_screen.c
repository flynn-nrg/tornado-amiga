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

#include "simple_screen.h"

static const char *assetList[] = {
    "data/Capsule_logo.tndo", // Compressed raw pixel data and palette.
};

static int displayInstance;
static char *background;

// Note that loadRGB32 palettes take up 770 bytes.
static uint32_t pal[770];

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

  effect->numAssets = sizeof(assetList) / sizeof(char *);
  effect->Assets = (void **)tndo_malloc(sizeof(void *) * effect->numAssets, 0);
  effect->assetSizes = (int *)tndo_malloc(sizeof(int) * effect->numAssets, 0);
  if (!loadAssets(effect->Assets, &assetList[0], effect->assetSizes,
                  effect->numAssets, tornadoOptions, 0)) {
    tndo_memory_shutdown(tornadoOptions);
    if (tornadoOptions & LOGGING) {
      printf("FATAL - Asset loading failed!\n");
    }
    exit(1);
  }

  // Palette is stored in LoadRGB32 format so we need to convert it first.
  loadRGB32toRGB((uint32_t *)effect->Assets[0], pal);

  // 320x256 8 bitplanes. No sprites and no padding.
  displayInstance = display_init((uint32_t *)effect->Assets[0], tornadoOptions, RTG_NORMAL, 0, 0, 0);

  // The first 3080 bytes are the palette in LoadRGB32 format.
  background = (char *)effect->Assets[0] + 3080;
}

t_canvas *renderSimpleScreen(int frame) {
  t_canvas *c = display_get(displayInstance);
  unsigned char *chunky = c->p.pix8;

  // Trivial copy...
  memcpy(chunky, background, 320 * 256);

  return c;
}

void freeSimpleScreen(tornadoEffect *effect) {
	display_end(displayInstance);
}
