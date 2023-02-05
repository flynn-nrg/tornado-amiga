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

// For the zoom editor.
#include <keyboard.h>
#include <ptr_bridges.h>
#include <system.h>

#include "zoom.h"
#ifdef __AMIGA__
#include "amiga/zoom_inner.h"
#endif
#include "zoom_util.h"

// Rocket control
#include "sync.h"

// Rocket exported data
#include "zoom_x.h"
#include "zoom_y.h"
#include "zoom_z.h"

// --------------------------------------------------------------------------
// Zoom effect settings.
// --------------------------------------------------------------------------
#define ZOOM_SCREEN_X 320
#define ZOOM_SCREEN_Y 180
#define NUM_TILES 16
#define TILE_SIZE 256 * 256
#define NUM_INSTANCES 4
#define ROUND_ROBIN 2
#define FRAME_0 effect->Assets[2].Data

// ---------------------------------------------------------------------------
// Zoom variables.
// ---------------------------------------------------------------------------
static unsigned char ***allTxtPtr1;
static unsigned char ***allTxtPtr2;
static unsigned char ***allTxtPtr3;
static unsigned char ***allTxtPtr4;

static unsigned char ***allMipMaps1;
static unsigned char ***allMipMaps2;
static unsigned char ***allMipMaps3;
static unsigned char ***allMipMaps4;

static int zoomIterations1;
static int zoomIterations2;
static int zoomIterations3;
static int zoomIterations4;

static zoomIter *iterations1;
static zoomIter *iterations2;
static zoomIter *iterations3;
static zoomIter *iterations4;

static unsigned int _pal_tmp[256];
static float classic_div = 320.0f;
static unsigned char **canvas1;
static unsigned char **canvas2;
static unsigned char **canvas3;
static unsigned char **canvas4;
static int round_robin = 0;

#ifdef __DEBUG_CODE
static int telemetryEnabled;
static TelemetryData **zoomTelemetry;
#endif

static TornadoAsset assetList[] = {
    {
        .Name = (uint8_t *)"data/zoom.tndo", // Roundrobin 1
    },
    {
        .Name = (uint8_t *)"data/zoom2.tndo", // Roundrobin 2
    },
    {
        .Name = (uint8_t *)"data/zoom_frame0.tndo", // Frame 0
    },
    {
        .Name = (uint8_t *)"data/zoom.pal", // Palette
    },
};

static unsigned int pal16[] = {0x1070a,  0x1070a,  0x81015,  0x161e23,
                               0x272f34, 0x394146, 0x4d555a, 0x626a6f,
                               0x777f84, 0x8b9398, 0xa0a8ad, 0xb5bdc2,
                               0xc7cfd4, 0xd8e0e5, 0xe6eef3, 0xf2faff};

// Rocket control...
static const struct sync_track *zoom_z;
static const struct sync_track *zoom_delta_x;
static const struct sync_track *zoom_delta_y;
static const struct sync_track *zoom_instance;

// Sliders
static imguiRocketSlider sliders[4];

void tracksZoom(struct sync_device *rocket) {
#ifndef __AMIGA__
  zoom_z = sync_get_track(rocket, "zoom.z");
  zoom_delta_x = sync_get_track(rocket, "zoom.deltaX");
  zoom_delta_y = sync_get_track(rocket, "zoom.deltaY");
  zoom_instance = sync_get_track(rocket, "zoom.instance");

  snprintf(sliders[0].varName, 255, "%s", "zoom.z");
  snprintf(sliders[1].varName, 255, "%s", "zoom.deltaX");
  snprintf(sliders[2].varName, 255, "%s", "zoom.deltaY");
  snprintf(sliders[3].varName, 255, "%s", "zoom.instance");

  sliders[0].attached = 1;
  sliders[0].min = 320.0f;
  sliders[0].max = 81600.0f;

  sliders[1].attached = 1;
  sliders[1].min = -4000.0f;
  sliders[1].max = 4000.0f;

  sliders[2].attached = 1;
  sliders[2].min = -4000.0f;
  sliders[2].max = 4000.0f;

  sliders[3].attached = 1;
  sliders[3].min = 0.0f;
  sliders[3].max = 3.0f;

#endif
}

