#ifndef COMPILER_SPECIFIC_H
#define COMPILER_SPECIFIC_H

/*
 * GCC 14 ELF override for the NDK compiler-specific.h.
 *
 * GCC 14 removed __asm("reg") on function parameters, so __REG__
 * must strip register info. __STDARGS__ and other GCC-specific
 * attributes (__saveds, __chip, __interrupt, etc.) are not supported
 * by the bare-metal ELF toolchain.
 */

#if __STDC__
#define __CLIB_PROTOTYPE(a) a
#else
#define __CLIB_PROTOTYPE(a) ()
#endif

#define __ASM__
#define __REG__(r, p) p
#define __STDARGS__
#define __SAVE_DS__
#define __FAR__
#define __INTERRUPT__
#define __CHIP__
#define __FAST__

/* The ELF toolchain's sys-include headers use lowercase variants of
 * these keywords which are built-in to the Bebbo GCC but not in
 * the bare-metal m68k-amiga-elf-gcc toolchain. */
#ifndef __stdargs
#define __stdargs
#endif
#ifndef __saveds
#define __saveds
#endif
#ifndef __chip
#define __chip
#endif
#ifndef __far
#define __far
#endif
#ifndef __interrupt
#define __interrupt
#endif

#endif /* COMPILER_SPECIFIC_H */
