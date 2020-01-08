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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <tndo_assert.h>

#include <dos/dos.h>
#include <exec/exec.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "c2p.h"
#include "copper.h"
#include "custom_regs.h"
#include "graphics.h"
#include "memory.h"
#include "ptr_bridges.h"
#include "tornado_settings.h"

#define CHUNKY_SIZE (128 * 1024)
static void *chunky = 0;

static void allocSprites(graphics_t *graph, uint8_t *ptr) {
  for (uint32_t i = 0; i < graph->numSprites; i++) {
    graph->sprites[i] = ptr;
    ptr += graph->spriteSize;
  }
}

void loadRGB32toRGB(uint32_t *source, uint32_t *dst) {
  uint32_t numCols = source[0] >> 16;
  source++;
  uint32_t k = 0;
  for (uint32_t i = 0; i < numCols; i++) {
    uint32_t r = source[k++] >> 24;
    uint32_t g = source[k++] >> 24;
    uint32_t b = source[k++] >> 24;
    dst[i] = b | (g << 8) | (r << 16);
  }
}

void setSpriteControlWords(graphics_t *graph, graphicsOptions *gOptions,
                           int mode) {

  for (uint32_t i = 0; i < gOptions->numSprites; i++) {

    uint16_t *cwords = (uint16_t *)gOptions->sprites[i];
    uint32_t spr_x = gOptions->spritesX[i] >> 2;
    uint32_t spr_x_fine = gOptions->spritesX[i] & 0x3;
    uint32_t spr_y = gOptions->spritesY[i];
    uint32_t spr_endy = spr_y + graph->h;
    uint32_t attach = gOptions->spritesAttach[i];

    // SPRXPOS
    // Bits 15-8 specify the vertical start position, bits V7 - V0.
    // Bits 7-0 specify the horizontal start position, bits H8 - H1.
    cwords[0] = ((spr_x >> 1) & 0xff) | ((spr_y & 0xff) << 8);
    // SPRXCTL
    //  Bit 0 is bit H0 of horizontal start.
    //  Bit 1 is bit V8 of vertical stop.
    cwords[4] =
        (spr_x & 0x1) | ((spr_endy & 0x100) >> 7) |
        //  Bit 2 is bit V8 of vertical start.
        ((spr_y & 0x100) >> 6) |
        // Super hires finetune.
        (spr_x_fine << 3) |
        //  Bit 7 is the attach bit. This bit is valid only for odd-numbered
        (attach << 7) |
        // Bits 15-8 specify vertical stop position for a sprite image,
        ((spr_endy & 0xff) << 8);

    if (mode == SPRITE_SETUP_CHIP) {
      uint16_t *chip_cwords = (uint16_t *)graph->sprites[i];
      chip_cwords[0] = cwords[0];
      chip_cwords[4] = cwords[4];
    }
  }
}

void genPalette(unsigned int *pal, int numcols) {
  for (int i = 0; i < numcols; i++) {
    float f0 = i;
    float f1 = f0 / numcols;
    float f2 = pow(f1, 0.4545);
    int v = 255.0 * f2;
    pal[i] = (v) | (v << 8) | (v << 16);
  }
}

void loadPalette(unsigned int *pal0rgb, int len) {
  int lim = 256 / 32;
  int co, lin;

  for (co = 0; co < lim; co++) {
    // Bank 0 low, bank 0 high, bank1 low...
    static const unsigned int banks[16] = {
        0x0C60, 0x0E60, 0x2C60, 0x2E60, 0x4C60, 0x4E60, 0x6C60, 0x6E60,
        0x8C60, 0x8E60, 0xAC60, 0xAE60, 0xCC60, 0xCE60, 0xEC60, 0xEE60};
    WR(BPLCON3_ADDR, banks[co * 2]); // Color bank select, border color is color
                                     // 0, lowres sprites

    int i = co * 32;

    for (lin = COLOR00_ADDR; lin <= COLOR31_ADDR; lin += 2) {
      unsigned int c = ((pal0rgb[i] >> 4) & 0xf) |
                       (((pal0rgb[i] >> 12) & 0xf) << 4) |
                       (((pal0rgb[i] >> 20) & 0xf) << 8);
      WR(lin, c);
      i++;
    }

    WR(BPLCON3_ADDR, banks[co * 2 + 1]); // Color bank select, border color is
                                         // color 0, lowres sprites

    i = co * 32;
    for (lin = COLOR00_ADDR; lin <= COLOR31_ADDR; lin += 2) {
      unsigned int c = (pal0rgb[i] & 0xf) | (((pal0rgb[i] >> 8) & 0xf) << 4) |
                       (((pal0rgb[i] >> 16) & 0xf) << 8);
      WR(lin, c);
      i++;
    }
  }
}

