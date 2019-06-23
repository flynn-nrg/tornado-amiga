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

// This is a dummy file
// Graphics on dev platform should use sdl_window.h services

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "graphics.h"
#include "memory.h"
#include "ptr_bridges.h"
#include "tornado_settings.h"

// Allocate planar buffers once to avoid wasting memory.
static int planarAllocated = 0;
static unsigned char *reusablePlanar1 = 0;
static unsigned char *reusablePlanar2 = 0;
static void *reusablePlanarOrig1 = 0;
static void *reusablePlanarOrig2 = 0;

void genPalette(unsigned int *pal, int numcols) {
  for (int i = 0; i < numcols; i++) {
    float f0 = i;
    float f1 = f0 / numcols;
    float f2 = pow(f1, 0.4545);
    int v = 255.0 * f2;
    pal[i] = (v) | (v << 8) | (v << 16);
  }
}

void loadRGB32toRGB(uint32_t *source, uint32_t *dst) {
  uint32_t numCols = source[0];

  numCols = ENDI4(numCols);
  numCols >>= 16;

  source++;
  uint32_t k = 0;
  for (uint32_t i = 0; i < numCols; i++) {
    uint32_t r = source[k++];
    r = ENDI4(r);
    r >>= 24;
    uint32_t g = source[k++];
    g = ENDI4(g);
    g >>= 24;
    uint32_t b = source[k++];
    b = ENDI4(b);
    b >>= 24;
    dst[i] = b | (g << 8) | (r << 16);
  }
}

void loadPalette(unsigned int *pal0rgb, int len) {}

graphics_t *initGraphics(graphicsOptions *gOptions) { return 0; }