static float current_z;
static float current_delta_x;
static float current_delta_y;
static int current_instance;
static unsigned char ***rocketInstances[NUM_INSTANCES];

static int displayInstance;

void trackDataZoom(int row, imguiOverlayData *overlayData) {
#ifndef __AMIGA__
  if (sliders[0].attached) {
    current_z = sync_get_val(zoom_z, row);
  } else {
    current_z = sliders[0].current;
  }

  if (sliders[1].attached) {
    current_delta_x = sync_get_val(zoom_delta_x, row) / 10000.0f;
  } else {
    current_delta_x = sliders[1].current;
  }

  if (sliders[2].attached) {
    current_delta_y = sync_get_val(zoom_delta_y, row) / 10000.0f;
  } else {
    current_delta_y = sliders[2].current;
  }

  if (sliders[3].attached) {
    current_instance = (int)sync_get_val(zoom_instance, row);
  } else {
    current_instance = sliders[3].current;
  }

  sliders[0].current = current_z;
  sliders[1].current = current_delta_x;
  sliders[2].current = current_delta_y;
  sliders[3].current = current_instance;

  overlayData->sliderNum = 4;
  overlayData->sliders = sliders;

#endif
}

void flipZoom(int requires_forefront) {
  if (requires_forefront) {
    display_forefront(displayInstance);
  }

#ifdef __DEBUG_CODE
  prof_reset_chrono();
#endif

  display_flip(displayInstance);

#ifdef __DEBUG_CODE
  float f = prof_get_time("c2p", ZOOM_SCREEN_X * ZOOM_SCREEN_Y);
  if (telemetryEnabled) {
    appendTelemetry(f, zoomTelemetry[1]);
  }
#endif
}

static void fixIter(zoomIter *iteration, int src, int dst) {
  iteration[dst].mipMapIdx = iteration[src].mipMapIdx;
  for (int x = 0; x < 320; x++) {
    iteration[dst].ix[x] = iteration[src].ix[x];
    iteration[dst].iTileX[x] = iteration[src].iTileX[x];
  }

  for (int y = 0; y < 180; y++) {
    iteration[dst].iy[y] = iteration[src].iy[y];
    iteration[dst].iTileY[y] = iteration[src].iTileY[y];
  }
}

