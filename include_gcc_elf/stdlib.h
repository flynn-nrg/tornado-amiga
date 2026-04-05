#ifndef _GCC_ELF_STDLIB_H
#define _GCC_ELF_STDLIB_H

/*
 * Minimal stdlib.h for the m68k-amiga-elf-gcc toolchain.
 * Function implementations are provided by vc.lib / amiga.lib at link time.
 */

#include <stddef.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void exit(int status);
void abort(void);
int atoi(const char *nptr);
long atol(const char *nptr);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
int abs(int j);
long labs(long j);

#endif
