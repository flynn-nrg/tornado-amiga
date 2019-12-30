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

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <canvas.h>
#include <dprint.h>
#include <memory.h>
#include <tndo_assert.h>

#define NBITPLANES (2)

// ---------------------------------------------------------------------------

#define MAX_LEN (1024)

static const unsigned char g_pCharset[2048] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   24,  24,  24,  24,  24,  0,
    24,  0,   108, 108, 0,   0,   0,   0,   0,   0,   108, 108, 254, 108, 254,
    108, 108, 0,   24,  62,  96,  60,  6,   124, 24,  0,   0,   102, 172, 216,
    54,  106, 204, 0,   56,  108, 104, 118, 220, 206, 123, 0,   24,  24,  48,
    0,   0,   0,   0,   0,   12,  24,  48,  48,  48,  24,  12,  0,   48,  24,
    12,  12,  12,  24,  48,  0,   0,   102, 60,  255, 60,  102, 0,   0,   0,
    24,  24,  126, 24,  24,  0,   0,   0,   0,   0,   0,   0,   24,  24,  48,
    0,   0,   0,   126, 0,   0,   0,   0,   0,   0,   0,   0,   0,   24,  24,
    0,   3,   6,   12,  24,  48,  96,  192, 0,   60,  102, 110, 126, 118, 102,
    60,  0,   24,  56,  120, 24,  24,  24,  24,  0,   60,  102, 6,   12,  24,
    48,  126, 0,   60,  102, 6,   28,  6,   102, 60,  0,   28,  60,  108, 204,
    254, 12,  12,  0,   126, 96,  124, 6,   6,   102, 60,  0,   28,  48,  96,
    124, 102, 102, 60,  0,   126, 6,   6,   12,  24,  24,  24,  0,   60,  102,
    102, 60,  102, 102, 60,  0,   60,  102, 102, 62,  6,   12,  56,  0,   0,
    24,  24,  0,   0,   24,  24,  0,   0,   24,  24,  0,   0,   24,  24,  48,
    0,   6,   24,  96,  24,  6,   0,   0,   0,   0,   126, 0,   126, 0,   0,
    0,   0,   96,  24,  6,   24,  96,  0,   0,   60,  102, 6,   12,  24,  0,
    24,  0,   124, 198, 222, 214, 222, 192, 120, 0,   60,  102, 102, 126, 102,
    102, 102, 0,   124, 102, 102, 124, 102, 102, 124, 0,   30,  48,  96,  96,
    96,  48,  30,  0,   120, 108, 102, 102, 102, 108, 120, 0,   126, 96,  96,
    120, 96,  96,  126, 0,   126, 96,  96,  120, 96,  96,  96,  0,   60,  102,
    96,  110, 102, 102, 62,  0,   102, 102, 102, 126, 102, 102, 102, 0,   60,
    24,  24,  24,  24,  24,  60,  0,   6,   6,   6,   6,   6,   102, 60,  0,
    198, 204, 216, 240, 216, 204, 198, 0,   96,  96,  96,  96,  96,  96,  126,
    0,   198, 238, 254, 214, 198, 198, 198, 0,   198, 230, 246, 222, 206, 198,
    198, 0,   60,  102, 102, 102, 102, 102, 60,  0,   124, 102, 102, 124, 96,
    96,  96,  0,   120, 204, 204, 204, 204, 220, 126, 0,   124, 102, 102, 124,
    108, 102, 102, 0,   60,  102, 112, 60,  14,  102, 60,  0,   126, 24,  24,
    24,  24,  24,  24,  0,   102, 102, 102, 102, 102, 102, 60,  0,   102, 102,
    102, 102, 60,  60,  24,  0,   198, 198, 198, 214, 254, 238, 198, 0,   195,
    102, 60,  24,  60,  102, 195, 0,   195, 102, 60,  24,  24,  24,  24,  0,
    254, 12,  24,  48,  96,  192, 254, 0,   60,  48,  48,  48,  48,  48,  60,
    0,   192, 96,  48,  24,  12,  6,   3,   0,   60,  12,  12,  12,  12,  12,
    60,  0,   16,  56,  108, 198, 0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   254, 24,  24,  12,  0,   0,   0,   0,   0,   0,   0,   60,  6,
    62,  102, 62,  0,   96,  96,  124, 102, 102, 102, 124, 0,   0,   0,   60,
    96,  96,  96,  60,  0,   6,   6,   62,  102, 102, 102, 62,  0,   0,   0,
    60,  102, 126, 96,  60,  0,   28,  48,  124, 48,  48,  48,  48,  0,   0,
    0,   62,  102, 102, 62,  6,   60,  96,  96,  124, 102, 102, 102, 102, 0,
    24,  0,   24,  24,  24,  24,  12,  0,   12,  0,   12,  12,  12,  12,  12,
    120, 96,  96,  102, 108, 120, 108, 102, 0,   24,  24,  24,  24,  24,  24,
    12,  0,   0,   0,   236, 254, 214, 198, 198, 0,   0,   0,   124, 102, 102,
    102, 102, 0,   0,   0,   60,  102, 102, 102, 60,  0,   0,   0,   124, 102,
    102, 124, 96,  96,  0,   0,   62,  102, 102, 62,  6,   6,   0,   0,   124,
    102, 96,  96,  96,  0,   0,   0,   60,  96,  60,  6,   124, 0,   48,  48,
    124, 48,  48,  48,  28,  0,   0,   0,   102, 102, 102, 102, 62,  0,   0,
    0,   102, 102, 102, 60,  24,  0,   0,   0,   198, 198, 214, 254, 108, 0,
    0,   0,   198, 108, 56,  108, 198, 0,   0,   0,   102, 102, 102, 60,  24,
    48,  0,   0,   126, 12,  24,  48,  126, 0,   14,  24,  24,  112, 24,  24,
    14,  0,   24,  24,  24,  24,  24,  24,  24,  0,   112, 24,  24,  14,  24,
    24,  112, 0,   114, 156, 0,   0,   0,   0,   0,   0,   15,  60,  240, 195,
    15,  60,  240, 0,   255, 192, 192, 192, 192, 192, 192, 192, 255, 0,   0,
    0,   0,   0,   0,   0,   255, 3,   3,   3,   3,   3,   3,   3,   192, 192,
    192, 192, 192, 192, 192, 192, 3,   3,   3,   3,   3,   3,   3,   3,   192,
    192, 192, 192, 192, 192, 192, 255, 0,   0,   0,   0,   0,   0,   0,   255,
    3,   3,   3,   3,   3,   3,   3,   255, 0,   0,   0,   15,  24,  24,  24,
    24,  0,   0,   0,   255, 0,   0,   0,   0,   0,   0,   0,   240, 24,  24,
    24,  24,  24,  24,  24,  24,  24,  24,  24,  24,  24,  24,  24,  15,  0,
    0,   0,   0,   24,  24,  24,  240, 0,   0,   0,   0,   0,   0,   0,   255,
    24,  24,  24,  24,  24,  24,  24,  255, 0,   0,   0,   0,   24,  24,  24,
    31,  24,  24,  24,  24,  24,  24,  24,  248, 24,  24,  24,  24,  24,  24,
    24,  255, 24,  24,  24,  24,  255, 193, 195, 199, 193, 193, 192, 255, 255,
    131, 195, 227, 131, 131, 3,   255, 255, 192, 193, 193, 199, 195, 193, 255,
    255, 3,   131, 131, 227, 195, 131, 255, 255, 192, 204, 223, 223, 204, 192,
    255, 255, 3,   3,   243, 243, 3,   3,   255, 255, 192, 192, 207, 207, 192,
    192, 255, 255, 3,   99,  243, 243, 99,  3,   255, 255, 192, 195, 204, 204,
    195, 192, 255, 255, 3,   195, 51,  51,  195, 3,   255, 0,   127, 127, 96,
    111, 111, 108, 108, 26,  26,  26,  26,  26,  26,  26,  26,  0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   24,  0,
    24,  24,  24,  24,  24,  0,   0,   12,  62,  108, 62,  12,  0,   0,   28,
    54,  48,  120, 48,  48,  126, 0,   66,  60,  102, 60,  66,  0,   0,   0,
    195, 102, 60,  24,  60,  24,  24,  0,   24,  24,  24,  0,   24,  24,  24,
    0,   60,  96,  60,  102, 60,  6,   60,  0,   102, 102, 0,   0,   0,   0,
    0,   0,   126, 129, 157, 177, 157, 129, 126, 0,   28,  36,  68,  60,  0,
    126, 0,   0,   0,   51,  102, 204, 102, 51,  0,   0,   62,  6,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   126, 0,   0,   0,   0,   126, 129, 185,
    165, 185, 165, 129, 126, 126, 0,   0,   0,   0,   0,   0,   0,   60,  102,
    60,  0,   0,   0,   0,   0,   24,  24,  126, 24,  24,  0,   126, 0,   120,
    12,  24,  48,  124, 0,   0,   0,   120, 12,  24,  12,  120, 0,   0,   0,
    24,  48,  96,  0,   0,   0,   0,   0,   0,   0,   102, 102, 102, 102, 127,
    96,  62,  122, 122, 58,  10,  10,  10,  0,   0,   0,   24,  24,  0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   24,  48,  48,  112, 48,  48,  48,
    0,   0,   0,   56,  68,  68,  56,  0,   124, 0,   0,   0,   204, 102, 51,
    102, 204, 0,   0,   64,  198, 76,  88,  50,  102, 207, 2,   64,  198, 76,
    88,  62,  98,  196, 14,  192, 35,  102, 44,  217, 51,  103, 1,   24,  0,
    24,  48,  96,  102, 60,  0,   48,  24,  60,  102, 126, 102, 102, 0,   12,
    24,  60,  102, 126, 102, 102, 0,   24,  102, 60,  102, 126, 102, 102, 0,
    113, 142, 60,  102, 126, 102, 102, 0,   102, 0,   60,  102, 126, 102, 102,
    0,   24,  36,  60,  102, 126, 102, 102, 0,   31,  60,  60,  111, 124, 204,
    207, 0,   30,  48,  96,  96,  48,  30,  12,  24,  48,  24,  126, 96,  120,
    96,  126, 0,   12,  24,  126, 96,  120, 96,  126, 0,   24,  102, 126, 96,
    120, 96,  126, 0,   102, 0,   126, 96,  120, 96,  126, 0,   48,  24,  60,
    24,  24,  24,  60,  0,   12,  24,  60,  24,  24,  24,  60,  0,   24,  102,
    60,  24,  24,  24,  60,  0,   102, 0,   60,  24,  24,  24,  60,  0,   120,
    108, 102, 246, 102, 108, 120, 0,   113, 206, 230, 246, 222, 206, 198, 0,
    48,  24,  60,  102, 102, 102, 60,  0,   12,  24,  60,  102, 102, 102, 60,
    0,   24,  102, 60,  102, 102, 102, 60,  0,   113, 142, 60,  102, 102, 102,
    60,  0,   195, 60,  102, 102, 102, 102, 60,  0,   0,   198, 108, 56,  108,
    198, 0,   0,   63,  102, 110, 126, 118, 102, 252, 0,   48,  24,  102, 102,
    102, 102, 60,  0,   12,  24,  102, 102, 102, 102, 60,  0,   24,  36,  102,
    102, 102, 102, 60,  0,   102, 0,   102, 102, 102, 102, 60,  0,   6,   8,
    195, 102, 60,  24,  24,  0,   192, 192, 252, 198, 252, 192, 192, 0,   60,
    102, 102, 108, 102, 102, 108, 96,  48,  24,  60,  6,   62,  102, 62,  0,
    12,  24,  60,  6,   62,  102, 62,  0,   24,  102, 60,  6,   62,  102, 62,
    0,   113, 142, 60,  6,   62,  102, 62,  0,   102, 0,   60,  6,   62,  102,
    62,  0,   24,  36,  60,  6,   62,  102, 62,  0,   0,   0,   126, 27,  127,
    216, 119, 0,   0,   0,   60,  96,  96,  96,  60,  24,  48,  24,  60,  102,
    126, 96,  60,  0,   12,  24,  60,  102, 126, 96,  60,  0,   24,  102, 60,
    102, 126, 96,  60,  0,   102, 0,   60,  102, 126, 96,  60,  0,   48,  24,
    0,   24,  24,  24,  12,  0,   12,  24,  0,   24,  24,  24,  12,  0,   24,
    102, 0,   24,  24,  24,  12,  0,   0,   102, 0,   24,  24,  24,  12,  0,
    96,  252, 24,  60,  102, 102, 60,  0,   113, 142, 0,   124, 102, 102, 102,
    0,   48,  24,  0,   60,  102, 102, 60,  0,   12,  24,  0,   60,  102, 102,
    60,  0,   24,  102, 0,   60,  102, 102, 60,  0,
};

