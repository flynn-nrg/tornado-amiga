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

#include "assets.h"
#include "memory.h"
#include "splash.h"

static TornadoAsset *splashAssets;

static int palette[256 * 3 + 2];

static int cur_screen = 0;
static int isChunkyBuffer = 0;
static int screenSizeX, screenSizeY, screenDepth;
static unsigned char *chunkybuffer;

void splash_end() {}

unsigned char *splash_init(const char *const *splashFiles, int numFiles,
                           int sizeX, int sizeY, int offset, int depth,
                           int tornadoOptions) {
  screenSizeX = sizeX;
  screenSizeY = sizeY;
  screenDepth = depth;

  if (numFiles > 0) {
    splashAssets =
        (TornadoAsset *)tndo_malloc(sizeof(TornadoAsset) * numFiles, 0);
    for (int i = 0; i < numFiles; i++) {
      splashAssets[i].Name = (uint8_t *)splashFiles[i];
    }
    if (!loadAssets(splashAssets, numFiles, tornadoOptions, 0)) {
      return 0;
    }
  }

  if (!chunkybuffer) {
    chunkybuffer = (unsigned char *)tndo_malloc(
        sizeX * sizeY * (depth < 8 ? 1 : (depth / 8)), 0);
  }
  return chunkybuffer;
}

void splash_fadeout(uint32_t *pal, int numSteps) {}

void splash_show() {}

void splash_set_palette(void *pal) {}

void splash_forefront() {}

void splash_swap_screens(void) {}

int splash_checkinput(void) {
  int mustExit = 0;

  return mustExit;
}

int splash_load_sample(const char *sampleFile, unsigned int i) { return 0; }

void splash_play_sample() {}

void splash_end_sample() {}
