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

#include <custom_regs.h>
#include <hardware/custom.h>

#include "chrono.h"
#include "system.h"

static int init = 0;
static const float bus_clock_freq = 0.709379f * 10000000.0f;
static float cpu_clock_freq = 50000000.0f;

extern void freq_estimation(void);

static t_ccap _samples[8];

static void sort_array(t_ccap *array, int len) {
  while (len > 1) {

    int ordered = 1;
    int i;
    for (i = 0; i < (len - 1); i++) {
      t_ccap a = array[i];
      t_ccap b = array[i + 1];
      if (a > b) {
        array[i] = b;
        array[i + 1] = a;
        ordered = 0;
      }
    }
    len--;
    if (ordered)
      break;
  }
}

static void _chrono_init() {
  int i;
  for (i = 0; i < 8; i++) {
    ciab_start();
    freq_estimation();
    _samples[i] = 0xffff - ciab_stop();
  }

  // Median
  sort_array(_samples, 8);
  float ms = chrono_ccap2ms(_samples[4]);
  ms *= 0.95f; // Compensate for cycle losses
  if (ms == 0.0f)
    ms = 1.0f;
  cpu_clock_freq = (0x20000 * 1000) / ms;

  ciab_start();
  init = 1;
}

float chrono_get_cpu_freq() {
  if (!init)
    _chrono_init();

  return cpu_clock_freq;
}

void chrono_reset() {
  if (!init)
    _chrono_init();

  ciab_start();
}

t_ccap chrono_get() {
  if (!init)
    _chrono_init();

  t_ccap delta = 0xffff - ciab_stop();
  return (t_ccap)delta;
}

float chrono_ccap2ms(t_ccap t) {
  float sg = (1.0f / bus_clock_freq) * (float)(t * 10);
  return sg * 1000.0f;
}

unsigned int chrono_ccap2cycles(t_ccap t) {
  float sg = (1.0f / bus_clock_freq) * (float)(t * 10);
  return (unsigned int)(sg * cpu_clock_freq);
}
