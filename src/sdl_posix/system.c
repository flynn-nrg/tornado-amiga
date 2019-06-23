/*
Copyright (c) 2019 Luis Pons.
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
