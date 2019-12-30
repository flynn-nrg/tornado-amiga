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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <tndo_assert.h>

#include <dos/dos.h>
#include <exec/exec.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "custom_regs.h"
#include "graphics.h"
#include "memory.h"
#include "ptr_bridges.h"
#include "tornado_settings.h"

static void *zeroBlock = 0;
static unsigned int palZero[256];
static copper_list_t copperNull = {0, 0, 0};

static void insertZeroedSpritesInternal(copper_list_t *cop, int start) {
  if (zeroBlock == 0)
    zeroBlock = tndo_malloc(256, TNDO_ALLOC_CHIP);

  if (start == 0) {
    CI(cop, DMACON_OFFS, 0x8000 | SPREN); // Enable sprites
  }

  // Sprites pointing to black image
  unsigned int i, v = ptr_bridge(zeroBlock);
  for (i = start; i < 8; i++) {
    CI(cop, SPR0PTH_OFFS + i * 4, v >> 16);
    CI(cop, SPR0PTL_OFFS + i * 4, v & 0xffff);
  }
}

void insertZeroedSprites(copper_list_t *cop) {
  insertZeroedSpritesInternal(cop, 0);
}

// Insert bitplane pointers in a copper list.
// cop->replacePtr must point to where the bitplane pointers should go
// if your are over-writting the pointers!
void insertBPLPTR(copper_list_t *cop, void *screen_bpls, int depth, int w,
                  int h, int first_plane) {
  unsigned int v = ptr_bridge(screen_bpls);
  for (int i = BPL1PTH_OFFS + (first_plane * 4);
       i <= BPL1PTH_OFFS + (depth - 1) * 4; i += 4) {
    CI(cop, i, v >> 16);
    CI(cop, i + 2, v & 0xffff);
    v += w * h / 8;
  }
}