void initZoom(unsigned int tornadoOptions, tornadoEffect *effect) {

  static int init = 0;
  if (init)
    return;
  init = 1;

#ifdef __DEBUG_CODE
  // Telemetry
  telemetryEnabled = effect->wantTelemetry;

  if (telemetryEnabled) {
    zoomTelemetry = effect->telemetry;
  }
#endif
  effect->numAssets = sizeof(assetList) / sizeof(TornadoAsset);
  effect->Assets = (TornadoAsset *)assetList;
  if (!loadAssets(assetList, effect->numAssets, tornadoOptions, 0)) {
    tndo_memory_shutdown(tornadoOptions);
    if (tornadoOptions & LOGGING) {
      printf("failed!\n");
    }
    exit(1);
  }

  // ---------------------------------------------------------------------------
  // Allocate buffers and generate copper lists and palettes.
  // ---------------------------------------------------------------------------
  loadRGB32toRGB((uint32_t *)effect->Assets[3].Data, _pal_tmp);

  displayInstance =
      display_init(_pal_tmp, tornadoOptions, SCR_16_9_4BPL, 0, 0, 0);

  // ---------------------------------------------------------------------------
  // Effect initialization.
  // ---------------------------------------------------------------------------
  // Packed zoom data...
  canvas1 =
      (unsigned char **)tndo_malloc(NUM_TILES * sizeof(unsigned char *), 0);
  int delta = 0;
  for (int i = 0; i < NUM_TILES; i++) {
    canvas1[i] = ((unsigned char *)effect->Assets[0].Data) + delta;
    delta += TILE_SIZE;
  }

  canvas2 =
      (unsigned char **)tndo_malloc(NUM_TILES * sizeof(unsigned char *), 0);
  delta = 0;
  for (int i = 0; i < NUM_TILES; i++) {
    canvas2[i] = ((unsigned char *)effect->Assets[1].Data) + delta;
    delta += TILE_SIZE;
  }

  // Mipmaps...
  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Generating mipmaps...");
  }

  allMipMaps1 =
      (unsigned char ***)tndo_malloc(NUM_TILES * sizeof(unsigned char **), 0);
  for (int i = 0; i < NUM_TILES; i++) {
    allMipMaps1[i] = (unsigned char **)tndo_malloc(
        ZOOM_MIPMAPS * sizeof(unsigned char *), 0);
    int mSize = ZOOM_TXT_SIZE;
    unsigned char **mipMaps1 = allMipMaps1[i];
    mipMaps1[0] = canvas1[i];

    for (int j = 1; j < ZOOM_MIPMAPS; j++) {
      mipMaps1[j] = downSample(mipMaps1[j - 1], mSize);
      mSize /= 2;
    }
  }

  allMipMaps2 =
      (unsigned char ***)tndo_malloc(NUM_TILES * sizeof(unsigned char **), 0);
  for (int i = 0; i < NUM_TILES; i++) {
    allMipMaps2[i] = (unsigned char **)tndo_malloc(
        ZOOM_MIPMAPS * sizeof(unsigned char *), 0);
    int mSize = ZOOM_TXT_SIZE;
    unsigned char **mipMaps2 = allMipMaps2[i];
    mipMaps2[0] = canvas2[i];

    for (int j = 1; j < ZOOM_MIPMAPS; j++) {
      mipMaps2[j] = downSample(mipMaps2[j - 1], mSize);
      mSize /= 2;
    }
  }

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("done\n");
  }

  // Transform texture into pointer to pointer arrays (1 for each mipmap level)
  allTxtPtr1 = (unsigned char ***)tndo_malloc(
      ZOOM_MIPMAPS * sizeof(unsigned char **), 0);
  for (int i = 0; i < ZOOM_MIPMAPS; i++) {
    allTxtPtr1[i] = (unsigned char **)tndo_malloc(
        ZOOM_TXT_SIZE * ZOOM_TXT_SIZE * sizeof(unsigned char *), 0);
  }

  allTxtPtr2 = (unsigned char ***)tndo_malloc(
      ZOOM_MIPMAPS * sizeof(unsigned char **), 0);
  for (int i = 0; i < ZOOM_MIPMAPS; i++) {
    allTxtPtr2[i] = (unsigned char **)tndo_malloc(
        ZOOM_TXT_SIZE * ZOOM_TXT_SIZE * sizeof(unsigned char *), 0);
  }

  allTxtPtr3 = (unsigned char ***)tndo_malloc(
      ZOOM_MIPMAPS * sizeof(unsigned char **), 0);
  for (int i = 0; i < ZOOM_MIPMAPS; i++) {
    allTxtPtr3[i] = (unsigned char **)tndo_malloc(
        ZOOM_TXT_SIZE * ZOOM_TXT_SIZE * sizeof(unsigned char *), 0);
  }

  allTxtPtr4 = (unsigned char ***)tndo_malloc(
      ZOOM_MIPMAPS * sizeof(unsigned char **), 0);
  for (int i = 0; i < ZOOM_MIPMAPS; i++) {
    allTxtPtr4[i] = (unsigned char **)tndo_malloc(
        ZOOM_TXT_SIZE * ZOOM_TXT_SIZE * sizeof(unsigned char *), 0);
  }

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Computing texture pointers and loop constants...");
  }

  // For rocket control.
  rocketInstances[0] = allTxtPtr1;
  rocketInstances[1] = allTxtPtr2;
  rocketInstances[2] = allTxtPtr3;
  rocketInstances[3] = allTxtPtr4;

  zoomIterations1 = 300;
  unsigned char *initial_image = (unsigned char *)FRAME_0;
  initial_image += 200; // skip palette.

  iterations1 =
      initIterations(initial_image, allTxtPtr1, allMipMaps1, allMipMaps2,
                     allMipMaps3, allMipMaps4, zoomIterations1, 0);

  zoomIterations2 = 300;
  iterations2 = initIterations(canvas2[8], allTxtPtr2, allMipMaps1, allMipMaps2,
                               allMipMaps3, allMipMaps4, zoomIterations2, 300);

  zoomIterations3 = 300;
  iterations3 = initIterations(canvas1[6], allTxtPtr3, allMipMaps1, allMipMaps2,
                               allMipMaps3, allMipMaps4, zoomIterations3, 600);

  zoomIterations4 = 300;
  iterations4 = initIterations(canvas1[9], allTxtPtr4, allMipMaps1, allMipMaps2,
                               allMipMaps3, allMipMaps4, zoomIterations4, 900);

  // Iteration 0 needs to be copied from 1.
  fixIter(iterations1, 1, 0);
  fixIter(iterations2, 1, 0);
  fixIter(iterations3, 1, 0);

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("done\n");
  }
}

