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
#include <string.h>

#include "lzh.h"
#include "lzh_loader.h"
#include "memory.h"
#include "tndo_file.h"

// Load a compressed asset. You have to allocate enough memory for the
// uncompressed data or bad things will happen! You most likely do not want to
// call this directly and should let the asset manager take care of things for
// you.
int lzhLoadFile(FILE *source, unsigned char *dest, int fileSize) {
  void *compData = tndo_get_packed_data_buffer(fileSize);

  // Consume payload.
  int read = tndo_fread(compData, fileSize, 1, source);
  if (read != 1) {
    fprintf(stderr, "FATAL - Failed to read LZW compressed data. Aborting.\n");
    free(compData);
    return 1;
  }

  // Call the depack routine.
  lzh_uncompress((uint8_t *)compData, (uint8_t *)dest, fileSize);

  return 0;
}

// Unpack a compressed asset from src into dst.
// You must have enough memory allocated in dst to do this
// or bad things will happen.
int lzhUnpack(unsigned char *source, unsigned char *dest, int size) {
  // Call the depack routine.
  lzh_uncompress(source, (uint8_t *)dest, size);

  return 0;
}
