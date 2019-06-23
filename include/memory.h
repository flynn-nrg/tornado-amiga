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

#ifndef MEMORY_H
#define MEMORY_H

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

#endif
