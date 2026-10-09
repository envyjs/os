#ifndef ENVY_BROWSER_IMAGES_H
#define ENVY_BROWSER_IMAGES_H
#include "../FS/CDFS/cdfs.h"
#include "../Runtime/Image/image.h"
#include "css.h"
void images_init(cdfs *fs);
uint32_t images_decode_count(void);
int images_resolve(const char *source,const char *base,char path[256]);
int images_load(const char *source,uint32_t width,uint32_t height,image_surface *result);
void images_draw(const image_surface *image,int32_t x,int32_t y,uint32_t width,uint32_t height);
void images_background(const css_style *style,int32_t x,int32_t y,uint32_t width,uint32_t height);
#endif
