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

#include <assert.h>

#include "bstream.h"
#include "de-encapsulator.h"

static void linear_rebuild(signed short *dst, signed short *residuals) {
  int i;
  register int c = dst[-1];
  register int d = 0;
  for (i = 0; i < BLOCK_LEN; i += 2) {
    d = residuals[i + 0];
    d = d + c;
    dst[i] = d;

    c = residuals[i + 1];
    c = c + d;
    dst[i + 1] = c;
  }
}

static void coeffpack_rebuild(signed short *dst, signed short *residuals,
                              unsigned int coeff) {
  int c0, c1, c2;

  unpack_coeff(coeff, &c0, &c1, &c2);
  int i;
  // Unrolled version
  register int a = dst[-3];
  register int b = dst[-2];
  register int c = dst[-1];
  register int d = 0;
  for (i = 0; i < BLOCK_LEN; i += 4) {
    d = residuals[i + 0];
    d += ((c * c0) + (b * c1) + (a * c2)) >> 3;
    dst[i] = d;

    a = residuals[i + 1];
    a += ((d * c0) + (c * c1) + (b * c2)) >> 3;
    dst[i + 1] = a;

    b = residuals[i + 2];
    b += ((a * c0) + (d * c1) + (c * c2)) >> 3;
    dst[i + 2] = b;

    c = residuals[i + 3];
    c += ((b * c0) + (a * c1) + (d * c2)) >> 3;
    dst[i + 3] = c;
  }
}

static void decode_block_residuals(signed short *dst, bstream_t *bs) {
  int j, k;
  int len_len = bstream_readu(bs, 4); // bitlen of subsegment len codes
  int block_base = 1 + bstream_readu(bs, 4);
  for (j = 0; j < BLOCK_LEN; j += BLOCK_SUBSEGMENT) {
    int nbits = block_base;
    if (len_len)
      nbits += bstream_readu(bs, len_len);
    for (k = 0; k < BLOCK_SUBSEGMENT; k++)
      *dst++ = bstream_reads(bs, nbits);
  }
}

static signed short _residuals[BLOCK_LEN + 3];

void de_encapsulate_channel(void *dst, int out_type, unsigned short *src,
                            int num_samples, int addbits, unsigned int *pal) {
  int i, j;
  bstream_t bs_low;
  bstream_init(&bs_low, src);

  signed char *dst8 = (signed char *)dst;
  signed short *dst16 = (signed short *)dst;
  signed short *_residuals_start = _residuals + 3;
  unsigned int p0 = 0, p1 = 0, p2 = 0; // rembenber last 3 samples
  for (i = 0; i < num_samples; i += BLOCK_LEN) {
    int idx = 0;
    int linear = bstream_readu(
        &bs_low, 1); // Does the block use a simple linear predictor?
    if (!linear) {
      // if (model == SMP_FIXED) idx = bstream_readu (&bs_low, 2);
      idx = bstream_readu(&bs_low, 7);
    }
    decode_block_residuals(_residuals_start, &bs_low);

    _residuals_start[-1] = p0, _residuals_start[-2] = p1,
    _residuals_start[-3] = p2;

    if (linear)
      linear_rebuild(_residuals_start, _residuals_start);
    else
      coeffpack_rebuild(_residuals_start, _residuals_start, pal[idx]);

    p0 = _residuals_start[BLOCK_LEN - 1], p1 = _residuals_start[BLOCK_LEN - 2],
    p2 = _residuals_start[BLOCK_LEN - 3];

    if (out_type == DE_ENCAPSULE_8) {
      for (j = 0; j < BLOCK_LEN; j++)
        dst8[i + j] = _residuals_start[j] - 128;
    } else if (out_type == DE_ENCAPSULE_16) {
      for (j = 0; j < BLOCK_LEN; j++)
        dst16[i + j] = (_residuals_start[j] << addbits) - 32768;
    } else {
      assert(0);
    }
  }
}
