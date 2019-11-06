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

#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <exec/interrupts.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

// Tornado includes...
#include "aga.h"
#include "assets.h"
#include "audio.h"
#include "audio_lowlevel.h"
#include "c2p.h"
#include "copper.h"
#include "cpu.h"
#include "custom_regs.h"
#include "ddpcm_lowlevel.h"
#include "debug.h"
#include "display.h"
#include "graphics.h"
#include "hardware_check.h"
#include "keyboard.h"
#include "memory.h"
#include "mod_replay.h"
#include "paula_output.h"
#include "splash.h"
#include "system.h"
#include "tndo_assert.h"
#include "tndo_file.h"
#include "tndo_time.h"
#include "tornado_settings.h"

#include "demo.h"

size_t __stack = 65536; /* 64KB stack-size */
static int fenv;

static unsigned int scratch_pal[256];

// OS friendly VBL interrupt.
static struct Interrupt tndoVBL;

int main(int argc, char **argv) {

  demoParams *dp = calloc(1, sizeof(demoParams));

  // Demo settings
  demoSettings(dp);

  // Perform hardware requirement check.
  hardware_t *hw =
      hardware_check(dp->minCPU, dp->minChip, dp->minFast, dp->tornadoOptions);

  // Set high priority.
  SetTaskPri(FindTask(0), 119);

  // ---------------------------------------------------------------------------
  // Memory logging
  // ---------------------------------------------------------------------------
  memoryLog *log;
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
  // Command line processing.
  // ---------------------------------------------------------------------------
  int initialEffect = 0;
  if (argc > 1) {
    initialEffect = atoi(argv[1]);
  }

  // ---------------------------------------------------------------------------
  // Timing service.
  // ---------------------------------------------------------------------------
  timeInit();

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
    splash_bg_init();
    demoSplash(dp->tornadoOptions);
  }
  // ---------------------------------------------------------------------------
  // Demo initialisation.
  // ---------------------------------------------------------------------------
  demoInit(dp->tornadoOptions, initialEffect);

  // We are loading files. Shut down the VFS subsystem.
  if (dp->tornadoOptions & ENABLE_TNDO_VFS) {
    tndo_vfs_end();
  }

  // Signal the memory manager that we are done with the init stage.
  // This will also free the packed data buffer.
  tndo_memory_init_done();

  // Release timer.device.
  timeEnd();

  // ---------------------------------------------------------------------------
  // WARNING: No OS calls beyond this point!!!
  // ---------------------------------------------------------------------------

  if (dp->tornadoOptions & KILL_OS) {
    closeOS(hw->vbr);

    if (dp->tornadoOptions & INSTALL_LEVEL2) {
      installLevel2(hw->vbr);
    }

    if (dp->tornadoOptions & INSTALL_LEVEL3) {
      int *paulaOutputVBLCallback = Get_PaulaOutput_VertBCallback();
      installLevel3(hw->vbr, paulaOutputVBLCallback, (int *)demoVBL);
    }

    if (dp->tornadoOptions & USE_AUDIO) {
      void *mixRoutine;
      switch (dp->audioMode) {
      case OUTPUT_14_BIT_STEREO:
        if (dp->tornadoOptions & DDPCM_STREAMING) {
          mixRoutine = getDDPCMMixRoutine16();
        } else {
          mixRoutine = getMixRoutine16();
        }
        break;
      case OUTPUT_8_BIT_STEREO:
        if (dp->tornadoOptions & INTERLEAVED_AUDIO) {
          mixRoutine = getMixRoutine8i();
        } else {
          mixRoutine = getMixRoutine8();
        }
        break;
      default:
        printf("FATAL - Unsupported audio mode. Aborting.\n");
        exit(1);
      }
      setAudioMode(PCM_REPLAY_MODE);
      PaulaOutput_Init(mixRoutine, dp->mixState, dp->mixState2, dp->audioPeriod,
                       dp->audioMode);
      PaulaOutput_Start();
    }
  } else {
    if (dp->tornadoOptions & VERBOSE_DEBUGGING) {
      printf("DEBUG - Tornado is running in system friendly mode. No direct "
             "hardware access!\n");
    }

    if (dp->tornadoOptions & INSTALL_LEVEL3) {
      setupVBLChain(0, (int *)demoVBL);
      tndoVBL.is_Node.ln_Type = NT_INTERRUPT;
      tndoVBL.is_Node.ln_Pri = -60;
      tndoVBL.is_Node.ln_Name = "Tornado VBL";
      tndoVBL.is_Data = 0;
      tndoVBL.is_Code = VBLChain;
      AddIntServer(INTB_VERTB, &tndoVBL);
    }
  }

  // ---------------------------------------------------------------------------
  // Main demo loop begins here...
  // ---------------------------------------------------------------------------

  demoMain(dp->tornadoOptions, log);

  // ---------------------------------------------------------------------------
  // Main demo loop ends...
  // ---------------------------------------------------------------------------

  if (dp->tornadoOptions & KILL_OS) {
    if (dp->tornadoOptions & USE_AUDIO) {
      int am = getAudioMode();
      if (am == P61_REPLAY_MODE) {
        p61End();
      } else {
        PaulaOutput_ShutDown();
      }
    }
    restoreOS(hw->vbr);
  } else {
    // Remove interrupt VBL server
    if (dp->tornadoOptions & INSTALL_LEVEL3) {
      RemIntServer(INTB_VERTB, &tndoVBL);
    }
  }

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

  if (dp->tornadoOptions & SHOW_SPLASH) {
    splash_bg_end();
  }

  if (dp->tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Memory manager shutdown.\n");
  }

  tndo_memory_shutdown(dp->tornadoOptions);

  free(dp);

  exit(0);
}
