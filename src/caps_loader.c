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
#include <tndo_assert.h>

#include "bstream.h"
#include "caps_loader.h"
#include "de-encapsulator.h"
#include "memory.h"
#include "ptr_bridges.h"
#include "tndo.h"
#include "tndo_file.h"
#include "tornado_settings.h"

static capsData_t *capsData;
static unsigned int pal[NUM_COEFF_SETS];

static void swap4(unsigned int *data, int len) {
  int i;
  for (i = 0; i < len; i++)
    data[i] = ENDI4(data[i]);
}

static void swap2(unsigned short *data, int len) {
  int i;
  for (i = 0; i < len; i++)
    data[i] = ENDI(data[i]);
}

capsData_t *loadCaps(FILE *fd, uint32_t numSamples, uint32_t bitsPerSample,
                     uint32_t addBits, uint32_t capsModel, int tornadoOptions) {
  long size;
  int out_type;
  int encoded_len = 0;

  tndo_fseek(fd, 0, SEEK_END);
  size = tndo_ftell(fd);
  tndo_fseek(fd, sizeof(TndoHeader), SEEK_SET);
  size -= sizeof(TndoHeader);

  // Allocate capsData struct.
  capsData = (capsData_t *)tndo_malloc(sizeof(capsData_t), 0);

  // Load coefficient palette.
  size_t coeffs = tndo_fread(&pal[0], sizeof(unsigned int), NUM_COEFF_SETS, fd);
  tndo_assert(coeffs == (NUM_COEFF_SETS));

#ifndef __AMIGA__
  swap4(pal, NUM_COEFF_SETS);
#endif

  size -= NUM_COEFF_SETS * sizeof(unsigned int);

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf(" Encoded payload: %li\n", size);
  }

  switch (bitsPerSample) {
  case 8:
    out_type = DE_ENCAPSULE_8;
    break;
  case 16:
    out_type = DE_ENCAPSULE_16;
    break;
  default:
    printf("Unsupported bits per sample setting. Aborting.\n");
    abort();
  }

  // Temporary buffer.
  void *buffer = malloc(size);

  // Left channel.
  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf(" Loading sample data...\n");
  }

  tndo_fread(&encoded_len, 1, 4, fd);
  encoded_len = ENDI4(encoded_len);

  size_t buf_read = tndo_fread(buffer, encoded_len, 1, fd);
  tndo_assert(buf_read == 1);

#ifndef __AMIGA__
  swap2((unsigned short *)buffer, encoded_len / sizeof(short));
#endif

  capsData->left_buffer =
      (char *)tndo_malloc(numSamples * (bitsPerSample / 8), 0);

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf(" Unpacking sample data...\n");
  }

  de_encapsulate_channel((void *)capsData->left_buffer, out_type,
                         (unsigned short *)buffer, numSamples, addBits, pal);

  // Right channel.
  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf(" Loading sample data...\n");
  }

  tndo_fread(&encoded_len, 1, 4, fd);
  encoded_len = ENDI4(encoded_len);

  buf_read = tndo_fread(buffer, encoded_len, 1, fd);
  tndo_assert(buf_read == 1);

#ifndef __AMIGA__
  swap2((unsigned short *)buffer, encoded_len / sizeof(short));
#endif

  capsData->right_buffer =
      (char *)tndo_malloc(numSamples * (bitsPerSample / 8), 0);

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf(" Unpacking sample data...");
    fflush(stdout);
  }

  de_encapsulate_channel(capsData->right_buffer, out_type,
                         (unsigned short *)buffer, numSamples, addBits, pal);

  free(buffer);

  return capsData;
}
