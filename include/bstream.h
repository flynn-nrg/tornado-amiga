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

#ifndef BSTREAM_H
#define BSTREAM_H

typedef union {
  unsigned int ubits;
  signed int sbits;
} bitbuff_u;

typedef struct {
  unsigned short *ptr;
  int idx;
  int nbits;
  bitbuff_u b;
} bstream_t;

static inline void bstream_rewind(bstream_t *bs) {
  bs->idx = 0;
  bs->nbits = 0;
  bs->b.ubits = 0;
}

static inline void bstream_init(bstream_t *bs, unsigned short *data) {
  bs->ptr = data;
  bstream_rewind(bs);
}

static inline int bstream_reads(bstream_t *bs, int nbits) {
  if (nbits > bs->nbits) {
    unsigned int newb = bs->ptr[bs->idx];
    bs->idx++;
    bs->b.ubits |= newb << (16 - bs->nbits);
    bs->nbits += 16;
  }

  int r = bs->b.sbits >> (32 - nbits);
  bs->b.ubits <<= nbits;
  bs->nbits -= nbits;
  return r;
}

static inline int bstream_readu(bstream_t *bs, int nbits) {
  if (nbits > bs->nbits) {
    unsigned int newb = bs->ptr[bs->idx];
    bs->idx++;
    bs->b.ubits |= newb << (16 - bs->nbits);
    bs->nbits += 16;
  }

  int r = bs->b.ubits >> (32 - nbits);
  bs->b.ubits <<= nbits;
  bs->nbits -= nbits;
  return r;
}

static inline void bstream_write(bstream_t *bs, int bits, int nbits) {
  bits = bits & ((1 << nbits) - 1); // keep sign at bay
  bs->b.ubits |= bits << ((32 - bs->nbits) - nbits);
  bs->nbits += nbits;

  if (bs->nbits > 16) {
    bs->ptr[bs->idx] = bs->b.ubits >> 16;
    bs->idx++;
    bs->b.ubits <<= 16;
    bs->nbits -= 16;
  }
}

static inline void bstream_flush(bstream_t *bs) {
  if (bs->nbits > 0) {
    bs->ptr[bs->idx] = bs->b.ubits >> 16;
    bs->idx++;
    bs->b.ubits = 0;
    bs->nbits = 0;
  }
  bs->ptr[bs->idx] = 0;
  bs->idx++;
}

/*
typedef struct { int v, b, s; } bstest_t;
#define TEST_LEN  (10000000)
void bstest ()
{
    bstream_t bs;
    static bstest_t buffy [TEST_LEN];
    static unsigned short tmp [TEST_LEN/2];

    bstream_init (&bs, tmp);

    int i;
    for (i=0; i<TEST_LEN; i++)
    {
        int nbits = rand() & 7;
        if (nbits == 0)
            nbits = 1;
        int bits  = rand() & ((1 << nbits) - 1);

        buffy[i].s = 0;
        if ((rand() & 1) == 1)
        {
            buffy[i].s = 1;
            int l = 32 - nbits;
            bits = (bits << l) >> l;
        }

        buffy[i].v = bits;
        buffy[i].b = nbits;

        bstream_write (&bs, bits, nbits);
    }

    bstream_flush (&bs);
    bstream_rewind (&bs);
    for (i=0; i<TEST_LEN; i++)
    {
        int nbits = buffy[i].b;
        int bits = 0;
        if (buffy[i].s == 0)
            bits = bstream_readu (&bs, nbits);
        else
            bits = bstream_reads (&bs, nbits);

        assert(bits == buffy[i].v);
    }

}
*/

#endif
