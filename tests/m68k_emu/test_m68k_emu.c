// Headless tests for the emulated 68k core (src/sdl_posix/m68k_emu.c).
//
// Every test routine in asm/ is assembled by vasm with the same CPU and
// syntax flags as the Amiga build, as a flat binary. The runner loads each one
// into the emulator, calls it and checks the outcome. Crash reports for the
// fault tests are printed when the runner is started with -v.
//
// Usage: test_m68k_emu <dir with .bin files> [-v]

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tndo_m68k.h"

static const char *bin_dir;
static int verbose;
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

static uint32_t load(const char *name) {
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s.bin", bin_dir, name);
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("can't open %s\n", path);
    exit(2);
  }
  static uint8_t buf[65536];
  size_t n = fread(buf, 1, sizeof(buf), f);
  fclose(f);
  uint32_t addr = tndo_m68k_load_code(buf, (uint32_t)n, name);
  if (!addr) {
    printf("can't load %s\n", name);
    exit(2);
  }
  return addr;
}

static void init(const tndo_m68k_config *cfg) {
  if (tndo_m68k_init(cfg)) {
    printf("tndo_m68k_init failed\n");
    exit(2);
  }
}

// Calls 'name', checks the status and prints the report in verbose mode.
static tndo_m68k_status run(const char *name, tndo_m68k_regs *regs,
                            tndo_m68k_status expected, tndo_m68k_result *res) {
  uint32_t entry = load(name);
  tndo_m68k_status st = tndo_m68k_call(entry, regs, NULL, 0, res);
  CHECK(st == expected, "%s: got %s, expected %s (%s)", name,
        tndo_m68k_status_name(st), tndo_m68k_status_name(expected),
        res->message);
  if (verbose && st != TNDO_M68K_OK) {
    printf("---- %s ----", name);
    tndo_m68k_report(res, stdout);
  }
  return st;
}

static void test_basics(void) {
  tndo_m68k_result res;
  tndo_m68k_regs r;

  init(NULL);

  tndo_m68k_regs_init(&r);
  r.d[0] = 40;
  r.d[1] = 2;
  run("add", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 42 && r.d[0] == 42, "add: d0=%u", res.d0);
  CHECK(res.instructions == 2, "add: %llu instructions",
        (unsigned long long)res.instructions);

  // Byte buffers are shared with no copy.
  uint8_t *buf = tndo_m68k_alloc(1000, TNDO_M68K_FAST);
  CHECK(buf != NULL, "alloc");
  tndo_m68k_regs_init(&r);
  r.a[0] = tndo_m68k_addr(buf);
  r.d[0] = 999;
  r.d[1] = 0x5A;
  run("fill", &r, TNDO_M68K_OK, &res);
  int ok = 1;
  for (int i = 0; i < 999; i++)
    ok &= buf[i] == 0x5A;
  CHECK(ok && buf[999] == 0, "fill: buffer contents");
  CHECK(r.a[0] == tndo_m68k_addr(buf) + 999, "fill: a0=%08X", r.a[0]);
  CHECK(tndo_m68k_host(r.a[0] - 999, 999) == buf, "host/addr round trip");
  CHECK(tndo_m68k_host(0, 4) == NULL, "host(0) must be NULL");
  CHECK(tndo_m68k_addr(&ok) == 0, "addr(stack var) must be 0");

  // Chip memory sits in the chip arena.
  uint8_t *chip = tndo_m68k_alloc(64, TNDO_M68K_CHIP);
  CHECK(chip && tndo_m68k_addr(chip) >= TNDO_M68K_CHIP_BASE &&
            tndo_m68k_addr(chip) < 0x200000,
        "chip alloc at %08X", tndo_m68k_addr(chip));

  // 16/32-bit data must be big-endian.
  uint8_t *longs = tndo_m68k_alloc(4 * 4, TNDO_M68K_FAST);
  for (int i = 0; i < 4; i++)
    tndo_m68k_wr32(longs + 4 * i, 0x01000000u * (i + 1));
  tndo_m68k_regs_init(&r);
  r.a[0] = tndo_m68k_addr(longs);
  r.d[0] = 4;
  run("sum32", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 0x0A000000u, "sum32: d0=%08X", res.d0);

  // C ABI stack arguments.
  uint32_t args[2] = {1000, 337};
  uint32_t entry = load("stackargs");
  CHECK(tndo_m68k_call(entry, NULL, args, 2, &res) == TNDO_M68K_OK,
        "stackargs: %s", res.message);
  CHECK(res.d0 == 1337, "stackargs: d0=%u", res.d0);

  tndo_m68k_regs_init(&r);
  run("preserve", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 42, "preserve: d0=%u", res.d0);

  // 020+ integer ops and the FPU: (7 * 6 + (-1)) * 0.5 = 20.5 -> 20 (round
  // to nearest even).
  tndo_m68k_regs_init(&r);
  r.d[0] = 7;
  r.d[1] = 6;
  r.d[2] = 0xFF;
  run("cpu040", &r, TNDO_M68K_ABI_VIOLATION, &res); // extb.l clobbers d2
  CHECK(res.d0 == 20, "cpu040: d0=%d", (int)res.d0);
  CHECK(res.abi_clobbered == (1u << 2), "cpu040: clobbered=%X",
        res.abi_clobbered);

  // Generated code: running from a data buffer is allowed by default.
  uint8_t *gen = tndo_m68k_alloc(16, TNDO_M68K_FAST);
  tndo_m68k_wr16(gen, 0x7007);     // moveq #7,d0
  tndo_m68k_wr16(gen + 2, 0x4E75); // rts
  tndo_m68k_regs_init(&r);
  r.a[0] = tndo_m68k_addr(gen);
  run("jump_a0", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 7, "generated code: d0=%u", res.d0);

  tndo_m68k_shutdown();
}

