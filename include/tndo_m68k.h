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

// Emulated 68k for the POSIX host build (TORNADO_68K_EMU).
//
// Runs 68k machine code on an emulated 68040 (Musashi) inside its own 32-bit,
// big-endian address space. Every memory access is checked: a crash in 68k
// code comes back as a tndo_m68k_result instead of taking the host down.
//
// Address map (see manual/Emulated68k.md):
//   0x00000000-0x00000FFF  null guard page, any access faults
//   0x00001000-0x000013FF  vector table (VBR), read-only for 68k code
//   0x00002000-...         "chip" arena (max. 0x1FE000 bytes, ends below 2MB)
//   0x00BFD000-0x00BFEFFF  CIA registers      -> hw_policy
//   0x00DFF000-0x00DFFFFF  custom chip regs   -> hw_policy
//   0x06FF0000-...         supervisor stack (exception frames)
//   0x07000000-...         user stack, unmapped below it (overflow faults)
//   0x08000000-...         "fast" arena: code, data and tndo_m68k_alloc()
//   0xFFFF0000-0xFFFFFFFF  trap page: return sentinel, exception stubs and
//                          one thunk per external symbol (host bindings)
//
// 68k code always runs in user mode, as under AmigaOS. Only one call can run
// at a time: the emulated CPU is a single global, and calls are not reentrant.

#ifndef TNDO_M68K_H
#define TNDO_M68K_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Address map.
#define TNDO_M68K_VECTORS_BASE 0x00001000u
#define TNDO_M68K_CHIP_BASE 0x00002000u
#define TNDO_M68K_CHIP_MAX 0x001FE000u
#define TNDO_M68K_CIA_BASE 0x00BFD000u
#define TNDO_M68K_CIA_END 0x00BFF000u
#define TNDO_M68K_CUSTOM_BASE 0x00DFF000u
#define TNDO_M68K_CUSTOM_END 0x00E00000u
#define TNDO_M68K_SSTACK_BASE 0x06FF0000u
#define TNDO_M68K_USTACK_BASE 0x07000000u
#define TNDO_M68K_FAST_BASE 0x08000000u
#define TNDO_M68K_TRAP_BASE 0xFFFF0000u
#define TNDO_M68K_RETURN_SENTINEL TNDO_M68K_TRAP_BASE

// Unused input registers are filled with this, see tndo_m68k_regs_init().
#define TNDO_M68K_POISON 0xBADC0DEDu

// Where tndo_m68k_alloc() places memory.
#define TNDO_M68K_FAST 0
#define TNDO_M68K_CHIP 1

// What happens when 68k code touches the custom chip or CIA registers.
typedef enum {
  TNDO_M68K_HW_FAULT = 0, // Stop and report (default).
  TNDO_M68K_HW_IGNORE,    // Reads return 0, writes are dropped. Logged once.
  TNDO_M68K_HW_HOOK,      // Forward to hw_read/hw_write in the config.
} tndo_m68k_hw_policy;

typedef struct {
  uint32_t fast_size;   // Fast arena size in bytes. Default 16MB.
  uint32_t chip_size;   // Chip arena size, <= TNDO_M68K_CHIP_MAX (default).
  uint32_t stack_size;  // User stack size. Default 64KB.
  uint64_t instr_budget; // Max. instructions per call, 0 = no limit.
                         // Default 100M. Catches infinite loops.
  tndo_m68k_hw_policy hw_policy;
  // Self-modifying code. Off (default): a write into a code range is a
  // CODE_WRITE fault, because it breaks on the 040/060 caches. On: it is
  // allowed and reported once per PC. Only meant for 68000 (A500) code.
  int allow_smc;
  // Off (default): code may run from any RAM, so generated code works. It
  // still needs a cache flush (CacheClearU) before it runs on a 040/060.
  // On: running code outside the loaded code ranges is an EXEC_FAULT.
  int strict_exec;
  // Fail the call if d2-d7/a2-a6 are not preserved or the stack is not
  // balanced on return. Default on.
  int abi_check;
  // Used when hw_policy == TNDO_M68K_HW_HOOK. size is 1, 2 or 4.
  uint32_t (*hw_read)(uint32_t addr, int size, void *user);
  void (*hw_write)(uint32_t addr, int size, uint32_t value, void *user);
  void *hw_user;
} tndo_m68k_config;

