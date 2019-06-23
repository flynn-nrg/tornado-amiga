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

#ifdef __AMIGA__
#include <dos/dos.h>
#include <exec/exec.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "custom_regs.h"
#endif

#include "debug.h"
#include "memory.h"
#include "ptr_bridges.h"
#include "tornado_settings.h"

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

// Scratch pad memory.
static int scratchInit = 0;
static uint32_t scratchSize = 0;
static uint32_t scratchMax = 0;
static void *scratchPad = 0;

// Packed data buffer.
static void *packedDataBuffer = 0;
static void *packedDataBufferAligned = 0;
static uint32_t packedDataBufferSize = 0;
static uint32_t packedDataBufferMaxUsed = 0;

void *tndo_get_packed_data_buffer(uint32_t size) {
  tndo_assert(size <= packedDataBufferSize);
  if (size > packedDataBufferMaxUsed) {
    packedDataBufferMaxUsed = size;
  }
  return packedDataBufferAligned;
}

void init_chipmem_scratchpad(int size) {
#ifdef __AMIGA__
  tndo_assert(scratchInit == 0);
  scratchSize = size;
  scratchPad = tndo_malloc(size, TNDO_ALLOC_CHIP);
  tndo_assert(scratchPad);
  scratchInit = 1;
#endif
}

void *get_chipmem_scratchpad_addr(int required_size) {
#ifdef __AMIGA__
  tndo_assert(scratchInit);
  tndo_assert(required_size <= scratchSize);
#ifdef __DEBUG_CODE
  if (required_size > scratchMax) {
    scratchMax = required_size;
  }
#endif
  return scratchPad;
#else
  tndo_assert(0 && "Not for sdl/posix use");
  return 0;
#endif
}

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
  return 0;
#endif
}

int tndo_memory_init(uint32_t fastNeeded, uint32_t chipNeeded,
                     uint32_t chipScratchNeeded, uint32_t packedBufferNeeded) {

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

  packedDataBuffer = calloc(packedBufferNeeded, sizeof(uint8_t));
  packedDataBufferAligned = alignMem(packedDataBuffer, 4);
  packedDataBufferSize = packedBufferNeeded;
  if (!packedDataBuffer) {
    return TNDO_ENOMEM;
  }

  masterMemoryPool = (tndoMemoryPool *)calloc(1, sizeof(tndoMemoryPool));
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

  init_chipmem_scratchpad(chipScratchNeeded);
#endif

  return 0;
}

void tndo_memory_init_done() {
  if (packedDataBuffer) {
    free(packedDataBuffer);
  }

#ifdef __DEBUG_CODE
  printf("DEBUG - Maximum packed data buffer size: %u\n",
         packedDataBufferMaxUsed);
#endif

  initDone = 1;
}

uint32_t tndo_memory_used() { return totalFastAllocated; }

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
    printf("%u/%u bytes of scratch chip memory used.\n", scratchMax,
           scratchSize);
#endif
  }

#ifdef __AMIGA__
  for (int i = 0; i < numChipAllocs; i++) {
    FreeVec(chipAllocs[i]);
  }
#endif

  for (int i = 0; i < (int)numFastAllocs; i++) {
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

  if (PBRG(masterMemoryPool->reusableFastCurrent) -
          PBRG(masterMemoryPool->reusableFastOrig) >
      maxFastPoolUsage) {
    maxFastPoolUsage = PBRG(masterMemoryPool->reusableFastCurrent) -
                       PBRG(masterMemoryPool->reusableFastOrig);
  }

  masterMemoryPool->reusableChipCurrent = masterMemoryPool->reusableChipOrig;
  masterMemoryPool->reusableFastCurrent = masterMemoryPool->reusableFastOrig;
#endif
}

// Tornado memory allocator. The rules:
// Memory allocations are always 64bit aligned.
// You are allowed to allocate reusable memory during initialization.
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
        tndo_assert(0 && "CHIP memory pool is not big enough.");
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
        tndo_fprintf(stderr, "DEBUG - Fast memory pool deficit: %u\n",
                     FastPoolDeficit);
        tndo_assert(0 && "FAST memory pool is not big enough.");
        return 0;
      }
#ifndef __AMIGA__
      // Allocate memory for real on POSIX target so that sanitizer can catch
      // overruns.
      ptr = malloc(size + align_pad);
#endif
      return alignMem(ptr, alignment);
    }

  } else {
    // Persistent memory cannot be allocated after init is done.
    if (initDone) {
      tndo_assert(0 &&
                  "Persistent memory cannot be allocated after init is done");
      return 0;
    }
    if (options & TNDO_ALLOC_CHIP) {
#ifdef __AMIGA__
      void *ptr = allocChipmem(size + align_pad);
      if (!ptr) {
        fprintf(stderr,
                "FATAL - Failed to allocate %zu bytes of chipmem. Aborting.\n",
                size + align_pad);
        exit(0);
      }
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
      if (!ptr) {
        fprintf(stderr,
                "FATAL - Failed to allocate %zu bytes of fastmem. Aborting.\n",
                size + align_pad);
        exit(0);
      }
      fastAllocs[numFastAllocs] = ptr;
      numFastAllocs++;
      if (!(options & TNDO_PRIVATE_MEM)) {
        totalFastAllocated += size + align_pad;
      }
      return alignMem(ptr, alignment);
    }
  }
}

void *tndo_malloc_ex(size_t size, uint32_t options) {
  return tndo_malloc_internal(size, options, 4);
}

void *tndo_malloc_align_ex(size_t size, uint32_t options,
                           size_t pow2_alignment) {
  if (pow2_alignment < 4)
    pow2_alignment = 4;
  return tndo_malloc_internal(size, options, pow2_alignment);
}
