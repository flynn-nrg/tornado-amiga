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

#include "lzcommon.h"

#ifndef LZSS_H
#define LZSS_H

#define LZSS_LITERAL 0
#define LZSS_WINDOW_255 1
#define LZSS_WINDOW_4095_4 2
#define LZSS_WINDOW_4095_12 3
#define LZSS_WINDOW_65535 4

#define LZSS_CODELEN_8_8 1
#define LZSS_CODELEN_12_4 2
#define LZSS_CODELEN_12_12 3
#define LZSS_CODELEN_16_16 4

typedef struct lzssCtx {
  uint32_t consumed;
  uint32_t usedBytes;
  uint32_t nibbleFlag;
} lzssCtx;

compressedData *lzss_compress(void *data, uint32_t size, uint32_t windowSize,
                              uint32_t verbose);
void lzss_uncompress(uint8_t *data, uint8_t *dst, uint32_t size);

#endif
