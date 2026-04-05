#ifndef INCLUDE_ZOOM_INNER_H
#define INCLUDE_ZOOM_INNER_H

#include "asmparm.h"

#ifdef __GCC_ELF__

static inline void renderZoom_asm(unsigned char ***allTxtPtr,
                                  unsigned char *chunky,
                                  zoomIter *iteration) {
  register unsigned char ***_a0 __asm__("a0") = allTxtPtr;
  register unsigned char *_a1 __asm__("a1") = chunky;
  register zoomIter *_a2 __asm__("a2") = iteration;
  __asm__ volatile("jsr _renderZoom_asm"
                   : "+r"(_a0), "+r"(_a1), "+r"(_a2)
                   :
                   : "cc", "memory");
}

#else

void renderZoom_asm(__ASMPARM("a0", unsigned char ***allTxtPtr),
                    __ASMPARM("a1", unsigned char *chunky),
                    __ASMPARM("a2", zoomIter *iteration));

#endif

#endif
