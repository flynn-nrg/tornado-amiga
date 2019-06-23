/*
Copyright (c) 2019 Luis Pons

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

#include <stdint.h>
#include <string.h>

#include "canvas.h"
#include "chrono.h"
#include "dprint.h"
#include "memory.h"
#include "prof.h"

#ifdef __DEBUG_CODE

#define MAX_TIMES (64)

typedef struct {
  const char *aTitle;
  int iterations;
  t_ccap time;
} t_tsmp;

static t_tsmp aTimes[MAX_TIMES];
static int g_iTimerNo = 0;

void prof_reset() {
  prof_reset_chrono();
  g_iTimerNo = 0;
}

static int _enabled = 0;

void prof_enabled(int enabled) { _enabled = enabled; }

void prof_reset_chrono() { chrono_reset(); }

float prof_get_time(const char *pRutina, int iterations) {
  float ms = 0.0;
  if (!_enabled) {
    aTimes[g_iTimerNo].time = chrono_get();
    ms = chrono_ccap2ms(aTimes[g_iTimerNo].time);
    return ms;
  }

  if (g_iTimerNo < MAX_TIMES) {
    aTimes[g_iTimerNo].time = chrono_get();
    aTimes[g_iTimerNo].iterations = iterations;
    aTimes[g_iTimerNo].aTitle = pRutina;
    ms = chrono_ccap2ms(aTimes[g_iTimerNo].time);
    prof_reset_chrono();
    g_iTimerNo++;
  }

  prof_reset_chrono();
  return ms;
}

void prof_show_times(t_canvas *s, unsigned int color, int y) {
  if (!_enabled)
    return;

  int t;
  float total = 0;
  float cpu = chrono_get_cpu_freq();

  dprint_locate(0, y);
  dprint_color(color);
  dprint(s, "CPU = %d Mhz\n", (int)(cpu * (1.0f / 1000000.0f)));

  for (t = 0; t < g_iTimerNo; t++) {
    float ms = chrono_ccap2ms(aTimes[t].time);
    float cyc = 0.0f;
    if (aTimes[t].iterations > 0)
      cyc = ((float)chrono_ccap2cycles(aTimes[t].time)) /
            (float)aTimes[t].iterations;
    float perc = 100.0f * (ms / 50.0f);
    total += ms;
    dprint_locate(0, y + t + 1);
    dprint(s, "%s", aTimes[t].aTitle);
    dprint_locate(14, y + t + 1);
    dprint(s, "= %.1f ms  ", ms);
    dprint_locate(27, y + t + 1);
    dprint(s, "%.2f c/i  ", cyc);
  }

  dprint(s, "\n");
  dprint(s, "Total = %d%% of frame time, %.1f fps\n",
         (int)(100.0f * (total / (1000.0f / 50.0f))), 1000.0f / total);

  g_iTimerNo = 0;
}

#else

void prof_enabled(int) {}
void prof_reset() {}
void prof_reset_chrono() {}
float prof_get_time(const char *, int) { return 0.0f; }
void prof_show_times(t_canvas *, unsigned int debug_color, int y) {}

#endif
