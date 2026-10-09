#ifndef ENVY_IMAGE_STRING_H
#define ENVY_IMAGE_STRING_H
#include <stddef.h>
void *memcpy(void *,const void *,size_t);
void *memmove(void *,const void *,size_t);
void *memset(void *,int,size_t);
int image_memcmp(const void *,const void *,size_t);
size_t image_strlen(const char *);
int image_strcmp(const char *,const char *);
int image_strncmp(const char *,const char *,size_t);
char *image_strncpy(char *,const char *,size_t);
char *image_strchr(const char *,int);
char *image_strstr(const char *,const char *);
#define memcmp image_memcmp
#define strlen image_strlen
#define strcmp image_strcmp
#define strncmp image_strncmp
#define strncpy image_strncpy
#define strchr image_strchr
#define strstr image_strstr
#endif
