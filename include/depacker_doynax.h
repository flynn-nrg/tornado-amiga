#ifndef DEPACKER_DOYNAX_H
#define DEPACKER_DOYNAX_H

#include "asmparm.h"

#ifdef __GCC_ELF__

static inline void doynaxdepack(unsigned char *src, unsigned char *dst) {
  register unsigned char *_a0 __asm__("a0") = src;
  register unsigned char *_a1 __asm__("a1") = dst;
  __asm__ volatile("jsr _doynaxdepack"
                   : "+r"(_a0), "+r"(_a1)
                   :
                   : "cc", "memory");
}

#else

void doynaxdepack(__ASMPARM("a0", unsigned char *src),
                  __ASMPARM("a1", unsigned char *dst));

#endif

#endif
