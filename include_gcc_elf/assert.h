#ifndef _GCC_ELF_ASSERT_H
#define _GCC_ELF_ASSERT_H

/*
 * Minimal assert.h for the m68k-amiga-elf-gcc toolchain.
 */

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
extern void abort(void);
#define assert(expr)                                                           \
  ((expr) ? ((void)0) : (void)__assert_fail(#expr, __FILE__, __LINE__))
static inline void __assert_fail(const char *expr, const char *file, int line) {
  (void)expr;
  (void)file;
  (void)line;
  abort();
}
#endif

#endif
