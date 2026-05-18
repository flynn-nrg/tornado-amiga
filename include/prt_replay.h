#ifndef PRT_REPLAY_H
#define PRT_REPLAY_H

#ifdef __AMIGA__

#include "asmparm.h"

#ifdef __GCC_ELF__

static inline int prtInit(void *chip_mem, void *prt_data) {
  register void *_a1 __asm__("a1") = chip_mem;
  register void *_a2 __asm__("a2") = prt_data;
  register int _ret __asm__("d0");
  __asm__ volatile("jsr _prtInit"
                   : "=r"(_ret), "+r"(_a1), "+r"(_a2)
                   :
                   : "cc", "memory");
  return _ret;
}

#else

int prtInit(__ASMPARM("a1", void *chip_mem), __ASMPARM("a2", void *prt_data));

#endif

#else
int prtInit(void *chip_mem, void *prt_data);
#endif

void prtEnd(void);
void prtVBL(void);
void *getPrtVBL(void);
#endif
