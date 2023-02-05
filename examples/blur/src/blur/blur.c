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
#include <string.h>

#include <assets.h>
#include <canvas.h>
#include <demo.h>
#include <display.h>
#include <imgui_overlay.h>
#include <memory.h>
#include <ptr_bridges.h>
#include <tndo_assert.h>
#include <tornado_settings.h>

#include <debug.h>
#include <prof.h>
#include <telemetry.h>

#include "blur.h"
#include "bm_load.h"

// Rocket control
#include "sync.h"

// Rocket data traks
#include "blur_blur1.h"
#include "blur_blur2.h"
#include "blur_blur3.h"
#include "blur_inst1.h"
#include "blur_inst2.h"
#include "blur_inst3.h"
#include "blur_x1.h"
#include "blur_x2.h"
#include "blur_x3.h"
#include "blur_y1.h"
#include "blur_y2.h"
#include "blur_y3.h"

#define SX 320
#define SY 180
#define TM 4.0f

#define PI (3.14159265359f)

static unsigned int _pal_tmp[256];

static t_canvas background;

static unsigned int zero_line[2048];

#define CACHE_ENTRIES (3)

static unsigned int *brush_cache[CACHE_ENTRIES] = {0, 0, 0};
static int brush_cache_size = 0;

static TornadoAsset *assets = 0;

static int n_banners = 0;
static int displayInstance = 0;
static sprite_options so;

static int telemetryEnabled;
static TelemetryData **blurTelemetry;

// Rocket control...
static const struct sync_track *banner_inst1;
static const struct sync_track *banner_x1;
static const struct sync_track *banner_y1;
static const struct sync_track *banner_blur1;

static const struct sync_track *banner_inst2;
static const struct sync_track *banner_x2;
static const struct sync_track *banner_y2;
static const struct sync_track *banner_blur2;

static const struct sync_track *banner_inst3;
static const struct sync_track *banner_x3;
static const struct sync_track *banner_y3;
static const struct sync_track *banner_blur3;

static int current_b1;
static float current_b1_x;
static float current_b1_y;
static float current_b1_b;

static int current_b2;
static float current_b2_x;
static float current_b2_y;
static float current_b2_b;

static int current_b3;
static float current_b3_x;
static float current_b3_y;
static float current_b3_b;

// Sliders
static imguiRocketSlider sliders[12];

