#ifndef ASMPARM_H
#define ASMPARM_H

#ifdef __AMIGA__

#ifdef __VBCC__
#define __ASMPARM(__reg__, __decl__) __reg(__reg__) __decl__
#endif

#ifdef __GCC__
#define __ASMPARM(__reg__, __decl__) __decl__ __asm(__reg__)
#endif

#ifndef __ASMPARM
#error "need to be able to define register parameters"
#endif

#else

// not amiga --> ignore register
#define __ASMPARM(__reg__, __decl__) __decl__

#endif

#endif
