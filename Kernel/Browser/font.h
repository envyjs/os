#ifndef ENVY_BROWSER_FONT_H
#define ENVY_BROWSER_FONT_H
#include "../FS/CDFS/cdfs.h"
void font_init(cdfs *fs);
void font_face(const char *family,const char *sources,const char *stylesheet);
int font_select(const char *families);
int32_t font_advance(int font,uint32_t codepoint,uint32_t size);
void font_draw(int font,int32_t x,int32_t top,uint32_t codepoint,uint32_t size,uint32_t color);
uint32_t font_rasterization_count(void);
#endif