static uint8_t blur_pal[] = {
    "\000\000\000\000\000\000\001\001\001\001\001\001\002\002\003\002\002\003"
    "\003\003\004\003\003\004\003\004\005\004\005\006\004\005\006\005\006\010"
    "\005\006\011\005\006\011\006\007\012\007\010\014\007\010\014\010\011\015"
    "\010\012\016\010\012\016\011\013\017"
    "\012\014\021\012\014\021\012\014\022\013\015\023\014\017\025\014\017\025"
    "\015\020\026\015\020\027"
    "\016\021\030\016\021\030\017\022\032\017\023\033\020\024\034\021\025\036"
    "\022\026\037\022\026\037"
    "\022\026\040\023\027!\024\030#\024\031$\025\032%\026\033'\027\034("
    "\027\034)\030\035*\030\035*"
    "\031\037,\031\037-\032\040.\034\"\061\034\"\061\034#\062\035$\063\036%"
    "\066\036%\066\037&\067"
    "!(:!(:!);\"*=#+>#+?%.B&/C&/"
    "D'\060F(\061G(\062H)\063I*\064K+\065L+\065M,\066O-\067"
    "P.\071R/"
    ":T\060;U\060;V\061=X\062>Y\062>Z\063?[\064@]\065B_"
    "\066Ca\067Db\067Dc\071Ff\071"
    "Ff:Hh;Ij<Jk=Km=Km>Mp?Nq@OsAPtAPuCSxCSyDTzEU|FW~GX\177HY\201IZ\203IZ\203K"
    "]\206K]\207L^\210M_\212M_\213Ob\216Pc\217Pc\220Re\223Rf\224Sg\225Th\227U"
    "i\230Vj\232Wl\234Wl\235Yn\240Zo\241Zo\242[q\244]r\245_t\246`u\247av\247d"
    "y\251ey\251fz\252h|\253j~\254k\177\255m\200\256n\201\256o\202\257r\205\261"
    "s\205\261t\206\262v\210\263w\211\263z\214\265{\215\266|\215\266}\216\267"
    "\200\221\270\201\222\271\203\223\272\204\224\273\206\226\274\210\230\275"
    "\211\230\275\212\231\276\215\234\300\215\234\300\217\236\301\221\240\302"
    "\222\240\302\223\241\303\226\244\305\227\245\305\231\246\306\232\247\307"
    "\233\250\307\234\251\310\236\253\311\240\255\312\242\256\313\243\257\314"
    "\244\260\314\245\261\315\247\263\316\251\264\317\251\264\317\254\267\321"
    "\255\270\321\256\271\322\260\272\323\261\273\324\262\274\324\265\277\326"
    "\265\277\326\267\301\327\271\302\330\272\303\331\273\304\331\275\306\332"
    "\276\307\333\277\307\333\300\310\334\303\313\336\303\313\336\304\314\336"
    "\306\315\337\307\316\340\310\317\340\311\320\341\313\322\342\315\324\343"
    "\315\324\343\317\325\344\320\326\345\321\327\345\322\330\346\324\332\347"
    "\325\332\347\326\333\350\330\335\351\331\336\352\332\337\352\332\337\352"
    "\333\340\353\335\341\354\336\342\354\337\343\355\341\345\356\342\346\357"
    "\343\347\357\343\347\357\344\350\360\346\351\361\347\352\361\347\352\361"
    "\350\353\362\352\355\363\353\356\364\354\356\364\354\356\364\355\357\365"
    "\357\361\366\360\362\366\360\362\366\361\363\367\361\363\367\363\364\370"
    "\364\365\371\365\366\371\365\366\371\366\367\372\370\371\373\370\371\373"
    "\371\372\373\371\372\373\372\373\374\372\373\374\374\374\375\375\375\376"
    "\375\375\376\376\376\376\376\376\376",
};

typedef struct {
  char head[2];
  unsigned short w, h;
  unsigned short lines_indices[1];
} rle_t;

