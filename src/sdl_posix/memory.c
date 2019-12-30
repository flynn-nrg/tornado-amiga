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
#include <tndo_assert.h>

#ifdef __AMIGA__
#include <dos/dos.h>
#include <exec/exec.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "custom_regs.h"
#endif

#include "memory.h"
#include "ptr_bridges.h"
#include "tornado_settings.h"

// static void * allocChipmem (int);
// static void * alignChipmem(void *);

// non-reusable memory cannot be allocated after init.
static uint32_t initDone = 0;

static tndoMemoryPool *masterMemoryPool = 0;
static uint32_t numFastAllocs = 0;
static uint32_t totalFastAllocated = 0;
static uint32_t maxFastPoolUsage = 0;
static uint32_t FastPoolDeficit = 0;

static void *fastAllocs[10240];

#ifdef __AMIGA__
static uint32_t numChipAllocs = 0;
static uint32_t totalChipAllocated = 0;
static uint32_t maxChipPoolUsage = 0;
static uint32_t ChipPoolDeficit = 0;

static void *chipAllocs[1024];

#endif

// using "long" to enable 64 bits compatibility
static void *alignMem(void *ptr, int alignment) {

  unsigned long mask = 0;
  mask -= 1; // 0xfff...ff
  mask <<= alignment;
  unsigned long m = PBRG(ptr);
  m = (m + ((1 << alignment) - 1)) & mask;
  return ptr_bridge_back(m);
}

static void *allocChipmem(int bytes) {
#ifdef __AMIGA__
  // Should be aligned to 8 bytes for correct bitplanes
  void *ptr = AllocVec(bytes, MEMF_CHIP | MEMF_CLEAR);
  return ptr;
#else
  tndo_assert(0 && "not an Amiga");
#endif
}

int tndo_memory_init(uint32_t fastNeeded, uint32_t chipNeeded) {

  numFastAllocs = 0;
  maxFastPoolUsage = 0;
  FastPoolDeficit = 0;
  totalFastAllocated = 0;

#ifdef __AMIGA__
  numChipAllocs = 0;
  maxChipPoolUsage = 0;
  ChipPoolDeficit = 0;
  totalChipAllocated = 0;
#endif

  masterMemoryPool = calloc(1, sizeof(tndoMemoryPool));
  if (!masterMemoryPool) {
    return TNDO_ENOMEM;
  }

  masterMemoryPool->reusableFastOrig = calloc(fastNeeded, sizeof(uint8_t));
  if (!masterMemoryPool->reusableFastOrig) {
    return TNDO_ENOMEM_FAST;
  }
  masterMemoryPool->reusableFastCurrent = masterMemoryPool->reusableFastOrig;
  masterMemoryPool->reusableFastMax =
      ptr_bridge_back(PBRG(masterMemoryPool->reusableFastOrig) + fastNeeded);

#ifdef __AMIGA__
  masterMemoryPool->reusableChipOrig =
      AllocVec(chipNeeded, MEMF_CHIP | MEMF_CLEAR);
  if (!masterMemoryPool->reusableChipOrig) {
    return TNDO_ENOMEM_CHIP;
  }
  masterMemoryPool->reusableChipCurrent = masterMemoryPool->reusableChipOrig;
  masterMemoryPool->reusableChipMax =
      ptr_bridge_back(PBRG(masterMemoryPool->reusableChipOrig) + chipNeeded);
#endif

  return 0;
}

void tndo_memory_init_done() { initDone = 1; }

void tndo_memory_shutdown(unsigned int tornadoOptions) {

  if (tornadoOptions & MEMORY_PROFILING) {
    printf("Memory profile:\n");
    printf("---------------\n");
    printf("%u fast memory allocations for a total of %u bytes.\n",
           numFastAllocs, totalFastAllocated);
    printf("%u bytes of fast memory pool used.\n", maxFastPoolUsage);
#ifdef __AMIGA__
    printf("%u chip memory allocations for a total of %u bytes.\n",
           numChipAllocs, totalChipAllocated);
    printf("%u bytes of chip memory pool used.\n", maxChipPoolUsage);
#endif
  }

#ifdef __AMIGA__
  for (int i = 0; i < numChipAllocs; i++) {
    FreeVec(chipAllocs[i]);
  }
#endif

  for (int i = 0; i < numFastAllocs; i++) {
    free(fastAllocs[i]);
  }

#ifdef __AMIGA__
  FreeVec(masterMemoryPool->reusableChipOrig);
#endif
  free(masterMemoryPool->reusableFastOrig);
  free(masterMemoryPool);
}

