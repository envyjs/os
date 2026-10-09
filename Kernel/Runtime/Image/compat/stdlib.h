#ifndef ENVY_IMAGE_STDLIB_H
#define ENVY_IMAGE_STDLIB_H
#include <stddef.h>
void *image_alloc(size_t size);
void *image_realloc(void *pointer,size_t size);
void image_free(void *pointer);
long image_strtol(const char *text,char **end,int base);
long long image_strtoll(const char *text,char **end,int base);
void image_qsort(void *base,size_t count,size_t size,int (*compare)(const void *,const void *));
#define malloc image_alloc
#define realloc image_realloc
#define free image_free
#define strtol image_strtol
#define strtoll image_strtoll
#define qsort image_qsort
#endif