static void test_faults(void) {
  tndo_m68k_result res;
  tndo_m68k_regs r;
  tndo_m68k_config cfg;

  tndo_m68k_default_config(&cfg);
  cfg.instr_budget = 100000;
  init(&cfg);

  tndo_m68k_regs_init(&r);
  r.a[0] = 0;
  run("null_write", &r, TNDO_M68K_NULL_ACCESS, &res);
  CHECK(res.fault_addr == 0 && res.fault_size == 4 && res.fault_write,
        "null_write: addr=%08X size=%d w=%d", res.fault_addr, res.fault_size,
        res.fault_write);

  tndo_m68k_regs_init(&r);
  run("execbase", &r, TNDO_M68K_NULL_ACCESS, &res);
  CHECK(strstr(res.message, "ExecBase") != NULL, "execbase: %s",
        res.message);

  tndo_m68k_regs_init(&r);
  r.a[0] = 0x40000000;
  run("read_a0", &r, TNDO_M68K_BUS_ERROR, &res);
  CHECK(res.fault_addr == 0x40000000 && !res.fault_write, "read_a0: %08X",
        res.fault_addr);

  // Uninitialized address register: the poison value is unmapped.
  tndo_m68k_regs_init(&r);
  run("read_a0", &r, TNDO_M68K_BUS_ERROR, &res);
  CHECK(res.fault_addr == TNDO_M68K_POISON, "poison a0: %08X",
        res.fault_addr);

  // Reading past the end of the fast arena.
  tndo_m68k_regs_init(&r);
  r.a[0] = TNDO_M68K_FAST_BASE + cfg.fast_size - 2;
  run("read_a0", &r, TNDO_M68K_BUS_ERROR, &res);
  CHECK(strstr(res.message, "crosses the end") != NULL, "straddle: %s",
        res.message);

  tndo_m68k_regs_init(&r);
  run("custom_write", &r, TNDO_M68K_HW_ACCESS, &res);
  CHECK(res.fault_addr == 0xDFF180 && res.fault_size == 2,
        "custom_write: %08X/%d", res.fault_addr, res.fault_size);

  tndo_m68k_regs_init(&r);
  run("illegal", &r, TNDO_M68K_EXCEPTION, &res);
  CHECK(res.vector == 4, "illegal: vector %d", res.vector);
  CHECK(res.pc == tndo_m68k_symbol("illegal") + 2, "illegal: pc=%08X",
        res.pc);

  tndo_m68k_regs_init(&r);
  run("divzero", &r, TNDO_M68K_EXCEPTION, &res);
  CHECK(res.vector == 5, "divzero: vector %d", res.vector);

  tndo_m68k_regs_init(&r);
  run("privilege", &r, TNDO_M68K_EXCEPTION, &res);
  CHECK(res.vector == 8, "privilege: vector %d", res.vector);

  tndo_m68k_regs_init(&r);
  run("trap", &r, TNDO_M68K_EXCEPTION, &res);
  CHECK(res.vector == 35, "trap: vector %d", res.vector);

  tndo_m68k_regs_init(&r);
  run("loop", &r, TNDO_M68K_TIMEOUT, &res);
  CHECK(res.instructions == cfg.instr_budget + 1, "loop: %llu",
        (unsigned long long)res.instructions);

  tndo_m68k_regs_init(&r);
  run("clobber", &r, TNDO_M68K_ABI_VIOLATION, &res);
  CHECK(res.abi_clobbered == ((1u << 2) | (1u << 11)), "clobber: %X",
        res.abi_clobbered);

  tndo_m68k_regs_init(&r);
  run("unbalanced", &r, TNDO_M68K_ABI_VIOLATION, &res);
  CHECK(strstr(res.message, "-4 bytes") != NULL, "unbalanced: %s",
        res.message);

  tndo_m68k_regs_init(&r);
  run("recursion", &r, TNDO_M68K_BUS_ERROR, &res);
  CHECK(strstr(res.message, "stack overflow") != NULL, "recursion: %s",
        res.message);

  tndo_m68k_regs_init(&r);
  r.a[0] = 0;
  run("jump_a0", &r, TNDO_M68K_EXEC_FAULT, &res);
  CHECK(res.pc == 0, "jump null: pc=%08X", res.pc);

  tndo_m68k_regs_init(&r);
  r.a[0] = tndo_m68k_symbol("add") + 1;
  run("jump_a0", &r, TNDO_M68K_ODD_PC, &res);

  tndo_m68k_regs_init(&r);
  run("vector_write", &r, TNDO_M68K_VECTOR_WRITE, &res);

  uint8_t *buf = tndo_m68k_alloc(16, TNDO_M68K_FAST);
  tndo_m68k_regs_init(&r);
  r.a[0] = tndo_m68k_addr(buf);
  run("nested", &r, TNDO_M68K_BUS_ERROR, &res);
  CHECK(tndo_m68k_rd32(buf) == 0xbadc0ded, "nested: first write landed");
  CHECK(res.regs.a[0] == 0x40000000, "nested: a0=%08X", res.regs.a[0]);
  // movem, bsr, bsr, move.l, move.l, move.l (faults).
  CHECK(res.trace_len == 6, "nested: trace_len=%d", res.trace_len);

  // A call after a fault starts from a clean CPU.
  tndo_m68k_regs_init(&r);
  r.d[0] = 1;
  r.d[1] = 2;
  run("add", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 3, "add after faults: d0=%u", res.d0);

  tndo_m68k_shutdown();
}

