/*
Copyright (c) 2026, Miguel Mendez. All rights reserved.
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

// A plasma with its inner loop written twice: in C (renderPlasmaC) and in 68k
// assembler (amiga/plasma_inner.s). On the Amiga the asm is called directly.
// On POSIX with TORNADO_68K_EMU it runs in the emulated 68k, so the asm can be
// tested and debugged without an Amiga.
//
// Keys:
//   1  C version.
//   2  68k version (the default where there is one).
//   3  Diff: runs both, paints the pixels that differ white and prints the
//      count.
//   B  (POSIX emulator only) runs the 68k version on a buffer one line too
//      short, to show the crash report.
// F1 shows the timings. PLASMA_MODE=c|68k|diff|crash in the environment sets
// the starting mode, for scripted runs.

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <canvas.h>
#include <display.h>
#include <keyboard.h>
#include <memory.h>
#include <prof.h>
#include <tornado_settings.h>

#include <demo.h>

#include "plasma.h"

#if defined(TORNADO_68K_EMU)
#include <tndo_m68k.h>
#define HAVE_ASM 1
#elif defined(__AMIGA__)
#include "amiga/plasma_inner.h"
#define HAVE_ASM 1
#endif

#define W 320
#define H 256
#define DIFF_COLOR 255

// The last key pressed, set by demo.c in debug builds.
extern int demo_last_key;

enum { MODE_C, MODE_ASM, MODE_DIFF };
static const char *modeNames[] = {"C", "68k", "diff 68k vs C"};

static int displayInstance;
static uint32_t pal[256];
static uint16_t sine[256];
static unsigned char *reference;
static int mode;
static int lastDiff = -1;

void renderPlasmaC(unsigned char *chunky, const uint16_t *s, int t) {
  for (int y = 0; y < H; y++) {
    int row = s[(3 * y - 2 * t) & 255];
    for (int x = 0; x < W; x++) {
      *chunky++ = (unsigned char)(s[(2 * x + t) & 255] +
                                  s[(x + y + 3 * t) & 255] + row);
    }
  }
}

#if defined(TORNADO_68K_EMU)

#ifndef TORNADO_68K_SHARED_HEAP
#error "plasma.c needs TORNADO_68K_SHARED_HEAP: the 68k code writes the frame buffer in place."
#endif

// 68k side of the data. The frame buffer comes from tndo_malloc(), which
// TORNADO_68K_SHARED_HEAP places in 68k memory, so it is passed as is. The
// sine table holds words: the 68k needs a big-endian copy.
static uint32_t entry;
static uint32_t sine68k;
static unsigned char *shortBuf;
static int crash;
static uint64_t instructions;

static void initAsm(void) {
  // The module is listed in DEMO_68K_OBJS and loaded by tndo_m68k_init().
  entry = tndo_m68k_symbol("_renderPlasma_asm");
  if (!entry) {
    fprintf(stderr, "FATAL - _renderPlasma_asm not loaded. Add "
                    "plasma/amiga/plasma_inner.o to DEMO_68K_OBJS.\n");
    abort();
  }
  void *s = tndo_m68k_alloc(sizeof(sine), TNDO_M68K_FAST);
  tndo_m68k_to_be16(s, sine, 256);
  sine68k = tndo_m68k_addr(s);

  // For the crash demo: one line short of a frame.
  shortBuf = (unsigned char *)tndo_malloc(W * (H - 1), 0);
}

static void renderAsm(unsigned char *chunky, int frame) {
  tndo_m68k_regs r;
  tndo_m68k_result res;
  tndo_m68k_regs_init(&r);
  r.a[0] = tndo_m68k_addr_or_die(chunky);
  r.a[1] = sine68k;
  r.d[0] = (uint32_t)frame;
  if (tndo_m68k_call(entry, &r, NULL, 0, &res) != TNDO_M68K_OK) {
    tndo_m68k_report(&res, stderr);
    abort();
  }
  if (!instructions) {
    printf("68k plasma: %llu instructions per frame.\n",
           (unsigned long long)res.instructions);
  }
  instructions = res.instructions;
}

#elif defined(__AMIGA__)

static void initAsm(void) {}

static void renderAsm(unsigned char *chunky, int frame) {
  renderPlasma_asm(chunky, sine, frame);
}

#endif

void flipPlasma(int requires_forefront) {
  if (requires_forefront) {
    display_forefront(displayInstance);
  }
  display_flip(displayInstance);
}

void initPlasma(unsigned int tornadoOptions, tornadoEffect *effect) {
  static int init = 0;
  if (init)
    return;
  init = 1;

  for (int i = 0; i < 256; i++) {
    float a = (float)i * 3.14159265f / 128.0f;
    sine[i] = (uint16_t)lrintf(42.0f + 42.0f * sinf(a));

    float c = (float)i * 3.14159265f / 126.0f;
    int r = (int)(128.0f + 127.0f * sinf(c));
    int g = (int)(128.0f + 127.0f * sinf(c * 2.0f + 1.0f));
    int b = (int)(128.0f + 127.0f * sinf(c * 3.0f + 2.0f));
    pal[i] = (uint32_t)((r << 16) | (g << 8) | b);
  }
  pal[DIFF_COLOR] = 0xffffff;

  // 320x256 8 bitplanes. No sprites and no padding.
  displayInstance = display_init(pal, tornadoOptions, SCR_NORMAL, 0, 0, 0);
  reference = (unsigned char *)tndo_malloc(W * H, 0);

#ifdef HAVE_ASM
  initAsm();
  mode = MODE_ASM;
#else
  mode = MODE_C;
#endif
#ifndef __AMIGA__
  const char *env = getenv("PLASMA_MODE");
  if (env) {
    if (!strcmp(env, "c")) {
      mode = MODE_C;
#ifdef HAVE_ASM
    } else if (!strcmp(env, "diff")) {
      mode = MODE_DIFF;
#endif
#ifdef TORNADO_68K_EMU
    } else if (!strcmp(env, "crash")) {
      crash = 1;
#endif
    }
  }
#endif
  printf("Plasma: %s version. Keys: 1 C, 2 68k, 3 diff", modeNames[mode]);
#ifdef TORNADO_68K_EMU
  printf(", B crash");
#endif
  printf(".\n");
}

static void keys(void) {
  int newMode = mode;
  switch (demo_last_key) {
  case KEY_1:
    newMode = MODE_C;
    break;
  case KEY_2:
    newMode = MODE_ASM;
    break;
  case KEY_3:
    newMode = MODE_DIFF;
    break;
#ifdef TORNADO_68K_EMU
  case KEY_B:
    crash = 1;
    break;
#endif
  }
#ifndef HAVE_ASM
  if (newMode != MODE_C) {
    printf("Plasma: no 68k version in this build (TORNADO_68K_EMU=false).\n");
    newMode = MODE_C;
  }
#endif
  if (newMode != mode) {
    mode = newMode;
    lastDiff = -1;
    printf("Plasma: %s version.\n", modeNames[mode]);
  }
}

t_canvas *renderPlasma(int frame) {
  t_canvas *c = display_get(displayInstance);
  unsigned char *chunky = c->p.pix8;

  keys();

#ifdef TORNADO_68K_EMU
  if (crash) {
    printf("Plasma: running the 68k version on a buffer one line short...\n");
    renderAsm(shortBuf, frame); // Does not return.
  }
#endif

#ifdef __DEBUG_CODE
  prof_reset_chrono();
#endif

  switch (mode) {
  case MODE_C:
    renderPlasmaC(chunky, sine, frame);
#ifdef __DEBUG_CODE
    prof_get_time("plasma C", W * H);
#endif
    break;
#ifdef HAVE_ASM
  case MODE_ASM:
    renderAsm(chunky, frame);
#ifdef __DEBUG_CODE
    prof_get_time("plasma 68k", W * H);
#endif
    break;
  case MODE_DIFF: {
    renderAsm(chunky, frame);
    renderPlasmaC(reference, sine, frame);
    int diff = 0;
    for (int i = 0; i < W * H; i++) {
      if (chunky[i] != reference[i]) {
        chunky[i] = DIFF_COLOR;
        diff++;
      }
    }
    if (diff != lastDiff) {
      printf("Plasma: %d pixels differ.\n", diff);
      lastDiff = diff;
    }
    break;
  }
#endif
  }

  return c;
}

void freePlasma(tornadoEffect *effect) {}
