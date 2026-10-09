#include "image.h"
#include "support.h"
#include "compat/stdlib.h"
#include "compat/string.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_SIMD
#define STBI_NO_THREAD_LOCALS
#define STBI_NO_FAILURE_STRINGS
#define STBI_ASSERT(x) ((void)0)
#define STBI_MAX_DIMENSIONS 8192
#define STBI_JPEG_MAX_OUTPUT_DIMENSION 2048
#define STBI_MALLOC(n) image_alloc(n)
#define STBI_REALLOC(p,n) image_realloc(p,n)
#define STBI_FREE(p) image_free(p)
#include "vendor/stb_image.h"
#define NSVG_NO_STDIO
#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "vendor/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "vendor/nanosvgrast.h"
void image_reset(void){image_heap_reset();}
uint32_t image_generation(void){return image_heap_generation();}
static int valid(int32_t w,int32_t h){return w>0&&h>0&&w<=8192&&h<=8192&&(uint32_t)w<=33554432u/(uint32_t)h;}
int image_decode(const void *data,uint32_t size,uint32_t width,uint32_t height,image_surface *out){
    *out=(image_surface){0};image_heap_reset();if(!data||!size||size>8u*1024u*1024u||width>8192||height>8192)return 0;
    const uint8_t *bytes=data;int w,h,channels;
    if((size>=8&&bytes[0]==137&&bytes[1]=='P'&&bytes[2]=='N'&&bytes[3]=='G')||(size>=2&&bytes[0]==255&&bytes[1]==216)){
        if(!stbi_info_from_memory(bytes,(int)size,&w,&h,&channels)||!valid(w,h))return 0;
        uint32_t natural_width=(uint32_t)w,natural_height=(uint32_t)h;
        uint8_t *pixels=stbi_load_from_memory(bytes,(int)size,&w,&h,&channels,4);
        if(!pixels||image_heap_failed())return 0;
        *out=(image_surface){pixels,(uint32_t)w,(uint32_t)h,natural_width,natural_height};return 1;
    }
    char *xml=image_alloc(size+1);if(!xml)return 0;memcpy(xml,data,size);xml[size]=0;
    if(!strstr(xml,"<svg"))return 0;
    NSVGimage *svg=nsvgParse(xml,"px",96);if(!svg)return 0;
    if(!(svg->width>0&&svg->height>0&&svg->width<=8192&&svg->height<=8192)||image_heap_failed()){nsvgDelete(svg);return 0;}
    uint32_t natural_width=(uint32_t)ceilf(svg->width),natural_height=(uint32_t)ceilf(svg->height);
    float scale=1;if(width||height){float sx=width?(float)width/svg->width:0,sy=height?(float)height/svg->height:0;scale=sx>sy?sx:sy;}
    float scaled_width=ceilf(svg->width*scale),scaled_height=ceilf(svg->height*scale);
    if(!(scaled_width>0&&scaled_height>0&&scaled_width<=8192&&scaled_height<=8192)){nsvgDelete(svg);return 0;}
    w=(int)scaled_width;h=(int)scaled_height;if(!valid(w,h)){nsvgDelete(svg);return 0;}
    uint8_t *pixels=image_alloc((size_t)w*(size_t)h*4);NSVGrasterizer *raster=nsvgCreateRasterizer();
    if(!pixels||!raster){nsvgDelete(svg);return 0;}memset(pixels,0,(size_t)w*(size_t)h*4);
    nsvgRasterize(raster,svg,0,0,scale,pixels,w,h,w*4);nsvgDeleteRasterizer(raster);nsvgDelete(svg);
    if(image_heap_failed())return 0;
    *out=(image_surface){pixels,(uint32_t)w,(uint32_t)h,natural_width,natural_height};return 1;
}