typedef struct {
  uint32_t d[8];
  uint32_t a[8]; // a[7] is ignored on input: the emulator owns the stack.
} tndo_m68k_regs;

typedef enum {
  TNDO_M68K_OK = 0,
  TNDO_M68K_BUS_ERROR,      // Access to unmapped memory.
  TNDO_M68K_NULL_ACCESS,    // Access to the null guard page.
  TNDO_M68K_VECTOR_WRITE,   // Write to the vector table.
  TNDO_M68K_HW_ACCESS,      // Custom chip / CIA access with HW_FAULT policy.
  TNDO_M68K_CODE_WRITE,     // Write into code, unless allow_smc is set.
  TNDO_M68K_EXEC_FAULT,     // Running code from a non executable address.
  TNDO_M68K_ODD_PC,         // Instruction at an odd address.
  TNDO_M68K_EXCEPTION,      // CPU exception (illegal, div by zero, trap...).
  TNDO_M68K_TIMEOUT,        // instr_budget exceeded.
  TNDO_M68K_ABI_VIOLATION,  // Callee-saved register clobbered / sp moved.
  TNDO_M68K_BAD_CALL,       // Not initialized, reentrant call, bad entry.
  TNDO_M68K_UNRESOLVED,     // Use of an external symbol nobody defined/bound.
} tndo_m68k_status;

#define TNDO_M68K_TRACE_LEN 16

typedef struct {
  tndo_m68k_status status;
  uint32_t d0;             // Return value.
  uint64_t instructions;   // Instructions executed by this call.
  uint64_t cycles;         // Musashi's 68040 cycle estimate, not 060 timing.

  // Valid when status != TNDO_M68K_OK.
  uint32_t pc;             // PC of the faulting instruction.
  uint32_t fault_addr;     // Bad address, for memory faults.
  int fault_size;          // 1, 2 or 4.
  int fault_write;         // 1 if the access was a write.
  int vector;              // Exception vector number, for EXCEPTION.
  uint32_t abi_clobbered;  // ABI_VIOLATION: bit n = d<n>, bit 8+n = a<n>.
  char message[192];       // One line description.

  // Registers at return, or at the start of the faulting instruction.
  tndo_m68k_regs regs;
  uint32_t sr;
  // The last instructions executed, oldest first, ending with the faulting
  // one.
  uint32_t trace[TNDO_M68K_TRACE_LEN];
  int trace_len;
} tndo_m68k_result;

// Lifecycle. tndo_m68k_init() loads every registered module (see below). It
// is optional: the first call to tndo_m68k_alloc(), tndo_m68k_symbol(),
// tndo_m68k_bind() or a loader initializes with the default config, and
// aborts if a module fails to load. Call it yourself to use another config.
void tndo_m68k_default_config(tndo_m68k_config *cfg);
int tndo_m68k_init(const tndo_m68k_config *cfg); // NULL = defaults. 0 = ok.
void tndo_m68k_shutdown(void);

// Hunk object modules, as written by vasm -Fhunk. A module is registered once
// (the build does it from a constructor, see m68k_embed) and loaded by every
// tndo_m68k_init(). 'data' must stay valid. All modules share one address
// space: exported symbols (xdef) are visible to every module, and an
// exported name defined twice is a load error.
void tndo_m68k_register_module(const char *name, const void *data,
                               uint32_t len);
// Loads a module now, into an initialized emulator. 0 = ok.
int tndo_m68k_load_hunk(const char *name, const void *data, uint32_t len);

