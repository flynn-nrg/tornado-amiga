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
