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
#include "display_null.h"
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

// BASS
#ifndef __AMIGA__
#include "bass.h"
#include "display_sdl.h"
#endif

// Do NOT touch this include!
#include "demo.h"

// Your demo effects come here...
#include "blur/blur.h"

#ifdef __DEBUG_CODE
#warning "Building with debugging and profiling enabled!"
#endif

#define BLUR_ENABLE (1 << 1)

#define ALL_ENABLE 0xffffffff

#define EFFECT_MASK (ALL_ENABLE)

#define SMPTE(m, s, f) (((60 * 50) * m) + (50 * s) + f)

static tornadoEffect effects[] = {

#if BLUR_ENABLE & EFFECT_MASK
    {
        .minTime = SMPTE(0, 0, 0),
        .debug_color = 0xff,
        .debug_pos_y = 0,
        .wantTelemetry = 8,
        .init = initBlur,
        .flip = flipBlur,
        .free = freeBlur,
        .vbl = vblBlur,
        .render = renderBlurCode,
        .rocketRender = renderBlurCodeRocket,
        .tracks = tracksBlur,
        .trackData = trackDataBlur,
    },
#endif
    {
        .minTime = SMPTE(0, 24, 0),
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

// ---------------------------------------------------------------------------
// Rocket control...
// ---------------------------------------------------------------------------
#ifndef __AMIGA__
struct sync_device *rocket;
static int rocket_control = 0;
static int running = 0;
static int is_playing = 0;

static const float bpm = 120.0f; /* beats per minute */
static const int rpb = 8;        /* rows per beat */
// LLVM-3.5 doesn't accept this as constexpr, need to compute it later.
// static const double row_rate = ((double) bpm / 50) * rpb;
HSTREAM stream;

static void demo_pause(void *d, int flag) {
  HSTREAM h = *((HSTREAM *)d);
  if (flag) {
    BASS_ChannelPause(h);
    running = 0;
    is_playing = 0;
  } else {
    BASS_ChannelPlay(h, 0);
    running = 1;
    is_playing = 1;
  }
}

static int music_is_playing(void *d) {
  HSTREAM h = *((HSTREAM *)d);
  return BASS_ChannelIsActive(h) == BASS_ACTIVE_PLAYING;
}

static void music_set_row(void *d, int row) {
  HSTREAM h = *((HSTREAM *)d);
  QWORD pos = BASS_ChannelSeconds2Bytes(h, row / 50);
  BASS_ChannelSetPosition(h, pos, BASS_POS_BYTE);
  epoch = row;
  rocketTime = 0;
}

static struct sync_cb demo_cb = {demo_pause, music_set_row, music_is_playing};
#endif

static const char *dataPath = "blur_data.tndo";

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
  dp->tornadoOptions =
      KILL_OS | LOGGING | INSTALL_LEVEL3 | INSTALL_LEVEL2 | USE_AUDIO;
#ifdef __DEBUG_CODE
  dp->tornadoOptions |=
      VERBOSE_DEBUGGING | MEMORY_PROFILING | EMIT_CONTAINER_SCRIPT;
#endif
  dp->minFast = 32 * 1024 * 1024;
  dp->minChip = 1000 * 1024;
  dp->fastMemPool = FAST_MEM_POOL_SIZE * 1024 * 1024;
  dp->chipMemPool = 2 * 1024;
  dp->chipScratch = 1024 * 700;
  dp->packedData =
      128 * 1024; // Enough space for the LZW dictionary + stream buffers.
  dp->tndoContainerPath = dataPath;
  dp->numDisplays = 1;
  // dp->vfsLoaderCallback = loaderCallback;
  my_dp = dp;
}

// --------------------------------------------------------------------------
// Music initialisation.
// --------------------------------------------------------------------------
static int *audioSizes;
static int numAudioAssets;
static void **audioAssets;

// SDL/Posix soundtrack
#define bassTrack "data/brut_blur.mp3"

const char *audioList[] = {"data/brut_ddpcm.tndo"};

void demoAudioInit(unsigned int tornadoOptions) {
  if (tornadoOptions & USE_AUDIO) {
    numAudioAssets = sizeof(audioList) / sizeof(char *);
    audioSizes = (int *)tndo_malloc(sizeof(int) * numAudioAssets, 0);
    audioAssets = (void **)tndo_malloc(sizeof(void *) * numAudioAssets, 0);
    if (tornadoOptions & VERBOSE_DEBUGGING) {
      printf("DEBUG - Loading audio data...");
      fflush(stdout);
    }
    if (!loadAssets(&audioAssets[0], &audioList[0], &audioSizes[0],
                    numAudioAssets, tornadoOptions, my_dp)) {
      tndo_memory_shutdown(tornadoOptions);
      if (tornadoOptions & VERBOSE_DEBUGGING) {
        printf("failed!\n");
      }
      exit(1);
    }

    musicSecond = my_dp->sampleRate * (my_dp->bitsPerSample / 8);
    int offset = (effects[currentEffect].minTime / 50) * musicSecond;
    musicBeginL = *my_dp->mixState;
    musicBeginR = *my_dp->mixState2;
    *my_dp->mixState = musicBeginL + offset;
    *my_dp->mixState2 = musicBeginR + offset;

    if (tornadoOptions & VERBOSE_DEBUGGING) {
      printf("done.\n");
    }
  }

#ifndef __AMIGA__
  // Bass initialization.
  if (tornadoOptions & USE_AUDIO) {
    if (!BASS_Init(-1, 44100, 0, 0, 0)) {
      fprintf(stderr, "FATAL - Failed to init bass. Aborting.\n");
      exit(1);
    }

    stream = BASS_StreamCreateFile(0, bassTrack, 0, 0, BASS_STREAM_PRESCAN);
    if (!stream) {
      fprintf(stderr, "FATAL - Failed to open music file <%s>\n", bassTrack);
      exit(1);
    }
  }
#endif
}

void demoSplash(unsigned int tornadoOptions) {}

// --------------------------------------------------------------------------
// Asset loading and effect initilisation for the entire demo.
// --------------------------------------------------------------------------

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

#ifndef __AMIGA__
  if (tornadoOptions & ENABLE_ROCKET) {
    sdl_display_set_delay(18);
    rocket_control = 1;
    stepMode = 1;
    rocket = sync_create_device("sync");
    if (!rocket) {
      fprintf(stderr, "FATAL - Could not create rocket device. Aborting.\n");
      exit(1);
    }

    if (sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT)) {
      fprintf(stderr,
              "FATAL - Could not connect to rocket editor. Aborting.\n");
      exit(1);
    }

    if (tornadoOptions & STEP_MODE) {
      stepMode = 1;
    }
  }