zoomIter *initIterations(unsigned char *texture, unsigned char ***allTxtPtr,
                         unsigned char ***allMipMaps1,
                         unsigned char ***allMipMaps2,
                         unsigned char ***allMipMaps3,
                         unsigned char ***allMipMaps4, int numIter,
                         int zoom_track_start) {

  float fx;
  float fy;
  int iy;
  unsigned char ix;
  int texelIndex, textureIndex;
  unsigned char **mipMaps;

  float tileY;
  float tileX;
  float foo;
  float tX;
  float tY;
  float tileOrigX;
  float tileOrigY;
  float tileSize;

  zoomIter *iterations;
  unsigned char **txtPtr;

  round_robin = 0;

  // Replace each pixel by a pointer to its texture for each mipmap level.
  for (int j = 0; j < ZOOM_MIPMAPS; j++) {
    txtPtr = allTxtPtr[j];
    for (int i = 0; i < ZOOM_TXT_SIZE * ZOOM_TXT_SIZE; i++) {
      texelIndex = (int)texture[i];
      textureIndex = texelIndex;
      switch (round_robin % ROUND_ROBIN) {
      case 0:
        mipMaps = allMipMaps1[textureIndex];
        break;
      case 1:
        mipMaps = allMipMaps2[textureIndex];
        break;
      case 2:
        mipMaps = allMipMaps3[textureIndex];
        break;
      case 3:
        mipMaps = allMipMaps4[textureIndex];
        break;
      }

      round_robin++;

      txtPtr[i] = mipMaps[j];
    }
  }

  iterations = (zoomIter *)tndo_malloc(numIter * sizeof(zoomIter), 0);

  for (int i = 0; i < numIter; i++) {
    float div = sync_zoom_z_track[zoom_track_start + i];
    float delta = 256.0f / div;
    float m_delta = delta * 256.0f;
    float xorig =
        (256.0f -
         ((delta + sync_zoom_deltaX_track[zoom_track_start + i]) * 320.0f)) /
        2.0f;
    float yorig =
        (256.0f -
         ((delta + sync_zoom_deltaY_track[zoom_track_start + i]) * 320.0f)) /
        2.0f;
    float pixel_size = div / 256.0f;

    fx = xorig;
    fy = yorig;
    tX = modff(xorig, &foo);
    tY = modff(yorig, &foo);

    // Choose the best mipmap for each tile size.
    if (pixel_size > 128.0) {
      iterations[i].mipMapIdx = 0;
      tileSize = 256.0f;

    } else if (pixel_size > 64.0f) {
      iterations[i].mipMapIdx = 1;
      m_delta /= 2.0f;
      tileSize = 128.0f;
    } else if (pixel_size > 32.0f) {
      iterations[i].mipMapIdx = 2;
      m_delta /= 4.0f;
      tileSize = 64.0f;
    } else if (pixel_size > 16.0f) {
      iterations[i].mipMapIdx = 3;
      m_delta /= 8.0f;
      tileSize = 32.0f;
    } else if (pixel_size > 8.0f) {
      iterations[i].mipMapIdx = 4;
      m_delta /= 16.0f;
      tileSize = 16.0f;
    } else {
      iterations[i].mipMapIdx = 5;
      m_delta /= 32.0f;
      tileSize = 8.0f;
    }

    tileOrigX = tX * tileSize;
    tileOrigY = tY * tileSize;
    tileY = tileOrigY;
    tileX = tileOrigX;

    if (tileY >= tileSize) {
      tileY = 0.0f;
    }

    if (tileX >= tileSize) {
      tileX = 0.0f;
    }

    unsigned char prevx;
    ix = (unsigned char)floorf(fx);
    prevx = ix;
    for (int x = 0; x < 320; x++) {
      ix = (unsigned char)floorf(fx);
      if (prevx != ix) {
        tileX = 0;
        prevx = ix;
      }

      iterations[i].ix[x] = ix;
      iterations[i].iTileX[x] = (int)floorf(tileX);
      fx = fx + delta;
      tileX = tileX + m_delta;
    }

    int prevy;
    iy = (int)floorf(fy);
    prevy = iy;
    for (int y = 0; y < 180; y++) {
      iy = (int)floorf(fy);
      if (tileY > (tileSize)) {
        tileY = tileSize;
      }
      if (prevy < iy) {
        tileY = 0;
        prevy = iy;
      }
      iterations[i].iy[y] = iy * 256;
      int tyTemp = (int)floorf(tileY);
      iterations[i].iTileY[y] = (tyTemp * (int)tileSize);
      tileY = tileY + m_delta;
      fy = fy + delta;
    }
  }

  return iterations;
}