static void test_policies(void) {
  tndo_m68k_result res;
  tndo_m68k_regs r;
  tndo_m68k_config cfg;

  // HW_IGNORE: custom chip writes are dropped.
  tndo_m68k_default_config(&cfg);
  cfg.hw_policy = TNDO_M68K_HW_IGNORE;
  init(&cfg);
  tndo_m68k_regs_init(&r);
  run("custom_write", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 1, "custom_write ignored: d0=%u", res.d0);
  tndo_m68k_shutdown();

  // Default: self-modifying code is a fault (040/060 caches).
  tndo_m68k_default_config(&cfg);
  init(&cfg);
  tndo_m68k_regs_init(&r);
  run("smc", &r, TNDO_M68K_CODE_WRITE, &res);
  CHECK(res.fault_size == 2 && res.fault_write &&
            res.fault_addr == tndo_m68k_symbol("smc") + 8,
        "smc: addr=%08X size=%d", res.fault_addr, res.fault_size);
  tndo_m68k_shutdown();

  // allow_smc (68000 code): the patch runs, with a warning.
  tndo_m68k_default_config(&cfg);
  cfg.allow_smc = 1;
  init(&cfg);
  tndo_m68k_regs_init(&r);
  run("smc", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 5, "smc: d0=%u (patched moveq #5)", res.d0);
  tndo_m68k_shutdown();

  // strict_exec: running code from a data buffer is a fault.
  tndo_m68k_default_config(&cfg);
  cfg.strict_exec = 1;
  init(&cfg);
  uint8_t *data = tndo_m68k_alloc(16, TNDO_M68K_FAST);
  tndo_m68k_wr16(data, 0x4E75); // rts
  tndo_m68k_regs_init(&r);
  r.a[0] = tndo_m68k_addr(data);
  run("jump_a0", &r, TNDO_M68K_EXEC_FAULT, &res);
  tndo_m68k_shutdown();

  // ABI check disabled.
  tndo_m68k_default_config(&cfg);
  cfg.abi_check = 0;
  init(&cfg);
  tndo_m68k_regs_init(&r);
  run("clobber", &r, TNDO_M68K_OK, &res);
  tndo_m68k_shutdown();

  // Calls without init.
  CHECK(tndo_m68k_call(TNDO_M68K_FAST_BASE, NULL, NULL, 0, &res) ==
            TNDO_M68K_BAD_CALL,
        "call before init");
}

