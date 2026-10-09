#ifndef ENVY_JS_SETJMP_H
#define ENVY_JS_SETJMP_H
typedef unsigned int jmp_buf[6];
int envy_setjmp(void *) __attribute__((returns_twice));
void envy_longjmp(void *,int) __attribute__((noreturn));
#define setjmp envy_setjmp
#define longjmp envy_longjmp
#endif