static char g_pszPrintBuffer[MAX_LEN];

static int g_wX = 0;
static int g_wY = 0;
static int g_wColor = 0xffffff;
static int g_wPrintMode = PRINTMODE_SHADOW;

// ---------------------------------------------------------------------------

void dprint_locate(int wX, int wY) {
  g_wX = wX;
  g_wY = wY;
}

// ---------------------------------------------------------------------------

void dprint_color(int wColor) { g_wColor = wColor; }

// ---------------------------------------------------------------------------

void dprint(t_canvas *s, const char *pszFormat, ...) {
  va_list arglist;
  va_start(arglist, pszFormat);
  vsprintf(g_pszPrintBuffer, pszFormat, arglist);
  va_end(arglist);

  g_pszPrintBuffer[MAX_LEN - 1] = 0;
  dprint_at(s, g_pszPrintBuffer, g_wX, g_wY, g_wColor);
}

// ---------------------------------------------------------------------------

void dprint_mode(int wMode) {
  if (wMode < PRINTMODE_INVALID)
    g_wPrintMode = wMode;
}

// ---------------------------------------------------------------------------

#define PIXEL(N) pChunk[N] = pChunk[N] >> 1;

static void SquarePlanar(unsigned char *pChunk, int wModulo, int wColor) {
  int t;
  for (t = 0; t < 8; t++) {
    pChunk[0] = 0;
    pChunk[4] = 0;
#if NBITPLANES == 3
    pChunk[12] = 0;
#endif
    pChunk += wModulo;
  }
}

