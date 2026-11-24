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

#include "debug.h"
#include "memory.h"
#include "ptr_bridges.h"
#include "tornado_settings.h"

// TORNADO_68K_SHARED_HEAP (POSIX + TORNADO_68K_EMU): all tndo_malloc() memory
// comes from the emulated 68k's arenas, so 68k code can use any tndo buffer
// directly. The reusable pools then behave as on the Amiga (a bump pool reset
// by tndo_free()), chip memory is available and limited to the chip arena,
// and ASan redzones separate the blocks. See manual/Emulated68k.md.
#ifdef TORNADO_68K_SHARED_HEAP
#include "tndo_m68k.h"

// Fast arena size on top of the reusable pool, in MB: persistent
// allocations, 68k modules and their data.
#ifndef TORNADO_68K_FAST_EXTRA_MB
#define TORNADO_68K_FAST_EXTRA_MB 256
#endif

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define SHARED_HEAP_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) && !defined(SHARED_HEAP_ASAN)
#define SHARED_HEAP_ASAN 1
#endif
#ifdef SHARED_HEAP_ASAN
#include <sanitizer/asan_interface.h>
#define POOL_POISON(p, n) ASAN_POISON_MEMORY_REGION((p), (n))
#define POOL_UNPOISON(p, n) ASAN_UNPOISON_MEMORY_REGION((p), (n))
#else
#define POOL_POISON(p, n) ((void)(p), (void)(n))
#define POOL_UNPOISON(p, n) ((void)(p), (void)(n))
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

// non-reusable memory cannot be allocated after init.
static uint32_t initDone = 0;

static tndoMemoryPool *masterMemoryPool = 0;
static uint32_t numFastAllocs = 0;
static uint32_t totalFastAllocated = 0;
static uint32_t maxFastPoolUsage = 0;
static uint32_t FastPoolDeficit = 0;

#ifndef TORNADO_68K_SHARED_HEAP
static void *fastAllocs[10240];
#endif

#if defined(__AMIGA__) || defined(TORNADO_68K_SHARED_HEAP)
static uint32_t numChipAllocs = 0;
static uint32_t totalChipAllocated = 0;
static uint32_t maxChipPoolUsage = 0;
static uint32_t ChipPoolDeficit = 0;
#endif
#ifdef __AMIGA__
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
#if defined(__AMIGA__) || defined(TORNADO_68K_SHARED_HEAP)
  tndo_assert(scratchInit == 0);
  scratchSize = size;
  scratchPad = tndo_malloc(size, TNDO_ALLOC_CHIP);
  tndo_assert(scratchPad);
  scratchInit = 1;
#endif
}

void *get_chipmem_scratchpad_addr(int required_size) {
#if defined(__AMIGA__) || defined(TORNADO_68K_SHARED_HEAP)
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

#ifdef TORNADO_68K_SHARED_HEAP
static uint32_t sharedFastPoolSize = 0;
static uint32_t sharedChipPoolSize = 0;

static void *shared_alloc(size_t size, int chip, int alignment) {
  void *ptr = tndo_m68k_alloc_aligned((uint32_t)size,
                                      chip ? TNDO_M68K_CHIP : TNDO_M68K_FAST,
                                      1u << alignment);
  if (!ptr) {
    fprintf(stderr,
            "FATAL - 68k %s arena exhausted allocating %d bytes (shared "
            "heap). Aborting.\n",
            chip ? "chip" : "fast", (int)size);
    exit(1);
  }
  return ptr;
}

// Bump allocation from a reusable pool, the same arithmetic as the Amiga
// path so that pool usage and deficits match. Only [ptr, ptr+size) becomes
// addressable: the padding between blocks stays poisoned.
static void *shared_pool_alloc(void **current, void *max, size_t size,
                               int alignment, uint32_t *deficit,
                               const char *what) {
  void *ptr = *current;
  *current = ptr_bridge_back(PBRG(*current) + size + (1 << alignment));
  if (PBRG(*current) > PBRG(max)) {
    *deficit += PBRG(*current) - PBRG(max);
    tndo_fprintf(stderr, "DEBUG - %s memory pool deficit: %u\n", what,
                 *deficit);
    tndo_assert(0 && "Reusable memory pool is not big enough.");
    return 0;
  }
  ptr = alignMem(ptr, alignment);
  POOL_UNPOISON(ptr, size);
  return ptr;
}
#endif

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

#ifdef TORNADO_68K_SHARED_HEAP
  if (!tndo_m68k_initialized()) {
    tndo_m68k_config cfg;
    tndo_m68k_default_config(&cfg);
    cfg.fast_size = fastNeeded + TORNADO_68K_FAST_EXTRA_MB * 1024u * 1024u;
    if (tndo_m68k_init(&cfg)) {
      return TNDO_ENOMEM_FAST;
    }
  }
  sharedFastPoolSize = fastNeeded;
  sharedChipPoolSize = chipNeeded;
  masterMemoryPool->reusableFastOrig = shared_alloc(fastNeeded, 0, 4);
  POOL_POISON(masterMemoryPool->reusableFastOrig, fastNeeded);
  masterMemoryPool->reusableChipOrig = shared_alloc(chipNeeded, 1, 4);
  POOL_POISON(masterMemoryPool->reusableChipOrig, chipNeeded);
  masterMemoryPool->reusableChipCurrent = masterMemoryPool->reusableChipOrig;
  masterMemoryPool->reusableChipMax =
      ptr_bridge_back(PBRG(masterMemoryPool->reusableChipOrig) + chipNeeded);
#else
  masterMemoryPool->reusableFastOrig = calloc(fastNeeded, sizeof(uint8_t));
#endif
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
#ifdef TORNADO_68K_SHARED_HEAP
  if (chipScratchNeeded) {
    init_chipmem_scratchpad(chipScratchNeeded);
  }
#endif

  return 0;
}

