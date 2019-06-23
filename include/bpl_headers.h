
#ifndef _BPL_HEADERS_
#define _BPL_HEADERS_

#include <tndo_assert.h>

typedef struct {
  unsigned char type, typeb;
  unsigned short w, h;
  unsigned short FILL;
  unsigned char data[1];
} t_bpl_head;

// Data
// 0 -> pal R G B
//[256 * 3] -> bitplanes

#define MODE_BPL (100)
#define MODE_SPR4 (101)
#define MODE_SPR16 (102)

static inline int _get_bpls_size(int w, int h, int nbpls) {
  return (w >> 3) * h * nbpls;
}

static inline int _get_spr_size(int w, int h, int nbpls) {
  tndo_assert((w & 0x3f) == 0);
  tndo_assert((nbpls == 2) || (nbpls == 4));
  return (w >> 3) * (h + 2) * nbpls;
}

#endif // _BPL_HEADERS_
