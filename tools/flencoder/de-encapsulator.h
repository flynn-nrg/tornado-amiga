/*
Copyright (c) 2019 Luis Pons

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

#ifndef DE_ENCAPSULATOR_H
#define DE_ENCAPSULATOR_H

#define BLOCK_LEN (128)
// Subgements of len 4 show some (small) potential for 10 bits samples.
#define BLOCK_SUBSEGMENT (8)

#define NUM_COEFF_SETS (128)

static inline int _dmask(int b) { return (1 << b) - 1; }
static inline int _dcent(int b) { return (1 << (b - 1)); }

#define BC0 (6)
#define BC1 (5)
#define BC2 (5)

static inline void unpack_coeff(unsigned int pack, int *c0, int *c1, int *c2) {
  int br0 = pack & 0xff;
  int br1 = (pack >> 8) & 0xff;
  int br2 = (pack >> 16) & 0xff;
  *c0 = (br0 - _dcent(BC0));
  *c1 = (br1 - _dcent(BC1));
  *c2 = (br2 - _dcent(BC2));
}

#define DE_ENCAPSULE_8 (1)
#define DE_ENCAPSULE_16 (2)

void de_encapsulate_channel(void *dst, int out_type, unsigned short *src,
                            int num_samples, int addbits, unsigned int *pal);

// In big endian
typedef struct {
  unsigned char head[4]; // "CAPS"
  unsigned char info[4]; // TBD
  unsigned int nsamples;
  unsigned int sample_rate;
  unsigned int coeffs_pal[NUM_COEFF_SETS];
  unsigned int channel_left_bytes;
  // unsigned short channel_left_data [...];
  // unsigned int  channel_right_bytes;
  // unsigned short channel_right_data [...];
} caps_header_t;

#endif
