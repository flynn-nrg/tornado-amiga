#ifndef INCLUDE_CPU_H
#define INCLUDE_CPU_H

#include "asmparm.h"

#define FAST_MEM 0x00020004
#define CHIP_MEM 0x00020002

int getCPU();
int isVampire();
int * getVBR();

int getMEM(
    __ASMPARM("d1", int memtype)
);

#endif