void tracksBlur(struct sync_device *rocket) {
#ifndef __AMIGA__
  banner_inst1 = sync_get_track(rocket, "blur.inst1");
  banner_x1 = sync_get_track(rocket, "blur.x1");
  banner_y1 = sync_get_track(rocket, "blur.y1");
  banner_blur1 = sync_get_track(rocket, "blur.blur1");

  banner_inst2 = sync_get_track(rocket, "blur.inst2");
  banner_x2 = sync_get_track(rocket, "blur.x2");
  banner_y2 = sync_get_track(rocket, "blur.y2");
  banner_blur2 = sync_get_track(rocket, "blur.blur2");

  banner_inst3 = sync_get_track(rocket, "blur.inst3");
  banner_x3 = sync_get_track(rocket, "blur.x3");
  banner_y3 = sync_get_track(rocket, "blur.y3");
  banner_blur3 = sync_get_track(rocket, "blur.blur3");

  snprintf(sliders[0].varName, 255, "%s", "blur.inst1");
  snprintf(sliders[1].varName, 255, "%s", "blur.x1");
  snprintf(sliders[2].varName, 255, "%s", "blur.y1");
  snprintf(sliders[3].varName, 255, "%s", "blur.blur1");

  snprintf(sliders[4].varName, 255, "%s", "blur.inst2");
  snprintf(sliders[5].varName, 255, "%s", "blur.x2");
  snprintf(sliders[6].varName, 255, "%s", "blur.y2");
  snprintf(sliders[7].varName, 255, "%s", "blur.blur2");

  snprintf(sliders[8].varName, 255, "%s", "blur.inst3");
  snprintf(sliders[9].varName, 255, "%s", "blur.x3");
  snprintf(sliders[10].varName, 255, "%s", "blur.y3");
  snprintf(sliders[11].varName, 255, "%s", "blur.blur3");

  sliders[0].attached = 1;
  sliders[0].min = 0.0f;
  sliders[0].max = 29.0f;

  sliders[1].attached = 1;
  sliders[1].min = -320.0f;
  sliders[1].max = 320.0f;

  sliders[2].attached = 1;
  sliders[2].min = -180.0f;
  sliders[2].max = 180.0f;

  sliders[3].attached = 1;
  sliders[3].min = 0.0f;
  sliders[3].max = 1.0f;

  sliders[4].attached = 1;
  sliders[4].min = 0.0f;
  sliders[4].max = 29.0f;

  sliders[5].attached = 1;
  sliders[5].min = -320.0f;
  sliders[5].max = 320.0f;

  sliders[6].attached = 1;
  sliders[6].min = -180.0f;
  sliders[6].max = 180.0f;

  sliders[7].attached = 1;
  sliders[7].min = 0.0f;
  sliders[7].max = 1.0f;

  sliders[8].attached = 1;
  sliders[8].min = 0.0f;
  sliders[8].max = 29.0f;

  sliders[9].attached = 1;
  sliders[9].min = -320.0f;
  sliders[9].max = 320.0f;

  sliders[10].attached = 1;
  sliders[10].min = -180.0f;
  sliders[10].max = 180.0f;

  sliders[11].attached = 1;
  sliders[11].min = 0.0f;
  sliders[11].max = 1.0f;

#endif
}

void trackDataBlur(int row, imguiOverlayData *overlayData) {
#ifndef __AMIGA__
  if (sliders[0].attached) {
    current_b1 = sync_get_val(banner_inst1, row);
  } else {
    current_b1 = sliders[0].current;
  }

  if (sliders[1].attached) {
    current_b1_x = sync_get_val(banner_x1, row);
  } else {
    current_b1_x = sliders[1].current;
  }

  if (sliders[2].attached) {
    current_b1_y = sync_get_val(banner_y1, row);
  } else {
    current_b1_y = sliders[2].current;
  }

  if (sliders[3].attached) {
    current_b1_b = sync_get_val(banner_blur1, row);
  } else {
    current_b1_b = sliders[3].current;
  }

  sliders[0].current = current_b1;
  sliders[1].current = current_b1_x;
  sliders[2].current = current_b1_y;
  sliders[3].current = current_b1_b;

  if (sliders[4].attached) {
    current_b2 = sync_get_val(banner_inst2, row);
  } else {
    current_b2 = sliders[4].current;
  }

  if (sliders[5].attached) {
    current_b2_x = sync_get_val(banner_x2, row);
  } else {
    current_b2_x = sliders[5].current;
  }

  if (sliders[6].attached) {
    current_b2_y = sync_get_val(banner_y2, row);
  } else {
    current_b2_y = sliders[6].current;
  }

  if (sliders[7].attached) {
    current_b2_b = sync_get_val(banner_blur2, row);
  } else {
    current_b2_b = sliders[7].current;
  }

  sliders[4].current = current_b2;
  sliders[5].current = current_b2_x;
  sliders[6].current = current_b2_y;
  sliders[7].current = current_b2_b;

  if (sliders[8].attached) {
    current_b3 = sync_get_val(banner_inst3, row);
  } else {
    current_b3 = sliders[8].current;
  }

  if (sliders[9].attached) {
    current_b3_x = sync_get_val(banner_x3, row);
  } else {
    current_b3_x = sliders[9].current;
  }

  if (sliders[10].attached) {
    current_b3_y = sync_get_val(banner_y3, row);
  } else {
    current_b3_y = sliders[10].current;
  }

  if (sliders[11].attached) {
    current_b3_b = sync_get_val(banner_blur3, row);
  } else {
    current_b3_b = sliders[11].current;
  }

  sliders[8].current = current_b3;
  sliders[9].current = current_b3_x;
  sliders[10].current = current_b3_y;
  sliders[11].current = current_b3_b;

  overlayData->sliderNum = 12;
  overlayData->sliders = sliders;

#endif
}

