#ifndef PTR_BRIDGES_H
#define PTR_BRIDGES_H

#ifdef __AMIGA__
#define ENDI(V) (V)
#else
#define ENDI(V) (((V & 0xff) << 8) | (V >> 8))
#endif

#ifdef __AMIGA__
#define ENDI4(V) (V)
#else
#define ENDI4(V)                                                               \
  (((V)&0xff) << 24) | (((V)&0xff00) << 8) | (((V)&0xff0000) >> 8) |           \
      (((V)&0xff000000) >> 24)
#endif

#ifdef __AMIGA__
#define ENDF(RES, PTR)                                                         \
  { RES = *PTR; }
#else
#define ENDF(RES, PTR)                                                         \
  {                                                                            \
    union {                                                                    \
      unsigned int r;                                                          \
      float f;                                                                 \
    } brgf;                                                                    \
    brgf.r = *(unsigned int *)PTR;                                             \
    brgf.r = ((brgf.r & 0xff) << 24) | ((brgf.r & 0xff00) << 8) |              \
             ((brgf.r & 0xff0000) >> 8) | ((brgf.r & 0xff000000) >> 24);       \
    RES = brgf.f;                                                              \
  }
#endif

// We use long to enable 64 bits systems compatibility

static inline unsigned long ptr_bridge(void *ptr) {
  union {
    void *ptr;
    unsigned long v;
  } tmp;
  tmp.ptr = ptr;
  return tmp.v;
}

static inline void *ptr_bridge_back(unsigned long mem) {
  union {
    void *ptr;
    unsigned long v;
  } tmp;
  tmp.v = mem;
  return tmp.ptr;
}

#define PBRG(PTR) ptr_bridge((void *)(PTR))

#endif
