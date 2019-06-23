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
#include <string.h>

#include "lzw.h"
#include "lzw_loader.h"
#include "memory.h"
#include "tndo_file.h"

// Load a compressed asset. You have to allocate enough memory for the
// uncompressed data or bad things will happen! You most likely do not want to
// call this directly and should let the asset manager take care of things for
// you.
int lzwLoadFile(FILE *source, unsigned char *dest, int fileSize) {
  void *compData = tndo_get_packed_data_buffer(fileSize);

  // Consume payload.
  int read = tndo_fread(compData, fileSize, 1, source);
  if (read != 1) {
    free(compData);
    return 1;
  }

  // Call the depack routine.
  lzw_uncompress((uint8_t *)compData, (uint8_t *)dest, fileSize);

  return 0;
}

// Unpack a compressed asset from src into dst.
// You must have enough memory allocated in dst to do this
// or bad things will happen.
int lzwUnpack(unsigned char *source, unsigned char *dest, int size) {
  // Call the depack routine.
  lzw_uncompress(source, (uint8_t *)dest, size);

  return 0;
}