static void Square8(unsigned char *pChunk, int wModulo, int wColor) {
  int t;
  for (t = 0; t < 8; t++) {
    PIXEL(0);
    PIXEL(1);
    PIXEL(2);
    PIXEL(3);
    PIXEL(4);
    PIXEL(5);
    PIXEL(6);
    PIXEL(7);
    pChunk += wModulo;
  }
}

static void Square16(unsigned short *pChunk, int wModulo, int wColor) {
  int t;
  for (t = 0; t < 8; t++) {
    pChunk[0] = pChunk[1] = pChunk[2] = pChunk[3] = wColor;
    pChunk[4] = pChunk[5] = pChunk[6] = pChunk[7] = wColor;
    pChunk += wModulo;
  }
}

static void Square32(unsigned int *pChunk, int wModulo, int wColor) {
  int t;
  for (t = 0; t < 8; t++) {
    pChunk[0] = pChunk[1] = pChunk[2] = pChunk[3] = wColor;
    pChunk[4] = pChunk[5] = pChunk[6] = pChunk[7] = wColor;
    pChunk += wModulo;
  }
}

// ---------------------------------------------------------------------------

#undef PIXEL
#define PIXEL(n)                                                               \
  if (nPix8 < 0)                                                               \
    pChunk[n] = wColor;                                                        \
  nPix8 = (int)(((unsigned int)nPix8) << 1);

