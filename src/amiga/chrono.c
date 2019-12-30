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