void tndo_free() {
#ifdef __AMIGA__
  if (PBRG(masterMemoryPool->reusableChipCurrent) -
          PBRG(masterMemoryPool->reusableChipOrig) >
      maxChipPoolUsage) {
    maxChipPoolUsage = PBRG(masterMemoryPool->reusableChipCurrent) -
                       PBRG(masterMemoryPool->reusableChipOrig);
  }
#endif

  if (PBRG(masterMemoryPool->reusableFastCurrent) -
          PBRG(masterMemoryPool->reusableFastOrig) >
      maxFastPoolUsage) {
    maxFastPoolUsage = PBRG(masterMemoryPool->reusableFastCurrent) -
                       PBRG(masterMemoryPool->reusableFastOrig);
  }
#ifdef __AMIGA__
  masterMemoryPool->reusableChipCurrent = masterMemoryPool->reusableChipOrig;
#endif
  masterMemoryPool->reusableFastCurrent = masterMemoryPool->reusableFastOrig;
}

// Tornado memory allocator. The rules:
// Memory allocations are always 64bit aligned.
// You are not allowed to allocate reusable memory during initialization.
// You are not allowed to allocate persistent memory during runtime.
// Clean memory can only be requested for persistent memory. Reusable
// memory is only meant to be used as scratch memory.
static void *tndo_malloc_internal(size_t size, uint32_t options,
                                  int alignment) {
  // 4 bits alignment allows:
  // - consistent performance measuring, as the buffer is datacache line aligned
  // - correct address for bitplane data
  int align_pad = 1 << alignment;
  if (options & TNDO_REUSABLE_MEM) {
    // Do we want reusable chip or fast mem?
    if (options & TNDO_ALLOC_CHIP) {
#ifdef __AMIGA__
      void *ptr = masterMemoryPool->reusableChipCurrent;
      masterMemoryPool->reusableChipCurrent = ptr_bridge_back(
          PBRG(masterMemoryPool->reusableChipCurrent) + size + align_pad);
      // This memory pool is not big enough.
      if (PBRG(masterMemoryPool->reusableChipCurrent) >
          PBRG(masterMemoryPool->reusableChipMax)) {
        ChipPoolDeficit += PBRG(masterMemoryPool->reusableChipCurrent) -
                           PBRG(masterMemoryPool->reusableChipMax);
        return 0;
      }
      return alignMem(ptr, alignment);
#else
      tndo_assert(0 && "not an Amiga");
      return 0;
#endif

    } else {
      void *ptr = masterMemoryPool->reusableFastCurrent;
      masterMemoryPool->reusableFastCurrent = ptr_bridge_back(
          PBRG(masterMemoryPool->reusableFastCurrent) + size + align_pad);
      // This memory pool is not big enough.
      if (PBRG(masterMemoryPool->reusableFastCurrent) >
          PBRG(masterMemoryPool->reusableFastMax)) {
        FastPoolDeficit += PBRG(masterMemoryPool->reusableFastCurrent) -
                           PBRG(masterMemoryPool->reusableFastMax);
        return 0;
      }
      return alignMem(ptr, alignment);
    }

  } else {
    // Persistent memory cannot be allocated after init is done.
    if (initDone) {
      return 0;
    }
    if (options & TNDO_ALLOC_CHIP) {
#ifdef __AMIGA__
      void *ptr = allocChipmem(size + align_pad);
      if (!ptr)
        return 0;
      chipAllocs[numChipAllocs] = ptr;
      numChipAllocs++;
      totalChipAllocated += size + align_pad;
      return alignMem(ptr, alignment);
#else
      tndo_assert(0 && "not an Amiga");
      return 0;
#endif
    } else {
      void *ptr = calloc(1, size + align_pad);
      if (!ptr)
        return 0;
      fastAllocs[numFastAllocs] = ptr;
      numFastAllocs++;
      totalFastAllocated += size + align_pad;
      return alignMem(ptr, alignment);
    }
  }
}

void *tndo_malloc(size_t size, uint32_t options) {
  return tndo_malloc_internal(size, options, 4);
}

void *tndo_malloc_align(size_t size, uint32_t options, size_t pow2_alignment) {
  if (pow2_alignment < 4)
    pow2_alignment = 4;
  return tndo_malloc_internal(size, options, pow2_alignment);
}
