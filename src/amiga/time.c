/*
Copyright (c) 2019 Miguel Mendez

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software in a
product, an acknowledgment in the product documentation would be appreciated but
is not required.

    2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <clib/exec_protos.h>
#include <clib/timer_protos.h>
#include <devices/timer.h>

struct Device *TimerBase;
static struct IORequest timereq;
static uint32_t time_available = 0;

void timeInit(void) {
  LONG error;
  error = OpenDevice(TIMERNAME, 0, &timereq, 0);
  if (error) {
    fprintf(stderr, "FATAL - Could not open timer.device! Time services will "
                    "not be available.\n");
  } else {
    time_available = 1;
    TimerBase = timereq.io_Device;
  }
}

void timeEnd(void) {
  if (time_available) {
    CloseDevice(&timereq);
  }
}

void timeGet(struct timeval *t) {
  if (time_available) {
    GetSysTime(t);
  }
}

uint32_t timeDiffSec(struct timeval *t1, struct timeval *t2) {
  if (time_available) {
    SubTime(t2, t1);
    return (uint32_t)t2->tv_secs;
  } else {
    return 0;
  }
}