#endif

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

#ifndef __AMIGA__
    if (tornadoOptions & ENABLE_ROCKET) {
      effects[i].tracks(rocket);
    }
#endif
  }

#ifdef __AMIGA__
  timeGet(&initEnd);
  initTime = timeDiffSec(&initBegin, &initEnd);
#endif

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - demoInit() completed in %u seconds.\n", initTime);
  }

#ifdef __DEBUG_CODE
  emit_container_script("build_container.sh", "blur_data.tndo");
#endif

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

#ifndef __AMIGA__
  if (tornadoOptions & USE_AUDIO) {
    BASS_Start();
    QWORD pos = BASS_ChannelSeconds2Bytes(stream, epoch / 50);
    BASS_ChannelSetPosition(stream, pos, BASS_POS_BYTE);
    BASS_ChannelPlay(stream, 0);
    if (rocket_control) {
      BASS_ChannelPause(stream);
    }
  }
#endif

  int requires_forefront = 0;
  lastFrame = -1;

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

    if (rocket_control) {
      if (stepMode) {
        oldTime = epoch + rocketTime;
      }
    }

    imguiOverlayData overlayData = {
        oldTime, // SMPTE Time.
        0,       // Number of sliders.
        0,       // Slider array.
    };

    // Rocket control...
    if (rocket_control) {
      if (sync_update(rocket, oldTime, &demo_cb, (void *)&stream)) {
        sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
      }
      if (running) {
        rocketTime++;
      }
      // Row data fetching is done in absolute time.
      if (e->trackData)
        e->trackData(oldTime, &overlayData);
      // Update UI.
      imgui_overlay_set(&overlayData);
      // Render Rocket frame
      if (e->rocketRender)
        c = e->rocketRender(effectLocalTime);

    } else {
      imgui_overlay_set(&overlayData);
      if (e->render)
        c = e->render(effectLocalTime);
    }
