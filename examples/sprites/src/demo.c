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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For the timing services
#ifdef __AMIGA__
#include <clib/timer_protos.h>
#endif

// Tornado includes...
#include <aga.h>
#include <assets.h>
#include <audio.h>
#include <canvas.h>
#include <copper.h>
#include <cpu.h>
#include <custom_regs.h>
#include <debug.h>
#include <graphics.h>
#include <hardware_check.h>
#include <imgui_overlay.h>
#include <keyboard.h>
#include <memory.h>
#include <mod_replay.h>
#include <paula_output.h>
#include <prof.h>
#include <splash.h>
#include <system.h>
#include <tndo_time.h>
#include <tornado_settings.h>

// Rocket control...
#include "sync.h"

// Audio and display overlay
#ifndef __AMIGA__
#include "display_sdl.h"
#include "sdl_audio.h"
#endif

// Do NOT touch this include!
#include "demo.h"

// Your demo effects come here...
#include "sprites/sprites.h"

#ifdef __DEBUG_CODE
#warning "Building with debugging and profiling enabled!"
#endif

#define SPRITES_ENABLE (1 << 1)

#define ALL_ENABLE 0xffffffff
#define EFFECT_MASK (ALL_ENABLE)

#define SMPTE(m, s, f) (((60 * 50) * m) + (50 * s) + f)

static tornadoEffect effects[] = {

#if SPRITES_ENABLE & EFFECT_MASK
    {
        .minTime = SMPTE(0, 0, 0),
        .debug_color = 0xff,
        .debug_pos_y = 0,
        .wantTelemetry = 0,
        .init = initSprites,
        .flip = flipSprites,
        .free = freeSprites,
        .vbl = vblSprites,
        .render = renderSprites,
        .rocketRender = renderNull,
        .tracks = tracksNull,
        .trackData = trackDataNull,
    },
#endif
    {
        .minTime = SMPTE(0, 30, 0),
        .init = initNull,
        .flip = 0,
        .free = freeNull,
        .vbl = vblNull,
        .render = renderNull,
        .rocketRender = renderNull,
        .tracks = tracksNull,
        .trackData = trackDataNull,
    },
    {
        .minTime = 50 * 9999,
        .init = initNull,
        .flip = 0,
        .free = freeNull,
        .vbl = vblNull,
        .render = renderNull,
        .rocketRender = renderNull,
        .tracks = tracksNull,
        .trackData = trackDataNull,
    }};

static const int numEffects = (sizeof(effects) / sizeof(tornadoEffect));

// ---------------------------------------------------------------------------
// Demo variables.
// ---------------------------------------------------------------------------
int demoInitDone = 0;
static demoParams *my_dp;
// Start on the first effect by default.
static int currentEffect = 0;
static int epoch = 0;
static int rocketTime = 0;
static int stepMode = 0;
static int musicSecond = 0;
static char *musicBeginL = 0;
static char *musicBeginR = 0;
static int timelerp = 1;
static int lastFrame = 0;

int ambient_benchmark = 0;
int demo_last_key = 0;

// ---------------------------------------------------------------------------
// Timing variables.
// ---------------------------------------------------------------------------
#ifdef __AMIGA__
static struct timeval initBegin;
static struct timeval initEnd;
static struct timeval initEffectBegin;
static struct timeval initEffectEnd;
#endif

// --------------------------------------------------------------------------
// Loader callback. Only available when using VFS container mode.
// --------------------------------------------------------------------------
void loaderCallback(int loaded, int total) {
  printf("Loaded %i out of %i assets.\n", loaded, total);
}

// ---------------------------------------------------------------------------
// Set all your run time options here.
// ---------------------------------------------------------------------------

#ifdef __AMIGA__
#define FAST_MEM_POOL_SIZE 3
#else
#define FAST_MEM_POOL_SIZE 10
#endif

void demoSettings(demoParams *dp) {
  dp->minCPU = MIN_CPU_040;
  dp->tornadoOptions = CLOSE_OS | LOGGING | INSTALL_LEVEL3 | INSTALL_LEVEL2;
#ifdef __DEBUG_CODE
  dp->tornadoOptions |= VERBOSE_DEBUGGING | MEMORY_PROFILING;
#endif
  dp->minFast = 1 * 1024 * 1024;
  dp->minChip = 1000 * 1024;
  dp->fastMemPool = FAST_MEM_POOL_SIZE * 1024 * 1024;
  dp->chipMemPool = 2 * 1024;
  dp->chipScratch = 1024 * 700;
  dp->packedData = 1 * 1024 * 1024; // The size of the biggest packed file.
  // dp->tndoContainerPath = dataPath;
  dp->numDisplays = 1;
  // dp->vfsLoaderCallback = loaderCallback;
  my_dp = dp;
}

// --------------------------------------------------------------------------
// Music initialisation.
// --------------------------------------------------------------------------
void demoAudioInit(unsigned int tornadoOptions) {}

void demoSplash(unsigned int tornadoOptions) {}

// --------------------------------------------------------------------------
// Asset loading and effect initilisation for the entire demo.
// --------------------------------------------------------------------------

static TornadoAsset audioList[] = {
    {.Name = (uint8_t *)"data/p61.announcetro"},
};

static void *chipBuffer;

