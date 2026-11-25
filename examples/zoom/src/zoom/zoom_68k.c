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

// POSIX build with TORNADO_68K_EMU: renderZoom_asm() runs the Amiga inner
// loop (amiga/zoom_inner.s) in the emulated 68k instead of renderZoomC().
//
// The pixel data needs no conversion: textures and the frame buffer come
// from tndo_malloc(), which TORNADO_68K_SHARED_HEAP places in 68k memory.
// What does need converting:
// - allTxtPtr: host tables of 64-bit pointers. Each one gets a 68k copy of
//   32-bit big-endian addresses, built on first use. The tables are filled
//   in initZoom() and never change afterwards.
// - zoomIter: host ints. Each call copies the iteration, byte swapped, into
//   a 68k buffer (3KB, negligible next to the 57600 pixels).
//
// Set ZOOM_68K_CHECK=1 in the environment to also run renderZoomC() and
// compare the two frames pixel by pixel.

#ifdef TORNADO_68K_EMU

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tndo_m68k.h>

#include "zoom.h"

#include "amiga/zoom_inner.h"

#ifndef TORNADO_68K_SHARED_HEAP
#error "zoom_68k.c needs TORNADO_68K_SHARED_HEAP: the 68k code reads the textures and writes the frame buffer in place."
#endif

#define ZOOM_W 320
#define ZOOM_H 180
#define TXT_ENTRIES (ZOOM_TXT_SIZE * ZOOM_TXT_SIZE)
#define MAX_TABLES 8

// zoom_inner.s hardcodes the zoomIter layout (its EQUs).
_Static_assert(offsetof(zoomIter, mipMapIdx) == 0, "zoomIter.mipMapIdx");
_Static_assert(offsetof(zoomIter, ix) == 4, "zoomIter.ix");
_Static_assert(offsetof(zoomIter, iy) == 4 + 320, "zoomIter.iy");
_Static_assert(offsetof(zoomIter, iTileX) == 4 + 320 + 180 * 4,
               "zoomIter.iTileX");
_Static_assert(offsetof(zoomIter, iTileY) == 4 + 320 + 180 * 4 + 320 * 4,
               "zoomIter.iTileY");

typedef struct {
  unsigned char ***host;
  uint32_t m68k; // 68k address of ZOOM_MIPMAPS pointers to pointer tables.
} txtTable;

static txtTable tables[MAX_TABLES];
static int numTables;
static uint32_t entry;
static zoomIter *iter68k; // Host view of the 68k copy of the iteration.
static int check = -1;
static unsigned char *checkBuf;
static int checkReports;

// 68k version of an allTxtPtr table, built on first use.
static uint32_t txtTable68k(unsigned char ***allTxtPtr) {
  for (int i = 0; i < numTables; i++) {
    if (tables[i].host == allTxtPtr) {
      return tables[i].m68k;
    }
  }
  if (numTables == MAX_TABLES) {
    fprintf(stderr, "FATAL - zoom_68k: more than %d texture tables.\n",
            MAX_TABLES);
    abort();
  }

  uint32_t mips[ZOOM_MIPMAPS];
  for (int j = 0; j < ZOOM_MIPMAPS; j++) {
    mips[j] = tndo_m68k_ptr_table((void *const *)allTxtPtr[j], TXT_ENTRIES);
  }
  uint8_t *top = tndo_m68k_alloc(sizeof(mips), TNDO_M68K_FAST);
  tndo_m68k_to_be32(top, mips, ZOOM_MIPMAPS);

  tables[numTables].host = allTxtPtr;
  tables[numTables].m68k = tndo_m68k_addr(top);
  return tables[numTables++].m68k;
}

static void copyIteration(zoomIter *dst, const zoomIter *src) {
  tndo_m68k_wr32(&dst->mipMapIdx, (uint32_t)src->mipMapIdx);
  memcpy(dst->ix, src->ix, sizeof(src->ix));
  tndo_m68k_to_be32(dst->iy, (const uint32_t *)src->iy, ZOOM_H);
  tndo_m68k_to_be32(dst->iTileX, (const uint32_t *)src->iTileX, ZOOM_W);
  tndo_m68k_to_be32(dst->iTileY, (const uint32_t *)src->iTileY, ZOOM_H);
}

// Reports the first frames where the 68k and C versions differ.
static void compare(const unsigned char *asmFrame, const unsigned char *cFrame,
                    const zoomIter *it) {
  int diff = 0;
  int first = -1;
  for (int i = 0; i < ZOOM_W * ZOOM_H; i++) {
    if (asmFrame[i] != cFrame[i]) {
      if (first < 0) {
        first = i;
      }
      diff++;
    }
  }
  if (diff && checkReports < 10) {
    checkReports++;
    int x = first % ZOOM_W;
    int y = first / ZOOM_W;
    fprintf(stderr,
            "zoom_68k: %d pixels differ from renderZoomC (mipmap %d). First "
            "at %d,%d: asm %d, C %d (iy %d, tileX %d, tileY %d)\n",
            diff, it->mipMapIdx, x, y, asmFrame[first], cFrame[first],
            it->iy[y], it->iTileX[x], it->iTileY[y]);
  }
}

void renderZoom_asm(unsigned char ***allTxtPtr, unsigned char *chunky,
                    zoomIter *iteration) {
  if (!entry) {
    entry = tndo_m68k_symbol("_renderZoom_asm");
    if (!entry) {
      fprintf(stderr, "FATAL - zoom_68k: _renderZoom_asm not loaded. Add "
                      "zoom/amiga/zoom_inner.o to DEMO_68K_OBJS.\n");
      abort();
    }
    iter68k = tndo_m68k_alloc(sizeof(zoomIter), TNDO_M68K_FAST);
  }
  if (check < 0) {
    const char *env = getenv("ZOOM_68K_CHECK");
    check = env && atoi(env);
    if (check) {
      checkBuf = malloc(ZOOM_W * ZOOM_H);
    }
  }

  copyIteration(iter68k, iteration);

  tndo_m68k_regs r;
  tndo_m68k_regs_init(&r);
  r.a[0] = txtTable68k(allTxtPtr);
  r.a[1] = tndo_m68k_addr_or_die(chunky);
  r.a[2] = tndo_m68k_addr(iter68k);
  tndo_m68k_call_or_die(entry, &r);

  if (check) {
    renderZoomC(allTxtPtr, checkBuf, iteration);
    compare(chunky, checkBuf, iteration);
  }
}

#endif // TORNADO_68K_EMU