void saturate(unsigned char *chunky, int numPixels, int threshold) {
  for (int i = 0; i < numPixels; i++) {
    if (15 - chunky[i] <= threshold) {
      chunky[i] = 15;
    }
  }
}

void blur(unsigned char *chunky, int w, int h, int numPasses) {
  for (int p = 0; p < numPasses; p++) {
    for (int y = 0; y < h; y += 2) {
      for (int x = 0; x < w; x += 2) {
        unsigned char c =
            (chunky[x + w * y] + chunky[x + 1 + w * y] +
             chunky[x + (y + 1) * w] + chunky[x + 1 + (y + 1) * w]) /
            4;
        chunky[x + w * y] = c;
        chunky[x + 1 + w * y] = c;
        chunky[x + (y + 1) * w] = c;
        chunky[x + 1 + (y + 1) * w] = c;
      }
    }
  }
}

#define BASE_FRAME 0
#define LAST_FRAME 1200
#define FADE_FRAMES 16

// Zoom VBL callback
void vblZoom(int frame) {

  if ((frame >= (LAST_FRAME - FADE_FRAMES)) && (frame < LAST_FRAME)) {
    display_set_fade_white(displayInstance, FADE_FRAMES - (LAST_FRAME - frame));
  }

#ifdef __AMIGA__
  display_set_copper(displayInstance);
#endif
}

t_canvas *renderZoom(int frame) {
  t_canvas *c = display_get(displayInstance);
  unsigned char *chunky = c->p.pix8;
  zoomIter *zi;
  unsigned char ***allTxts;
  char logBuffer[255];

  if (frame < (zoomIterations1)) {
    zi = &iterations1[frame];
    allTxts = allTxtPtr1;
  } else if (frame < (zoomIterations1 + zoomIterations2)) {
    zi = &iterations2[frame - zoomIterations1 - BASE_FRAME];
    allTxts = allTxtPtr2;
  } else if (frame < (zoomIterations1 + zoomIterations2 + zoomIterations3 +
                      BASE_FRAME)) {
    zi = &iterations3[frame - zoomIterations1 - zoomIterations2 - BASE_FRAME];
    allTxts = allTxtPtr3;
  } else if (frame < zoomIterations1 + zoomIterations2 + zoomIterations3 +
                         zoomIterations4 + BASE_FRAME) {
    zi = &iterations4[frame - zoomIterations1 - zoomIterations2 -
                      zoomIterations3 - BASE_FRAME];
    allTxts = allTxtPtr4;
  } else {
    return c;
  }

#ifdef __DEBUG_CODE
  prof_reset_chrono();
#endif

#ifdef __AMIGA__
  renderZoom_asm(allTxts, chunky, zi);
#else
  renderZoomC(allTxts, chunky, zi);
#endif

#ifdef __DEBUG_CODE
  float f = prof_get_time("zoom", c->w * c->h);
  if (telemetryEnabled) {
    appendTelemetry(f, zoomTelemetry[0]);
  }
#endif

  return c;
}

