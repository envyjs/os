#ifndef ENVY_JS_STDLIB_H
#define ENVY_JS_STDLIB_H
#include <stddef.h>
void *envy_js_alloc(void *,void *,int);
void envy_js_abort(void) __attribute__((noreturn));
int envy_atoi(const char *);
#define abort envy_js_abort
#define atoi envy_atoi
#define realloc(p,n) envy_js_alloc(NULL,(p),(int)(n))
#define malloc(n) envy_js_alloc(NULL,NULL,(int)(n))
#define free(p) ((void)envy_js_alloc(NULL,(p),0))
#define alloca(n) __builtin_alloca(n)
static inline int abs(int n){return n<0?-n:n;}
#endif
