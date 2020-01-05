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

#include <fenv.h>
#include <getopt.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Rocket includes
#include "sync.h"

// Tornado includes...
#include "assets.h"
#include "audio.h"
#include "audio_lowlevel.h"
#include "cpu.h"
#include "debug.h"
#include "display.h"
#include "graphics.h"
#include "hardware_check.h"
#include "keyboard.h"
#include "memory.h"
#include "mod_replay.h"
#include "paula_output.h"
#include "screen_dump.h"
#include "system.h"
#include "tndo_assert.h"
#include "tndo_file.h"
#include "tornado_settings.h"

#include "demo.h"

static int fenv;

static void usage() {
  printf("Usage: mydemo -h -d -i <effectNumer>\n");
  printf("-d : Dump the framebuffer to a bmp file in /tmp after rendering each "
         "frame.\n");
  printf("-i <effectNumer> : Start from the given effect.\n");
  printf("-h : Enable hot swappable assets.\n");
  printf("-r : Enable rocket.\n");
  printf("-s : Step mode. Render every frame and then increase the time.\n");
  exit(0);
}

int main(int argc, char **argv) {

  demoParams *dp = (demoParams *)calloc(1, sizeof(demoParams));

  // Demo settings
  demoSettings(dp);

  // Perform hardware requirement check.
  hardware_t *hw =
      hardware_check(dp->minCPU, dp->minChip, dp->minFast, dp->tornadoOptions);

  // ---------------------------------------------------------------------------
  // Memory logging
  // ---------------------------------------------------------------------------
  memoryLog *log = 0;
  if (dp->tornadoOptions & MEMORY_LOGGING) {
    log = memLogInit(DEFAULT_MEM_LOG_SIZE);
  }

  // ---------------------------------------------------------------------------
  // Memory manager initialisation.
  // ---------------------------------------------------------------------------
  if (dp->tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Memory manager init...");
  }

  // Reusable memory pool size.
  int m_res = tndo_memory_init(dp->fastMemPool, dp->chipMemPool,
                               dp->chipScratch, dp->packedData);

  if (dp->tornadoOptions & VERBOSE_DEBUGGING) {
    printf("done\n");
  }
  switch (m_res) {
  case TNDO_ENOMEM:
    printf("FATAL - Could not initialise memory manager. Aborting.\n");
    exit(1);
  case TNDO_ENOMEM_FAST:
    printf("FATAL - Could not allocate fast memory pool. Aborting.\n");
    exit(1);
  case TNDO_ENOMEM_CHIP:
    printf("FATAL - Could not allocate chip memory pool. Aborting.\n");
    exit(1);
  }

  // ---------------------------------------------------------------------------
  // Process command line options.
  // ---------------------------------------------------------------------------
  int ch;
  int initialEffect = 0;
  while ((ch = getopt(argc, argv, "i:dhrs")) != -1) {
    switch (ch) {
    case 'd':
      dp->tornadoOptions |= ENABLE_SCREEN_DUMP;
      break;
    case 'h':
      dp->tornadoOptions |= HOTSWAPPABLE_ASSETS;
      break;
    case 'i':
      initialEffect = atoi(optarg);
      break;
    case 'r':
      dp->tornadoOptions |= ENABLE_ROCKET;
      break;
    case 's':
      dp->tornadoOptions |= STEP_MODE;
      break;
    case '?':
    default:
      usage();
    }
  }
  argc -= optind;
  argv += optind;

  // ---------------------------------------------------------------------------
  // Toggle screen dump switch.
  // ---------------------------------------------------------------------------
  if (dp->tornadoOptions & ENABLE_SCREEN_DUMP) {
    screen_dump_enable();
  }

  // ---------------------------
  // Tornado VFS init if needed.
  // ---------------------------
  if (dp->tornadoOptions & ENABLE_TNDO_VFS) {
    tndo_assert(dp->tndoContainerPath);
    int res = tndo_vfs_init(dp->tndoContainerPath, dp->tornadoOptions,
                            dp->vfsLoaderCallback);
    if (res != TNDO_FILE_OK) {
      fprintf(stderr, "FATAL - tndo_vfs_init() failed. Aborting.\n");
      exit(EXIT_FAILURE);
    }
  }

  // --------------------------------
  // Setup floating point environment
  // --------------------------------
  fenv = fegetround();
  fesetround(FE_TOWARDZERO);

  // --------------------------------
  // Display subsystem
  // --------------------------------
  tndo_assert(dp->numDisplays > 0);
  display_subsystem_init(dp->numDisplays);

  // ---------------------------------------------------------------------------
  // Splash screen/animation before init/load
  // ---------------------------------------------------------------------------
  if (dp->tornadoOptions & SHOW_SPLASH) {
    demoSplash(dp->tornadoOptions);
  }
  // ---------------------------------------------------------------------------
  // Demo initialisation.
  // ---------------------------------------------------------------------------
  demoInit(dp->tornadoOptions, initialEffect);

  // Signal the memory manager that we are done with the init stage.
  // This will also free the packed data buffer.
  tndo_memory_init_done();

  if (dp->tornadoOptions & USE_AUDIO) {
    // void *mixRoutine = getMixRoutine();
    // PaulaOutput_Init(mixRoutine, dp->mixState, REPLAY_PERIOD_22050,
    // OUTPUT_14_BIT_STEREO); PaulaOutput_Start();
  }

  // We are loading files. Shut down the VFS subsystem.
  if (dp->tornadoOptions & ENABLE_TNDO_VFS) {
    tndo_vfs_end();
  }

  // ---------------------------------------------------------------------------
  // Main demo loop begins here...
  // ---------------------------------------------------------------------------

  demoMain(dp->tornadoOptions, log);

  // ---------------------------------------------------------------------------
  // Main demo loop ends...
  // ---------------------------------------------------------------------------

  // Memory logging flush...
  if (dp->tornadoOptions & MEMORY_LOGGING) {
    if (!memLogSave(DEFAULT_LOG_FILE, log))
      printf("ERROR - Failed to save log.\n");
    memLogFree(log);
  }

  // -----------------------------------
  // Restore floating point environment.
  // -----------------------------------
  fesetround(fenv);

  // ---------------------------
  // Shutdown display subsystem.
  // ---------------------------
  display_subsystem_end();

  // ---------------------------------------------------------------------------
  // Do NOT forget to free all your memory resources!!!
  // ---------------------------------------------------------------------------

  free(hw);
  demoFree();

  if (dp->tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Memory manager shutdown.\n");
  }

  tndo_memory_shutdown(dp->tornadoOptions);

  free(dp);

  exit(0);
}
