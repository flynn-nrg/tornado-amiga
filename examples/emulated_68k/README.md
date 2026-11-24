This example shows how to run 68k assembler routines on the posix/SDL build. See [Running 68k assembler on the host](../../manual/Emulated68k.md) for the details.

The effect is a plasma with its inner loop written twice: in C (`renderPlasmaC()` in `src/plasma/plasma.c`) and in 68k assembler (`src/plasma/amiga/plasma_inner.s`). The Amiga build calls the assembler routine directly. The posix build runs the same `.s` file in the emulated 68k, because `Makefile_sdl_posix` sets `TORNADO_68K_EMU` and lists it in `DEMO_68K_OBJS`.

`plasma.c` shows how the data is shared:

* The frame buffer is a `tndo_malloc()` buffer, so it already lives in 68k memory. The 68k code writes it in place: only its address needs converting, with `tndo_m68k_addr_or_die()`.
* The sine table holds words, which the 68k reads big-endian. It gets a 68k copy, made with `tndo_m68k_to_be16()`.

Keys:

* 1: the C version.
* 2: the 68k version (the default).
* 3: runs both versions and paints the pixels that differ in white. The number of differing pixels is printed to the console.
* B: runs the 68k version on a buffer one line too short. The emulator stops at the first write past the end, and prints this report before aborting:

```
*** 68k fault: UNALLOCATED
    write of 1 byte(s) at $08A76FD0: 0 byte(s) past the end of the 81600 byte block at $08A63110 <fast RAM>
    PC $08000044 _renderPlasma_asm+$44, after 902472 instructions
Last instructions:
    ...
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

`PLASMA_MODE=c`, `68k`, `diff` or `crash` in the environment sets the starting mode:

```
mmendez$ make -f Makefile_sdl_posix
mmendez$ PLASMA_MODE=diff ./emulated_68k.elf
```

Build with `TORNADO_68K_EMU=false` (after a `make -f Makefile_sdl_posix clean`) to get a posix build with the C version only.