static void makeTexture_rle(unsigned int *pixels, const rle_t *rle, int w,
                            int h) {
  int y;
  const unsigned short *lines = &rle->lines_indices[0];
  const unsigned short *cmds = lines + h;
  for (y = 0; y < h; y++) {
    unsigned int *line0 = &pixels[y * w];
    unsigned int *linem1 = &pixels[(y - 1) * w];
    if (y == 0)
      linem1 = zero_line;
    int sx = 0;

    int rd = lines[y];
    rd = ENDI(rd);
    int x = 0;
    while (x < w) {
      int cmd = cmds[rd];
      cmd = ENDI(cmd);
      rd++;
      int bit = cmd >> 15;
      int len = cmd & 0x7fff;
      if (bit) {
        for (; len > 0; len--, x++) {
          int sy = linem1[x];
          line0[x] = sx + sy;
          sx++; // span at 1
        }
      } else {
        for (; len > 0; len--, x++) {
          int sy = linem1[x];
          line0[x] = sx + sy;
        }
      }
    }
    tndo_assert(x <= w);
  }
}

static void clearScreen(t_canvas *bg, int y_offs, unsigned char *chunky) {

  for (int y = 0; y < SY; y++) {
    unsigned char *src = bg->p.pix8 + y_offs * bg->w;
    memcpy(chunky, src, SX);
    y_offs++;
    chunky += SX;
  }
}

typedef struct blur_t {
  int xa;
  int ya;
  float aa;
} blur_t;

static void getBlur(const int xa, const int ya, blur_t *blur) {
  blur->xa = xa;
  blur->ya = ya;
  blur->aa = (xa + xa + 1) * (ya + ya + 1);
}

typedef struct bounds_t {
  int xmin, xmax;
  int ymin, ymax;
  // int *pixels;
  int tw, th;
  // int t;
  int m;
} bounds_t;

static short int ytab[2 * SY];
static short int xtab[2 * SX];

#define BOUND(x, l, h) ((x <= l) ? l : (x >= h) ? h : x)

static void fillTab(short int *tab, const float ic, const int ie,
                    const int tmax, const int tsmax, const int w) {
  for (int i = 0; i < ie; i++) {
    int ii = (tmax >> 1) - TM * (ic - i);
    int iii = ii;
    int i0 = iii - w - 1;
    int i1 = iii + w;
    i0 = BOUND(i0, 0, (tmax - 1));
    i1 = BOUND(i1, 0, (tmax - 1));
    tab[2 * i + 0] = i0;
    tab[2 * i + 1] = i1;
  }
}

// ---------------------------------------------------------------------------

static int cache_current = 0;
static int cache_banners[CACHE_ENTRIES] = {-1, -1, -1};

static int banner_cache_find(int banner) {
  int i;
  for (i = 0; i < CACHE_ENTRIES; i++)
    if (banner == cache_banners[i])
      return i;
  return -1;
}

static void banner_cache_update(int banner) {
  int i;

  // Already existing?
  int ce = banner_cache_find(banner);
  if (ce != -1)
    return;

  rle_t *rle = (rle_t *)assets[banner].Data;

  tndo_assert((rle->w * rle->h * (int)sizeof(int)) < brush_cache_size);
  makeTexture_rle(brush_cache[cache_current], rle, rle->w, rle->h);

  cache_banners[cache_current] = banner;
  // tndo_fprintf (stderr, "change!\n");

  cache_current++;
  if (cache_current == CACHE_ENTRIES)
    cache_current = 0;
}

