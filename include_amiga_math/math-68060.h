#ifndef __math_68060
#define __math_68060

#ifndef __AMIGA__
#error "only should use for amiga target"
#endif

#ifndef __GCC__
#error "only should use for gcc"
#endif

static inline float modff(float x, float* y) {
    double a,b;
    a = x;
    b = *y;
    a = modf(a, &b);
    *y = b;
    return a;
}

static inline float sqrtf (float x) {
    return sqrt(x);
}

static inline float powf (float x, float y) {
    return pow(x,y);
}
static inline float sinf (float x) {
    return sin(x);
}
static inline float cosf (float x) {
    return cos(x);
}

#endif
