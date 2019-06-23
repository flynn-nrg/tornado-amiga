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

#include <stdio.h>
#include <stdlib.h>

#include "aga.h"
#include "cpu.h"
#include "hardware_check.h"
#include "system.h"
#include "tornado_settings.h"

hardware_t *hardware_check(int minCpu, int minChip, int minFast,
                           unsigned int tornadoOptions) {

  hardware_t *hw = calloc(sizeof(hardware_t), 1);

  hw->cpu = getCPU();

  if (hw->cpu == 8) {
    hw->vampire = 1;
  }

  if (tornadoOptions & LOGGING) {
    printf("Tornado Amiga Demo System %s\n", TORNADO_VERSION);
    printf("(C) 2017-2019 Capsule\n\n");
    printf("System check:\n");
    printf("-------------\n");
    if (hw->vampire) {
      printf("Processor: Apollo 68080 [680%i0+ needed] : ", minCpu);
    } else {
      printf("Processor: 680%i0 [680%i0+ needed] : ", hw->cpu, minCpu);
    }
  }

  if (hw->cpu < minCpu) {
    if (tornadoOptions & LOGGING) {
      printf("FAILED.\n");
    }
    free(hw);
    exit(1);
  }

  if (tornadoOptions & LOGGING) {
    printf("OK.\n");
  }

  hw->vbr = getVBR();

  if (tornadoOptions & LOGGING) {
    printf("Checking for AGA: ");
  }

  hw->aga = checkAGA();
  if (!hw->aga) {
    if (tornadoOptions & LOGGING) {
      printf("FAILED.\n");
    }
    free(hw);
    exit(1);
  }

  if (tornadoOptions & LOGGING) {
    printf("OK.\n");
  }

  hw->chip = getMEM(CHIP_MEM);
  if (tornadoOptions & LOGGING) {
    printf("Chip memory: %d [%d needed] : ", hw->chip, minChip);
  }
  if (hw->chip < minChip) {
    if (tornadoOptions & LOGGING) {
      printf("FAILED.\n");
    }
    free(hw);
    exit(1);
  }

  if (tornadoOptions & LOGGING) {
    printf("OK.\n");
  }

  hw->fast = getMEM(FAST_MEM);

  if (tornadoOptions & LOGGING) {
    printf("Fast memory: %d [%d needed] : ", hw->fast, minFast);
  }

  if (hw->fast < minFast) {
    if (tornadoOptions & LOGGING) {
      printf("FAILED.\n");
    }
    free(hw);
    exit(1);
  }

  if (tornadoOptions & LOGGING) {
    printf("OK.\n\n");
  }

  return hw;
}
