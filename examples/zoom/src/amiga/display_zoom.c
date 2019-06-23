#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tndo_assert.h>

#include <aga.h>
#include <assets.h>
#include <c2p.h>
#include <canvas.h>
#include <copper.h>
#include <cpu.h>
#include <custom_regs.h>
#include <debug.h>
#include <display_zoom.h>
#include <graphics.h>
#include <hardware_check.h>
#include <memory.h>
#include <ptr_bridges.h>
#include <system.h>
#include <tornado_settings.h>

#define NUM_PALS 1
#define NUM_NOISE_FRAMES 8
#define NUM_NOISE_FRAMES_MAX 16
#define FADE_TO_WHITE_STEPS 25
#define FADE_TO_BLACK_STEPS 0
#define FADE_STEPS FADE_TO_WHITE_STEPS + FADE_TO_BLACK_STEPS

static graphics_t *_graph;
static unsigned int *fadePal[FADE_STEPS + 1];
static unsigned int *pal64;
static t_canvas _fb = {.w = 0, .h = 0, .bypp = 1};
static unsigned char *_chunky;
static unsigned char *_planar[2];
static copper_list_t *_copper;
static unsigned char *noise_frames[16];
static int _p;
static int fadeIndex;
static copper_list_t _pal_cop = {0, 0, 0};

static void clean_bpls() {
  memset(_graph->planar1, 0, _graph->w * _graph->h);
  memset(_graph->planar2, 0, _graph->w * _graph->h);
}

void display_zoom_setpal(int i) {
  fadeIndex = i;
  _pal_cop.commands = _graph->pal_ref->commands;
  _pal_cop.curr = _graph->pal_ref->curr;
  _pal_cop.len = _graph->pal_ref->len;
  copperInsertPalette(&_pal_cop, fadePal[fadeIndex], 1 << _graph->depth,
                      _graph->bplcon3);
}

static void gen_noise_frames(int num_frames) {
  srand(0xdeadbeef);
  tndo_assert(num_frames < NUM_NOISE_FRAMES_MAX);

  int frame_size = (_fb.w * _fb.h) / 8;

  for (int i = 0; i < num_frames; i++) {
    noise_frames[i] = tndo_malloc(frame_size, TNDO_ALLOC_CHIP);
    unsigned char *frame_ptr = noise_frames[i];
    for (int j = 0; j < frame_size; j++) {
      frame_ptr[j] = (unsigned char)rand();
    }
  }
}

