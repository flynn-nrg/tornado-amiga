/*
Copyright (c) 2026, Miguel Mendez. All rights reserved.

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

// Emulated 68k core for the POSIX host build. See include/tndo_m68k.h and
// manual/Emulated68k.md.
//
// How a call works:
//  - The CPU is reset, VBR points at our vector table and the CPU drops to
//    user mode with the user stack pointer at the top of the user stack.
//  - The return address pushed for the routine is TNDO_M68K_RETURN_SENTINEL,
//    in the trap page. All 256 vectors also point into the trap page, one
//    stub per vector.
//  - The instruction hook runs before every instruction. When the PC enters
//    the trap page the call is over: a return if it is the sentinel, an
//    exception otherwise. The hook also keeps a trace of recent PCs and
//    enforces the instruction budget.
//  - Every memory access goes through the m68k_read/write callbacks below,
//    which translate and bounds-check it. A bad access records a fault and
//    ends the timeslice. Once a fault is recorded, writes are dropped and
//    reads return 0, so the faulting instruction can't do more damage.

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m68k.h"
#include "m68k_emu_internal.h"
#include "tndo_m68k.h"

// m68k_emu_glue.c
void tndo_m68k_glue_saved_regs(unsigned int out[16]);

// AddressSanitizer: the fast and chip arenas are poisoned except for live
// allocations, so ASan catches host C overflows between 68k buffers. The
// emulator's own accessors are not instrumented (M68K_NO_ASAN): 68k accesses
// are checked against the shadow explicitly (cfg.alloc_check) and reported as
// 68k faults. The whole accessor chain carries the attribute because clang
// does not inline instrumented functions into non-instrumented ones.
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define M68K_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) && !defined(M68K_ASAN)
#define M68K_ASAN 1
#endif

#ifdef M68K_ASAN
#include <sanitizer/asan_interface.h>
#define M68K_NO_ASAN __attribute__((no_sanitize("address")))
#define POISON(p, n) ASAN_POISON_MEMORY_REGION((p), (n))
#define UNPOISON(p, n) ASAN_UNPOISON_MEMORY_REGION((p), (n))
#else
#define M68K_NO_ASAN
#define POISON(p, n) ((void)(p), (void)(n))
#define UNPOISON(p, n) ((void)(p), (void)(n))
#endif

#define NUM_VECTORS 256
#define TRAP_STUBS (TNDO_M68K_TRAP_BASE + 0x400) // Vector n -> TRAP_STUBS+4n.
#define SSTACK_SIZE 0x1000
#define NOP_OPCODE 0x4E71
#define MAX_CODE_RANGES 256
#define MAX_WARNINGS 32
#define STACK_OVERFLOW_ZONE 0x10000
#define EXECUTE_SLICE 200000
// Unallocated gap left after every tndo_m68k_alloc() block.
#define REDZONE 16
#define REDZONE_SCAN_LIMIT (1u << 20)

enum { R_FAST, R_CHIP, R_USTACK, R_SSTACK, R_VECTORS, R_COUNT };

typedef struct {
  uint32_t base;
  uint32_t size;
  uint8_t *mem;
  int writable;
} ram_region;

typedef struct {
  char *name;
  uint32_t addr;
  uint32_t size;
  int exported;
} m68k_symbol;

static struct {
  int initialized;
  int busy;
  tndo_m68k_config cfg;

  ram_region ram[R_COUNT];
  int last_region;
  uint32_t fast_top;
  uint32_t chip_top;

  struct {
    uint32_t start, end;
  } code[MAX_CODE_RANGES];
  int ncode;
  uint32_t code_lo, code_hi;

  m68k_symbol *syms;
  int nsyms;
  int syms_cap;

  uint32_t warned_pc[MAX_WARNINGS];
  int nwarned;

  // Per call state.
  tndo_m68k_result *res;
  int stop;
  int returned;
  int resetting;
  uint64_t icount;
  uint32_t trace[TNDO_M68K_TRACE_LEN];
} g;

static const char *status_names[] = {
    "OK",          "BUS_ERROR",       "NULL_ACCESS", "VECTOR_WRITE",
    "HW_ACCESS",   "CODE_WRITE",      "EXEC_FAULT",  "ODD_PC",
    "EXCEPTION",   "TIMEOUT",         "ABI_VIOLATION", "BAD_CALL",
    "UNRESOLVED",  "UNALLOCATED",
};

const char *tndo_m68k_status_name(tndo_m68k_status status) {
  if ((unsigned)status < sizeof(status_names) / sizeof(status_names[0])) {
    return status_names[status];
  }
  return "UNKNOWN";
}

static const char *vector_name(int v) {
  switch (v) {
  case 2:
    return "access fault (bus error)";
  case 3:
    return "address error";
  case 4:
    return "illegal instruction";
  case 5:
    return "integer divide by zero";
  case 6:
    return "CHK/CHK2 out of bounds";
  case 7:
    return "TRAPcc/TRAPV";
  case 8:
    return "privilege violation";
  case 9:
    return "trace";
  case 10:
    return "line-A emulator ($Axxx opcode)";
  case 11:
    return "line-F emulator ($Fxxx opcode / unimplemented FPU instruction)";
  case 13:
    return "coprocessor protocol violation";
  case 14:
    return "format error";
  case 15:
    return "uninitialized interrupt";
  case 24:
    return "spurious interrupt";
  }
  if (v >= 25 && v <= 31)
    return "autovector interrupt";
  if (v >= 32 && v <= 47)
    return "TRAP #n";
  if (v >= 48 && v <= 54)
    return "FPU exception";
  if (v >= 56 && v <= 58)
    return "MMU exception";
  return "exception";
}

// ---------------------------------------------------------------------------
// Address map helpers.
// ---------------------------------------------------------------------------

static inline M68K_NO_ASAN ram_region *find_ram(uint32_t addr,
                                               uint32_t size) {
  ram_region *r = &g.ram[g.last_region];
  uint32_t off = addr - r->base;
  if (off < r->size && size <= r->size - off) {
    return r;
  }
  for (int i = 0; i < R_COUNT; i++) {
    r = &g.ram[i];
    off = addr - r->base;
    if (off < r->size && size <= r->size - off) {
      g.last_region = i;
      return r;
    }
  }
  return NULL;
}

static int in_hw(uint32_t addr) {
  return (addr >= TNDO_M68K_CIA_BASE && addr < TNDO_M68K_CIA_END) ||
         (addr >= TNDO_M68K_CUSTOM_BASE && addr < TNDO_M68K_CUSTOM_END);
}

static const char *describe(uint32_t addr) {
  static const char *names[R_COUNT] = {"fast RAM", "chip RAM", "user stack",
                                       "supervisor stack", "vector table"};
  for (int i = 0; i < R_COUNT; i++) {
    ram_region *r = &g.ram[i];
    if (addr - r->base < r->size)
      return names[i];
  }
  if (addr < TNDO_M68K_VECTORS_BASE)
    return "null guard page";
  if (addr >= TNDO_M68K_CIA_BASE && addr < TNDO_M68K_CIA_END)
    return "CIA registers";
  if (addr >= TNDO_M68K_CUSTOM_BASE && addr < TNDO_M68K_CUSTOM_END)
    return "custom chip registers";
  if (addr >= TNDO_M68K_TRAP_BASE)
    return "trap page";
  return "unmapped memory";
}

static int in_code(uint32_t addr, uint32_t size) {
  if (addr >= g.code_hi || addr + size <= g.code_lo)
    return 0;
  for (int i = 0; i < g.ncode; i++) {
    if (addr < g.code[i].end && addr + size > g.code[i].start)
      return 1;
  }
  return 0;
}

// Spelled out instead of calling tndo_m68k_rd32() & co: those are
// instrumented, and would not be inlined here.
static inline M68K_NO_ASAN uint32_t load_be(const uint8_t *p, int size) {
  switch (size) {
  case 1:
    return p[0];
  case 2:
    return ((uint32_t)p[0] << 8) | p[1];
  default:
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
  }
}

static inline M68K_NO_ASAN void store_be(uint8_t *p, int size, uint32_t v) {
  switch (size) {
  case 1:
    p[0] = (uint8_t)v;
    break;
  case 2:
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
    break;
  default:
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
    break;
  }
}

static uint32_t last_pc(void) {
  if (g.icount == 0)
    return m68k_get_reg(NULL, M68K_REG_PC);
  return g.trace[(g.icount - 1) % TNDO_M68K_TRACE_LEN];
}

// ---------------------------------------------------------------------------
// Faults.
// ---------------------------------------------------------------------------

static void capture_regs(tndo_m68k_result *r, int mid_instruction) {
  if (mid_instruction) {
    // The instruction is half done: report the registers it started with.
    unsigned int saved[16];
    tndo_m68k_glue_saved_regs(saved);
    for (int i = 0; i < 8; i++) {
      r->regs.d[i] = saved[i];
      r->regs.a[i] = saved[8 + i];
    }
  } else {
    for (int i = 0; i < 8; i++) {
      r->regs.d[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + i));
      r->regs.a[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_A0 + i));
    }
  }
  r->sr = m68k_get_reg(NULL, M68K_REG_SR);
  if (r->sr & 0x2000) {
    // Supervisor mode (exception processing): show the user stack pointer.
    r->regs.a[7] = m68k_get_reg(NULL, M68K_REG_USP);
  }
}

static void fault(tndo_m68k_status status, int mid_instruction, uint32_t addr,
                  int size, int write, const char *fmt, ...) {
  if (g.stop)
    return;
  g.stop = 1;
  tndo_m68k_result *r = g.res;
  r->status = status;
  r->pc = last_pc();
  r->fault_addr = addr;
  r->fault_size = size;
  r->fault_write = write;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(r->message, sizeof(r->message), fmt, ap);
  va_end(ap);
  capture_regs(r, mid_instruction);
  m68k_end_timeslice();
}

static void warn_once(const char *fmt, ...) {
  uint32_t pc = last_pc();
  for (int i = 0; i < g.nwarned; i++) {
    if (g.warned_pc[i] == pc)
      return;
  }
  if (g.nwarned == MAX_WARNINGS)
    return;
  g.warned_pc[g.nwarned++] = pc;
  char where[96];
  tndo_m68k_symbolize(pc, where, sizeof(where));
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "68k: warning at $%08X %s: ", pc, where);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  if (g.nwarned == MAX_WARNINGS)
    fprintf(stderr, "68k: further warnings suppressed\n");
}

// Slow path for accesses that are not plain RAM.
static uint32_t access_fault(uint32_t addr, int size, int write,
                             uint32_t value) {
  const char *op = write ? "write" : "read";

  if (addr >= M68K_THUNK_BASE && addr < M68K_THUNK_END) {
    // An unresolved external points at its thunk (see m68k_emu_hunk.c).
    const char *name =
        m68k_hunk_import((int)((addr - M68K_THUNK_BASE) / 4), NULL, NULL);
    fault(TNDO_M68K_UNRESOLVED, 1, addr, size, write,
          "%s of unresolved external %s (define it in a module or bind it "
          "with tndo_m68k_bind_data)",
          op, name ? name : "?");
    return 0;
  }

  if (addr < TNDO_M68K_VECTORS_BASE) {
    if (!write && addr == 4 && size == 4) {
      fault(TNDO_M68K_NULL_ACCESS, 1, addr, size, write,
            "read of ExecBase ($4): AmigaOS is not available in the "
            "emulator");
    } else {
      fault(TNDO_M68K_NULL_ACCESS, 1, addr, size, write,
            "%s of %d byte(s) at $%08X: null pointer (page 0 is a guard "
            "page)",
            op, size, addr);
    }
    return 0;
  }

  if (in_hw(addr)) {
    const char *what = addr >= TNDO_M68K_CUSTOM_BASE ? "custom chip register"
                                                     : "CIA register";
    switch (g.cfg.hw_policy) {
    case TNDO_M68K_HW_IGNORE:
      warn_once("ignored %s of %s $%08X", op, what, addr);
      return 0;
    case TNDO_M68K_HW_HOOK:
      if (write) {
        if (g.cfg.hw_write)
          g.cfg.hw_write(addr, size, value, g.cfg.hw_user);
        return 0;
      }
      return g.cfg.hw_read ? g.cfg.hw_read(addr, size, g.cfg.hw_user) : 0;
    default:
      fault(TNDO_M68K_HW_ACCESS, 1, addr, size, write,
            "%s of %s $%08X: Amiga hardware is not emulated (see "
            "hw_policy)",
            op, what, addr);
      return 0;
    }
  }

  ram_region *r = find_ram(addr, 1);
  if (r == &g.ram[R_VECTORS] && write) {
    fault(TNDO_M68K_VECTOR_WRITE, 1, addr, size, write,
          "write to the vector table at $%08X", addr);
    return 0;
  }
  if (r) {
    fault(TNDO_M68K_BUS_ERROR, 1, addr, size, write,
          "%s of %d bytes at $%08X crosses the end of %s", op, size, addr,
          describe(addr));
    return 0;
  }

  uint32_t below = TNDO_M68K_USTACK_BASE - addr;
  if (below > 0 && below <= STACK_OVERFLOW_ZONE) {
    fault(TNDO_M68K_BUS_ERROR, 1, addr, size, write,
          "%s of %d byte(s) at $%08X: user stack overflow (stack_size is "
          "%u bytes)",
          op, size, addr, g.cfg.stack_size);
    return 0;
  }

  fault(TNDO_M68K_BUS_ERROR, 1, addr, size, write,
        "%s of %d byte(s) at $%08X: %s", op, size, addr, describe(addr));
  return 0;
}

// ---------------------------------------------------------------------------
// Musashi memory callbacks.
// ---------------------------------------------------------------------------

static inline M68K_NO_ASAN int is_arena(const ram_region *r) {
  return r == &g.ram[R_FAST] || r == &g.ram[R_CHIP];
}

// 1 if any byte of [p, p+size) is outside a live allocation.
static inline M68K_NO_ASAN int unallocated(const uint8_t *p, int size) {
#ifdef M68K_ASAN
  return __asan_region_is_poisoned((void *)p, size) != NULL;
#else
  (void)p;
  (void)size;
  return 0;
#endif
}

static int poisoned_at(const ram_region *r, uint32_t off) {
#ifdef M68K_ASAN
  return __asan_address_is_poisoned(r->mem + off);
#else
  (void)r;
  (void)off;
  return 0;
#endif
}

// An arena access outside any allocation. Finds the nearest allocation to
// describe the overrun ("4 bytes past the end of the 64 byte block at ...").
static void alloc_fault(const ram_region *r, uint32_t addr, int size,
                        int write) {
  const char *op = write ? "write" : "read";
  uint32_t off = addr - r->base;
  uint32_t bad = off;
  while (bad < off + size && !poisoned_at(r, bad))
    bad++;

  uint32_t limit = bad > REDZONE_SCAN_LIMIT ? bad - REDZONE_SCAN_LIMIT : 0;
  uint32_t end = bad;
  while (end > limit && poisoned_at(r, end - 1))
    end--;
  if (end > limit) {
    uint32_t start = end - 1;
    while (start > limit && !poisoned_at(r, start - 1))
      start--;
    char where[96];
    tndo_m68k_symbolize(r->base + start, where, sizeof(where));
    fault(TNDO_M68K_UNALLOCATED, 1, addr, size, write,
          "%s of %d byte(s) at $%08X: %u byte(s) past the end of the %u "
          "byte block at $%08X %s",
          op, size, addr, bad - end, end - start, r->base + start, where);
    return;
  }
  uint32_t next = bad;
  while (next < r->size && next - bad < REDZONE_SCAN_LIMIT &&
         poisoned_at(r, next))
    next++;
  if (next < r->size && next - bad < REDZONE_SCAN_LIMIT) {
    fault(TNDO_M68K_UNALLOCATED, 1, addr, size, write,
          "%s of %d byte(s) at $%08X: %u byte(s) before the block at $%08X",
          op, size, addr, next - bad, r->base + next);
    return;
  }
  fault(TNDO_M68K_UNALLOCATED, 1, addr, size, write,
        "%s of %d byte(s) at $%08X: %s that was never allocated", op, size,
        addr, describe(addr));
}

static inline M68K_NO_ASAN uint32_t mem_read(uint32_t addr, int size) {
  if (g.stop)
    return 0;
  ram_region *r = find_ram(addr, size);
  if (r) {
    uint8_t *p = r->mem + (addr - r->base);
    if (g.cfg.alloc_check == TNDO_M68K_ALLOC_CHECK_ALL && is_arena(r) &&
        unallocated(p, size)) {
      alloc_fault(r, addr, size, 0);
      return 0;
    }
    return load_be(p, size);
  }
  return access_fault(addr, size, 0, 0);
}

static inline M68K_NO_ASAN uint32_t mem_fetch(uint32_t addr, int size) {
  if (g.resetting) {
    // Reset reads the initial SSP and PC from address 0. The real values
    // are set by tndo_m68k_call() right after the reset.
    return addr == 0 ? TNDO_M68K_SSTACK_BASE + SSTACK_SIZE : 0;
  }
  if (g.stop) {
    // The call is over: feed NOPs until the timeslice ends.
    return size == 2 ? NOP_OPCODE : (NOP_OPCODE << 16) | NOP_OPCODE;
  }
  return mem_read(addr, size);
}

static inline M68K_NO_ASAN void mem_write(uint32_t addr, int size,
                                          uint32_t value) {
  if (g.stop)
    return;
  ram_region *r = find_ram(addr, size);
  if (r && r->writable) {
    if (g.cfg.alloc_check != TNDO_M68K_ALLOC_CHECK_OFF && is_arena(r) &&
        unallocated(r->mem + (addr - r->base), size)) {
      alloc_fault(r, addr, size, 1);
      return;
    }
    if (in_code(addr, size)) {
      if (!g.cfg.allow_smc) {
        fault(TNDO_M68K_CODE_WRITE, 1, addr, size, 1,
              "self-modifying code: write of %d byte(s) to code at $%08X. "
              "It breaks on the 040/060 caches (allow_smc for 68000 code)",
              size, addr);
        return;
      }
      warn_once("self-modifying code: write to $%08X (allow_smc). It "
                "breaks on a 040/060",
                addr);
    }
    store_be(r->mem + (addr - r->base), size, value);
    return;
  }
  access_fault(addr, size, 1, value);
}

unsigned int M68K_NO_ASAN m68k_read_memory_8(unsigned int a) { return mem_read(a, 1); }
unsigned int M68K_NO_ASAN m68k_read_memory_16(unsigned int a) { return mem_read(a, 2); }
unsigned int M68K_NO_ASAN m68k_read_memory_32(unsigned int a) { return mem_read(a, 4); }
unsigned int M68K_NO_ASAN m68k_read_immediate_16(unsigned int a) { return mem_fetch(a, 2); }
unsigned int M68K_NO_ASAN m68k_read_immediate_32(unsigned int a) { return mem_fetch(a, 4); }
unsigned int M68K_NO_ASAN m68k_read_pcrelative_8(unsigned int a) { return mem_read(a, 1); }
unsigned int M68K_NO_ASAN m68k_read_pcrelative_16(unsigned int a) { return mem_read(a, 2); }
unsigned int M68K_NO_ASAN m68k_read_pcrelative_32(unsigned int a) { return mem_read(a, 4); }
void M68K_NO_ASAN m68k_write_memory_8(unsigned int a, unsigned int v) { mem_write(a, 1, v); }
void M68K_NO_ASAN m68k_write_memory_16(unsigned int a, unsigned int v) {
  mem_write(a, 2, v);
}
void M68K_NO_ASAN m68k_write_memory_32(unsigned int a, unsigned int v) {
  mem_write(a, 4, v);
}

// The disassembler must never fault.
static M68K_NO_ASAN uint32_t peek(uint32_t addr, int size) {
  ram_region *r = find_ram(addr, size);
  return r ? load_be(r->mem + (addr - r->base), size) : 0;
}
unsigned int M68K_NO_ASAN m68k_read_disassembler_8(unsigned int a) { return peek(a, 1); }
unsigned int M68K_NO_ASAN m68k_read_disassembler_16(unsigned int a) { return peek(a, 2); }
unsigned int M68K_NO_ASAN m68k_read_disassembler_32(unsigned int a) { return peek(a, 4); }

// ---------------------------------------------------------------------------
// Instruction hook.
// ---------------------------------------------------------------------------

static void exception(int vector) {
  // The CPU is in supervisor mode and has pushed an exception frame on the
  // supervisor stack: SR.w, PC.l, format/vector.w.
  uint32_t isp = m68k_get_reg(NULL, M68K_REG_A7);
  uint32_t stacked_sr = peek(isp, 2);
  uint32_t stacked_pc = peek(isp + 2, 4);
  uint32_t pc = last_pc();
  if (vector >= 32 && vector <= 47) {
    fault(TNDO_M68K_EXCEPTION, 0, 0, 0, 0, "TRAP #%d at $%08X", vector - 32,
          pc);
  } else if (vector == 4 || vector == 10 || vector == 11) {
    fault(TNDO_M68K_EXCEPTION, 0, 0, 0, 0, "%s: opcode $%04X at $%08X",
          vector_name(vector), peek(pc, 2), pc);
  } else {
    fault(TNDO_M68K_EXCEPTION, 0, 0, 0, 0,
          "%s (vector %d) at $%08X, stacked PC $%08X", vector_name(vector),
          vector, pc, stacked_pc);
  }
  g.res->vector = vector;
  g.res->sr = stacked_sr;
}

static void trap_page(uint32_t pc) {
  if (pc == TNDO_M68K_RETURN_SENTINEL) {
    g.returned = 1;
    g.stop = 1;
    m68k_end_timeslice();
    return;
  }
  if (pc >= TRAP_STUBS && pc < TRAP_STUBS + NUM_VECTORS * 4 &&
      (pc & 3) == 0) {
    exception((int)((pc - TRAP_STUBS) / 4));
    return;
  }
  fault(TNDO_M68K_EXEC_FAULT, 0, pc, 2, 0, "jump into the trap page ($%08X)",
        pc);
}

// The PC reached the thunk of an external symbol: run the bound host
// function, then return to the 68k caller as rts would.
static void host_call(uint32_t pc) {
  int idx = (int)((pc - M68K_THUNK_BASE) / 4);
  tndo_m68k_host_fn fn = NULL;
  void *user = NULL;
  const char *name = m68k_hunk_import(idx, &fn, &user);
  if (!fn || (pc & 3)) {
    fault(TNDO_M68K_UNRESOLVED, 0, pc, 0, 0,
          "call to unresolved external %s (define it in a module or bind it "
          "with tndo_m68k_bind)",
          name ? name : "?");
    return;
  }

  tndo_m68k_regs regs;
  for (int i = 0; i < 8; i++) {
    regs.d[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + i));
    regs.a[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_A0 + i));
  }
  uint32_t sp = regs.a[7];
  fn(&regs, user);
  for (int i = 0; i < 8; i++)
    m68k_set_reg((m68k_register_t)(M68K_REG_D0 + i), regs.d[i]);
  for (int i = 0; i < 7; i++)
    m68k_set_reg((m68k_register_t)(M68K_REG_A0 + i), regs.a[i]);

  if (!find_ram(sp, 4)) {
    fault(TNDO_M68K_BUS_ERROR, 0, sp, 4, 0,
          "return from host function %s: bad stack pointer $%08X", name, sp);
    return;
  }
  uint32_t ret = peek(sp, 4);
  m68k_set_reg(M68K_REG_A7, sp + 4);
  m68k_set_reg(M68K_REG_PC, ret);
  // Musashi fetches from the new PC without calling the hook again: run it
  // here so the return address gets the usual checks (sentinel, odd PC...).
  tndo_m68k_instr_hook(ret);
}

void tndo_m68k_instr_hook(unsigned int pc) {
  if (g.stop)
    return;
  if (pc >= M68K_THUNK_BASE && pc < M68K_THUNK_END) {
    host_call(pc);
    return;
  }
  if (pc >= TNDO_M68K_TRAP_BASE) {
    trap_page(pc);
    return;
  }

  g.trace[g.icount % TNDO_M68K_TRACE_LEN] = pc;
  g.icount++;

  if (pc & 1) {
    fault(TNDO_M68K_ODD_PC, 0, pc, 2, 0,
          "instruction at odd address $%08X (address error on real "
          "hardware)",
          pc);
    return;
  }
  ram_region *r = find_ram(pc, 2);
  if (!r || r == &g.ram[R_VECTORS]) {
    fault(TNDO_M68K_EXEC_FAULT, 0, pc, 2, 0, "PC $%08X is in %s", pc,
          describe(pc));
    return;
  }
  if (g.cfg.strict_exec && !in_code(pc, 2)) {
    fault(TNDO_M68K_EXEC_FAULT, 0, pc, 2, 0,
          "PC $%08X is outside any code range (strict_exec)", pc);
    return;
  }
  if (g.cfg.instr_budget && g.icount > g.cfg.instr_budget) {
    fault(TNDO_M68K_TIMEOUT, 0, 0, 0, 0,
          "no return after %" PRIu64 " instructions (instr_budget): "
          "infinite loop?",
          g.cfg.instr_budget);
  }
}

// ---------------------------------------------------------------------------
// Lifecycle and memory.
// ---------------------------------------------------------------------------

void tndo_m68k_default_config(tndo_m68k_config *cfg) {
  memset(cfg, 0, sizeof(*cfg));
  cfg->fast_size = 16 * 1024 * 1024;
  cfg->chip_size = TNDO_M68K_CHIP_MAX;
  cfg->stack_size = 64 * 1024;
  cfg->instr_budget = 100 * 1000 * 1000;
  cfg->hw_policy = TNDO_M68K_HW_FAULT;
  cfg->allow_smc = 0;
  cfg->strict_exec = 0;
  cfg->abi_check = 1;
  cfg->alloc_check = TNDO_M68K_ALLOC_CHECK_WRITES;
}

static int setup_region(int idx, uint32_t base, uint32_t size, int writable) {
  ram_region *r = &g.ram[idx];
  r->base = base;
  r->size = size;
  r->writable = writable;
  r->mem = calloc(1, size);
  return r->mem ? 0 : -1;
}

int tndo_m68k_init(const tndo_m68k_config *cfg) {
  if (g.initialized)
    tndo_m68k_shutdown();

  memset(&g, 0, sizeof(g));
  if (cfg) {
    g.cfg = *cfg;
  } else {
    tndo_m68k_default_config(&g.cfg);
  }

  if (g.cfg.chip_size > TNDO_M68K_CHIP_MAX) {
    fprintf(stderr, "68k: chip_size $%X is larger than $%X\n",
            g.cfg.chip_size, TNDO_M68K_CHIP_MAX);
    return -1;
  }
  if (g.cfg.stack_size < 256 ||
      g.cfg.stack_size > TNDO_M68K_FAST_BASE - TNDO_M68K_USTACK_BASE) {
    fprintf(stderr, "68k: invalid stack_size %u\n", g.cfg.stack_size);
    return -1;
  }
  if (g.cfg.fast_size == 0 || g.cfg.fast_size > 0x70000000u) {
    fprintf(stderr, "68k: invalid fast_size %u\n", g.cfg.fast_size);
    return -1;
  }

  if (setup_region(R_FAST, TNDO_M68K_FAST_BASE, g.cfg.fast_size, 1) ||
      setup_region(R_CHIP, TNDO_M68K_CHIP_BASE, g.cfg.chip_size, 1) ||
      setup_region(R_USTACK, TNDO_M68K_USTACK_BASE, g.cfg.stack_size, 1) ||
      setup_region(R_SSTACK, TNDO_M68K_SSTACK_BASE, SSTACK_SIZE, 1) ||
      setup_region(R_VECTORS, TNDO_M68K_VECTORS_BASE, NUM_VECTORS * 4, 0)) {
    fprintf(stderr, "68k: out of memory\n");
    for (int i = 0; i < R_COUNT; i++)
      free(g.ram[i].mem);
    memset(&g, 0, sizeof(g));
    return -1;
  }

  for (int v = 0; v < NUM_VECTORS; v++) {
    tndo_m68k_wr32(g.ram[R_VECTORS].mem + v * 4, TRAP_STUBS + v * 4);
  }
  // Nothing in the arenas is allocated yet.
  POISON(g.ram[R_FAST].mem, g.ram[R_FAST].size);
  POISON(g.ram[R_CHIP].mem, g.ram[R_CHIP].size);
#ifndef M68K_ASAN
  if (g.cfg.alloc_check != TNDO_M68K_ALLOC_CHECK_OFF)
    fprintf(stderr, "68k: alloc_check needs an AddressSanitizer build, it is "
                    "disabled\n");
#endif

  g.code_lo = 0xFFFFFFFFu;
  g.code_hi = 0;

  m68k_init();
  m68k_set_cpu_type(M68K_CPU_TYPE_68040);
  g.initialized = 1;

  if (m68k_hunk_load_registered()) {
    tndo_m68k_shutdown();
    return -1;
  }
  return 0;
}

void m68k_emu_ensure_init(void) {
  if (g.initialized)
    return;
  if (tndo_m68k_init(NULL)) {
    fprintf(stderr, "68k: initialization failed\n");
    abort();
  }
}

int tndo_m68k_initialized(void) { return g.initialized; }

void tndo_m68k_get_config(tndo_m68k_config *cfg) {
  if (g.initialized) {
    *cfg = g.cfg;
  } else {
    tndo_m68k_default_config(cfg);
  }
}

void tndo_m68k_set_options(const tndo_m68k_config *cfg) {
  m68k_emu_ensure_init();
  tndo_m68k_config c = *cfg;
  // The address map is fixed once the emulator is up.
  c.fast_size = g.cfg.fast_size;
  c.chip_size = g.cfg.chip_size;
  c.stack_size = g.cfg.stack_size;
  g.cfg = c;
}

void tndo_m68k_shutdown(void) {
  for (int i = 0; i < R_COUNT; i++) {
    if (g.ram[i].mem)
      UNPOISON(g.ram[i].mem, g.ram[i].size);
    free(g.ram[i].mem);
  }
  for (int i = 0; i < g.nsyms; i++)
    free(g.syms[i].name);
  free(g.syms);
  m68k_hunk_reset();
  memset(&g, 0, sizeof(g));
}

void *tndo_m68k_alloc_aligned(uint32_t size, int where, uint32_t align) {
  m68k_emu_ensure_init();
  if (align < 16)
    align = 16;
  if (align & (align - 1))
    return NULL;
  ram_region *r = &g.ram[where == TNDO_M68K_CHIP ? R_CHIP : R_FAST];
  uint32_t *top = where == TNDO_M68K_CHIP ? &g.chip_top : &g.fast_top;
  uint64_t off = ((uint64_t)*top + align - 1) & ~(uint64_t)(align - 1);
  // A redzone follows every block: it stays poisoned, so overruns into the
  // next block are caught (by ASan on the host, by alloc_check in 68k code).
  if (off + size + REDZONE > r->size)
    return NULL;
  *top = (uint32_t)(off + size + REDZONE);
  uint8_t *p = r->mem + off;
  UNPOISON(p, size);
  memset(p, 0, size);
  return p;
}

void *tndo_m68k_alloc(uint32_t size, int where) {
  return tndo_m68k_alloc_aligned(size, where, 16);
}

uint32_t tndo_m68k_addr(const void *ptr) {
  const uint8_t *p = (const uint8_t *)ptr;
  for (int i = 0; i < R_COUNT; i++) {
    ram_region *r = &g.ram[i];
    if (r->mem && p >= r->mem && p < r->mem + r->size)
      return r->base + (uint32_t)(p - r->mem);
  }
  return 0;
}

void *tndo_m68k_host(uint32_t addr, uint32_t len) {
  if (!g.initialized)
    return NULL;
  ram_region *r = find_ram(addr, len ? len : 1);
  return r ? r->mem + (addr - r->base) : NULL;
}

// ---------------------------------------------------------------------------
// Code and symbols.
// ---------------------------------------------------------------------------

void tndo_m68k_add_code_range(uint32_t addr, uint32_t len) {
  if (g.ncode == MAX_CODE_RANGES || len == 0) {
    fprintf(stderr, "68k: can't add code range $%08X\n", addr);
    return;
  }
  g.code[g.ncode].start = addr;
  g.code[g.ncode].end = addr + len;
  g.ncode++;
  if (addr < g.code_lo)
    g.code_lo = addr;
  if (addr + len > g.code_hi)
    g.code_hi = addr + len;
}

void tndo_m68k_add_symbol(const char *name, uint32_t addr, uint32_t size) {
  m68k_emu_add_symbol(name, addr, size, 0);
}

void m68k_emu_add_symbol(const char *name, uint32_t addr, uint32_t size,
                         int exported) {
  if (g.nsyms == g.syms_cap) {
    int cap = g.syms_cap ? g.syms_cap * 2 : 64;
    m68k_symbol *s = realloc(g.syms, cap * sizeof(*s));
    if (!s)
      return;
    g.syms = s;
    g.syms_cap = cap;
  }
  size_t n = strlen(name) + 1;
  char *copy = malloc(n);
  if (!copy)
    return;
  memcpy(copy, name, n);
  g.syms[g.nsyms].name = copy;
  g.syms[g.nsyms].addr = addr;
  g.syms[g.nsyms].size = size;
  g.syms[g.nsyms].exported = exported;
  g.nsyms++;
}

uint32_t m68k_emu_find_export(const char *name, int *found) {
  for (int i = 0; i < g.nsyms; i++) {
    if (g.syms[i].exported && !strcmp(g.syms[i].name, name)) {
      *found = 1;
      return g.syms[i].addr;
    }
  }
  *found = 0;
  return 0;
}

uint32_t tndo_m68k_symbol(const char *name) {
  m68k_emu_ensure_init();
  int found;
  uint32_t addr = m68k_emu_find_export(name, &found);
  if (found)
    return addr;
  for (int i = 0; i < g.nsyms; i++) {
    if (!strcmp(g.syms[i].name, name))
      return g.syms[i].addr;
  }
  return 0;
}

uint32_t tndo_m68k_arg(const tndo_m68k_regs *regs, int n) {
  // regs->a[7] points at the return address, the arguments follow it.
  return peek(regs->a[7] + 4 + 4 * (uint32_t)n, 4);
}

void tndo_m68k_symbolize(uint32_t addr, char *buf, int buflen) {
  const m68k_symbol *best = NULL;
  for (int i = 0; i < g.nsyms; i++) {
    const m68k_symbol *s = &g.syms[i];
    if (s->addr > addr)
      continue;
    if (s->size && addr - s->addr >= s->size)
      continue;
    if (!best || s->addr > best->addr)
      best = s;
  }
  if (best) {
    snprintf(buf, buflen, "%s+$%X", best->name, addr - best->addr);
  } else if (addr == TNDO_M68K_RETURN_SENTINEL) {
    snprintf(buf, buflen, "<host caller>");
  } else {
    snprintf(buf, buflen, "<%s>", describe(addr));
  }
}

uint32_t tndo_m68k_load_code(const void *code, uint32_t len,
                             const char *name) {
  uint8_t *dst = tndo_m68k_alloc(len, TNDO_M68K_FAST);
  if (!dst)
    return 0;
  memcpy(dst, code, len);
  uint32_t addr = tndo_m68k_addr(dst);
  tndo_m68k_add_code_range(addr, len);
  if (name)
    tndo_m68k_add_symbol(name, addr, len);
  return addr;
}

// ---------------------------------------------------------------------------
// Calls.
// ---------------------------------------------------------------------------

void tndo_m68k_regs_init(tndo_m68k_regs *regs) {
  for (int i = 0; i < 8; i++) {
    regs->d[i] = TNDO_M68K_POISON;
    regs->a[i] = TNDO_M68K_POISON;
  }
}

static tndo_m68k_status bad_call(tndo_m68k_result *res, const char *msg) {
  res->status = TNDO_M68K_BAD_CALL;
  snprintf(res->message, sizeof(res->message), "%s", msg);
  return res->status;
}

static void copy_trace(tndo_m68k_result *res) {
  uint64_t n = g.icount < TNDO_M68K_TRACE_LEN ? g.icount : TNDO_M68K_TRACE_LEN;
  for (uint64_t i = 0; i < n; i++)
    res->trace[i] = g.trace[(g.icount - n + i) % TNDO_M68K_TRACE_LEN];
  res->trace_len = (int)n;
}

static void check_abi(tndo_m68k_result *res, const tndo_m68k_regs *in,
                      uint32_t entry_sp) {
  uint32_t clobbered = 0;
  for (int i = 2; i < 8; i++) {
    if (res->regs.d[i] != in->d[i])
      clobbered |= 1u << i;
  }
  for (int i = 2; i < 7; i++) {
    if (res->regs.a[i] != in->a[i])
      clobbered |= 1u << (8 + i);
  }
  uint32_t expected_sp = entry_sp + 4;
  if (!clobbered && res->regs.a[7] == expected_sp)
    return;

  char list[64] = "";
  int len = 0;
  for (int i = 0; i < 16; i++) {
    if (clobbered & (1u << i))
      len += snprintf(list + len, sizeof(list) - len, "%s%c%d",
                      len ? "," : "", i < 8 ? 'd' : 'a', i & 7);
  }
  res->status = TNDO_M68K_ABI_VIOLATION;
  res->abi_clobbered = clobbered;
  if (res->regs.a[7] != expected_sp) {
    snprintf(res->message, sizeof(res->message),
             "stack not balanced on return: sp=$%08X, expected $%08X "
             "(%+d bytes)%s%s",
             res->regs.a[7], expected_sp, (int)(res->regs.a[7] - expected_sp),
             clobbered ? "; clobbered: " : "", list);
  } else {
    snprintf(res->message, sizeof(res->message),
             "callee-saved register(s) not preserved: %s", list);
  }
}

tndo_m68k_status tndo_m68k_call(uint32_t entry, tndo_m68k_regs *io,
                                const uint32_t *stack_args, int nargs,
                                tndo_m68k_result *res) {
  tndo_m68k_result local;
  if (!res)
    res = &local;
  memset(res, 0, sizeof(*res));

  if (!g.initialized)
    return bad_call(res, "tndo_m68k_init() has not been called");
  if (g.busy)
    return bad_call(res, "reentrant call: the emulated CPU is already busy");
  if (entry == 0)
    return bad_call(res, "entry point is 0: symbol not found? (is the "
                         "module listed in DEMO_68K_OBJS?)");
  if ((entry & 1) || !find_ram(entry, 2) ||
      find_ram(entry, 2) == &g.ram[R_VECTORS])
    return bad_call(res, "entry point is not in 68k RAM");
  if (nargs < 0 || (uint32_t)(nargs + 1) * 4 > g.cfg.stack_size / 2)
    return bad_call(res, "too many stack arguments");

  tndo_m68k_regs in;
  if (io) {
    in = *io;
  } else {
    tndo_m68k_regs_init(&in);
  }

  // Stack: arguments (right to left), then the return address.
  ram_region *us = &g.ram[R_USTACK];
  uint32_t sp = us->base + us->size;
  for (int i = nargs - 1; i >= 0; i--) {
    sp -= 4;
    tndo_m68k_wr32(us->mem + (sp - us->base), stack_args[i]);
  }
  sp -= 4;
  tndo_m68k_wr32(us->mem + (sp - us->base), TNDO_M68K_RETURN_SENTINEL);
  uint32_t entry_sp = sp;

  g.busy = 1;
  g.res = res;
  g.stop = 0;
  g.returned = 0;
  g.icount = 0;

  g.resetting = 1;
  m68k_pulse_reset();
  g.resetting = 0;
  m68k_set_reg(M68K_REG_VBR, TNDO_M68K_VECTORS_BASE);
  // Musashi's M68K_REG_SR only changes the flags, it does not swap stack
  // pointers. So switch to user mode first: from then on M68K_REG_ISP writes
  // the banked supervisor stack pointer and M68K_REG_USP writes A7.
  m68k_set_reg(M68K_REG_SR, 0x0000); // User mode, interrupts enabled.
  m68k_set_reg(M68K_REG_ISP, TNDO_M68K_SSTACK_BASE + SSTACK_SIZE);
  m68k_set_reg(M68K_REG_USP, entry_sp);
  for (int i = 0; i < 8; i++)
    m68k_set_reg((m68k_register_t)(M68K_REG_D0 + i), in.d[i]);
  for (int i = 0; i < 7; i++)
    m68k_set_reg((m68k_register_t)(M68K_REG_A0 + i), in.a[i]);
  m68k_set_reg(M68K_REG_PC, entry);

  uint64_t cycles = 0;
  int idle = 0;
  while (!g.stop) {
    int c = m68k_execute(EXECUTE_SLICE);
    cycles += c > 0 ? (uint64_t)c : 0;
    if (c <= 0 && ++idle > 4) {
      fault(TNDO_M68K_BAD_CALL, 0, 0, 0, 0, "the CPU stopped executing");
    }
  }

  res->instructions = g.icount;
  res->cycles = cycles;
  copy_trace(res);

  if (g.returned) {
    res->status = TNDO_M68K_OK;
    res->pc = last_pc();
    capture_regs(res, 0);
    res->d0 = res->regs.d[0];
    if (g.cfg.abi_check)
      check_abi(res, &in, entry_sp);
    if (io)
      *io = res->regs;
  }

  g.res = NULL;
  g.busy = 0;
  return res->status;
}

uint32_t tndo_m68k_call_or_die(uint32_t entry, tndo_m68k_regs *io) {
  tndo_m68k_result res;
  if (tndo_m68k_call(entry, io, NULL, 0, &res) != TNDO_M68K_OK) {
    tndo_m68k_report(&res, stderr);
    fflush(stderr);
    abort();
  }
  return res.d0;
}

// ---------------------------------------------------------------------------
// Crash report.
// ---------------------------------------------------------------------------

static void print_insn(FILE *out, uint32_t pc, int current) {
  char where[96];
  char dis[128];
  tndo_m68k_symbolize(pc, where, sizeof(where));
  if (find_ram(pc, 2)) {
    m68k_disassemble(dis, pc, M68K_CPU_TYPE_68040);
  } else {
    snprintf(dis, sizeof(dis), "<not mapped>");
  }
  fprintf(out, " %s $%08X %-28s %s\n", current ? "->" : "  ", pc, where, dis);
}

void tndo_m68k_report(const tndo_m68k_result *res, FILE *out) {
  char where[96];
  fprintf(out, "\n*** 68k %s: %s\n",
          res->status == TNDO_M68K_OK ? "call" : "fault",
          tndo_m68k_status_name(res->status));
  if (res->message[0])
    fprintf(out, "    %s\n", res->message);
  if (res->status == TNDO_M68K_BAD_CALL) {
    fputc('\n', out);
    return;
  }

  tndo_m68k_symbolize(res->pc, where, sizeof(where));
  fprintf(out, "    PC $%08X %s, after %" PRIu64 " instructions\n", res->pc,
          where, res->instructions);

  if (res->trace_len) {
    fprintf(out, "\nLast instructions:\n");
    // The last entry is the faulting instruction. Before it, tight loops
    // (a block of 1 or 2 instructions repeated 3+ times) are collapsed.
    int last = res->trace_len - 1;
    int i = 0;
    while (i < last) {
      int collapsed = 0;
      for (int p = 1; p <= 2 && !collapsed; p++) {
        int reps = 1;
        while (i + (reps + 1) * p <= last &&
               !memcmp(&res->trace[i], &res->trace[i + reps * p],
                       p * sizeof(uint32_t)))
          reps++;
        if (reps >= 3) {
          for (int k = 0; k < p; k++)
            print_insn(out, res->trace[i + k], 0);
          fprintf(out, "    ... %d more times\n", reps - 1);
          i += reps * p;
          collapsed = 1;
        }
      }
      if (!collapsed)
        print_insn(out, res->trace[i++], 0);
    }
    print_insn(out, res->trace[last], 1);
  }

  fprintf(out, "\nRegisters%s:\n",
          res->status == TNDO_M68K_OK ? "" : " (at the faulting instruction)");
  for (int row = 0; row < 4; row++) {
    const uint32_t *v = row < 2 ? res->regs.d : res->regs.a;
    int base = (row & 1) * 4;
    fprintf(out, "   ");
    for (int i = base; i < base + 4; i++)
      fprintf(out, " %c%d=%08X", row < 2 ? 'D' : 'A', i, v[i]);
    fputc('\n', out);
  }
  fprintf(out, "    SR=%04X\n", res->sr & 0xFFFF);

  // Best effort backtrace: return addresses on the stack that point into
  // code, up to the host caller's sentinel.
  ram_region *us = &g.ram[R_USTACK];
  uint32_t sp = res->regs.a[7];
  fprintf(out, "\nBacktrace (best effort):\n");
  fprintf(out, "    #0 $%08X %s\n", res->pc, where);
  if (us->mem && sp - us->base < us->size) {
    int frame = 1;
    uint32_t prev = 0;
    int repeats = 0;
    for (uint32_t a = sp & ~1u; a + 4 <= us->base + us->size; a += 2) {
      uint32_t v = peek(a, 4);
      if (v == TNDO_M68K_RETURN_SENTINEL) {
        if (repeats)
          fprintf(out, "    ... %d more frames at $%08X\n", repeats, prev);
        fprintf(out, "    #%d <host caller>\n", frame + repeats);
        break;
      }
      if ((v & 1) || !in_code(v, 2))
        continue;
      a += 2;
      if (v == prev) {
        // Recursion: count the frames instead of printing each one.
        repeats++;
        continue;
      }
      if (repeats) {
        fprintf(out, "    ... %d more frames at $%08X\n", repeats, prev);
        frame += repeats;
        repeats = 0;
      }
      if (frame >= 16) {
        fprintf(out, "    ...\n");
        break;
      }
      tndo_m68k_symbolize(v, where, sizeof(where));
      fprintf(out, "    #%d $%08X %s\n", frame++, v, where);
      prev = v;
    }
  }
  fputc('\n', out);
}

// ---------------------------------------------------------------------------
// Data helpers.
// ---------------------------------------------------------------------------

uint32_t tndo_m68k_addr_or_die(const void *ptr) {
  if (!ptr)
    return 0;
  uint32_t addr = tndo_m68k_addr(ptr);
  if (!addr) {
    fprintf(stderr,
            "68k: host pointer %p is not 68k memory. 68k code can only see "
            "memory from tndo_m68k_alloc(), or from tndo_malloc() with "
            "TORNADO_68K_SHARED_HEAP.\n",
            ptr);
    abort();
  }
  return addr;
}

void tndo_m68k_put_ptrs(void *dst, void *const *ptrs, uint32_t n) {
  uint8_t *d = (uint8_t *)dst;
  for (uint32_t i = 0; i < n; i++)
    tndo_m68k_wr32(d + 4 * i, tndo_m68k_addr_or_die(ptrs[i]));
}

uint32_t tndo_m68k_ptr_table(void *const *ptrs, uint32_t n) {
  uint8_t *table = tndo_m68k_alloc(n ? n * 4 : 4, TNDO_M68K_FAST);
  if (!table) {
    fprintf(stderr, "68k: out of fast memory for a %u entry pointer table\n",
            n);
    abort();
  }
  tndo_m68k_put_ptrs(table, ptrs, n);
  return tndo_m68k_addr(table);
}

void tndo_m68k_to_be16(void *dst, const uint16_t *src, uint32_t n) {
  uint8_t *d = (uint8_t *)dst;
  for (uint32_t i = 0; i < n; i++)
    tndo_m68k_wr16(d + 2 * i, src[i]);
}

void tndo_m68k_to_be32(void *dst, const uint32_t *src, uint32_t n) {
  uint8_t *d = (uint8_t *)dst;
  for (uint32_t i = 0; i < n; i++)
    tndo_m68k_wr32(d + 4 * i, src[i]);
}

void tndo_m68k_from_be16(uint16_t *dst, const void *src, uint32_t n) {
  const uint8_t *s = (const uint8_t *)src;
  for (uint32_t i = 0; i < n; i++)
    dst[i] = tndo_m68k_rd16(s + 2 * i);
}

void tndo_m68k_from_be32(uint32_t *dst, const void *src, uint32_t n) {
  const uint8_t *s = (const uint8_t *)src;
  for (uint32_t i = 0; i < n; i++)
    dst[i] = tndo_m68k_rd32(s + 4 * i);
}