#else

    if (e->render) {
      c = e->render(effectLocalTime);
      lastFrame = oldTime;
    }
#endif

    // Can't have profiling and mod replay at the same time.
    //#define MOD_REPLAY 1

#ifndef MOD_REPLAY
#warning "Profiling enabled: Mod replay during demo runtime will not work!!!"

    // Prof must reset before flip
    if (c)
      prof_show_times(c, e->debug_color, e->debug_pos_y);

    prof_reset();
    prof_reset_chrono();
#endif // MOD_REPLAY

    if (e->flip)
      e->flip(requires_forefront);

#ifndef MOD_REPLAY
#ifdef __DEBUG_CODE
    int pixels = c ? c->w * c->h : 0;
    prof_get_time("screen dump", pixels);
#endif

#endif // MOD_REPLAY

    requires_forefront = 0;

#ifdef __DEBUG_CODE
    int k = getKeyPress();
    demo_last_key = k;
    if (k == KEY_F1) {
      prof_enabled(timings);
      timings ^= 1;
    }

    // Fast forward 1 second inside of an effect.
    if (k == KEY_M) {
      epoch = getMasterTimer();
      resetMasterTimer();
      epoch += 50;
      *my_dp->mixState += musicSecond;
      *my_dp->mixState2 += musicSecond;
    }

    // Rewind 1 second behind inside of an effect.
    if (k == KEY_N) {
      int t = getMasterTimer();
      if (t >= 50) {
        epoch = t - 50;
        resetMasterTimer();
        *my_dp->mixState -= musicSecond;
        *my_dp->mixState2 -= musicSecond;
      }
    }

    // Jump to the next effect.
    if (k == KEY_P) {
      int e = currentEffect;
      e++;
      if (e < numEffects) {
        resetMasterTimer();
        epoch = effects[e].minTime;
        *my_dp->mixState =
            musicBeginL + (effects[e].minTime / 50) * musicSecond;
        *my_dp->mixState2 =
            musicBeginR + (effects[e].minTime / 50) * musicSecond;
#ifndef __AMIGA__
        QWORD pos = BASS_ChannelSeconds2Bytes(stream, effects[e].minTime / 50);
        BASS_ChannelSetPosition(stream, pos, BASS_POS_BYTE);
        BASS_ChannelPlay(stream, 0);
#endif
      }
    }

    // Jump to the previous effect.
    if (k == KEY_O) {
      int e = currentEffect;
      e--;
      if (e >= 0) {
        currentEffect--;
        resetMasterTimer();
        epoch = effects[currentEffect].minTime;
        *my_dp->mixState =
            musicBeginL + (effects[currentEffect].minTime / 50) * musicSecond;
        *my_dp->mixState2 =
            musicBeginR + (effects[currentEffect].minTime / 50) * musicSecond;
#ifndef __AMIGA__
        QWORD pos = BASS_ChannelSeconds2Bytes(stream, effects[e].minTime / 50);
        BASS_ChannelSetPosition(stream, pos, BASS_POS_BYTE);
        BASS_ChannelPlay(stream, 0);
#endif
      }
    }

#endif

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
}

// ---------------------------------------------------------------------------
// Do NOT forget to free all your memory resources!!!
// ---------------------------------------------------------------------------
void demoFree() {
  for (int i = 0; i < numEffects; i++) {
    effects[i].free(&effects[i]);
  };

    // Shut down rocket if necessary.
#ifndef __AMIGA__
  if (rocket_control) {
    sync_save_tracks(rocket);
    sync_destroy_device(rocket);
  }
#endif
}
