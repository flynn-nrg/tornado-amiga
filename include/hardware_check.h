#ifndef INCLUDE_HARDWARE_CHECK_H
#define INCLUDE_HARDWARE_CHECK_H

typedef struct {
  int cpu;
  int vampire;
  int *vbr;
  int aga;
  int chip;
  int fast;
} hardware_t;

hardware_t *hardware_check(int, int, int, unsigned int);

#endif