void tndo_memory_init_done() {
  if (packedDataBuffer) {
    free(packedDataBuffer);
  }

#ifdef __DEBUG_CODE
  printf("DEBUG - Maximum packed data buffer size: %d\n",
         (int)packedDataBufferMaxUsed);
#endif

  initDone = 1;
}

uint32_t tndo_memory_used() { return totalFastAllocated; }

void tndo_memory_shutdown(unsigned int tornadoOptions) {

  if (tornadoOptions & MEMORY_PROFILING) {
    printf("Memory profile:\n");
    printf("---------------\n");
    printf("%d fast memory allocations for a total of %d bytes.\n",
           (int)numFastAllocs, (int)totalFastAllocated);
    printf("%d bytes of fast memory pool used.\n", (int)maxFastPoolUsage);
#ifdef __AMIGA__
    printf("%d chip memory allocations for a total of %d bytes.\n",
           (int)numChipAllocs, (int)totalChipAllocated);
    printf("%d bytes of chip memory pool used.\n", (int)maxChipPoolUsage);
    printf("%d/%d bytes of scratch chip memory used.\n", (int)scratchMax,
           (int)scratchSize);
#endif
  }

#ifdef __AMIGA__
  for (int i = 0; i < numChipAllocs; i++) {
    FreeVec(chipAllocs[i]);
  }
#endif

#ifndef TORNADO_68K_SHARED_HEAP
  // With the shared heap everything lives in the 68k arenas, which are
  // released by tndo_m68k_shutdown().
  for (int i = 0; i < (int)numFastAllocs; i++) {
    free(fastAllocs[i]);
  }
#endif

#ifdef __AMIGA__
  FreeVec(masterMemoryPool->reusableChipOrig);
#endif
#ifndef TORNADO_68K_SHARED_HEAP
  free(masterMemoryPool->reusableFastOrig);
#endif
  free(masterMemoryPool);
}

void tndo_free() {
#if defined(__AMIGA__) || defined(TORNADO_68K_SHARED_HEAP)
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
#ifdef TORNADO_68K_SHARED_HEAP
  // Reusable memory is gone: using it after tndo_free() is an ASan error on
  // the host and an UNALLOCATED fault in 68k code.
  POOL_POISON(masterMemoryPool->reusableFastOrig, sharedFastPoolSize);
  POOL_POISON(masterMemoryPool->reusableChipOrig, sharedChipPoolSize);
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
#ifdef TORNADO_68K_SHARED_HEAP
      return shared_pool_alloc(&masterMemoryPool->reusableChipCurrent,
                               masterMemoryPool->reusableChipMax, size,
                               alignment, &ChipPoolDeficit, "CHIP");
#elif defined(__AMIGA__)
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
#ifdef TORNADO_68K_SHARED_HEAP
      return shared_pool_alloc(&masterMemoryPool->reusableFastCurrent,
                               masterMemoryPool->reusableFastMax, size,
                               alignment, &FastPoolDeficit, "FAST");
#else
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
#endif // TORNADO_68K_SHARED_HEAP
    }

  } else {
    // Persistent memory cannot be allocated after init is done.
    if (initDone) {
      tndo_assert(0 &&
                  "Persistent memory cannot be allocated after init is done");
      return 0;
    }
    if (options & TNDO_ALLOC_CHIP) {
#ifdef TORNADO_68K_SHARED_HEAP
      numChipAllocs++;
      totalChipAllocated += size + align_pad;
      return shared_alloc(size, 1, alignment);
#elif defined(__AMIGA__)
      void *ptr = allocChipmem(size + align_pad);
      if (!ptr) {
        fprintf(stderr,
                "FATAL - Failed to allocate %d bytes of chipmem. Aborting.\n",
                (int)(size + align_pad));
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
#ifdef TORNADO_68K_SHARED_HEAP
      numFastAllocs++;
      if (!(options & TNDO_PRIVATE_MEM)) {
        totalFastAllocated += size + align_pad;
      }
      return shared_alloc(size, 0, alignment);
#else
      void *ptr = calloc(1, size + align_pad);
      if (!ptr) {
        fprintf(stderr,
                "FATAL - Failed to allocate %d bytes of fastmem. Aborting.\n",
                (int)(size + align_pad));
        exit(0);
      }
      fastAllocs[numFastAllocs] = ptr;
      numFastAllocs++;
      if (!(options & TNDO_PRIVATE_MEM)) {
        totalFastAllocated += size + align_pad;
      }
      return alignMem(ptr, alignment);
#endif // TORNADO_68K_SHARED_HEAP
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

#ifdef __cplusplus
}
#endif