#ifdef __DEBUG_CODE
// Zoom editor. Disable for release version.
static float ze_div = 320.0f;
static float alphaStep = 1.0f;
static int changed = 0;
static float cameraDeltaX = 0.0f;
static float cameraDeltaY = 0.0f;

int getImagePtr(unsigned char ***allTxtPtr, unsigned char *chunky,
                zoomIter *iteration) {
  int ix, iy;
  unsigned char **txtPtr;
  unsigned char *source;
  int *iyPtr, *tileyPtr, *tilexPtr;
  unsigned char *ixPtr;

  txtPtr = allTxtPtr[iteration->mipMapIdx];
  iyPtr = iteration->iy;
  tileyPtr = iteration->iTileY;

  iy = (int)*iyPtr++;
  int tileY = *tileyPtr++;
  ixPtr = iteration->ix;
  tilexPtr = iteration->iTileX;

  ix = *ixPtr++;
  int tileX = *tilexPtr++;
  source = txtPtr[ix + iy];

  int s = ptr_bridge(source);

  for (int i = 0; i < 16; i++) {
    if (s == (int)ptr_bridge(canvas1[i])) {
      return i;
    }
  }

  return 999;
}

t_canvas *editZoom(int frame) {
  t_canvas *c = display_get(displayInstance);
  unsigned char *chunky = c->p.pix8;
  zoomIter *zi;
  unsigned char ***allTxts;

  float fx;
  float fy;
  int iy;
  unsigned char ix;

  float tileY;
  float tileX;
  float foo;
  float tX;
  float tY;
  float tileOrigX;
  float tileOrigY;
  float tileSize;

  zi = &iterations3[0];
  allTxts = allTxtPtr1;

  char logBuffer[256];

  int key = getKeyPress();
  switch (key) {
  case KEY_Z:
    ze_div -= alphaStep;
    changed = 1;
    break;
  case KEY_X:
    ze_div += alphaStep;
    changed = 1;
    break;
  case KEY_W:
    cameraDeltaY -= .00001;
    changed = 1;
    break;
  case KEY_S:
    cameraDeltaY += .00001;
    changed = 1;
    break;
  case KEY_A:
    cameraDeltaX -= .00001;
    changed = 1;
    break;
  case KEY_D:
    cameraDeltaX += .00001;
    changed = 1;
    break;
  case KEY_T:
    cameraDeltaY -= .001;
    changed = 1;
    break;
  case KEY_G:
    cameraDeltaY += .001;
    changed = 1;
    break;
  case KEY_F:
    cameraDeltaX -= .001;
    changed = 1;
    break;
  case KEY_H:
    cameraDeltaX += .001;
    changed = 1;
    break;
  case KEY_C:
    alphaStep *= 2.0;
    changed = 1;
    break;
  case KEY_V:
    alphaStep /= 2.0;
    changed = 1;
    break;
  case KEY_K:
    changed = 1;
    break;
  }

  // Calculate a single iteration
  float delta = 256.0f / ze_div;
  float m_delta = delta * 256.0f;
  float xorig = (256.0f - ((delta + cameraDeltaX) * 320.0f)) / 2.0f;
  float yorig = (256.0f - ((delta + cameraDeltaY) * 320.0f)) / 2.0f;
  float pixel_size = ze_div / 256.0f;

  fx = xorig;
  fy = yorig;
  tX = modff(xorig, &foo);
  tY = modff(yorig, &foo);

  // Choose the best mipmap for each tile size.
  if (pixel_size > 128.0) {
    zi->mipMapIdx = 0;
    tileSize = 256.0f;

  } else if (pixel_size > 64.0f) {
    zi->mipMapIdx = 1;
    m_delta /= 2.0f;
    tileSize = 128.0f;
  } else if (pixel_size > 32.0f) {
    zi->mipMapIdx = 2;
    m_delta /= 4.0f;
    tileSize = 64.0f;
  } else if (pixel_size > 16.0f) {
    zi->mipMapIdx = 3;
    m_delta /= 8.0f;
    tileSize = 32.0f;
  } else if (pixel_size > 8.0f) {
    zi->mipMapIdx = 4;
    m_delta /= 16.0f;
    tileSize = 16.0f;
  } else {
    zi->mipMapIdx = 5;
    m_delta /= 32.0f;
    tileSize = 8.0f;
  }

  tileOrigX = tX * tileSize;
  tileOrigY = tY * tileSize;
  tileY = tileOrigY;
  tileX = tileOrigX;

  if (tileY >= tileSize) {
    tileY = 0.0f;
  }

  if (tileX >= tileSize) {
    tileX = 0.0f;
  }

  unsigned char prevx;
  ix = (unsigned char)floorf(fx);
  prevx = ix;
  for (int x = 0; x < 320; x++) {
    ix = (unsigned char)floorf(fx);
    iy = (int)floorf(fy);
    if (prevx != ix) {
      tileX = 0;
      prevx = ix;
    }

    zi->ix[x] = ix;
    zi->iTileX[x] = (int)floorf(tileX);
    fx = fx + delta;
    tileX = tileX + m_delta;
  }

  int prevy;
  iy = (int)floorf(fy);
  prevy = iy;
  for (int y = 0; y < 180; y++) {
    iy = (int)floorf(fy);
    if (prevy != iy) {
      tileY = 0;
      prevy = iy;
    }
    zi->iy[y] = iy * 256;
    int tyTemp = (int)floorf(tileY);
    zi->iTileY[y] = tyTemp * (int)tileSize;
    tileY = tileY + m_delta;
    fy = fy + delta;
  }

  int source = getImagePtr(allTxts, chunky, zi);

  if (changed) {
    snprintf(
        logBuffer, 255,
        "Position: div: %f, deltaX: %f, deltaY: %f, alphaStep: %f, ptr: %i\n",
        ze_div, cameraDeltaX, cameraDeltaY, alphaStep, source);
    serialLog(logBuffer);
    changed = 0;
  }

#ifdef __AMIGA__
  renderZoom_asm(allTxts, chunky, zi);
#else
  renderZoomC(allTxts, chunky, zi);
#endif

  return c;
}
#endif

