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
#include <copper.h>
#include <cpu.h>
#include <custom_regs.h>
#include <debug.h>
#include <graphics.h>
#include <hardware_check.h>
#include <keyboard.h>
#include <memory.h>
#include <mod_replay.h>
#include <paula_output.h>
#include <splash.h>
#include <system.h>
#include <tndo_time.h>
#include <tornado_settings.h>

#include "anim_play.h"
#include "canvas.h"
#include "display_splash.h"

// Do NOT touch this include!
#include "demo.h"

#ifdef __DEBUG_CODE
#warning "Building with debugging and profiling enabled!"
#endif

#define ALL_ENABLE 0xffffffff

#define EFFECT_MASK (ALL_ENABLE)

#define SMPTE(m, s, f) (((60 * 50) * m) + (50 * s) + f)

static char logBuffer[256];

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

// ---------------------------------------------------------------------------
// Timing variables.
// ---------------------------------------------------------------------------
#ifdef __AMIGA__
static struct timeval initBegin;
static struct timeval initEnd;
static struct timeval initEffectBegin;
static struct timeval initEffectEnd;
#endif

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
  dp->tornadoOptions =
      KILL_OS | LOGGING | INSTALL_LEVEL3 | INSTALL_LEVEL2 | SHOW_SPLASH;
#ifdef __DEBUG_CODE
  dp->tornadoOptions |=
      VERBOSE_DEBUGGING | MEMORY_PROFILING | EMIT_CONTAINER_SCRIPT;
#endif
  dp->numDisplays = 1;
  dp->minFast = 2 * 1024 * 1024;
  dp->minChip = 512 * 1024;
  dp->fastMemPool = FAST_MEM_POOL_SIZE * 1024 * 1024;
  dp->chipMemPool = 2 * 1024;
  dp->chipScratch = 1024 * 128;
  dp->packedData =
      2 * 1024 *
      1024; // The size of the biggest file's dictionary + streaming buffers.
  my_dp = dp;
}

// --------------------------------------------------------------------------
// Music initialisation.
// --------------------------------------------------------------------------
void demoAudioInit(unsigned int tornadoOptions) {}

// ---------------------------------------------------------------------------
// Splash screen/animation before init/load
// ---------------------------------------------------------------------------
static int *animSizes;
static int numAnimAssets;
static void **animAssets;

const char *splashList[] = {"data/loading_640_360.tndo",
                            "data/loading_music.tndo"};
static TornadoAsset animList[] = {
    {
        .Name = (uint8_t *)"data/capsulflix.tndo",
        .Flags = ASSETS_IN_REUSABLE_MEM,
    },
};
const char *splashMod = "data/capsulflix.p61";

#define SPLASH_X 640
#define SPLASH_ANIM_X 320
#ifdef __AMIGA__
#define SPLASH_Y 360
#define SPLASH_ANIM_Y 180
#define SPLASH_Y_OFFSET (512 - 360) / 2
#define SPLASH_ANIM_Y_OFFSET (256 - 180) / 2
#else
#define SPLASH_Y 360
#define SPLASH_ANIM_Y 180
#define SPLASH_Y_OFFSET 0
#define SPLASH_ANIM_Y_OFFSET 0
#endif

void demoSplash(unsigned int tornadoOptions) {
  display_init_splash(0, 0, SPLASH_ANIM_X, SPLASH_ANIM_Y, SPLASH_ANIM_Y_OFFSET,
                      8, tornadoOptions | DOUBLE_BUFFERED_SPLASH);

  numAnimAssets = sizeof(animList) / sizeof(TornadoAsset);
  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Loading anim data...\n");
  }

  if (!loadAssets(animList, numAnimAssets, tornadoOptions, my_dp)) {
    tndo_memory_shutdown(tornadoOptions);
    if (tornadoOptions & VERBOSE_DEBUGGING) {
      printf("FATAL - Anim data loading failed!\n");
    }
    exit(1);
  }

  display_set_palette_splash((uint32_t *)animList[0].Data);
  uint32_t skip = display_get_palette_skip_splash(animList[0].Data);

  unsigned char *data = (unsigned char *)animList[0].Data;
  int num_frames = decoder_init(data + skip);

  t_canvas *c = display_get_splash();
  unsigned char *chunky_buffer = c->p.pix8;

  display_forefront_splash();

#ifdef __AMIGA__
  splash_load_mod(splashMod, tornadoOptions);
  splash_play_mod();
#endif
  for (int i = 0; i < num_frames; ++i) {
    decode_frame(chunky_buffer);
    display_flip_splash();
  }

  // Wait 1.5 seconds.
  display_waitvbl_splash(70);

#ifdef __AMIGA__
  splash_end_mod();
#endif

  display_end_splash((uint32_t *)animList[0].Data);

  // Free anim data.
  tndo_free();

  display_init_splash(splashList, sizeof(splashList) / sizeof(char *), SPLASH_X,
                      SPLASH_Y, SPLASH_Y_OFFSET, 8, tornadoOptions);

  // Show loading screen and kick-off asset loading.
  display_show_splash();
}

// --------------------------------------------------------------------------
// Asset loading and effect initilisation for the entire demo.
// --------------------------------------------------------------------------

void demoInit(unsigned int tornadoOptions, int initialEffect) {
  uint32_t before, after, initTime;

  demoInitDone = 0;

  // Wait 10 seconds so the loading music can start playing.
  display_waitvbl_splash(10 * 50);

  if (tornadoOptions & SHOW_SPLASH) {
    display_end_splash(0);
  }

  demoInitDone = 1;
}

// --------------------------------------------------------------------------
// Master VBL callback
// --------------------------------------------------------------------------
void demoVBL() {}

// ---------------------------------------------------------------------------
// Main demo loop begins here...
// ---------------------------------------------------------------------------
void demoMain(unsigned int tornadoOptions, memoryLog *log) {}

// ---------------------------------------------------------------------------
// Do NOT forget to free all your memory resources!!!
// ---------------------------------------------------------------------------
void demoFree() {}
