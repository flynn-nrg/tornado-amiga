/*
Copyright (c) 2019, Luis Pons. All rights reserved.

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

#include <SDL.h>

#include "hardware_check.h"

static int init = 0;
static unsigned int base_ms = 0;

int installLevel3(int *vectorBase, int *paulaOutputVBLCallback) { return 1; }
int installLevel2(int *vectorBase) { return 1; }
int closeOS(int *vectorBase) { return 1; }
int restoreOS(int *vectorBase) { return 1; }

void serialPutc(char c) { fputc(c, stderr); }

void ciab_start(void) {}
int ciab_stop(void) { return 1; }

int checkAGA() { return 1; }

// unused
unsigned int master_timer = 0;
unsigned int mouse_left = 0;
unsigned int mouse_right = 0;

static hardware_t *hw_dummy = 0;

hardware_t *hardware_check(int a, int b, int c, unsigned int d) {
  hw_dummy = (hardware_t *)malloc(sizeof(hardware_t));
  hw_dummy->vbr = 0;
  hw_dummy->chip = 1024 * 1024 * 2;
  hw_dummy->fast = 1024 * 1024 * 32;
  hw_dummy->cpu = 4;
  hw_dummy->aga = 1;
  return hw_dummy;
}

void resetMasterTimer() { init = 0; }

unsigned int getMasterTimer() {
  if (!init) {
    base_ms = SDL_GetTicks();
    init = 1;
  }
  unsigned int now = SDL_GetTicks();

  return (now - base_ms) / 20; // PAL
}

void waitVBL() {
  unsigned int now = SDL_GetTicks();
  unsigned int rem = now % 20;
  SDL_Delay(20 - rem);
}
