#ifndef ENVY_JS_STDIO_H
#define ENVY_JS_STDIO_H
#include <stddef.h>
#include <stdarg.h>
typedef struct { int unused; } FILE;
#define stderr ((FILE *)0)
#define stdout ((FILE *)0)
#define EOF (-1)
#define SEEK_END 2
#define SEEK_SET 0
int envy_vsnprintf(char *,size_t,const char *,va_list);
int envy_snprintf(char *,size_t,const char *,...);
int envy_sprintf(char *,const char *,...);
int envy_printf(const char *,...);
int envy_fputs(const char *,FILE *);
int envy_fputc(int,FILE *);
#define vsnprintf envy_vsnprintf
#define snprintf envy_snprintf
#define sprintf envy_sprintf
#define printf envy_printf
#define fputs envy_fputs
#define fputc envy_fputc
#define putchar(c) envy_fputc((c),stderr)
/* Scripts are supplied by the browser's CDFS loader, never libc file APIs. */
static inline FILE *fopen(const char *p,const char *m){(void)p;(void)m;return NULL;}
static inline int fclose(FILE *f){(void)f;return 0;}
static inline int fseek(FILE *f,long n,int w){(void)f;(void)n;(void)w;return -1;}
static inline long ftell(FILE *f){(void)f;return -1;}
static inline size_t fread(void *p,size_t n,size_t c,FILE *f){(void)p;(void)n;(void)c;(void)f;return 0;}
static inline int ferror(FILE *f){(void)f;return 1;}
int envy_fprintf(FILE *,const char *,...);
#define fprintf envy_fprintf
static inline size_t fwrite(const void *p,size_t n,size_t c,FILE *f){const char *s=p;for(size_t i=0;i<n*c;i++)envy_fputc(s[i],f);return c;}
#endif
