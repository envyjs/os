#ifndef ENVY_JS_STRING_H
#define ENVY_JS_STRING_H
#include "../../Image/compat/string.h"
char *envy_strcpy(char *,const char *);
char *envy_strcat(char *,const char *);
char *envy_strrchr(const char *,int);
#define strcpy envy_strcpy
#define strcat envy_strcat
#define strrchr envy_strrchr
#define strerror(n) "File access is provided by the browser"
static inline void *memchr(const void *p,int c,size_t n){const unsigned char *s=p;for(size_t i=0;i<n;i++)if(s[i]==(unsigned char)c)return (void *)(s+i);return NULL;}
#endif