// Externals (xref) that no module defines are left unresolved: calling or
// reading one fails with TNDO_M68K_UNRESOLVED and names the symbol. They can
// be bound to a host function, called with the 68k registers (stack arguments
// are read with tndo_m68k_arg()), or to data at a 68k address. 0 = ok.
typedef void (*tndo_m68k_host_fn)(tndo_m68k_regs *regs, void *user);
int tndo_m68k_bind(const char *name, tndo_m68k_host_fn fn, void *user);
int tndo_m68k_bind_data(const char *name, uint32_t addr);
// Inside a host function: the n-th 32-bit stack argument (C ABI).
uint32_t tndo_m68k_arg(const tndo_m68k_regs *regs, int n);

// Memory. Allocations are 16 byte aligned, zeroed, and live until shutdown.
void *tndo_m68k_alloc(uint32_t size, int where);
// Host pointer -> 68k address. Returns 0 if ptr is not inside the 68k RAM.
uint32_t tndo_m68k_addr(const void *ptr);
// 68k address -> host pointer. NULL unless [addr, addr+len) is 68k RAM.
void *tndo_m68k_host(uint32_t addr, uint32_t len);

// Code and symbols. tndo_m68k_load_code() copies position independent code
// into the fast arena, registers it as a code range and adds 'name' as a
// symbol at its start. It returns the 68k address, or 0 on error.
uint32_t tndo_m68k_load_code(const void *code, uint32_t len, const char *name);
void tndo_m68k_add_code_range(uint32_t addr, uint32_t len);
void tndo_m68k_add_symbol(const char *name, uint32_t addr, uint32_t size);
// Exported symbols win over local labels of the same name. 0 if not found.
uint32_t tndo_m68k_symbol(const char *name);
// Writes "name+$off" (or the bare address) into buf.
void tndo_m68k_symbolize(uint32_t addr, char *buf, int buflen);

// Calls. Fills every register with TNDO_M68K_POISON.
void tndo_m68k_regs_init(tndo_m68k_regs *regs);

// Runs the 68k routine at 'entry' until it returns with rts. 'io' holds the
// input registers and receives the output registers (it can be NULL).
// 'stack_args' are optional 32-bit C ABI arguments, pushed right to left.
tndo_m68k_status tndo_m68k_call(uint32_t entry, tndo_m68k_regs *io,
                                const uint32_t *stack_args, int nargs,
                                tndo_m68k_result *res);

// Same as tndo_m68k_call(), but a failure prints the report and aborts.
// Returns d0.
uint32_t tndo_m68k_call_or_die(uint32_t entry, tndo_m68k_regs *io);

// Prints a crash report: cause, symbolized PC, disassembly of the last
// instructions, registers and a best effort backtrace.
void tndo_m68k_report(const tndo_m68k_result *res, FILE *out);
const char *tndo_m68k_status_name(tndo_m68k_status status);

// Big-endian access to 68k memory through host pointers.
static inline uint16_t tndo_m68k_rd16(const void *p) {
  const uint8_t *b = (const uint8_t *)p;
  return (uint16_t)((b[0] << 8) | b[1]);
}
static inline uint32_t tndo_m68k_rd32(const void *p) {
  const uint8_t *b = (const uint8_t *)p;
  return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
         ((uint32_t)b[2] << 8) | b[3];
}
static inline void tndo_m68k_wr16(void *p, uint16_t v) {
  uint8_t *b = (uint8_t *)p;
  b[0] = (uint8_t)(v >> 8);
  b[1] = (uint8_t)v;
}
static inline void tndo_m68k_wr32(void *p, uint32_t v) {
  uint8_t *b = (uint8_t *)p;
  b[0] = (uint8_t)(v >> 24);
  b[1] = (uint8_t)(v >> 16);
  b[2] = (uint8_t)(v >> 8);
  b[3] = (uint8_t)v;
}

#ifdef __cplusplus
}
#endif

#endif // TNDO_M68K_H