static void fade_pal_white(unsigned int *dst, unsigned int *src_, float k,
                           int count) {
  uint8_t *src = (uint8_t *)src_;
  // k = 1.0f - k;
  for (int i = 0; i < count; i++) {
    int r = 255 - ((255 - src[1]) * k);
    int g = 255 - ((255 - src[2]) * k);
    int b = 255 - ((255 - src[3]) * k);
    src += 4;
    *dst++ = b | (g << 8) | (r << 16);
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

static void gen_fade_in(unsigned int *pal, int delta) {

  float delta_r[64];
  float delta_g[64];
  float delta_b[64];
  float current_r[64];
  float current_g[64];
  float current_b[64];

  for (int i = 0; i < 16; i++) {
    current_r[i] = (float)(pal[i] >> 16);
    current_g[i] = (float)((pal[i] >> 8) & 0xff);
    current_b[i] = (float)(pal[i] & 0xff);
    delta_r[i] = ((255.0f - current_r[i]) / (float)FADE_STEPS);
    delta_g[i] = ((255.0f - current_g[i]) / (float)FADE_STEPS);
    delta_b[i] = ((255.0f - current_b[i]) / (float)FADE_STEPS);
  }

  for (int j = delta + FADE_STEPS + 1; j > delta + 1; j--) {
    unsigned int *p = fadePal[j];
    for (int i = 0; i < 16; i++) {
      current_r[i] += delta_r[i];
      current_g[i] += delta_g[i];
      current_b[i] += delta_b[i];

      int r = (int)current_r[i];
      int g = (int)current_g[i];
      int b = (int)current_b[i];
      unsigned int rgb = (r << 16) | (g << 8) | (b);
      p[i] = rgb;
    }
  }
}

static unsigned int temp_pal[64];

void display_zoom_init(unsigned int *pal, unsigned int tornadoOptions) {
#ifdef FIVEBPL
  graphicsOptions go = {.screenMode = SCR_16_9_5BPL,
                        .flags = CHUNKY_BUFFERS | REUSE_PLANAR_BUFFERS |
                                 PALETTE_IN_COPPER,
                        .tornadoOptions = tornadoOptions,
                        .copperSize = 610, // Obtained from the logs.
                        .customPal = pal};
#else
  graphicsOptions go = {.screenMode = SCR_16_9_4BPL,
                        .flags = CHUNKY_BUFFERS | REUSE_PLANAR_BUFFERS |
                                 PALETTE_IN_COPPER,
                        .tornadoOptions = tornadoOptions,
                        .copperSize = 610, // Obtained from the logs.
                        .customPal = pal};
#endif
  _graph = initGraphics(&go);

  _chunky = _graph->chunky;
  _planar[0] = _graph->planar1;
  _planar[1] = _graph->planar2;
  _copper = _graph->copper;
  _p = 0;

  _fb.w = _graph->w;
  _fb.h = _graph->h;

  pal64 = pal;

  // Generate fade out palettes...
  for (int i = 0; i < (FADE_TO_WHITE_STEPS + FADE_TO_BLACK_STEPS + 1); i++) {
    fadePal[i] = tndo_malloc(256 * sizeof(int), 0);
  }

  for (int i = 1; i <= FADE_TO_WHITE_STEPS; i++) {
    fade_pal_white(fadePal[i - 1], pal, 1.0f - (i / (float)FADE_TO_WHITE_STEPS),
                   16);
  }

#if 0
  for (int i = 1; i <= FADE_TO_BLACK_STEPS; i++) {
	  fade_pal_black(fadePal[FADE_TO_WHITE_STEPS + i -1], fadePal[FADE_TO_WHITE_STEPS -1], 1.0f - (i / (float) FADE_TO_BLACK_STEPS), 16);
  }
#endif

#if 0
  gen_fade(temp_pal, 1 * (FADE_STEPS + 1));

  for (int i = 0; i < 16; i++) {
    int r = i * 16;
    int g = i * 16 / 2;
    int b = i * 16 / 3;
    unsigned int rgb = (r << 16) | (g << 8) | (b);
    temp_pal[i] = rgb;
  }

  gen_fade(temp_pal, 2 * (FADE_STEPS + 1));

  for (int i = 0; i < 16; i++) {
    int r = i * 16 / 3;
    int g = i * 16;
    int b = i * 16 / 2;
    unsigned int rgb = (r << 16) | (g << 8) | (b);
    temp_pal[i] = rgb;
  }

  gen_fade(temp_pal, 3 * (FADE_STEPS + 1));
#endif

  fadeIndex = FADE_STEPS;

  // gen_noise_frames(NUM_NOISE_FRAMES);

  //_graph->copper1->replacePtr = _graph->copper1->bplPtr + (4 * 2);
  //_graph->copper2->replacePtr = _graph->copper2->bplPtr + (4 * 2);
  // insertBPLPTR(_copper[0], noise_frames[0], 6, _fb.w, _fb.h, 4, 1);
  // insertBPLPTR(_copper[1], noise_frames[1], 6, _fb.w, _fb.h, 4, 1);
}

t_canvas *display_zoom_get() {
  _fb.p.pix8 = _chunky;
  return &_fb;
}

void display_zoom_forefront() {
  clean_bpls();
  WRL(COP1LCH_ADDR, PBRG(_graph->copper->commands));
}

void display_zoom_flip() {
#ifdef FIVEBPL
  c2p1x1_5_c5_060(_chunky, _planar[_p], _fb.w * _fb.h);
#else
  c2p1x1_4_c5_16_9(_chunky, _planar[_p]);
#endif
  copper_switch_choose(_graph->switch_bpls, _p);
  _p ^= 1;
}

void display_zoom_set_copper() {
  //	copper_switch_choose(_graph->switch_bpls, _p);
}

void display_zoom_end() {}