// Generate copper list.
void doCopper(graphics_t *graph, unsigned int tornadoOptions) {

  const int color_clock_speed = 280;
  const int lores_pixel_speed = 140;
  const int hires_pixel_speed = 70;
  const int shires_pixel_speed = 35;
  const int horizontal_blanc_stop = 0x40;
  const int pixel_per_data_fetch = 64;

  int min_row = 0x2c;

  // Special case for centered 16:9 screen.
  if (graph->h == 180) {
    min_row = 0x52;
  }

  int pixel_speed;
  int data_fetch_clocks;
  int disp_win_hstart, disp_win_vstart, disp_win_hstop, disp_win_vstop;
  int disp_win_x_size, disp_win_y_size;
  int ddfstrt, ddfstop, diwstrt, diwstop, diwhigh;

  int co, i, lin;
  int wd8 = graph->w >> 3;

  int ecsena = 1 << 0;
  int borderblank = 1 << 5;
  int color = 1 << 9;
  int hires = 0;
  int superhires = 0;
  int interlace = 0;
  int sprctl = 0;
  int coef = 4;

  switch (graph->w) {
  case 320:
    pixel_speed = lores_pixel_speed;
    break;
  case 640:
    pixel_speed = hires_pixel_speed;
    hires = 1 << 15;
    coef = 2;
    break;
  case 1280:
    pixel_speed = shires_pixel_speed;
    superhires = 1 << 6;
    break;
  default:
    fprintf(stderr, "WARNING - Unknown horizontal size %i\n", graph->w);
    pixel_speed = lores_pixel_speed;
  }

  switch (graph->h) {
  case 180:
  case 256:
    interlace = 0;
    break;
  case 360:
  case 512:
    interlace = 1 << 4;
    break;
  default:
    fprintf(stderr, "WARNING - Unknown vertical size %i\n", graph->h);
  }

  data_fetch_clocks = (pixel_per_data_fetch * pixel_speed) / color_clock_speed;
  ddfstrt = horizontal_blanc_stop - (data_fetch_clocks / coef);

  ddfstop =
      ddfstrt + (data_fetch_clocks * ((graph->w / pixel_per_data_fetch) - 1) -
                 (data_fetch_clocks - (data_fetch_clocks / coef)));

  disp_win_hstart = ((ddfstrt + (data_fetch_clocks / coef)) * 2) + 1;
  disp_win_vstart = min_row;
  diwstrt = ((disp_win_vstart & 0xff) * 0x0100) + (disp_win_hstart & 0xff);

  switch (graph->w) {
  case 320:
    disp_win_x_size = graph->w / (lores_pixel_speed / lores_pixel_speed);
    break;
  case 640:
    disp_win_x_size = graph->w / (lores_pixel_speed / hires_pixel_speed);
    break;
  case 1280:
    disp_win_x_size = graph->w / (lores_pixel_speed / shires_pixel_speed);
    break;
  default:
    fprintf(stderr, "WARNING - Unknown horizontal size %i\n", graph->w);
    disp_win_x_size = graph->w / (lores_pixel_speed / lores_pixel_speed);
  }

  disp_win_y_size = interlace ? (graph->h / 2) : graph->h;
  disp_win_hstop = disp_win_hstart + disp_win_x_size;
  disp_win_vstop = disp_win_vstart + disp_win_y_size;
  diwstop = ((disp_win_vstop & 0xff) * 0x100) + (disp_win_hstop & 0xff);
  diwhigh = (((disp_win_hstop & 0x100) >> 8) * 0x2000) +
            (((disp_win_vstop & 0x700) >> 8) * 0x100) +
            (((disp_win_hstart & 0x100) >> 8) * 0x20) +
            ((disp_win_vstart & 0x700) >> 8);

  copper_reserve(graph->copper, graph->copperSize);

  if (tornadoOptions & USE_SPRITES) {
    CI(graph->copper, DMACON_OFFS, 0x8000 | COPEN | BPLEN | SPREN);
  } else {
    CI(graph->copper, DMACON_OFFS, 0x8000 | COPEN | BPLEN);
    CI(graph->copper, DMACON_OFFS, 0x0 | SPREN); // Disable sprites
  }

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Screen parameters: DIWSTRT: %x, DIWSTOP: %x, DDFSTRT: %x, "
           "DDFSTOP: %x\n",
           diwstrt, diwstop, ddfstrt, ddfstop);
  }

  CI(graph->copper, DIWSTRT_OFFS, diwstrt);
  CI(graph->copper, DIWSTOP_OFFS, diwstop);
  CI(graph->copper, DDFSTRT_OFFS, ddfstrt);
  CI(graph->copper, DDFSTOP_OFFS, ddfstop);

  CI(graph->copper, BPLCON0_OFFS,
     ((graph->depth & 0x7) << 12) | ((graph->depth & 0x8) << 1) | color |
         ecsena | hires | superhires | interlace);

  CI(graph->copper, BPLCON1_OFFS, 0x0000); // Scrolls
  CI(graph->copper, BPLCON2_OFFS, 0x0224); // Priority over sprites and scroll?

  int bplcon3_base = borderblank | 0x0c42;
  graph->bplcon3 = bplcon3_base; // We need to store this one to be able to call
                                 // copperInsertPalette later.
  CI(graph->copper, BPLCON3_OFFS,
     bplcon3_base); // Color bank select, border color is color 0, lowres
                    // sprites
  CI(graph->copper, BPLCON4_OFFS,
     0x00ff); // display mask: XOR code for bpl and sprites

  CI(graph->copper, DIWHIGH_OFFS, diwhigh); //  DIWHIGH ECS

  if (tornadoOptions & USE_SPRITES) {
    CI(graph->copper, FMODE_OFFS, 0x000f); // FETCHMODE
  } else {
    CI(graph->copper, FMODE_OFFS, 0x000f); // FETCHMODE
    insertZeroedSprites(graph->copper);
  }

  if (interlace) {
    CI(graph->copper, BPL1MOD_OFFS, wd8); // Bitplanes modules
    CI(graph->copper, BPL2MOD_OFFS, wd8);
  } else {
    CI(graph->copper, BPL1MOD_OFFS, 0x0); // Bitplanes modules
    CI(graph->copper, BPL2MOD_OFFS, 0x0);
  }

  copper_switch_start(graph->switch_bpls, graph->copper);

  copper_switch_open_block(graph->switch_bpls);
  // Keep bpl 1 ptr ref.
  memcpy(graph->bpl1_ref, graph->copper, sizeof(copper_list_t));
  insertBPLPTR(graph->copper, graph->planar1, graph->depth, graph->w, graph->h,
               0);
  copper_switch_end_block(graph->switch_bpls);

  copper_switch_open_block(graph->switch_bpls);
  // Keep bpl 1 ptr ref.
  memcpy(graph->bpl2_ref, graph->copper, sizeof(copper_list_t));
  insertBPLPTR(graph->copper, graph->planar2, graph->depth, graph->w, graph->h,
               0);
  copper_switch_end_block(graph->switch_bpls);

  copper_switch_end(graph->switch_bpls);

  // ----- 320 pixels -------
  // ------------------------
  // SPR0|SPR1|SPR2|SPR3|SPR4
  if (tornadoOptions & USE_SPRITES) {
    // Keep sprite ptr ref.
    memcpy(graph->spr_ref, graph->copper, sizeof(copper_list_t));

    uint32_t sprOffH = SPR0PTH_OFFS;
    uint32_t sprOffL = SPR0PTL_OFFS;

    for (uint32_t i = 0; i < graph->numSprites; i++) {
      CI(graph->copper, sprOffH, ptr_bridge(graph->sprites[i]) >> 16);
      CI(graph->copper, sprOffL, ptr_bridge(graph->sprites[i]) & 0xffff);
      sprOffH += 4;
      sprOffL += 4;
    }

    insertZeroedSpritesInternal(graph->copper, graph->numSprites);
  }

  memcpy(graph->pal_ref, graph->copper, sizeof(copper_list_t));
  if (graph->pal256) {
    copperInsertPalette(graph->copper, graph->pal256, 1 << graph->depth,
                        bplcon3_base);
  }

  CI(graph->copper, 0xFFFF, 0xFFFE); // END OF COPPERLIST
  CI(graph->copper, 0xFFFF, 0xFFFE); // END OF COPPERLIST

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Copperlist usage: %i/%i\n", graph->copper->curr,
           graph->copper->len);
  }
}

