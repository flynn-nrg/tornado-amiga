/*
Copyright (c) 2019, Luis Pons. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