// ---------------------------------------------------------------------------
// Phase 2: hunk objects, linking and host bindings.
// ---------------------------------------------------------------------------

static int load_hunk_file(const char *name) {
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s.hunk", bin_dir, name);
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("can't open %s\n", path);
    exit(2);
  }
  static uint8_t buf[1 << 16];
  size_t n = fread(buf, 1, sizeof(buf), f);
  fclose(f);
  return tndo_m68k_load_hunk(name, buf, (uint32_t)n);
}

static tndo_m68k_status call_sym(const char *sym, tndo_m68k_regs *r,
                                 tndo_m68k_status expected,
                                 tndo_m68k_result *res) {
  tndo_m68k_status st = tndo_m68k_call(tndo_m68k_symbol(sym), r, NULL, 0, res);
  CHECK(st == expected, "%s: got %s, expected %s (%s)", sym,
        tndo_m68k_status_name(st), tndo_m68k_status_name(expected),
        res->message);
  if (verbose && st != TNDO_M68K_OK) {
    printf("---- %s ----", sym);
    tndo_m68k_report(res, stdout);
  }
  return st;
}

static void host_add(tndo_m68k_regs *r, void *user) {
  r->d[0] += r->d[1];
  (*(int *)user)++;
}

static void host_sub(tndo_m68k_regs *r, void *user) {
  r->d[0] += tndo_m68k_arg(r, 0) - tndo_m68k_arg(r, 1);
  (void)user;
}