void setCopper(copper_list_t *cop) {
  WRL(COP1LCH_ADDR, PBRG(cop->commands)); // Set copper ptr
  WR(COPJMP1_ADDR, 0xFFFF);
}

void copperAlloc(copper_list_t *list, int maxLen) {
  list->len = maxLen;
  list->curr = 0;
  list->commands = (unsigned int *)tndo_malloc(maxLen * 4, TNDO_ALLOC_CHIP);
}

void copperInsertPalette(copper_list_t *cop, unsigned int *pal0rgb, int len,
                         int bplcon3_base) {
  // Bank 0 low, bank 0 high, bank1 low...
  static const unsigned int banks[16] = {
      0x0C00, 0x0E00, 0x2C00, 0x2E00, 0x4C00, 0x4E00, 0x6C00, 0x6E00,
      0x8C00, 0x8E00, 0xAC00, 0xAE00, 0xCC00, 0xCE00, 0xEC00, 0xEE00};

  int lim;
  int co, lin;
  int i = 0;

  if (len > 32) {
    lim = len >> 5;
  } else {
    lim = 1;
  }

  for (co = 0; co < lim; co++) {
    // Color bank select, border color is color 0, lowres sprites
    CI(cop, BPLCON3_OFFS, bplcon3_base | banks[co * 2]);

    int i = co << 5;
    for (lin = COLOR00_OFFS; lin <= COLOR31_OFFS; lin += 2) {
      unsigned int c = ((pal0rgb[i] >> 4) & 0xf) |
                       (((pal0rgb[i] >> 12) & 0xf) << 4) |
                       (((pal0rgb[i] >> 20) & 0xf) << 8);
      CI(cop, lin, c);
      i++;
    }

    // Color bank select, border color is color 0, lowres sprites
    CI(cop, BPLCON3_OFFS, bplcon3_base | banks[co * 2 + 1]);

    i = co << 5;
    for (lin = COLOR00_OFFS; lin <= COLOR31_OFFS; lin += 2) {
      unsigned int c = (pal0rgb[i] & 0xf) | (((pal0rgb[i] >> 8) & 0xf) << 4) |
                       (((pal0rgb[i] >> 16) & 0xf) << 8);
      CI(cop, lin, c);
      i++;
    }
  }
}