void demoInit(unsigned int tornadoOptions, int initialEffect) {
  uint32_t before, after, initTime;

  demoInitDone = 0;

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - demoInit()\n");
  }

  if (initialEffect > numEffects) {
    fprintf(stderr, "FATAL - Tried to jump to effect %i but there's only %i.\n",
            initialEffect, numEffects);
    tndo_memory_shutdown(tornadoOptions);
    exit(1);
  }

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Starting demo from effect <%i>\n", initialEffect);
  }

  currentEffect = initialEffect;
  epoch = effects[initialEffect].minTime;

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - demoAudioInit()\n");
  }

  initTime = 0;

#ifdef __AMIGA__
  timeGet(&initBegin);
  timeGet(&initEffectBegin);
#endif

  demoAudioInit(tornadoOptions);

#ifdef __AMIGA__
  timeGet(&initEffectEnd);
  initTime = timeDiffSec(&initEffectBegin, &initEffectEnd);
#endif

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - demoAudioInit() completed in %u seconds\n", initTime);
  }

  for (int i = 0; i < numEffects; i++) {

    if (effects[i].wantTelemetry) {
      effects[i].telemetry =
          allocateTelemetry(effects[i].wantTelemetry,
                            effects[i + 1].minTime - effects[i].minTime);
    }

    if (tornadoOptions & VERBOSE_DEBUGGING) {
      printf("DEBUG - effectInit(%d)\n ", i);
      before = tndo_memory_used();
    }

#ifdef __AMIGA__
    timeGet(&initEffectBegin);
#endif
    effects[i].init(tornadoOptions, &effects[i]);

#ifdef __AMIGA__
    timeGet(&initEffectEnd);
    initTime = timeDiffSec(&initEffectBegin, &initEffectEnd);
#endif

    if (tornadoOptions & VERBOSE_DEBUGGING) {
      after = tndo_memory_used();
      printf("DEBUG - effectInit(%d) allocated %u bytes.\n", i, after - before);
      printf("DEBUG - effectInit(%d) completed in %u seconds.\n ", i, initTime);
    }
  }

  int numAudioAssets = sizeof(audioList) / sizeof(TornadoAsset);
  if (!loadAssets(audioList, numAudioAssets, tornadoOptions, my_dp)) {
    tndo_memory_shutdown(tornadoOptions);
    if (tornadoOptions & VERBOSE_DEBUGGING) {
      printf("failed!\n");
    }
    exit(1);
  }

  // Copy module to chip mem
#ifdef __AMIGA__
  chipBuffer = tndo_malloc(audioList[0].Size, TNDO_ALLOC_CHIP);
  memcpy(chipBuffer, audioList[0].Data, audioList[0].Size);
#endif

#ifdef __AMIGA__
  timeGet(&initEnd);
  initTime = timeDiffSec(&initBegin, &initEnd);
#endif

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - demoInit() completed in %u seconds.\n", initTime);
  }

  demoInitDone = 1;
}

// --------------------------------------------------------------------------
// Master VBL callback
// --------------------------------------------------------------------------
void demoVBL() {
  int oldTime = 0;

  oldTime = epoch;
  oldTime += getMasterTimer();
  tornadoEffect *e = &effects[currentEffect];
  if (e->vbl)
    e->vbl(oldTime - e->minTime);
}

// ---------------------------------------------------------------------------
// Main demo loop begins here...
// ---------------------------------------------------------------------------
void demoMain(unsigned int tornadoOptions, memoryLog *log) {

  int timings = 0;
  prof_enabled(timings);
  int loadEffect = 1;
  int mustExit = 0;
  int oldTime = 0;

  prof_reset();

  int requires_forefront = 0;
  lastFrame = -1;

  // Kick-off music. Note that this only works on Amiga!
#ifdef __AMIGA__
  p61Init(chipBuffer, 0, 0, 0);
#endif

  for (;;) {
    if (loadEffect) {
      loadEffect = 0;
      requires_forefront = 1;
    }
    tornadoEffect *e = &effects[currentEffect];
    oldTime = epoch;
    oldTime += getMasterTimer();
    if (lastFrame == oldTime) {
      continue;
    }

    int effectLocalTime = oldTime - e->minTime;
    t_canvas *c = 0;

#ifndef __AMIGA__

    imguiOverlayData overlayData = {
        oldTime, // SMPTE Time.
        0,       // Number of sliders.
        0,       // Slider array.
    };
    imgui_overlay_set(&overlayData);
    if (e->render)
      c = e->render(effectLocalTime);
#else

    if (e->render) {
      c = e->render(effectLocalTime);
      lastFrame = oldTime;
    }
#endif

    if (e->flip)
      e->flip(requires_forefront);

    requires_forefront = 0;

    if (mousePressL() || (e->render == renderNull)) {
      mustExit = 1;
    } else {
      while (effects[currentEffect + 1].minTime < oldTime) {
        currentEffect++;
        requires_forefront = 1;
        if (currentEffect >= numEffects) {
          mustExit = 1;
          break;
        }
        loadEffect = 1;
      }
    }
    if (mustExit) {
      break;
    }
  }

  // Stop music.
#ifdef __AMIGA__
  p61End();
#endif
}

// ---------------------------------------------------------------------------
// Do NOT forget to free all your memory resources!!!
// ---------------------------------------------------------------------------
void demoFree() {
  for (int i = 0; i < numEffects; i++) {
    effects[i].free(&effects[i]);
  };
}
