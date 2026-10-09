#ifndef ENVY_TTF_H
#define ENVY_TTF_H
#include <stdint.h>
typedef struct {const uint8_t *data;uint32_t size,cmap,cmap_size,glyf,glyf_size,loca,hmtx;
    uint16_t units,glyphs,metrics;int16_t ascent,descent;int long_loca;} ttf_font;
/* Font bytes remain owned by the caller. No allocation, floating point or hint VM. */
int ttf_init(ttf_font *font,const void *data,uint32_t size);
uint32_t ttf_glyph(const ttf_font *font,uint32_t codepoint);
int32_t ttf_advance(const ttf_font *font,uint32_t glyph,uint32_t pixels);
typedef void (*ttf_pixel)(void *context,int32_t x,int32_t y,uint8_t coverage);
/* Coordinates are relative to the pen and baseline. Scratch storage is shared. */
int ttf_render(const ttf_font *font,uint32_t glyph,uint32_t pixels,ttf_pixel pixel,void *context);
uint32_t ttf_utf8(const char **text);
#endif
