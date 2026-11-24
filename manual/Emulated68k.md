Running 68k assembler on the host
=================================

Inner loops usually end up in 68k assembler, and until now that code could only be tested on an emulator or real hardware. The posix/SDL build can run it too: your assembler routines are assembled with the same vasm flags as the Amiga build and executed by an emulated 68040 ([Musashi](https://github.com/kstenerud/Musashi)) that is linked into the demo.

This means you can:

* Run the C and the assembler version of an effect side by side and compare their output pixel by pixel.
* Get a readable report when the assembler code crashes: the faulting instruction, the last 16 instructions, the registers and a backtrace, all with symbol names.
* Catch bugs that real hardware lets through silently: writes past the end of a buffer, null pointers, clobbered registers, self-modifying code that breaks on the 040/060 caches.
* Use the debugger, the sanitisers, Rocket and the ImGui overlay as usual. The rest of the demo is still native code.

The Amiga build doesn't change: the same `.s` files are linked into the Amiga executable as before.

Setting it up
-------------

Musashi is an external dependency, like Rocket and ImGui. Clone it next to them (see [Getting Started](GettingStarted.md)):

```
mmendez$ cd ..            # the directory that contains tornado-amiga
mmendez$ git clone https://github.com/kstenerud/Musashi
```

You also need `vasmm68k_mot` in your `PATH`, which you already have if you build for the Amiga.

Then enable the emulator in your `Makefile_sdl_posix` and list the assembler files it should run:

```make
# Zoom
DEMO_OBJS += zoom/zoom.o
DEMO_OBJS += zoom/zoom_util.o

TORNADO_68K_EMU ?= true
ifeq ($(TORNADO_68K_EMU),true)
DEMO_68K_OBJS += zoom/amiga/zoom_inner.o
DEMO_OBJS += zoom/zoom_68k.o
endif
```

`DEMO_68K_OBJS` names the `.s` files of your demo, relative to `src` (with a `.o` extension, like the Amiga `DEMO_OBJS`). Framework routines go in `TORNADO_68K_OBJS`, relative to Tornado's `src` (e.g. `amiga/c2p1x1_8_c5_040.o`).

Each file is assembled into a hunk object, embedded into a C file by the `m68k_embed` tool and loaded into the emulator when the demo starts. Using `?=` means you can switch back to the C version from the command line with `make -f Makefile_sdl_posix TORNADO_68K_EMU=false`.

The build defines `TORNADO_68K_EMU` for your C code, and the assembler gets `-DTORNADO_68K_EMU` too. Note that changing the variable doesn't rebuild the objects that are already in `BUILDDIR`: run `make clean` when you switch.

The memory map
--------------

The emulated 68k has its own 32-bit, big-endian address space:

| Address | Contents |
|---------|----------|
| `$00000000-$00000FFF` | Null guard page. Any access faults. |
| `$00001000-$000013FF` | Vector table (VBR). Read-only for 68k code. |
| `$00002000-...` | "Chip" arena, below 2MB. |
| `$00BFD000-$00BFEFFF` | CIA registers. Faults by default. |
| `$00DFF000-$00DFFFFF` | Custom chip registers. Faults by default. |
| `$07000000-$0700FFFF` | User stack (64KB). Overflowing it faults. |
| `$08000000-...` | "Fast" arena: code, data and BSS of the modules, and your buffers. |
| `$FFFF0000-$FFFFFFFF` | Trap page: the return address of every call, exception handlers and host calls. |

68k code runs in user mode, as it would under AmigaOS.

Sharing memory with the 68k code
--------------------------------

With `TORNADO_68K_EMU` the posix build also turns on `TORNADO_68K_SHARED_HEAP`: `tndo_malloc()` allocates from the emulator's arenas (`TNDO_ALLOC_CHIP` from the chip arena, everything else from fast), so the 68k code can work on your buffers directly. The display's frame buffer, the assets and your textures all live there.

The 68k and the host see the same bytes, at different addresses. To pass a buffer to an assembler routine you convert its host pointer into a 68k address:

```c
#include <tndo_m68k.h>

uint32_t chunky68k = tndo_m68k_addr_or_die(chunky);
```

`tndo_m68k_addr_or_die()` aborts with an explanation if the pointer isn't in 68k memory, for example a `malloc()` buffer or a stack variable. `tndo_m68k_addr()` returns 0 instead, and `tndo_m68k_host()` goes the other way.

Byte data such as chunky pixels, textures and 8-bit tables needs no conversion. Everything else does, because the host is little-endian and has 64-bit pointers:

* **Words and longs.** The 68k reads them big-endian. Convert tables once with `tndo_m68k_to_be16()` and `tndo_m68k_to_be32()`, read results back with `tndo_m68k_from_be16()` and `tndo_m68k_from_be32()`, and use `tndo_m68k_rd16/rd32/wr16/wr32()` for single values.
* **Pointers.** A host pointer is 8 bytes and the 68k expects 4. `tndo_m68k_put_ptrs()` writes a table of 68k addresses, and `tndo_m68k_ptr_table()` allocates and fills one.
* **Structs.** The 68k code relies on your offsets, which only match if every field is a byte or 32 bits wide and there are no pointers. Copy them field by field. A `_Static_assert` on `offsetof()` catches layout changes.

Memory you need on the 68k side only can come from `tndo_m68k_alloc()`, which is zeroed and lives until shutdown.

The emulator is sized from your `demoSettings()`: the fast arena holds `fastMemory` plus 256MB for `tndo_malloc()`. Change the extra amount with `TORNADO_68K_FAST_EXTRA_MB` in your makefile if you need more.

Calling a routine
-----------------

You look the routine up by its exported name, fill in the registers and call it:

```c
uint32_t entry = tndo_m68k_symbol("_renderPlasma_asm");

tndo_m68k_regs r;
tndo_m68k_result res;
tndo_m68k_regs_init(&r);
r.a[0] = tndo_m68k_addr_or_die(chunky);
r.a[1] = sine68k;
r.d[0] = frame;
if (tndo_m68k_call(entry, &r, NULL, 0, &res) != TNDO_M68K_OK) {
  tndo_m68k_report(&res, stderr);
  abort();
}
```

When the routine returns, `r` holds the output registers and `res` the return value, the number of instructions executed and Musashi's cycle count. The cycle count is a 68040 estimate: it isn't 060 timing, so profile on real hardware. For routines that take C ABI stack arguments, pass them in the `stack_args` array.

`tndo_m68k_call_or_die()` does the same and aborts with the report on failure. It returns `d0`.

`tndo_m68k_regs_init()` fills every register with `$BADC0DED`, so a register you forgot to set shows up immediately in the report. The upper half of a register that is only written with `.w` operations keeps the pattern, as in `D2=BADC013F` in the report below.

The cleanest way to keep your effect code unchanged is a host function with the same name and signature as the assembler routine, which does the conversions and the call. The zoom example uses this approach in `zoom_68k.c`, and includes `zoom_inner.h` for both targets:

```c
#if defined(__AMIGA__) || defined(TORNADO_68K_EMU)
  renderZoom_asm(allTxts, chunky, zi);
#else
  renderZoomC(allTxts, chunky, zi);
#endif
```

The zoom routine takes a table of 6 pointers (one per mipmap level), each pointing to a table of 65536 texture pointers. `zoom_68k.c` builds a 68k copy of those tables the first time it sees them, and copies each frame's iteration data into a 68k buffer with `to_be32()`.

When something goes wrong
-------------------------

Every memory access is checked, and a failing call returns one of these statuses instead of corrupting the host:

| Status | Cause |
|--------|-------|
| `NULL_ACCESS` | Access to the first 4KB. |
| `BUS_ERROR` | Access to unmapped memory. |
| `UNALLOCATED` | Access to arena memory outside any allocation, see below. |
| `VECTOR_WRITE` | Write to the vector table. |
| `HW_ACCESS` | Access to the custom chips or the CIAs. |
| `CODE_WRITE` | Self-modifying code. |
| `EXEC_FAULT` | Jump to an address that isn't code. |
| `ODD_PC` | Jump to an odd address. |
| `EXCEPTION` | Illegal instruction, division by zero, `trap`, `chk`... |
| `TIMEOUT` | The call ran for more than 100 million instructions (an infinite loop). |
| `ABI_VIOLATION` | The routine returned with `d2-d7/a2-a6` changed or the stack unbalanced. |
| `UNRESOLVED` | The code used an external symbol (`xref`) that nobody defined. |

This is the report the `emulated_68k` example prints when you press B. It runs the plasma on a buffer that is one line too short:

```
*** 68k fault: UNALLOCATED
    write of 1 byte(s) at $08A76FD0: 0 byte(s) past the end of the 81600 byte block at $08A63110 <fast RAM>
    PC $08000044 _renderPlasma_asm+$44, after 902472 instructions
Last instructions:
    ...
    $08000038 _renderPlasma_asm+$38        move.w  D4, D1
    $0800003A _renderPlasma_asm+$3A        and.w   #$1fe, D1
    $0800003E _renderPlasma_asm+$3E        add.w   (A1,D1.w), D0
    $08000042 _renderPlasma_asm+$42        add.w   D5, D0
 -> $08000044 _renderPlasma_asm+$44        move.b  D0, (A0)+
Registers (at the faulting instruction):
    D0=00000078 D1=000001FE D2=BADC013F D3=00000000
    D4=000001FE D5=BADC0026 D6=000000FF D7=00000000
    A0=08A76FD0 A1=08A62F00 A2=BADC0DED A3=BADC0DED
    A4=BADC0DED A5=BADC0DED A6=BADC0DED A7=0700FFE4
    SR=0000
Backtrace (best effort):
    #0 $08000044 _renderPlasma_asm+$44
    #1 <host caller>
```

The registers are the ones from before the faulting instruction, and the faulting write never happens. The symbol names come from the assembler, which is why the posix build doesn't pass `-nosym` to vasm.

### Overruns and AddressSanitizer

The posix build uses AddressSanitizer (see [Debugging](Debugging.md)), and the emulator shares its bookkeeping. Each block in the arenas is followed by a 16-byte redzone, and the rest of the arena is poisoned. That gives you two things:

* A 68k write outside any block is reported as `UNALLOCATED`, with the block it missed.
* Host code that overruns a `tndo_malloc()` buffer is caught by ASan, as before.

By default only 68k writes are checked. Real hardware doesn't mind reading a few bytes past a table, and plenty of good assembler does exactly that. Set `alloc_check` to `TNDO_M68K_ALLOC_CHECK_ALL` to check reads too (see [Options](#options)).

### Self-modifying code and the hardware registers

On the 040 and 060 self-modifying code breaks, because the instruction cache doesn't see the writes. The emulator stops at the first write into a code range. If you target the 68000 (A500), set `allow_smc`: such writes are then allowed and logged once per instruction.

Code generated into a buffer at runtime is allowed to run by default. Remember that on the Amiga it still needs `CacheClearU()` first. Set `strict_exec` to only allow running code from the loaded modules.

There are no custom chips or CIAs in the emulator. A routine that touches them fails with `HW_ACCESS`. You can change `hw_policy` to `TNDO_M68K_HW_IGNORE` (reads return 0, writes are dropped) or to `TNDO_M68K_HW_HOOK` to handle the accesses yourself.

### Options

With the shared heap, `tndo_memory_init()` starts the emulator before your demo code runs. You can still change every option except the sizes afterwards, for instance in your effect's init function:

```c
tndo_m68k_config cfg;
tndo_m68k_get_config(&cfg);
cfg.alloc_check = TNDO_M68K_ALLOC_CHECK_ALL; // Check reads too.
cfg.allow_smc = 1;                           // 68000 code.
tndo_m68k_set_options(&cfg);
```

The options are `instr_budget` (0 disables the infinite loop check), `hw_policy` and its hooks, `allow_smc`, `strict_exec`, `abi_check` and `alloc_check`. Without the shared heap you can also pass a config to `tndo_m68k_init()` yourself, sizes included.

### Calling the host

An external symbol that no module defines can be bound to a host function, which receives the 68k registers:

```c
static void hostLog(tndo_m68k_regs *r, void *user) {
  printf("68k says d0=%08X\n", r->d[0]);
}

tndo_m68k_bind("_hostLog", hostLog, NULL);
```

A `jsr _hostLog` in the assembler then calls `hostLog()`. `tndo_m68k_bind_data()` binds a symbol to data at a 68k address instead.

Limitations
-----------

* The emulator is a 68040. It runs some instructions that the 060 traps to software, such as `movep` and 64-bit `mulu.l`/`divu.l`. FPU support is Musashi's, which doesn't cover every instruction.
* There is no AmigaOS and there are no custom chips. Routines must be pure computation on memory: inner loops, c2p, decompressors.
* The small data model (`a4`-relative `near` data) isn't supported.
* Calls can't be nested: a host function called from 68k code can't call back into the 68k.
* Speed is around 45 million instructions per second, below a real 060. The zoom inner loop (about 300K instructions per frame) and the plasma example (900K) still run at full frame rate.

Examples
--------

* `examples/emulated_68k`: a plasma written in C and in 68k assembler. Press 1 for the C version, 2 for the 68k version and 3 to show the difference between them (the pixels that differ turn white). B runs the 68k version on a buffer one line short and prints the report above. `PLASMA_MODE=c|68k|diff|crash` sets the starting mode.
* `examples/zoom`: the original `zoom_inner.s` running on the host. Run it with `ZOOM_68K_CHECK=1` to compare every frame against `renderZoomC()`.

The emulator's own tests are in `tests/m68k_emu`. Run them with `make` in that directory.
