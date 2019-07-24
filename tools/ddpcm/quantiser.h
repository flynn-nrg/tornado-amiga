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

#ifndef QUANTISER_H
#define QUANTISER_H

#define DELTA_MODE_WEIGHTED 0
#define DELTA_MODE_TOP 1

typedef struct {
  int16_t value;
  uint32_t score;
} q_entry;

int16_t *quantiser(int16_t *data, uint32_t numSamples, uint32_t deltaMode);
int16_t predict(int16_t y1, int16_t y2);

#endif
