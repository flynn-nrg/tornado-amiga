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

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "lzh.h"
#include "lzss.h"
#include "lzw.h"

compressedData *lzh_compress(void *data, uint32_t size, uint32_t dictSize,
                             uint32_t codeLen, uint32_t optimumDict,
                             uint32_t verbose) {

  uint32_t step = 1;

  if (verbose) {
    printf("-------------------\n");
    printf("Step %u - LZSS pass.\n", step);
    printf("-------------------\n");
  }

  compressedData *cLzss = lzss_compress(data, size, 0, verbose);

  step++;

  if (verbose) {
    printf("-------------------\n");
    printf("Step %u - LZW pass.\n", step);
    printf("-------------------\n");
  }

  compressedData *c = lzw_compress(cLzss->data, cLzss->size, dictSize, codeLen,
                                   optimumDict, verbose);

  return c;
}
