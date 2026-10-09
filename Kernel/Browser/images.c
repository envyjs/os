#include "images.h"
#include "../Drivers/Video/framebuffer.h"
static cdfs *filesystem;static uint8_t source_bytes[8u*1024u*1024u];
#define IMAGE_CACHE_BYTES (16u*1024u*1024u)
#define IMAGE_CACHE_ENTRIES 32
typedef struct {char path[256];image_surface image;uint32_t width,height,offset,bytes,stamp;int vector,active;} cache_entry;
static cache_entry cache[IMAGE_CACHE_ENTRIES];static uint8_t cache_pixels[IMAGE_CACHE_BYTES];static uint32_t cache_stamp,decode_count;
static char lower(char c){return c>='A'&&c<='Z'?(char)(c+32):c;}
static int same(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
int images_resolve(const char *source,const char *base,char path[256]){
    char joined[512];uint32_t n=0;if(*source!='/'){uint32_t last=0;for(uint32_t i=0;base[i];i++)if(base[i]=='/')last=i+1;
        if(last>=sizeof(joined))return 0;for(uint32_t i=0;i<last;i++)joined[n++]=base[i];}
    while(*source){if(*source==':'||*source=='\\'||*source=='?'||*source=='#'||n+1>=sizeof(joined))return 0;joined[n++]=*source++;}joined[n]=0;
    const char *p=joined;uint32_t out=1;path[0]='/';
    while(*p){while(*p=='/')p++;if(!*p)break;const char *start=p;while(*p&&*p!='/')p++;uint32_t size=(uint32_t)(p-start);
        if(size==1&&start[0]=='.')continue;
        if(size==2&&start[0]=='.'&&start[1]=='.'){if(out<=1)return 0;out--;while(out>1&&path[out-1]!='/')out--;continue;}
        if(out+size+1>=256)return 0;for(uint32_t i=0;i<size;i++)path[out++]=start[i];path[out++]='/';}
    if(out>1)out--;path[out]=0;
    return out>6&&lower(path[1])=='e'&&lower(path[2])=='n'&&lower(path[3])=='v'&&lower(path[4])=='y'&&path[5]=='/';
}
void images_init(cdfs *fs){filesystem=fs;for(uint32_t i=0;i<IMAGE_CACHE_ENTRIES;i++)cache[i].active=0;cache_stamp=decode_count=0;image_reset();}
uint32_t images_decode_count(void){return decode_count;}
static void evict(void){uint32_t oldest=IMAGE_CACHE_ENTRIES;for(uint32_t i=0;i<IMAGE_CACHE_ENTRIES;i++)if(cache[i].active&&(oldest==IMAGE_CACHE_ENTRIES||cache[i].stamp<cache[oldest].stamp))oldest=i;
    if(oldest<IMAGE_CACHE_ENTRIES)cache[oldest].active=0;}
static uint32_t cache_space(uint32_t bytes){
    for(;;){uint32_t offset=0;
        for(;;){uint32_t next=IMAGE_CACHE_ENTRIES;
            for(uint32_t i=0;i<IMAGE_CACHE_ENTRIES;i++)if(cache[i].active&&cache[i].offset>=offset&&(next==IMAGE_CACHE_ENTRIES||cache[i].offset<cache[next].offset))next=i;
            uint32_t end=next==IMAGE_CACHE_ENTRIES?IMAGE_CACHE_BYTES:cache[next].offset;
            if(bytes<=end-offset)return offset;if(next==IMAGE_CACHE_ENTRIES)break;offset=cache[next].offset+cache[next].bytes;
        }
        evict();
    }
}
int images_load(const char *source,uint32_t width,uint32_t height,image_surface *result){
    *result=(image_surface){0};char path[256];if(!filesystem||!source||!*source||!images_resolve(source,"/Envy/index.html",path))return 0;
    for(uint32_t i=0;i<IMAGE_CACHE_ENTRIES;i++)if(cache[i].active&&same(path,cache[i].path)&&(!cache[i].vector||(width==cache[i].width&&height==cache[i].height))){cache[i].stamp=++cache_stamp;*result=cache[i].image;return 1;}
    cdfs_node node;uint32_t got;
    if(cdfs_lookup(filesystem,path,&node)||(node.flags&2)||!node.size||node.size>sizeof(source_bytes))return 0;
    if(cdfs_read(filesystem,&node,0,source_bytes,node.size,&got)||got!=node.size)return 0;
    image_surface decoded;decode_count++;if(!image_decode(source_bytes,got,width,height,&decoded))return 0;
    int vector=!(source_bytes[0]==137||(got>=2&&source_bytes[0]==255&&source_bytes[1]==216));
    uint32_t w=decoded.width,h=decoded.height;
    if(!vector)while(w*h>2u*1024u*1024u){w=(w+1)/2;h=(h+1)/2;}
    uint32_t bytes=w*h*4;if(bytes>IMAGE_CACHE_BYTES){*result=decoded;return 1;}
    uint32_t slot=0;for(;slot<IMAGE_CACHE_ENTRIES;slot++)if(!cache[slot].active)break;
    if(slot==IMAGE_CACHE_ENTRIES){evict();for(slot=0;slot<IMAGE_CACHE_ENTRIES;slot++)if(!cache[slot].active)break;}
    uint32_t offset=cache_space(bytes);cache_entry *entry=&cache[slot];
    for(uint32_t y=0;y<h;y++)for(uint32_t x=0;x<w;x++){
        const uint8_t *in=decoded.pixels+((y*decoded.height/h)*decoded.width+x*decoded.width/w)*4;uint8_t *out=cache_pixels+offset+(y*w+x)*4;
        for(uint32_t channel=0;channel<4;channel++)out[channel]=in[channel];}
    uint32_t i=0;do{entry->path[i]=path[i];}while(path[i++]);entry->width=width;entry->height=height;entry->vector=vector;entry->offset=offset;entry->bytes=bytes;entry->stamp=++cache_stamp;entry->active=1;
    entry->image=(image_surface){cache_pixels+offset,w,h,decoded.natural_width,decoded.natural_height};*result=entry->image;return 1;
}
void images_draw(const image_surface *image,int32_t x,int32_t y,uint32_t width,uint32_t height){
    if(!image)return;framebuffer_image(image->pixels,image->width,image->height,x,y,width,height);
}
void images_background(const css_style *style,int32_t x,int32_t y,uint32_t width,uint32_t height){
    const framebuffer *fb=framebuffer_get();
    if(!fb||!style->background_image[0]||!width||!height||width>8192||height>8192||
       x>=(int32_t)fb->width||y>=(int32_t)fb->height||x+(int32_t)width<=0||y+(int32_t)height<=0)return;
    image_surface image;if(!images_load(style->background_image,0,0,&image))return;
    int32_t tile_width=(int32_t)image.natural_width,tile_height=(int32_t)image.natural_height;
    int mode=style->value[CSS_BACKGROUND_SIZE];
    if(mode==CSS_BG_COVER||mode==CSS_BG_CONTAIN){
        int use_width=width*image.natural_height>=height*image.natural_width;if(mode==CSS_BG_CONTAIN)use_width=!use_width;
        if(use_width){tile_width=(int32_t)width;tile_height=(int32_t)((image.natural_height*width+(mode==CSS_BG_COVER?image.natural_width-1:0))/image.natural_width);}
        else{tile_height=(int32_t)height;tile_width=(int32_t)((image.natural_width*height+(mode==CSS_BG_COVER?image.natural_height-1:0))/image.natural_height);}
    }else if(mode==CSS_BG_EXPLICIT){
        int32_t w=style->background_width,h=style->background_height;
        if(w!=CSS_AUTO)tile_width=css_resolve(w,(int32_t)width);if(h!=CSS_AUTO)tile_height=css_resolve(h,(int32_t)height);
        if(w==CSS_AUTO&&h!=CSS_AUTO)tile_width=(int32_t)image.natural_width*tile_height/(int32_t)image.natural_height;
        if(h==CSS_AUTO&&w!=CSS_AUTO)tile_height=(int32_t)image.natural_height*tile_width/(int32_t)image.natural_width;
    }
    if(tile_width<=0||tile_height<=0||tile_width>8192||tile_height>8192)return;
    /* Re-rasterize vector images at their CSS size; raster image dimensions stay intrinsic. */
    if(!images_load(style->background_image,(uint32_t)tile_width,(uint32_t)tile_height,&image))return;
    int32_t origin_x=x+css_resolve(style->value[CSS_BACKGROUND_X],(int32_t)width-tile_width);
    int32_t origin_y=y+css_resolve(style->value[CSS_BACKGROUND_Y],(int32_t)height-tile_height);
    int repeat=style->value[CSS_BACKGROUND_REPEAT];int32_t left=x<0?0:x,top=y<0?0:y,right=x+(int32_t)width,bottom=y+(int32_t)height;
    if(right>(int32_t)fb->width)right=(int32_t)fb->width;if(bottom>(int32_t)fb->height)bottom=(int32_t)fb->height;
    for(int32_t py=top;py<bottom;py++){int32_t dy=py-origin_y;if(repeat&2){dy%=tile_height;if(dy<0)dy+=tile_height;}else if(dy<0||dy>=tile_height)continue;
        uint32_t sy=(uint32_t)dy*image.height/(uint32_t)tile_height;
        for(int32_t px=left;px<right;px++){int32_t dx=px-origin_x;if(repeat&1){dx%=tile_width;if(dx<0)dx+=tile_width;}else if(dx<0||dx>=tile_width)continue;
            uint32_t sx=(uint32_t)dx*image.width/(uint32_t)tile_width;const uint8_t *p=image.pixels+(sy*image.width+sx)*4;
            framebuffer_blend(px,py,(uint32_t)p[0]<<16|(uint32_t)p[1]<<8|p[2],p[3]);}}
}
