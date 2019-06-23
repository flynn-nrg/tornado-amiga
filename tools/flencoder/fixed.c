/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000-2009  Josh Coalson
 * Copyright (C) 2011-2016  Xiph.Org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fixed.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif

#ifdef min
#undef min
#endif
#define min(x, y) ((x) < (y) ? (x) : (y))

#ifdef local_abs
#undef local_abs
#endif
#define local_abs(x) ((x) < 0 ? -(x) : (x))

// The complex predictors only work on hires signal (12-16) bit

void fixed_compute_residual(const int *data, int data_len, int order,
                            int *residual) {
  int i;

  switch (order) {

  case 0:
    for (i = 0; i < data_len; i++) {
      int t = data[i] - data[i - 1];
      residual[i] = t;
    }
    break;
  case 1:
    for (i = 0; i < data_len; i++) {
      residual[i] = data[i] - (data[i - 1] << 1) + data[i - 2];
    }
    break;
  case 2:
    for (i = 0; i < data_len; i++) {
      residual[i] =
          data[i] -
          (((data[i - 1] - data[i - 2]) << 1) + (data[i - 1] - data[i - 2])) -
          data[i - 3];
    }
    break;
  default:
    assert(0);
  }
}

void fixed_restore_signal(const int *residual, int data_len, int order,
                          short *data) {
  int i;

  switch (order) {

  case 0:
    for (i = 0; i < data_len; i++) {
      data[i] = residual[i] + data[i - 1];
    }
    break;
  case 1:
    for (i = 0; i < data_len; i++) {
      data[i] = residual[i] + (data[i - 1] << 1) - data[i - 2];
    }
    break;
  case 2:
    for (i = 0; i < data_len; i++) {
      data[i] =
          residual[i] +
          (((data[i - 1] - data[i - 2]) << 1) + (data[i - 1] - data[i - 2])) +
          data[i - 3];
    }
    break;
  default:
    assert(0);
  }
}