static void DrawCharPlanar(unsigned char ubChar, unsigned char *pChunk,
                           int wModulo, int wColor) {
  const unsigned char *pCharBits = g_pCharset + ((int)ubChar) * 8;
  int t;
  for (t = 0; t < 8; t++) {
    int nPix8;
    nPix8 = (int)pCharBits[t];
    pChunk[0] = nPix8;
    pChunk[4] = nPix8;
#if NBITPLANES == 3
    pChunk[8] = nPix8;
#endif
    pChunk += wModulo;
  }
}

static void DrawChar8(unsigned char ubChar, unsigned char *pChunk, int wModulo,
                      int wColor) {
  const unsigned char *pCharBits = g_pCharset + ((int)ubChar) * 8;
  int t;
  for (t = 0; t < 8; t++) {
    int nPix8 = (int)(((unsigned int)pCharBits[t]) << 24);
    PIXEL(0);
    PIXEL(1);
    PIXEL(2);
    PIXEL(3);
    PIXEL(4);
    PIXEL(5);
    PIXEL(6);
    PIXEL(7);
    pChunk += wModulo;
  }
}

static void DrawChar16(unsigned char ubChar, unsigned short *pChunk,
                       int wModulo, int wColor) {
  const unsigned char *pCharBits = g_pCharset + ((int)ubChar) * 8;
  int t;
  for (t = 0; t < 8; t++) {
    int nPix8 = (int)(((unsigned int)pCharBits[t]) << 24);
    PIXEL(0);
    PIXEL(1);
    PIXEL(2);
    PIXEL(3);
    PIXEL(4);
    PIXEL(5);
    PIXEL(6);
    PIXEL(7);
    pChunk += wModulo;
  }
}