// ---------------------------------------------------------------------------

static int prepareOne(const int banner, float blur_factor, float cx, float cy,
                      bounds_t *bounds) {
  blur_t blurMax;
  blur_t blur;

  rle_t *rle = (rle_t *)assets[banner].Data;

  banner_cache_update(banner);

  const float bmul = 12.0f * TM;
  const float bmax = -4.0f * 2.0f + bmul * 1.0f;
  const float bact = 2.0f + bmul * powf(blur_factor, 5.0f);
  getBlur(bmax, bmax, &blurMax);
  getBlur(bact, bact, &blur);

  const float mmMul = ((float)0xff8000) / blurMax.aa;
  const float mm = 0xff8000 - mmMul * blur.aa;
  bounds->m = ((mm < 0.0) ? 0.0 : mm) / blur.aa;

  bounds->tw = rle->w;
  bounds->th = rle->h;

  const int sxa = 1 + blur.xa / TM;
  const int sya = 1 + blur.ya / TM;

  const int sw = rle->w / (2 * TM);
  const int sh = rle->h / (2 * TM);

  const int xmin = cx - sw - sxa;
  const int xmax = 2 + cx + sw + sxa;
  const int ymin = cy - sh - sya;
  const int ymax = 1 + cy + sh + sya;

  bounds->xmin = BOUND(xmin, 0, (SX - 1));
  bounds->xmax = BOUND(xmax, 0, (SX - 1));
  bounds->ymin = BOUND(ymin, 0, (SY - 1));
  bounds->ymax = BOUND(ymax, 0, (SY - 1));

  // bounds->t = t;

  fillTab(ytab, cy, SY, rle->h, sh, blur.ya);
  fillTab(xtab, cx, SX, rle->w, sw, blur.xa);

  return (xmax - xmin) * (ymax - ymin);
}

static void blurRenderScanline(const int *const p0, const int *const p1,
                               const short int *xt, const int m, int num,
                               unsigned char *chunky) {
  do {
    int x0 = xt[0];
    int x1 = xt[1];
    int v0 = p0[x0];
    int v1 = p0[x1];
    int v2 = p1[x0];
    int v3 = p1[x1];
    int v = v3 - v2 + v0 - v1;
    *chunky++ = (m * v) >> 16;
    xt += 2;
  } while (--num);
}

static void blurRenderScanlineMix(const unsigned int *const p0,
                                  const unsigned int *const p1,
                                  const short int *xt, const int m, int num,
                                  unsigned char *chunky) {
  do {
    int x0 = xt[0];
    int x1 = xt[1];
    int v0 = p0[x0];
    int v1 = p0[x1];
    int v2 = p1[x0];
    int v3 = p1[x1];
    int v = v3 - v2 + v0 - v1;
    v = *chunky + ((m * v) >> 16);
    *chunky++ = (v < 255) ? v : 255;
    xt += 2;
  } while (--num);
}

static void fillOne(const bounds_t bounds, unsigned char *chunky, int banner) {
  int ce = banner_cache_find(banner);
  tndo_assert(ce != -1);
  unsigned int *brush = brush_cache[ce];

  for (int y = bounds.ymin; y <= bounds.ymax; y++) {
    int y0 = ytab[2 * y + 0];
    int y1 = ytab[2 * y + 1];
    const unsigned int *p0 = brush + y0 * bounds.tw; // bounds.pixels
    const unsigned int *p1 = brush + y1 * bounds.tw; // bounds.pixels
    const short int *xt = &xtab[2 * bounds.xmin];
    unsigned char *scr = chunky + y * SX;
    int num = 1 + bounds.xmax - bounds.xmin;

    // "Mix" variants allow mixing against the background.
    // #ifdef __AMIGA__
    //        //blurRenderScanlineAsm(p0, p1, xt, bounds.m, num, scr +
    //        bounds.xmin); blurRenderScanlineMixAsm(p0, p1, xt, bounds.m, num,
    //        scr + bounds.xmin);
    // #else
    // blurRenderScanline(p0, p1, xt, bounds.m, num, scr + bounds.xmin);
    blurRenderScanlineMix(p0, p1, xt, bounds.m, num, scr + bounds.xmin);
    // #endif
  }
}
/*
static int renderOne(const int t, const int frame, unsigned char *chunky) {
    bounds_t bounds;
    int loops = prepareOne(t, frame, &bounds);
    fillOne(bounds, chunky);
    return loops;
}
*/