#ifdef __DEBUG_CODE
#ifndef __AMIGA__
#define TELEMETRY_PATH "/tmp/zoom_frame.csv"
#else
#define TELEMETRY_PATH "T:zoom_frame.csv"
#endif

static const char *teleFooter =
    "set term png size 1280,960\nset datafile separator \",\"\nset xlabel "
    "\"frame\"\nset ylabel \"ms\"\nset output '/tmp/zoom.png'\nplot \"$data\" "
    "using 1 with lines title \"render\", \"$data\" using 2 with lines title "
    "\"c2p\", \"$data\" using ($1+$2) with lines title \"total\"\n";
#endif

void freeZoom(tornadoEffect *effect) {
#ifdef __DEBUG_CODE
  if (telemetryEnabled) {
    saveCombinedTelemetry(TELEMETRY_PATH, zoomTelemetry, 2, teleFooter);
  }
#endif
}

void renderZoomC(unsigned char ***allTxtPtr, unsigned char *chunky,
                 zoomIter *iteration) {

  int ix, iy;
  unsigned char **txtPtr;
  unsigned char *source;
  int *iyPtr, *tileyPtr, *tilexPtr;
  unsigned char *ixPtr;
  int tileY;
  int tileX;

  txtPtr = allTxtPtr[iteration->mipMapIdx];
  iyPtr = iteration->iy;
  tileyPtr = iteration->iTileY;

  for (int y = 0; y < 180; y++) {
    iy = (int)*iyPtr++;
    tileY = *tileyPtr++;
    ixPtr = iteration->ix;
    tilexPtr = iteration->iTileX;

    for (int x = 0; x < 320; x++) {
      ix = *ixPtr++;
      tileX = *tilexPtr++;
      source = txtPtr[ix + iy];
      *chunky++ = source[(tileX + tileY) & 0xffff];
    }
  }
}

