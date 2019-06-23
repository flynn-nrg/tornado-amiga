
#include <tndo_assert.h>

#include "cpu.h"

#define FAST_MEM 0x00020004
#define CHIP_MEM 0x00020002

int getCPU() {
  return 6; // 060
}

int *getVBR() { return 0; }

int getMEM(int memtype) {
  return 1024 * 1024 * 128; // miles de minolles de MB!
}
