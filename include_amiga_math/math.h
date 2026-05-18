#ifndef _MATH_H_
#define _MATH_H_

#ifndef __AMIGA__
#error "only should use for amiga target"
#endif

#if !defined(__GCC__) && !defined(__GCC_ELF__)
#error "only should use for gcc"
#endif

/*
 * This minimal implementation is 100% reliant on the traps
 * to emulate unimplemented instructions, do not use in
 * time-sensitive code.
 */


#include "math-68881.h"
#include "math-68060.h"

/*
 * float wrappers not already provided by math-68060.h
 * (sqrtf, powf, sinf, cosf, modff are in math-68060.h)
 */
static inline float floorf(float x) { return (float)floor((double)x); }
static inline float ceilf(float x)  { return (float)ceil((double)x); }
static inline float rintf(float x)  { return (float)rint((double)x); }
static inline float tanf(float x)   { return (float)tan((double)x); }
static inline float fabsf(float x)  { return (float)fabs((double)x); }
static inline float logf(float x)   { return (float)log((double)x); }
static inline float log10f(float x) { return (float)log10((double)x); }
static inline float expf(float x)   { return (float)exp((double)x); }
static inline float fmodf(float x, float y) { return (float)fmod((double)x, (double)y); }
static inline float atanf(float x)  { return (float)atan((double)x); }
static inline float atan2f(float x, float y) { return (float)atan2((double)x, (double)y); }
static inline float asinf(float x)  { return (float)asin((double)x); }
static inline float acosf(float x)  { return (float)acos((double)x); }

# ifndef HUGE_VAL
#  define HUGE_VAL (__builtin_huge_val())
# endif

# ifndef HUGE_VALF
#  define HUGE_VALF (__builtin_huge_valf())
# endif

# ifndef HUGE_VALL
#  define HUGE_VALL (__builtin_huge_vall())
# endif

# ifndef INFINITY
#  define INFINITY (__builtin_inff())
# endif

# ifndef NAN
#  define NAN (__builtin_nanf(""))
# endif

#endif
