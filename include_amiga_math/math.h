#ifndef _MATH_H_
#define _MATH_H_

#ifndef __AMIGA__
#error "only should use for amiga target"
#endif

#ifndef __GCC__
#error "only should use for gcc"
#endif

/*
 * This minimal implementation is 100% reliant on the traps
 * to emulate unimplemented instructions, do not use in
 * time-sensitive code.
 */


#include "math-68881.h"
#include "math-68060.h"

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
