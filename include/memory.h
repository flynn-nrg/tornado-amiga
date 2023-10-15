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

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TNDO_REUSABLE_MEM 1
#define TNDO_ALLOC_CHIP 1 << 1

// Memory that does not count towards the total.
#define TNDO_PRIVATE_MEM 1 << 2

#define TNDO_ENOMEM 1
#define TNDO_ENOMEM_FAST 1 << 1
#define TNDO_ENOMEM_CHIP 1 << 2

#define TNDO_MAX_CHIP_ADDRESS 0x1FFFFF

typedef struct {
  void *reusableChipOrig;
  void *reusableChipMax;
  void *reusableChipCurrent;
  void *reusableFastOrig;
  void *reusableFastMax;
  void *reusableFastCurrent;
} tndoMemoryPool;

#ifdef TNDO_MEMORY_DEBUG
#define tndo_malloc(size, options)                                             \
  (printf("DEBUG - tndo_malloc: %s:%i. Size %i, options: %i\n", __FILE__,      \
          __LINE__, size, options),                                            \
   tndo_malloc_ex(size, options))
#define tndo_malloc_align(size, options, pow2_alignment)                       \
  (printf("DEBUG - tndo_malloc_align: %s:%i. Size %i, options: %i, "           \
          "alignment: %i\n",                                                   \
          __FILE__, __LINE__, size, options, pow2_alignment),                  \
   tndo_malloc_align_ex(size, options, pow2_alignment))
#else
#define tndo_malloc tndo_malloc_ex
#define tndo_malloc_align tndo_malloc_align_ex
#endif

void tndo_free(void);
void tndo_memory_init_done(void);
void *tndo_malloc_ex(size_t, uint32_t);
void *tndo_get_packed_data_buffer(uint32_t);
void *tndo_malloc_align_ex(size_t, uint32_t, size_t);
void tndo_memory_shutdown(unsigned int);
int tndo_memory_init(uint32_t, uint32_t, uint32_t, uint32_t);
void *get_chipmem_scratchpad_addr(int);
void init_chipmem_scratchpad(int);
uint32_t tndo_memory_used();

#ifdef __cplusplus
}
#endif

#endif
