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
