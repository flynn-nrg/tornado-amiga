#ifndef _GCC_ELF_FENV_H
#define _GCC_ELF_FENV_H

/*
 * Minimal fenv.h for the m68k-amiga-elf-gcc toolchain targeting 68060.
 * Implemented inline using the 68060 FPCR (Floating-Point Control Register).
 *
 * FPCR rounding mode bits [5:4]:
 *   00 = Round to Nearest (default)
 *   01 = Round toward Zero
 *   10 = Round toward -Infinity
 *   11 = Round toward +Infinity
 */

#define FE_TONEAREST  0x00
#define FE_TOWARDZERO 0x10
#define FE_DOWNWARD   0x20
#define FE_UPWARD     0x30

#define FE_ROUNDING_MASK 0x30

static inline int fegetround(void) {
  int fpcr;
  __asm__ volatile("fmove%.l %%fpcr, %0" : "=dm"(fpcr));
  return fpcr & FE_ROUNDING_MASK;
}

static inline int fesetround(int round) {
  int fpcr;
  __asm__ volatile("fmove%.l %%fpcr, %0" : "=dm"(fpcr));
  fpcr = (fpcr & ~FE_ROUNDING_MASK) | (round & FE_ROUNDING_MASK);
  __asm__ volatile("fmove%.l %0, %%fpcr" : : "dm"(fpcr));
  return 0;
}

#endif
