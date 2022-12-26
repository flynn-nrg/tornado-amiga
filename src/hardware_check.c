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
    printf("(C) 2017-2021 Capsule\n\n");
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
