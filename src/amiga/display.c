/*
Copyright (c) 2019 Miguel Mendez
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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aga.h>
#include <assets.h>
#include <c2p.h>
#include <canvas.h>
#include <copper.h>
#include <cpu.h>
#include <custom_regs.h>
#include <debug.h>
#include <display.h>
#include <graphics.h>
#include <hardware_check.h>
#include <memory.h>
#include <ptr_bridges.h>
#include <system.h>
#include <tndo_assert.h>
#include <tornado_settings.h>

static int c2pInitDone[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static display_instance *instances;
static int maxDisplayInstances;
static sprite_options empty_so = {.num_sprites = 0};
static int lastInstance = 0;

static void clean_bpls(int instance) {
  memset(instances[instance].graph->planar1, 0,
         instances[instance].graph->w * instances[instance].graph->h);
  memset(instances[instance].graph->planar2, 0,
         instances[instance].graph->w * instances[instance].graph->h);
}

static void merge_pal(unsigned int *dst, uint8_t *src, int pos, int count) {
  for (int i = 0; i < count; i++) {
    int r = src[i * 3 + 0];
    int g = src[i * 3 + 1];
    int b = src[i * 3 + 2];
    dst[pos + i] = b | (g << 8) | (r << 16);
  }
}

static void fade_pal_black(unsigned int *dst, unsigned int *src_, float k,
                           int count) {
  uint8_t *src = (uint8_t *)src_;
  for (int i = 0; i < count; i++) {
    int r = src[1] * k;
    int g = src[2] * k;
    int b = src[3] * k;
    src += 4;
    *dst++ = b | (g << 8) | (r << 16);
  }
}

static void fade_pal_white(unsigned int *dst, unsigned int *src_, float k,
                           int count) {
  uint8_t *src = (uint8_t *)src_;
  k = 1.0f - k;
  for (int i = 0; i < count; i++) {
    int r = 255 - ((255 - src[1]) * k);
    int g = 255 - ((255 - src[2]) * k);
    int b = 255 - ((255 - src[3]) * k);
    src += 4;
    *dst++ = b | (g << 8) | (r << 16);
  }
}

// Set fade level: black = -8, normal = 0, white = 8
static void display_set_fade(int instance, unsigned int *faded_pal) {
  static copper_list_t pal_cop = {0, 0, 0};
  graphics_t *graph = instances[instance].graph;
  pal_cop.commands = graph->pal_ref->commands;
  pal_cop.curr = graph->pal_ref->curr;
  pal_cop.len = graph->pal_ref->len;
  copperInsertPalette(&pal_cop, faded_pal, 1 << graph->depth, graph->bplcon3);
}

// Set black fade level: normal = 0, black >= 16
void display_set_fade_black(int instance, int i) {
  if (i <= 0) {
    display_set_fade(instance, instances[instance].pal256);
    return;
  }
  i -= 1;
  if (i > 15) {
    i = 15;
  }
  display_set_fade(instance, &instances[instance].pal256black[i][0]);
}

// Set white fade level: normal = 0, white >= 16
void display_set_fade_white(int instance, int i) {
  if (i <= 0) {
    display_set_fade(instance, instances[instance].pal256);
    return;
  }
  i -= 1;
  if (i >= 15) {
    i = 15;
  }
  display_set_fade(instance, &instances[instance].pal256white[i][0]);
}

// Set sprites position. You most likely only want to do this during
// vblank as you're modifying the control words in chipmem.
void display_set_sprites_pos(int instance, sprite_options *so) {
  graphicsOptions go = {.numSprites = so->num_sprites};
  if (so->num_sprites > 0) {
    t_bpl_head *sprites = so->spr_data;
    uint8_t *spr_data = sprites->data;
    spr_data += (16 * 3); // Skip palette.
    go.spriteSize = instances[instance].sprSize;
    for (uint32_t i = 0; i < so->num_sprites; i++) {
      go.sprites[i] = spr_data;
      go.spritesX[i] = so->spritesX[i];
      go.spritesY[i] = so->spritesY[i];
      go.spritesAttach[i] = so->spritesAttach[i];
      spr_data += instances[instance].sprSize;
    }
  }

  setSpriteControlWords(instances[instance].graph, &go, SPRITE_SETUP_CHIP);
}

void display_subsystem_init(int numInstances) {
  instances = tndo_malloc(numInstances * sizeof(display_instance), 0);
  maxDisplayInstances = numInstances;
}

void display_subsystem_end() {
#ifdef __DEBUG_CODE
  printf("DEBUG - display_subsystem_end(): %u/%u display instances used.\n",
         lastInstance, maxDisplayInstances);
#endif
}

int display_init(unsigned int *pal, unsigned int options, int mode,
                 unsigned int padding_top, unsigned int padding_bottom,
                 sprite_options *so) {
  if (!so) {
    so = &empty_so;
  }

  tndo_assert(lastInstance < maxDisplayInstances);

  graphicsOptions go = {.screenMode = mode,
                        .paddingTop = padding_top,
                        .paddingBottom = padding_bottom,
                        .flags = CHUNKY_BUFFERS | REUSE_PLANAR_BUFFERS |
                                 CUSTOM_C2P | PALETTE_IN_COPPER,
                        .tornadoOptions = options,
                        .copperSize = 680, // Obtained from the logs.
                        .customPal = pal,
                        .numSprites = so->num_sprites};

  if (so->num_sprites > 0) {
    t_bpl_head *sprites = so->spr_data;
    uint8_t *spr_data = sprites->data;
    merge_pal(pal, spr_data, 240, 16);
    spr_data += (16 * 3); // Skip palette.
    instances[lastInstance].sprSize =
        _get_spr_size(sprites->w, sprites->h, 4) / so->num_sprites;
    go.spriteSize = instances[lastInstance].sprSize;
    go.tornadoOptions |= USE_SPRITES;
    instances[lastInstance].numSprites = so->num_sprites;
    for (uint32_t i = 0; i < so->num_sprites; i++) {
      go.sprites[i] = spr_data;

      go.spritesX[i] = so->spritesX[i];
      go.spritesY[i] = so->spritesY[i];
      go.spritesAttach[i] = so->spritesAttach[i];

      instances[lastInstance].display_sprites[i] = spr_data;
      spr_data += instances[lastInstance].sprSize;
    }
  }

  for (int i = 1; i <= 16; i++) {
    fade_pal_black(&instances[lastInstance].pal256black[i - 1][0], pal,
                   1.0f - (i / 16.0f), 256);
    fade_pal_white(&instances[lastInstance].pal256white[i - 1][0], pal,
                   i / 16.0f, 256);
  }

  instances[lastInstance].graph = initGraphics(&go);
  setSpriteControlWords(instances[lastInstance].graph, &go, SPRITE_SETUP_FAST);

  instances[lastInstance].paddingTop = padding_top;
  instances[lastInstance].paddingBottom = padding_bottom;
  instances[lastInstance].c2pSkip =
      padding_top * instances[lastInstance].graph->w;

  instances[lastInstance].mode = mode;

  if (c2pInitDone[mode] == 0) {
    switch (mode) {
    case SCR_NORMAL:
      c2p1x1_8_c5_040_init(instances[lastInstance].graph->w,
                           instances[lastInstance].graph->h, 0, 0, 0,
                           (instances[lastInstance].graph->w *
                            instances[lastInstance].graph->h) /
                               8,
                           0);
      break;

    case SCR_16_9:
      c2p1x1_8_c5_040_16_9_init(instances[lastInstance].graph->w,
                                instances[lastInstance].graph->h, 0, 0, 0,
                                (instances[lastInstance].graph->w *
                                 instances[lastInstance].graph->h) /
                                    8,
                                0);
      break;
    case SCR_16_9_4BPL:
      c2p1x1_4_c5_16_9_init(instances[lastInstance].graph->w,
                            instances[lastInstance].graph->h, 0, 0, 0,
                            (instances[lastInstance].graph->w *
                             instances[lastInstance].graph->h) /
                                8);
      break;
    case SCR_16_9_HL_8BPL:
      c2p1x1_8_c5_040_16_9_init(instances[lastInstance].graph->w,
                                instances[lastInstance].graph->h, 0, 0, 0,
                                (instances[lastInstance].graph->w *
                                 instances[lastInstance].graph->h) /
                                    8,
                                0);
      break;
    }
    c2pInitDone[mode] = 1;
  }

  instances[lastInstance].chunky = instances[lastInstance].graph->chunky;
  instances[lastInstance].planar[0] = instances[lastInstance].graph->planar1;
  instances[lastInstance].planar[1] = instances[lastInstance].graph->planar2;
  instances[lastInstance].p = 0;
  instances[lastInstance].pal256 = pal;

  int currentInstance = lastInstance;
  lastInstance++;
  return currentInstance;
}

static t_canvas _fb = {.w = 0, .h = 0, .bypp = 1};

t_canvas *display_get(int instance) {
  _fb.w = instances[instance].graph->w;
  _fb.h = instances[instance].graph->h;
  _fb.p.pix8 = instances[instance].chunky;
  return &_fb;
}

void display_forefront(int instance) {
  clean_bpls(instance);
  if (instances[instance].numSprites > 0) {
    for (int i = 0; i < instances[instance].numSprites; i++) {
      memcpy(instances[instance].graph->sprites[i],
             instances[instance].display_sprites[i],
             instances[instance].sprSize);
    }
  }
  WRL(COP1LCH_ADDR, PBRG(instances[instance].graph->copper->commands));
}

void display_set_copper(int instance) {}

static int prev_instance = 0;

void display_flip(int instance) {

  instances[instance].p ^= 1;

  switch (instances[instance].mode) {
  case SCR_NORMAL:
    c2p1x1_8_c5_040(instances[instance].chunky + instances[instance].c2pSkip,
                    instances[instance].planar[instances[instance].p]);
    break;
  case SCR_16_9:
    c2p1x1_8_c5_040_16_9(instances[instance].chunky +
                             instances[instance].c2pSkip,
                         instances[instance].planar[instances[instance].p]);
    break;
  case SCR_16_9_4BPL:
    c2p1x1_4_c5_16_9(instances[instance].chunky + instances[instance].c2pSkip,
                     instances[instance].planar[instances[instance].p]);
    break;
  case SCR_16_9_6BPL:
    c2p1x1_6_c5_gen(instances[instance].chunky + instances[instance].c2pSkip,
                    instances[instance].planar[instances[instance].p],
                    instances[instance].graph->w *
                        instances[instance].graph->h);
    break;
  case SCR_16_9_5BPL:
    c2p1x1_5_c5_060(instances[instance].chunky + instances[instance].c2pSkip,
                    instances[instance].planar[instances[instance].p],
                    instances[instance].graph->w *
                        instances[instance].graph->h);
    break;
  case SCR_16_9_HL_8BPL:
    c2p1x1_8_c5_040_16_9(instances[instance].chunky +
                             instances[instance].c2pSkip,
                         instances[instance].planar[instances[instance].p]);
    break;
  }

  copper_switch_choose(instances[instance].graph->switch_bpls,
                       instances[instance].p);
  if (prev_instance != instance) {
    if (instances[instance].numSprites > 0) {
      for (int i = 0; i < instances[instance].numSprites; i++) {
        memcpy(instances[instance].graph->sprites[i],
               instances[instance].display_sprites[i],
               instances[instance].sprSize);
      }
    }
    WRL(COP1LCH_ADDR, PBRG(instances[instance].graph->copper->commands));
    prev_instance = instance;
  }
}

void display_end(int instance) {}