static inline int back_height(int fb_h, int bc_h, int frame) {
  int range = bc_h - fb_h;
  int y = (range - 1) - (frame / 2);
  if (y < 0)
    y = 0;
  if (y >= range)
    y = range - 1;
  return y;
}

static void banner_func(float *px, float *py, float *pblur, int *pbanner,
                        float frame, float f0, float f1) {
  float ang = frame * f0;
  int banner = (int)(ang * (1.0f / PI));
  *pbanner = banner % n_banners;

  *pblur = 0.6f + 0.4f * sinf(frame * 0.15f);
  float semi = sinf(ang);
  float ry = 90.0f * cosf(ang);
  ry = (semi < 0.0f) ? -ry : ry;
  *px = (0.5f * (float)SX) + 10.0f * sinf(frame * f1);
  *py = (0.5f * (float)SY) + ry;
}

t_canvas *renderBlurCodeRocket(int row) {
  t_canvas *c = display_get(displayInstance);
  unsigned char *chunky = c->p.pix8;

  float f;
  prof_reset_chrono();

  clearScreen(&background, back_height(SY, background.h, row), chunky);

  f = prof_get_time("copy", background.w * background.h);
  if (telemetryEnabled) {
    appendTelemetry(f, blurTelemetry[0]);
  }

  int loops = 0;

  prof_reset_chrono();

  bounds_t bounds;

  int banner;
  float blur_factor, cx, cy;

  loops =
      prepareOne(current_b1, current_b1_b, current_b1_x, current_b1_y, &bounds);
  fillOne(bounds, chunky, current_b1);

  loops =
      prepareOne(current_b2, current_b2_b, current_b2_x, current_b2_y, &bounds);
  fillOne(bounds, chunky, current_b2);

  loops =
      prepareOne(current_b3, current_b3_b, current_b3_x, current_b3_y, &bounds);
  fillOne(bounds, chunky, current_b3);

  f = prof_get_time("summed_a1", loops);
  if (telemetryEnabled) {
    appendTelemetry(f, blurTelemetry[1]);
  }

  return c;
}

t_canvas *renderBlurCode(int frame) {
  t_canvas *c = display_get(displayInstance);
  unsigned char *chunky = c->p.pix8;

  float f;
  prof_reset_chrono();

  clearScreen(&background, back_height(SY, background.h, frame), chunky);

  f = prof_get_time("copy", background.w * background.h);
  if (telemetryEnabled) {
    appendTelemetry(f, blurTelemetry[0]);
  }

  int loops = 0;

  prof_reset_chrono();

  bounds_t bounds;

  int banner;
  float blur_factor, cx, cy;

  //    banner_func (&cx, &cy, &blur_factor, &banner,
  //             frame + 10.0f, 0.03f, 0.01f);
  banner = sync_blur_inst1_track[frame];
  cx = sync_blur_x1_track[frame];
  cy = sync_blur_y1_track[frame];
  blur_factor = sync_blur_blur1_track[frame];
  loops = prepareOne(banner, blur_factor, cx, cy, &bounds);
  fillOne(bounds, chunky, banner);

  // banner_func (&cx, &cy, &blur_factor, &banner,
  //             frame + 0.0f,  0.05f, 0.03f);
  banner = sync_blur_inst2_track[frame];
  cx = sync_blur_x2_track[frame];
  cy = sync_blur_y2_track[frame];
  blur_factor = sync_blur_blur2_track[frame];
  loops += prepareOne(banner, blur_factor, cx, cy, &bounds);
  fillOne(bounds, chunky, banner);

  // banner_func (&cx, &cy, &blur_factor, &banner,
  //              frame + 30.0f,  0.06f, 0.02f);
  banner = sync_blur_inst3_track[frame];
  cx = sync_blur_x3_track[frame];
  cy = sync_blur_y3_track[frame];
  blur_factor = sync_blur_blur3_track[frame];
  loops += prepareOne(banner, blur_factor, cx, cy, &bounds);
  fillOne(bounds, chunky, banner);

  f = prof_get_time("summed_a1", loops);
  if (telemetryEnabled) {
    appendTelemetry(f, blurTelemetry[1]);
  }

  return c;
}

