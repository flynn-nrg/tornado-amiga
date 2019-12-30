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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tndo_assert.h>

#include "bm_load.h"
#include "canvas.h"
#include "memory.h"
#include "ptr_bridges.h"

#if 0
#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"
#endif

/*
static void byteswap32 (unsigned int* buff, int len)
{
    int i;
    for (i=0; i<len; i++)
        buff[i] = ENDI4(buff[i]);
}

static void byteswap16 (unsigned short* buff, int len)
{
    int i;
    for (i=0; i<len; i++)
        buff[i] = ENDI(buff[i]);
}
*/
typedef struct {
  unsigned char type, typeb;
  unsigned short w, h;
  unsigned short FILL;
  unsigned char data[1];
} t_pic_head;

static int get_pix_size(int type) {
  switch (type) {
  case BM_QUA10:
    return 2;
  case BM_GREY5:
    return 1;
  case BM_NORM16:
    return 2;
  case BM_RGB23:
    return 2;
  case BM_QUA8:
    return 1;
  case BM_GREY4:
    return 1;
  case BM_GREY8:
    return 1;
  case BM_GREY6:
    return 1;
  case BM_GREY_ALPHA:
    return 2;
  default:
    tndo_assert(0);
  }
  return 0;
}

void bm_load(t_canvas *c, void *data, void *preallocated,
             int preallocated_size) {
  int size, i;
  t_pic_head *bm = (t_pic_head *)data;
  tndo_assert(bm);

  int w = bm->w;
  int h = bm->h;
  int bypp = get_pix_size(bm->type);
  w = ENDI(w);
  h = ENDI(h);

  int npal = 0;
  unsigned char *pal = &bm->data[w * h * bypp];
  if (bm->type == BM_QUA8)
    npal = 256;
  if (bm->type == BM_QUA10)
    npal = 1024;

  if (preallocated) {
    tndo_assert((w * h * bypp) <= preallocated_size);
    canvas_create(c, w, h, bypp, preallocated);
  } else
    canvas_reserve(c, w, h, bypp, npal);

  switch (bm->type) {
  case BM_GREY5:
  case BM_GREY6:
  case BM_GREY8: {
    unsigned char curr = 0;
    for (i = 0; i < w * h; i++) {
      curr += bm->data[i];
      c->p.pix8[i] = curr;
    }
  } break;

  case BM_GREY4: {
    unsigned char curr = 0;
    for (i = 0; i < w * h / 2; i++) {
      unsigned char pair = bm->data[i];
      unsigned char high = pair >> 4;
      unsigned char low = pair & 0xf;
      high += curr;
      high &= 0xf;
      low += high;
      low &= 0xf;
      curr = low;
      c->p.pix8[(i << 1) + 0] = high;
      c->p.pix8[(i << 1) + 1] = low;
    }
  } break;

  case BM_RGB23: {
    unsigned char curr0 = 0;
    unsigned char curr1 = 0;
    unsigned char *chann0 = &bm->data[0];
    unsigned char *chann1 = chann0 + w * h;
    for (i = 0; i < w * h; i++) {
      curr0 += chann0[i];
      curr1 += chann1[i];

      c->p.pix16[i] = (curr0 << 8) | curr1;
    }
  } break;

  case BM_GREY_ALPHA: {
    unsigned char curr0 = 0;
    unsigned char curr1 = 0;
    unsigned char *chann0 = &bm->data[0];
    unsigned char *chann1 = chann0 + w * h;
    for (i = 0; i < w * h; i++) {
      curr0 += chann0[i];
      curr1 += chann1[i];

      c->p.pix8[(i << 1) + 0] = curr0;
      c->p.pix8[(i << 1) + 1] = curr1;
    }
  } break;

  case BM_QUA8: {
    for (i = 0; i < 256; i++) {
      c->pal[i] =
          (pal[i * 3 + 0] << 16) | (pal[i * 3 + 1] << 8) | pal[i * 3 + 2];
      // printf ("%d %d %d\n", pal[i * 3 + 0], pal[i * 3 + 1], pal[i * 3 + 2]);
    }
    for (i = 0; i < w * h; i++) {
      c->p.pix8[i] = bm->data[i];
    }
  } break;

  case BM_QUA10:
  case BM_NORM16:
  default:
    tndo_assert(0);
  }

#if 0
    // Test
    printf("Escritura...\n");
    int res = stbi_write_png("/tmp/foo.png", 256, 256, 1, c->p.pix8, 0);
    printf("%i\n", res);
#endif
}
