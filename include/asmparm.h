#ifndef ASMPARM_H
#define ASMPARM_H

#ifdef __AMIGA__

#ifdef __VBCC__
#define __ASMPARM(__reg__, __decl__) __reg(__reg__) __decl__
#endif

#ifdef __GCC__
#define __ASMPARM(__reg__, __decl__) __decl__ __asm(__reg__)
#endif

#ifdef __GCC_ELF__
/*
 * GCC 14 removed the __asm("reg") syntax for function parameters.
 * For C-bodied functions (e.g. c2p.c, audio_ahi.c), __ASMPARM strips
 * the register info and produces a standard C declaration.
 *
 * For assembly function declarations, each header provides explicit
 * inline wrappers using register-pinned local variables + inline asm.
 * See system.h, c2p.h, etc.
 */
#define __ASMPARM(__reg__, __decl__) __decl__
#endif

#ifndef __ASMPARM
#error "need to be able to define register parameters"
#endif

#else

// not amiga --> ignore register
#define __ASMPARM(__reg__, __decl__) __decl__

#endif

#endif