void flipBlur(int requires_forefront) {
  if (requires_forefront) {
    display_forefront(displayInstance);
  }

  float f;
  prof_reset_chrono();
  display_flip(displayInstance);
  f = prof_get_time("c2p", SX * SY);
  if (telemetryEnabled) {
    appendTelemetry(f, blurTelemetry[2]);
  }
}

void vblBlur(int frame) {
#ifdef __AMIGA__
  display_set_copper(displayInstance);
#endif
}

static TornadoAsset rassetList[] = {
    {
        .Name = (uint8_t *)"data/blur-gr-1.rle", // Batman Group
    },
    {
        .Name = (uint8_t *)"data/blur-gr-2.rle", // Dekadence
    },
    {
        .Name = (uint8_t *)"data/blur-gr-3.rle", // Desire
    },
    {
        .Name = (uint8_t *)"data/blur-gr-4.rle", // Elude
    },
    {
        .Name = (uint8_t *)"data/blur-gr-5.rle", // Ephidrena
    },
    {
        .Name = (uint8_t *)"data/blur-gr-6.rle", // Fairlight
    },
    {
        .Name = (uint8_t *)"data/blur-gr-7.rle", // Focus Design
    },
    {
        .Name = (uint8_t *)"data/blur-gr-8.rle", // Haujobb
    },
    {
        .Name = (uint8_t *)"data/blur-gr-9.rle", // Lemon
    },
    {
        .Name = (uint8_t *)"data/blur-gr-10.rle", // Loonies
    },
    {
        .Name = (uint8_t *)"data/blur-gr-11.rle", // Nah Kolor
    },
    {
        .Name = (uint8_t *)"data/blur-gr-12.rle", // Nature
    },
    {
        .Name = (uint8_t *)"data/blur-gr-13.rle", // Oxyron
    },
    {
        .Name = (uint8_t *)"data/blur-gr-14.rle", // Scoopex
    },
    {
        .Name = (uint8_t *)"data/blur-gr-15.rle", // Skarla
    },
    {
        .Name = (uint8_t *)"data/blur-gr-16.rle", // Software Failure
    },
    {
        .Name = (uint8_t *)"data/blur-gr-17.rle", // Spaceballs
    },
    {
        .Name = (uint8_t *)"data/blur-gr-18.rle", // The Black Lotus
    },
    {
        .Name = (uint8_t *)"data/blur-gr-19.rle", // Tulou
    },
    {
        .Name = (uint8_t *)"data/blur-gr-20.rle", // Unique
    },
    {
        .Name = (uint8_t *)"data/blur-gr-21.rle", // RGBA
    },
    {
        .Name = (uint8_t *)"data/blur-gr-22.rle", // Ghostown
    },
    {
        .Name = (uint8_t *)"data/blur-gr-23.rle", // Onslaught
    },
    {
        .Name = (uint8_t *)"data/blur-gr-24.rle", // Paradox
    },
    {
        .Name = (uint8_t *)"data/blur-gr-25.rle", // Mystic Bytes
    },
    {
        .Name = (uint8_t *)"data/blur-gr-26.rle", // Dune
    },
    {
        .Name = (uint8_t *)"data/blur-gr-27.rle", // Sector One
    },
    {
        .Name = (uint8_t *)"data/blur-gr-28.rle", // New Beat
    },
    {
        .Name = (uint8_t *)"data/blur-gr-29.rle", // TSCC
    },
    {
        .Name = (uint8_t *)"data/blur-gr-30.rle", // IGN
    },
};