void copper_log(copper_list_t *cop) {
  int i;
  for (i = 0; i < cop->curr; i++) {
    fprintf(stderr, "%d: 0x%lx -> 0x%x, 0x%x\n", i,
            ptr_bridge(&cop->commands[i]), cop->commands[i] >> 16,
            cop->commands[i] & 0xffff);
  }
}

void copper_reserve(copper_list_t *list, int maxlen) {
  list->len = maxlen;
  list->curr = 0;
  // Aligned to 4 KB to allow easy jumping (16 low bits) inside the copperlist
  list->commands =
      (unsigned int *)tndo_malloc_align(maxlen * 4, TNDO_ALLOC_CHIP, 12);
  tndo_assert(list->commands);
}

void copper_switch_start(copper_switch_t *t, copper_list_t *cop) {
  t->cop = cop;
  t->switch_idx = cop->curr;
  t->curr_block = 0;

  CI(cop, NOOP_OFFS, 0);
  CI(cop, NOOP_OFFS, 0);
  CI(cop, NOOP_OFFS, 0);
}

void copper_switch_open_block(copper_switch_t *t) {
  t->blocks_indices[t->curr_block] = t->cop->curr;
  t->curr_block++;
  tndo_assert(t->curr_block < 16);
}

void copper_switch_end_block(copper_switch_t *t) {
  t->exits_indices[t->curr_block - 1] = t->cop->curr;

  CI(t->cop, NOOP_OFFS, 0);
  CI(t->cop, NOOP_OFFS, 0);
  CI(t->cop, NOOP_OFFS, 0);
}

void copper_switch_end(copper_switch_t *t) {
  unsigned int base_h16;
  t->exit_idx = t->cop->curr;

  copper_list_t *c = t->cop;

  // Switch command
  unsigned int *addr = &c->commands[t->blocks_indices[0]];
  unsigned int raddr = ptr_bridge(addr);
  base_h16 = raddr >> 16;
  c->commands[t->switch_idx + 0] = (COP2LCH_OFFS << 16) | (raddr >> 16);
  c->commands[t->switch_idx + 1] = (COP2LCL_OFFS << 16) | (raddr & 0xffff);
  c->commands[t->switch_idx + 2] = (COPJMP2_OFFS << 16) | 0xffff;

  // Exits
  int i;
  for (i = 0; i < t->curr_block; i++) {
    addr = &c->commands[t->exits_indices[i]];
    raddr = ptr_bridge(addr);
    tndo_assert(base_h16 ==
                (raddr >> 16)); // Check we are in the same 64KB block

    addr = &c->commands[t->exit_idx];
    raddr = ptr_bridge(addr);
    c->commands[t->exits_indices[i] + 0] = (COP2LCH_OFFS << 16) | (raddr >> 16);
    c->commands[t->exits_indices[i] + 1] =
        (COP2LCL_OFFS << 16) | (raddr & 0xffff);
    c->commands[t->exits_indices[i] + 2] = (COPJMP2_OFFS << 16) | 0xffff;
  }
}

void copper_switch_choose(copper_switch_t *t, int option) {
  copper_list_t *c = t->cop;
  // Switch command
  unsigned int *addr = &t->cop->commands[t->blocks_indices[option]];
  unsigned int raddr = ptr_bridge(addr);
  c->commands[t->switch_idx + 0] = (COP2LCH_OFFS << 16) | (raddr >> 16);
  c->commands[t->switch_idx + 1] = (COP2LCL_OFFS << 16) | (raddr & 0xffff);
  c->commands[t->switch_idx + 2] = (COPJMP2_OFFS << 16) | 0xffff;
}

void copper_modify_jump(copper_list_t *cop, copper_list_t *jmp) {
  copper_list_t tc = *cop;
  unsigned int *addr = &jmp->commands[jmp->curr];
  unsigned int raddr = ptr_bridge(addr);
  CI(&tc, COP2LCH_OFFS, raddr >> 16);
  CI(&tc, COP2LCL_OFFS, raddr & 0xffff);
  CI(&tc, COPJMP2_OFFS, 0xffff);
}
