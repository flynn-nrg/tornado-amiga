#ifndef INCLUDE_PAULA_OUTPUT_H
#define INCLUDE_PAULA_OUTPUT_H

#include "asmparm.h"

#define OUTPUT_8_BIT_MONO 0
#define OUTPUT_14_BIT_MONO 1
#define OUTPUT_8_BIT_STEREO 2
#define OUTPUT_14_BIT_STEREO 3

#define REPLAY_PERIOD_11025 322
#define REPLAY_PERIOD_22050 161
#define REPLAY_PERIOD_28867 124
#define REPLAY_PERIOD_28150 126

#ifdef __GCC_ELF__

static inline void PaulaOutput_Init(void *mixRoutine, char **mixState,
                                    char **mixState2, int replayPeriod,
                                    int mode) {
  register void *_a0 __asm__("a0") = mixRoutine;
  register char **_a1 __asm__("a1") = mixState;
  register char **_a2 __asm__("a2") = mixState2;
  register int _d0 __asm__("d0") = replayPeriod;
  register int _d1 __asm__("d1") = mode;
  __asm__ volatile("jsr _PaulaOutput_Init"
                   : "+r"(_a0), "+r"(_a1), "+r"(_a2), "+r"(_d0), "+r"(_d1)
                   :
                   : "cc", "memory");
}

#else

void PaulaOutput_Init(__ASMPARM("a0", void *mixRoutine),
                      __ASMPARM("a1", char **mixState),
                      __ASMPARM("a2", char **mixState2),
                      __ASMPARM("d0", int replayPeriod),
                      __ASMPARM("d1", int mode));

#endif

void PaulaOutput_Start(void);
void PaulaOutput_ShutDown(void);
void PaulaOutput_VertBCallback(void);
void *Get_PaulaOutput_VertBCallback(void);

#endif
