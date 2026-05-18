#ifndef _GCC_ELF_STDIO_H
#define _GCC_ELF_STDIO_H

/*
 * Minimal stdio.h for the m68k-amiga-elf-gcc toolchain.
 * Function implementations are provided by vc.lib / amiga.lib at link time.
 */

#include <stdarg.h>
#include <stddef.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef EOF
#define EOF (-1)
#endif

#define SEEK_SET (-1)
#define SEEK_CUR  0
#define SEEK_END  1

typedef struct _FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int printf(const char *fmt, ...);
int fprintf(FILE *stream, const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int vfprintf(FILE *stream, const char *fmt, va_list ap);
int vsprintf(char *buf, const char *fmt, va_list ap);
int puts(const char *s);
int fputs(const char *s, FILE *stream);
int fputc(int c, FILE *stream);
int putchar(int c);
int getchar(void);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
int fflush(FILE *stream);
int sscanf(const char *str, const char *fmt, ...);
int feof(FILE *stream);
int ferror(FILE *stream);
void rewind(FILE *stream);
int remove(const char *path);
int rename(const char *oldpath, const char *newpath);

#endif