static void test_hunks(void) {
  tndo_m68k_result res;
  tndo_m68k_regs r;
  char where[96];

  init(NULL);

  // Embedded modules are loaded by init and linked to each other.
  tndo_m68k_regs_init(&r);
  call_sym("_emb_main", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 43, "_emb_main: d0=%u", res.d0);
  // So is all the framework asm.
  CHECK(tndo_m68k_symbol("_c2p1x1_8_c5_040") != 0, "framework c2p loaded");
  CHECK(tndo_m68k_symbol("_PaulaOutput_Init") != 0, "framework paula loaded");

  // Relocations between code, data and chip bss.
  CHECK(load_hunk_file("reloc") == 0, "load reloc");
  tndo_m68k_regs_init(&r);
  call_sym("_reloc_sum", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 110, "_reloc_sum: d0=%u", res.d0);
  CHECK(r.a[0] >= TNDO_M68K_CHIP_BASE && r.a[0] < 0x200000,
        "chip bss at %08X", r.a[0]);
  CHECK(tndo_m68k_rd32(tndo_m68k_host(r.a[0], 4)) == 110, "chip bss write");
  CHECK(r.d[1] == tndo_m68k_symbol("_reloc_sum"), "data->code ptr %08X",
        r.d[1]);

  // Crash reports name local labels from the SYMBOL hunk.
  tndo_m68k_regs_init(&r);
  r.a[0] = 0;
  call_sym("_crash", &r, TNDO_M68K_NULL_ACCESS, &res);
  tndo_m68k_symbolize(res.pc, where, sizeof(where));
  CHECK(!strcmp(where, "inner_fault+$0"), "symbolized pc: %s", where);

  // Cross-module references, resolved when the second module is loaded.
  CHECK(load_hunk_file("mod_a") == 0, "load mod_a");
  tndo_m68k_regs_init(&r);
  r.d[0] = 5;
  call_sym("_twice_plus", &r, TNDO_M68K_UNRESOLVED, &res);
  CHECK(strstr(res.message, "_helper") != NULL, "unresolved: %s",
        res.message);
  CHECK(load_hunk_file("mod_b") == 0, "load mod_b");
  tndo_m68k_regs_init(&r);
  r.d[0] = 5;
  call_sym("_twice_plus", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 1020, "_twice_plus: d0=%u", res.d0);

  // Exported names must be unique.
  CHECK(load_hunk_file("dup") != 0, "duplicate _helper must fail");

  // Host bindings.
  CHECK(load_hunk_file("host") == 0, "load host");
  int calls = 0;
  CHECK(tndo_m68k_bind("_host_add", host_add, &calls) == 0, "bind add");
  CHECK(tndo_m68k_bind("_host_sub", host_sub, NULL) == 0, "bind sub");
  uint8_t *table = tndo_m68k_alloc(8, TNDO_M68K_FAST);
  tndo_m68k_wr32(table, 10000);
  tndo_m68k_wr32(table + 4, 200000);
  CHECK(tndo_m68k_bind_data("_host_table", tndo_m68k_addr(table)) == 0,
        "bind table");
  CHECK(tndo_m68k_bind("_helper", host_add, NULL) != 0,
        "binding a module symbol must fail");
  tndo_m68k_regs_init(&r);
  r.d[0] = 1;
  call_sym("_call_host", &r, TNDO_M68K_OK, &res);
  CHECK(res.d0 == 1 + 3 + 4 + 10000 + 200000 - 2, "_call_host: d0=%d",
        (int)res.d0);
  CHECK(calls == 2, "host_add calls: %d", calls);

  tndo_m68k_regs_init(&r);
  call_sym("_call_unbound", &r, TNDO_M68K_UNRESOLVED, &res);
  CHECK(strstr(res.message, "call to unresolved external _not_bound") != NULL,
        "%s", res.message);
  tndo_m68k_regs_init(&r);
  call_sym("_bsr_unbound", &r, TNDO_M68K_UNRESOLVED, &res);
  CHECK(strstr(res.message, "_not_bound") != NULL, "%s", res.message);
  tndo_m68k_regs_init(&r);
  call_sym("_read_unbound", &r, TNDO_M68K_UNRESOLVED, &res);
  CHECK(strstr(res.message, "read of unresolved external _not_bound_data") !=
            NULL,
        "%s", res.message);

  // A missing symbol gives a clear error.
  CHECK(tndo_m68k_call(tndo_m68k_symbol("_nope"), NULL, NULL, 0, &res) ==
                TNDO_M68K_BAD_CALL &&
            strstr(res.message, "symbol not found") != NULL,
        "missing symbol: %s", res.message);

  tndo_m68k_shutdown();

  // Registered modules come back after a shutdown/init cycle, host modules
  // loaded at runtime do not.
  init(NULL);
  CHECK(tndo_m68k_symbol("_emb_magic") != 0, "embedded module after reinit");
  CHECK(tndo_m68k_symbol("_call_host") == 0, "runtime module is gone");
  tndo_m68k_shutdown();
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage: %s <bin dir> [-v]\n", argv[0]);
    return 2;
  }
  bin_dir = argv[1];
  verbose = argc > 2 && !strcmp(argv[2], "-v");

  test_basics();
  test_faults();
  test_policies();
  test_hunks();

  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