graphics_t *initGraphics(graphicsOptions *gOptions) {
  graphics_t *graph = (graphics_t *)tndo_malloc(sizeof(graphics_t), 0);

  graph->screenMode = gOptions->screenMode;
  graph->options = gOptions->flags;
  graph->copperSize = gOptions->copperSize;
  graph->copper = tndo_malloc(sizeof(copper_list_t), 0);
  graph->pal_ref = tndo_malloc(sizeof(copper_list_t), 0);
  graph->spr_ref = tndo_malloc(sizeof(copper_list_t), 0);
  graph->bpl1_ref = tndo_malloc(sizeof(copper_list_t), 0);
  graph->bpl2_ref = tndo_malloc(sizeof(copper_list_t), 0);
  graph->switch_bpls = tndo_malloc(sizeof(copper_switch_t), 0);
  graph->numSprites = gOptions->numSprites;
  graph->spriteSize = gOptions->spriteSize;

  if (graph->options & PALETTE_IN_COPPER) {
    graph->pal256 = tndo_malloc(256 * sizeof(unsigned int), 0);
    tndo_assert(graph->pal256);
    if (gOptions->customPal) {
      memcpy(graph->pal256, gOptions->customPal, 256 * sizeof(unsigned int));
    }
  }

  switch (graph->screenMode) {
    // 320x256 8 bitplanes
  case SCR_NORMAL:
    if (!(graph->options & CUSTOM_C2P)) {
      c2p1x1_8_c5_init(320, 256, 0, 0, 0, 320 * 256 / 8);
    }
    graph->w = 320;
    graph->h = 256;
    graph->depth = 8;
    break;

    // 320x256 6 bitplanes
  case SCR_NORMAL_6BPL:
    graph->w = 320;
    graph->h = 256;
    graph->depth = 6;
    break;

    // 320x180 8 bitplanes
  case SCR_16_9:
    if (!(graph->options & CUSTOM_C2P)) {
      c2p1x1_8_c5_040_init(320, 180, 0, 0, 0, 320 * 180 / 8, 0);
    }
    graph->w = 320;
    graph->h = 180;
    graph->depth = 8;
    break;

    // 320x180 6 bitplanes
  case SCR_16_9_6BPL:
    if (!(graph->options & CUSTOM_C2P)) {
      c2p1x1_8_c5_040_init(320, 180, 0, 0, 0, 320 * 180 / 8, 0);
    }
    graph->w = 320;
    graph->h = 180;
    graph->depth = 6;
    break;

    // 320x180 4 bitplanes
  case SCR_16_9_4BPL:
    if (!(graph->options & CUSTOM_C2P)) {
      c2p1x1_4_c5_16_9_init(320, 180, 0, 0, 0, 320 * 180 / 8);
    }
    graph->w = 320;
    graph->h = 180;
    graph->depth = 4;
    break;

    // 320x180 5 bitplanes
  case SCR_16_9_5BPL:
    graph->w = 320;
    graph->h = 180;
    graph->depth = 5;
    break;

    // 320x180 6 bitplanes with 4 bitplanes chunky buffer + 2 native bitplanes
    // for overlay.
  case SCR_16_9_6_4_BPL:
    if (!(graph->options & CUSTOM_C2P)) {
      c2p1x1_4_c5_16_9_init(320, 180, 0, 0, 0, 320 * 180 / 8);
    }
    graph->w = 320;
    graph->h = 180;
    graph->depth = 6;
    break;

    // 640x180 4 bitplanes
  case SCR_16_9_H_4BPL:
    if (!(graph->options & CUSTOM_C2P)) {
      c2p1x1_4_c5_16_9_h_init(640, 180, 0, 0, 0, 640 * 180 / 8);
    }
    graph->w = 640;
    graph->h = 180;
    graph->depth = 4;
    break;

  case SCR_16_9_H_8BPL:
    if (!(graph->options & CUSTOM_C2P)) {
      c2p1x1_4_c5_16_9_h_init(640, 180, 0, 0, 0, 640 * 180 / 8);
    }
    graph->w = 640;
    graph->h = 360;
    graph->depth = 8;
    break;

  case SCR_16_9_8BPL_PLANAR:
	  c2p1x1_8_c5_040_scanline_init(320, 1, 0, 0, 0, 320 * 180 / 8, 0);
    graph->w = 320;
    graph->h = 180;
    graph->depth = 8;
    break;

  case SCR_16_9_H_8BPL_PLANAR:
    c2p1x1_8_c5_040_scanline_init(640, 1, 0, 0, 0, 640 * 180 / 8, 0);
    graph->w = 640;
    graph->h = 180;
    graph->depth = 8;
    break;

    // 640x512 8 bitplanes to debug multipass effects.
  case SCR_DEBUG4:
    if (!(graph->options & CUSTOM_C2P)) {
      c2p1x1_8_c5_init(640, 512, 0, 0, 0, 640 * 512 / 8);
    }
    graph->w = 640;
    graph->h = 512;
    graph->depth = 8;
    graph->planar1 = tndo_malloc(640 * 512, TNDO_ALLOC_CHIP);
    graph->planar2 = tndo_malloc(640 * 512, TNDO_ALLOC_CHIP);

    if (graph->options & GEN_DEFAULT_PALETTE) {
      genPalette(graph->pal256, 256);
    }
    break;

    // 320x256 8 bitplanes with float chunky buffer
  case SCR_FLOAT:
    if (!(graph->options & CUSTOM_C2P)) {
      c2p1x1_8_c5_init(320, 256, 0, 0, 0, 320 * 256 / 8);
    }
    graph->w = 320;
    graph->h = 256;
    graph->depth = 8;
    break;
  default:
    fprintf(stderr,
            "FATAL - Unkown mode %d (check tornado_setting.h for enum). "
            "Aborting.\n",
            graph->screenMode);
    abort();
  }

  // Allocate chunky buffer only once.
  // 128KiB should be enough for all the graphics modes.
  if (!chunky) {
    chunky = tndo_malloc(CHUNKY_SIZE, 0);
  }

  uint32_t padding = gOptions->paddingTop + gOptions->paddingBottom;
  tndo_assert((graph->w * (graph->h + padding)) < CHUNKY_SIZE);

  graph->chunky = chunky;

  uint32_t spriteAlloc = 0;
  if (gOptions->tornadoOptions & USE_SPRITES) {
    spriteAlloc = gOptions->numSprites * ((64 * graph->h) / 2);
  }

  if (graph->options & REUSE_PLANAR_BUFFERS) {
    uint8_t *scr = (uint8_t *)get_chipmem_scratchpad_addr(
        (graph->w * graph->h * 2) + spriteAlloc);
    graph->planar1 = scr;
    graph->planar2 = scr + (graph->w * graph->h);

    if (gOptions->tornadoOptions & USE_SPRITES) {
      allocSprites(graph, scr + (graph->w * graph->h * 2));
    }
  } else {
    graph->planar1 = tndo_malloc(graph->w * graph->h, TNDO_ALLOC_CHIP);
    graph->planar2 = tndo_malloc(graph->w * graph->h, TNDO_ALLOC_CHIP);
    uint8_t *spr = (uint8_t *)get_chipmem_scratchpad_addr(spriteAlloc);
    if (gOptions->tornadoOptions & USE_SPRITES) {
      allocSprites(graph, spr);
    }
  }

  if (graph->options & GEN_DEFAULT_PALETTE) {
    genPalette(graph->pal256, 2 << (graph->depth));
  }

  doCopper(graph, gOptions->tornadoOptions);

  if (graph->options & FAST_PLANAR) {
    graph->fastPlanar1 = tndo_malloc(graph->w * graph->h, 0);
    graph->fastPlanar2 = tndo_malloc(graph->w * graph->h, 0);
  }

  if (graph->options & G_BUFFER) {
    graph->gbuffer1 =
        tndo_malloc(graph->w * graph->h * SCR_GBUFFER_PIXEL_SIZE, 0);
    graph->gbuffer2 =
        tndo_malloc(graph->w * graph->h * SCR_GBUFFER_PIXEL_SIZE, 0);
  }

  return graph;
}