static void DrawChar32(unsigned char ubChar, unsigned int *pChunk, int wModulo,
                       int wColor) {
  const unsigned char *pCharBits = g_pCharset + ((int)ubChar) * 8;
  int t;
  for (t = 0; t < 8; t++) {
    int nPix8 = (int)(((unsigned int)pCharBits[t]) << 24);
    PIXEL(0);
    PIXEL(1);
    PIXEL(2);
    PIXEL(3);
    PIXEL(4);
    PIXEL(5);
    PIXEL(6);
    PIXEL(7);
    pChunk += wModulo;
  }
}

// ---------------------------------------------------------------------------

#define STR_MAX_LEN 1000

void dprint_at(t_canvas *s, const char *psCadena, int x, int y, int Color) {
  int wWidth, wHeight, wSize;
  g_wX = x;
  g_wY = y;

  wWidth = s->w;
  wHeight = s->h;
  wSize = s->bypp;
  if (g_wY < (wHeight / 8)) {
    int iXc = 0;
    while ((g_wX < (wWidth / 8)) && (psCadena[iXc] != 0) &&
           (iXc < STR_MAX_LEN)) {
      int sqx = g_wX * 8;
      int sqy = g_wY * 8;

      int poffs = (sqy * (wWidth >> 3)) * NBITPLANES;
      int subspan = (sqx >> 3) & 0x3;
#ifndef __AMIGA__
      subspan = (~subspan) & 0x3; // HACK! Fix
#endif
      poffs += ((sqx >> 5) * sizeof(int) * NBITPLANES) + subspan;
      int pmod = (wWidth >> 3) * NBITPLANES;
      if (g_wPrintMode == PRINTMODE_SHADOW) {
        int offs = (g_wX * 8 + 1) + (sqy + 1) * wWidth;
        if (wSize == 0)
          SquarePlanar(s->p.pix8 + poffs, pmod, 0);
        if (wSize == 1)
          Square8(s->p.pix8 + sqx + sqy * wWidth, wWidth, 0);
        if (wSize == 2)
          Square16(s->p.pix16 + sqx + sqy * wWidth, wWidth, 0);
        if (wSize == 4)
          Square32(s->p.pix32 + sqx + sqy * wWidth, wWidth, 0);
      }

      if (wSize == 0)
        DrawCharPlanar(psCadena[iXc], s->p.pix8 + poffs, pmod, Color);
      if (wSize == 1)
        DrawChar8(psCadena[iXc], s->p.pix8 + sqx + sqy * wWidth, wWidth, Color);
      if (wSize == 2)
        DrawChar16(psCadena[iXc], s->p.pix16 + sqx + sqy * wWidth, wWidth,
                   Color);
      if (wSize == 4)
        DrawChar32(psCadena[iXc], s->p.pix32 + sqx + sqy * wWidth, wWidth,
                   Color);

      if (psCadena[iXc] == '\n') {
        g_wX = 0, g_wY++;
      } else {
        g_wX++;
        if (g_wX >= (wWidth / 8)) {
          g_wX = 0, g_wY++;
        }
      }
      iXc++;
    }
  }
}

// ---------------------------------------------------------------------------
