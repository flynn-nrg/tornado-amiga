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

#include "assets.h"
#include "memory.h"
#include "splash.h"

static void **splashAssets;
static int *splashSizes;

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
    splashAssets = (void **)tndo_malloc(sizeof(void *) * numFiles, 0);
    splashSizes = (int *)tndo_malloc(sizeof(int) * numFiles, 0);
    if (!loadAssets(&splashAssets[0], &splashFiles[0], &splashSizes[0],
                    numFiles, tornadoOptions, 0)) {
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
