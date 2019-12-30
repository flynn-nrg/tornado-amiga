#ifndef PRT_REPLAY_H
#define PRT_REPLAY_H

#ifdef __AMIGA__

#include "asmparm.h"

int prtInit(__ASMPARM("a1", void *chip_mem), __ASMPARM("a2", void *prt_data));

#else
int prtInit(void *chip_mem, void *prt_data);
#endif

void prtEnd(void);
void prtVBL(void);
void *getPrtVBL(void);
#endif
