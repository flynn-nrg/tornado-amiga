#ifndef INCLUDE_CPU_H
#define INCLUDE_CPU_H

#include "asmparm.h"

#define FAST_MEM 0x00020004
#define CHIP_MEM 0x00020002

int getCPU();
int isVampire();
int *getVBR();

#ifdef __GCC_ELF__

static inline int getMEM(int memtype) {
  register int _d1 __asm__("d1") = memtype;
  register int _ret __asm__("d0");
  __asm__ volatile("jsr _getMEM"
                   : "=r"(_ret), "+r"(_d1)
                   :
                   : "cc", "memory");
  return _ret;
}

#else

int getMEM(__ASMPARM("d1", int memtype));

#endif

#endif
