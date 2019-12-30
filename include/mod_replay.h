#ifndef MOD_REPLAY_H
#define MOD_REPLAY_H

#ifdef __AMIGA__

#include "asmparm.h"

int p61Init(__ASMPARM("a0", void *module),
            __ASMPARM("a1", void *external_samples),
            __ASMPARM("a2", void *sample_buffer),
            __ASMPARM("d0", int cia_timer));

int p61InitOSLegal(__ASMPARM("a0", void *module),
                   __ASMPARM("a1", void *external_samples),
                   __ASMPARM("a2", void *sample_buffer),
                   __ASMPARM("d0", int cia_timer));

void p61SetVolOSLegal(__ASMPARM("d0", int volume));

#else
int p61Init(void *module, void *external_samples, void *sample_buffer,
            int cia_timer);
#endif

void p61End(void);
void p61EndOSLegal(void);

#endif
