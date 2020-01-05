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

void PaulaOutput_Init(__ASMPARM("a0", void *mixRoutine),
                      __ASMPARM("a1", char **mixState),
                      __ASMPARM("a2", char **mixState2),
                      __ASMPARM("d0", int replayPeriod),
                      __ASMPARM("d1", int mode));

void PaulaOutput_Start(void);
void PaulaOutput_ShutDown(void);
void PaulaOutput_VertBCallback(void);
void *Get_PaulaOutput_VertBCallback(void);

#endif
