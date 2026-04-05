#ifndef _GCC_ELF_STDINT_H
#define _GCC_ELF_STDINT_H

/*
 * The m68k-amiga-elf toolchain is freestanding (no system stdint.h).
 * Include GCC's own complete stdint definitions directly.
 * Also pull in stddef.h so that size_t is available -- VBCC's stdint.h
 * provides it, and many Tornado headers expect it after #include <stdint.h>.
 */
#include "stdint-gcc.h"
#include <stddef.h>

#endif
