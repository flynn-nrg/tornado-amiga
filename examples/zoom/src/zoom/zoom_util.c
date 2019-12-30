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

#include "memory.h"
#include "zoom_util.h"

int *histogram(unsigned char *canvas, int size) {
  int *h = (int *)tndo_malloc(256 * sizeof(int), 0);
  int p;
  unsigned char c;

  for (int i = 0; i < size; i++) {
    c = canvas[i];
    p = (int)c;
    h[p]++;
  }

  return h;
}

int bucket(int *histogram, int size) {
  int b = 0;

  // Let's pretend 0 is 1.
  b += histogram[0];

  for (int i = 1; i < 256; i++) {
    b += histogram[i] * i;
  }

  return b / size;
}

unsigned char *downSample(unsigned char *src, int size) {
  unsigned char *dst = (unsigned char *)tndo_malloc(size * size, 0);
  int s2 = size / 2;
  for (int y = 0; y < size; y += 2) {
    for (int x = 0; x < size; x += 2) {
      dst[x / 2 + (y / 2) * s2] =
          (src[x + y * size] + src[x + 1 + y * size] + src[x + (y + 1) * size] +
           src[x + 1 + (y + 1) * size]) /
          4;
    }
  }

  return dst;
}

float clamp(float low, float high, float x) {
  if (x < 0) {
    return low;
  } else if (x > 1) {
    return high;
  }
  return x;
}

float smoothstep(float low, float high, float x) {
  x = clamp(0.0f, 1.0f, (x - low) / (high - low));
  return x * x * (3 - 2 * x);
}