static TornadoAsset bassetList[] = {
    {
        .Name = (uint8_t *)"data/back_blur.tndo",
        .Flags = ASSETS_IN_REUSABLE_MEM,
    },
};

void initBlur(unsigned int tornadoOptions, tornadoEffect *effect) {

  static int init = 0;
  if (init)
    return;
  init = 1;

  int i;
  for (i = 0; i < 2048; i++)
    zero_line[i] = 0;

  n_banners = sizeof(rassetList) / sizeof(TornadoAsset);
  effect->numAssets = n_banners;
  effect->Assets = (TornadoAsset *)rassetList;
  assets = effect->Assets;
  if (!loadAssets(effect->Assets, effect->numAssets, tornadoOptions, 0)) {
    tndo_memory_shutdown(tornadoOptions);
    if (tornadoOptions & LOGGING) {
      printf("failed!\n");
    }
    exit(1);
  }

  // fix some endiannes issues
  for (int i = 0; i < effect->numAssets; i++) {
    rle_t *rle = (rle_t *)(effect->Assets[i].Data);
    tndo_assert(rle->head[0] == 'R' && rle->head[1] == 'L');
    rle->w = ENDI(rle->w);
    rle->h = ENDI(rle->h);
  }

  void *basset = 0;
  int sasset = 0;
  int loaded = loadAssets(bassetList, 1, tornadoOptions, 0);
  if (!loaded) {
    tndo_memory_shutdown(tornadoOptions);
    exit(1);
  }

  bm_load(&background, bassetList[0].Data, 0, 0);

  // Reserve brush caches
  brush_cache_size = 1024 * 1024;
  for (i = 0; i < CACHE_ENTRIES; i++)
    brush_cache[i] = (unsigned int *)tndo_malloc(brush_cache_size, 0);

  // Palette
  uint32_t k = 0;
  for (int i = 0; i < 256; i++) {
    uint32_t r = (uint32_t)blur_pal[k++];
    uint32_t g = (uint32_t)blur_pal[k++];
    uint32_t b = (uint32_t)blur_pal[k++];
    uint32_t rgb = (r << 16) | (g << 8) | (b);
    _pal_tmp[i] = rgb;
  }

  // Sprites
  so.num_sprites = 0;
  displayInstance = display_init(_pal_tmp, tornadoOptions, SCR_16_9, 0, 0, &so);
  // Telemetry
  telemetryEnabled = effect->wantTelemetry;

  if (telemetryEnabled) {
    blurTelemetry = effect->telemetry;
  }

  tndo_free();
}

#ifndef __AMIGA__
#define TELEMETRY_PATH "/tmp/blur_frame.csv"
#else
#define TELEMETRY_PATH "T:blur_frame.csv"
#endif

static const char *teleFooter =
    "set term png size 1280,960\nset datafile separator \",\"\nset xlabel "
    "\"frame\"\nset ylabel \"ms\"\nset output '/tmp/blur.png'\nplot \"$data\" "
    "using 1 with lines title \"copy\", \"$data\" using 2 with lines title "
    "\"sum\", \"$data\" using 3 with lines title \"c2p\", \"$data\" using "
    "($1+$2+$3) with lines title \"total\"\n";

void freeBlur(tornadoEffect *effect) {
  if (telemetryEnabled) {
    saveCombinedTelemetry(TELEMETRY_PATH, blurTelemetry, 3, teleFooter);
  }
}
