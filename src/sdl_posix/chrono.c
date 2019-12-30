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

#ifdef _WIN32

#include "chrono.h"

static inline unsigned long long GetHighPrecisionTime() { return 0; }

void chrono_reset() {}

t_ccap chrono_get() { return 0; }

float chrono_ccap2ms(t_ccap t) { return 0.0f; }

t_ccap chrono_ccap2cycles(t_ccap t) { return 0; }

#else

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "chrono.h"

// Timer de alta precision, standar POSIX. Devuelve microsegundos en
// formato "long long" (entero 64 bits). ATENCION: Es un tipo de GCC, no es del
// standard C

static unsigned long long g_last_time;

static inline unsigned long long GetHighPrecisionTime() {
  struct timeval t;
  gettimeofday(&t, NULL);
  // El sufijo LL indica que es un numero "long long"
  return 1000000LL * t.tv_sec + t.tv_usec;
}

static const float cpu_freq = 2900000000.0f;

float chrono_get_cpu_freq() { return cpu_freq; }

void chrono_reset() {
  unsigned long long now = GetHighPrecisionTime();
  g_last_time = now;
}

t_ccap chrono_get() {
  unsigned long long now = GetHighPrecisionTime();
  double elapsed = (double)(now - g_last_time);
  g_last_time = now;

  float cycles = elapsed * (cpu_freq / 1000000.0f);
  return (t_ccap)cycles;
}

float chrono_ccap2ms(t_ccap t) {
  float sg = (1.0f / cpu_freq) * (float)t;
  return sg * 1000.0f;
}

t_ccap chrono_ccap2cycles(t_ccap t) { return t; }

#endif