void renderClassicC(unsigned char *source, unsigned char *chunky, int frame) {
  float delta = 256.0f / classic_div;
  float xorig = (256.0f - (delta * 320.0f)) / 2.0f;
  float yorig = (256.0f - (delta * 320.0f)) / 2.0f;
  for (int y = 0; y < 180; y++) {
    for (int x = 0; x < 320; x++) {
      unsigned int xpos = (unsigned int)xorig + 0.5;
      unsigned int ypos = (unsigned int)(yorig + 0.5f);
      *chunky++ = source[xpos + (ypos * 256)];
      xorig += delta;
    }
    xorig = (256.0f - (delta * 320.0f)) / 2.0f;
    yorig += delta;
  }
  classic_div += 2.0f;
}

t_canvas *renderZoomRocket(int frame) {
  t_canvas *c = display_get(displayInstance);
  unsigned char *chunky = c->p.pix8;
  zoomIter *zi;
  unsigned char ***allTxts;

  float fx;
  float fy;
  int iy;
  unsigned char ix;

  float tileY;
  float tileX;
  float foo;
  float tX;
  float tY;
  float tileOrigX;
  float tileOrigY;
  float tileSize;

  zi = &iterations1[0];
  allTxts = rocketInstances[current_instance];

  // Calculate a single iteration
  float delta = 256.0f / current_z;
  float m_delta = delta * 256.0f;
  float xorig = (256.0f - ((delta + current_delta_x) * 320.0f)) / 2.0f;
  float yorig = (256.0f - ((delta + current_delta_y) * 320.0f)) / 2.0f;
  float pixel_size = current_z / 256.0f;

  fx = xorig;
  fy = yorig;
  tX = modff(xorig, &foo);
  tY = modff(yorig, &foo);

  // Choose the best mipmap for each tile size.
  if (pixel_size > 128.0) {
    zi->mipMapIdx = 0;
    tileSize = 256.0f;

  } else if (pixel_size > 64.0f) {
    zi->mipMapIdx = 1;
    m_delta /= 2.0f;
    tileSize = 128.0f;
  } else if (pixel_size > 32.0f) {
    zi->mipMapIdx = 2;
    m_delta /= 4.0f;
    tileSize = 64.0f;
  } else if (pixel_size > 16.0f) {
    zi->mipMapIdx = 3;
    m_delta /= 8.0f;
    tileSize = 32.0f;
  } else if (pixel_size > 8.0f) {
    zi->mipMapIdx = 4;
    m_delta /= 16.0f;
    tileSize = 16.0f;
  } else {
    zi->mipMapIdx = 5;
    m_delta /= 32.0f;
    tileSize = 8.0f;
  }

  tileOrigX = tX * tileSize;
  tileOrigY = tY * tileSize;
  tileY = tileOrigY;
  tileX = tileOrigX;

  if (tileY >= tileSize) {
    tileY = 0.0f;
  }

  if (tileX >= tileSize) {
    tileX = 0.0f;
  }

  unsigned char prevx;
  ix = (unsigned char)floorf(fx);
  prevx = ix;
  for (int x = 0; x < 320; x++) {
    ix = (unsigned char)floorf(fx);
    iy = (int)floorf(fy);
    if (prevx != ix) {
      tileX = 0;
      prevx = ix;
    }

    zi->ix[x] = ix;
    zi->iTileX[x] = (int)floorf(tileX);
    fx = fx + delta;
    tileX = tileX + m_delta;
  }

  int prevy;
  iy = (int)floorf(fy);
  prevy = iy;
  for (int y = 0; y < 180; y++) {
    iy = (int)floorf(fy);
    if (prevy != iy) {
      tileY = 0;
      prevy = iy;
    }
    zi->iy[y] = iy * 256;
    int tyTemp = (int)floorf(tileY);
    zi->iTileY[y] = tyTemp * (int)tileSize;
    tileY = tileY + m_delta;
    fy = fy + delta;
  }

#ifdef __AMIGA__
  renderZoom_asm(allTxts, chunky, zi);
#else
  renderZoomC(allTxts, chunky, zi);
#endif

  return c;
}
