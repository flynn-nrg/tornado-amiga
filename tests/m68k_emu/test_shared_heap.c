// Tests for TORNADO_68K_SHARED_HEAP: src/memory.c allocating from the 68k
// arenas.
//
// Usage: test_shared_heap <dir with .bin files>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "tndo_m68k.h"

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define TEST_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) && !defined(TEST_ASAN)
#define TEST_ASAN 1
#endif
#ifdef TEST_ASAN
#include <sanitizer/asan_interface.h>
#define POISONED(p) __asan_address_is_poisoned(p)
#else
#define POISONED(p) 1
#endif

static int failures;
static int checks;

#define CHECK(cond, ...)                                                       \
  do {                                                                         \
    checks++;                                                                  \
    if (!(cond)) {                                                             \
      failures++;                                                              \
      printf("FAIL %s:%d: %s: ", __FILE__, __LINE__, #cond);                   \
      printf(__VA_ARGS__);                                                     \
      printf("\n");                                                            \
    }                                                                          \
  } while (0)

static int in_fast(const void *p) {
  uint32_t a = tndo_m68k_addr(p);
  return a >= TNDO_M68K_FAST_BASE;
}

static int in_chip(const void *p) {
  uint32_t a = tndo_m68k_addr(p);
  return a >= TNDO_M68K_CHIP_BASE && a < TNDO_M68K_CHIP_BASE + TNDO_M68K_CHIP_MAX;
}

static uint32_t load_bin(const char *dir, const char *name) {
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s.bin", dir, name);
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("can't open %s\n", path);
    exit(2);
  }
  static uint8_t buf[4096];
  size_t n = fread(buf, 1, sizeof(buf), f);
  fclose(f);
  return tndo_m68k_load_code(buf, (uint32_t)n, name);
}

// fill.s: fills d0 bytes at a0 with d1.b.
static tndo_m68k_status fill(uint32_t entry, void *dst, uint32_t n,
                             uint8_t v) {
  tndo_m68k_regs r;
  tndo_m68k_result res;
  tndo_m68k_regs_init(&r);
  r.a[0] = tndo_m68k_addr(dst);
  r.d[0] = n;
  r.d[1] = v;
  return tndo_m68k_call(entry, &r, NULL, 0, &res);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage: %s <bin dir>\n", argv[0]);
    return 2;
  }

  // tndo_memory_init() brings up the emulator with an arena sized for the
  // pools.
  CHECK(tndo_memory_init(1 << 20, 64 << 10, 4 << 10, 1 << 10) == 0,
        "tndo_memory_init");
  CHECK(tndo_m68k_initialized(), "emulator initialized by memory init");
  uint32_t fill_entry = load_bin(argv[1], "fill");

  // Persistent allocations.
  uint8_t *p = tndo_malloc(100, 0);
  CHECK(p && in_fast(p), "persistent fast in the fast arena");
  CHECK(POISONED(p + 100) && !POISONED(p + 99), "persistent redzone");
  uint8_t *c = tndo_malloc(64, TNDO_ALLOC_CHIP);
  CHECK(c && in_chip(c), "persistent chip in the chip arena");
  uint8_t *a = tndo_malloc_align(10, 0, 8);
  CHECK(a && (tndo_m68k_addr(a) & 255) == 0, "256 byte alignment: %08X",
        tndo_m68k_addr(a));
  uint8_t *scratch = get_chipmem_scratchpad_addr(1024);
  CHECK(scratch && in_chip(scratch), "chip scratchpad in the chip arena");
  tndo_memory_init_done();

  // 68k code works on tndo buffers directly.
  CHECK(fill(fill_entry, p, 100, 0xAB) == TNDO_M68K_OK, "fill persistent");
  CHECK(p[0] == 0xAB && p[99] == 0xAB, "68k wrote the tndo buffer");
  CHECK(fill(fill_entry, p, 101, 0) == TNDO_M68K_UNALLOCATED,
        "68k overrun of a tndo buffer");

  // Reusable memory: a bump pool, as on the Amiga.
  uint8_t *r1 = tndo_malloc(32, TNDO_REUSABLE_MEM);
  uint8_t *r2 = tndo_malloc(32, TNDO_REUSABLE_MEM);
  CHECK(r1 && in_fast(r1) && r2 > r1 + 32, "reusable blocks: %p %p",
        (void *)r1, (void *)r2);
  CHECK(POISONED(r1 + 32) && !POISONED(r1 + 31), "reusable redzone");
  uint8_t *rc = tndo_malloc(16, TNDO_REUSABLE_MEM | TNDO_ALLOC_CHIP);
  CHECK(rc && in_chip(rc), "reusable chip");
  CHECK(fill(fill_entry, r1, 32, 1) == TNDO_M68K_OK, "fill reusable");

  // After tndo_free() the pool is gone, for the host and for 68k code.
  tndo_free();
  CHECK(POISONED(r1), "reusable block poisoned after tndo_free");
  CHECK(fill(fill_entry, r1, 4, 2) == TNDO_M68K_UNALLOCATED,
        "68k use after tndo_free");
  uint8_t *r1b = tndo_malloc(32, TNDO_REUSABLE_MEM);
  CHECK(r1b == r1 && !POISONED(r1b), "pool restarts at the same block");
  CHECK(fill(fill_entry, r1b, 32, 3) == TNDO_M68K_OK, "fill reused block");

  // Options can change after tndo_memory_init() brought the emulator up.
  tndo_m68k_config cfg;
  tndo_m68k_get_config(&cfg);
  uint32_t fast_size = cfg.fast_size;
  CHECK(cfg.alloc_check == TNDO_M68K_ALLOC_CHECK_WRITES, "default alloc_check");
  uint32_t read_entry = load_bin(argv[1], "read_a0");
  tndo_m68k_regs r;
  tndo_m68k_result res;
  tndo_m68k_regs_init(&r);
  r.a[0] = tndo_m68k_addr(p) + 100;
  CHECK(tndo_m68k_call(read_entry, &r, NULL, 0, &res) == TNDO_M68K_OK,
        "read past a block is allowed by default");
  cfg.alloc_check = TNDO_M68K_ALLOC_CHECK_ALL;
  cfg.fast_size = 1234;
  tndo_m68k_set_options(&cfg);
  tndo_m68k_get_config(&cfg);
  CHECK(cfg.fast_size == fast_size, "sizes don't change: %u", cfg.fast_size);
  tndo_m68k_regs_init(&r);
  r.a[0] = tndo_m68k_addr(p) + 100;
  CHECK(tndo_m68k_call(read_entry, &r, NULL, 0, &res) ==
            TNDO_M68K_UNALLOCATED,
        "read past a block with ALLOC_CHECK_ALL");

  tndo_memory_shutdown(0);

  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
