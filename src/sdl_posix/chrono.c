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
