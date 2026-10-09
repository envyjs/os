#include "framebuffer.h"
static framebuffer screen;
static int32_t clip_left,clip_top,clip_right=2147483647,clip_bottom=2147483647;
static uint8_t paint_opacity=255;
void framebuffer_paint_region(int32_t x,int32_t y,uint32_t width,uint32_t height,uint8_t opacity){
    clip_left=x;clip_top=y;clip_right=x+(int32_t)width;clip_bottom=y+(int32_t)height;paint_opacity=opacity;
}
void framebuffer_paint_reset(void){clip_left=clip_top=0;clip_right=2147483647;clip_bottom=2147483647;paint_opacity=255;}
static uint8_t frame_pixels[16u*1024u*1024u];static volatile uint8_t *front_pixels;static int composing;
static uint32_t console_x, console_y;
static int32_t pointer_x,pointer_y;static int pointer_visible,pointer_enabled;
enum { POINTER_WIDTH=18, POINTER_HEIGHT=26 };
/* The top-left pixel is the click hotspot. Include room for the shadow. */
static const char pointer_shape[24][17]={
    "#", "##", "#o#", "#oo#", "#ooo#", "#oooo#",
    "#ooooo#", "#oooooo#", "#ooooooo#", "#oooooooo#",
    "#ooooooooo#", "#oooooooooo#", "#ooooooooooo#",
    "#oooooooooooo#", "#ooooooo#######", "#oooo#oo#",
    "#ooo##oo#", "#oo#  #oo#", "#o#   #oo#", "##     #oo#",
    "#      #oo#", "        #oo#", "        #oo#", "         ##"
};
static uint8_t pointer_saved[POINTER_WIDTH*POINTER_HEIGHT*4];
static uint16_t read16(const uint8_t *p) { return p[0] | (uint16_t)p[1]<<8; }
static uint32_t read32(const uint8_t *p) { return p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
int framebuffer_init(void) {
    screen.pixels=0;front_pixels=0;composing=0;
    framebuffer_paint_reset();
    if(*(volatile uint32_t *)0x5400!=0x31454256) return 0;
    const uint8_t *info=(const uint8_t *)0x5200;
    uint32_t width=read16(info+18), height=read16(info+20), pitch=read16(info+16), address=read32(info+40);
    uint32_t bytes=info[25]/8;
    if((read16(info)&0x91)!=0x91 || info[27]!=6 || !address || !width || !height ||
        width>4096 || height>2048 || (info[25]!=24&&info[25]!=32) ||
        pitch<width*bytes || pitch>16384 || height>0x1000000u/pitch || address>0xffffffffu-height*pitch) return 0;
    uint32_t masks=31;
    if(read16((const uint8_t *)0x5004)>=0x0300) {
        pitch=read16(info+50); masks=54;
        if(pitch<width*bytes||pitch>16384||height>0x1000000u/pitch||address>0xffffffffu-height*pitch) return 0;
    }
    if(height>sizeof(frame_pixels)/pitch||!info[masks]||info[masks]>8||!info[masks+2]||info[masks+2]>8||!info[masks+4]||info[masks+4]>8 ||
        info[masks]+info[masks+1]>info[25]||info[masks+2]+info[masks+3]>info[25]||info[masks+4]+info[masks+5]>info[25]) return 0;
    screen=(framebuffer){(volatile uint8_t *)address,width,height,pitch,bytes,
        info[masks],info[masks+1],info[masks+2],info[masks+3],info[masks+4],info[masks+5]};
    console_x=console_y=8;
    framebuffer_rect(0,0,width,height,0x101820); return 1;
}
const framebuffer *framebuffer_get(void) { return screen.pixels?&screen:0; }
void framebuffer_begin_frame(void){
    if(!screen.pixels||composing)return;framebuffer_cursor_restore();front_pixels=screen.pixels;
    for(uint32_t i=0;i<screen.pitch*screen.height;i++)frame_pixels[i]=front_pixels[i];screen.pixels=frame_pixels;composing=1;
}
void framebuffer_end_frame(void){
    if(!composing)return;
    /* Copy only changed spans after the whole frame is ready. No clear is shown. */
    for(uint32_t y=0;y<screen.height;y++){uint32_t start=y*screen.pitch,end=start+screen.width*screen.bytes_per_pixel;
        while(start<end&&front_pixels[start]==frame_pixels[start])start++;
        while(end>start&&front_pixels[end-1]==frame_pixels[end-1])end--;
        for(uint32_t i=start;i<end;i++)front_pixels[i]=frame_pixels[i];}
    screen.pixels=front_pixels;front_pixels=0;composing=0;framebuffer_cursor_redraw();
}
void framebuffer_cursor_restore(void) {
    if(!pointer_visible||!screen.pixels)return;
    for(int32_t y=0;y<POINTER_HEIGHT;y++)for(int32_t x=0;x<POINTER_WIDTH;x++) {
        int32_t px=pointer_x+x,py=pointer_y+y;if(px<0||py<0||(uint32_t)px>=screen.width||(uint32_t)py>=screen.height)continue;
        volatile uint8_t *pixel=screen.pixels+(uint32_t)py*screen.pitch+(uint32_t)px*screen.bytes_per_pixel;
        for(uint32_t b=0;b<screen.bytes_per_pixel;b++)pixel[b]=pointer_saved[((uint32_t)y*POINTER_WIDTH+(uint32_t)x)*screen.bytes_per_pixel+b];
    }
    pointer_visible=0;
}
void framebuffer_cursor_redraw(void) {
    if(!pointer_enabled||pointer_visible||!screen.pixels)return;
    for(int32_t y=0;y<POINTER_HEIGHT;y++)for(int32_t x=0;x<POINTER_WIDTH;x++) {
        int32_t px=pointer_x+x,py=pointer_y+y;if(px<0||py<0||(uint32_t)px>=screen.width||(uint32_t)py>=screen.height)continue;
        volatile uint8_t *pixel=screen.pixels+(uint32_t)py*screen.pitch+(uint32_t)px*screen.bytes_per_pixel;
        for(uint32_t b=0;b<screen.bytes_per_pixel;b++)pointer_saved[((uint32_t)y*POINTER_WIDTH+(uint32_t)x)*screen.bytes_per_pixel+b]=pixel[b];
    }
    /* Save the entire backing first so neither outline nor shadow leaves trails. */
    for(int32_t y=0;y<24;y++)for(int32_t x=0;x<16;x++)
        if(pointer_shape[y][x]=='#'||pointer_shape[y][x]=='o')
            framebuffer_blend(pointer_x+x+1,pointer_y+y+2,0,80);
    for(int32_t y=0;y<24;y++)for(int32_t x=0;x<16;x++) {
        char ink=pointer_shape[y][x];
        if(ink=='#'||ink=='o')framebuffer_pixel(pointer_x+x,pointer_y+y,ink=='#'?0x15191e:0xffffff);
    }
    pointer_visible=1;
}
void framebuffer_cursor_move(int32_t x,int32_t y){framebuffer_cursor_restore();pointer_x=x;pointer_y=y;pointer_enabled=1;framebuffer_cursor_redraw();}
static void put_pixel(int32_t x,int32_t y,uint32_t rgb) {
    if(!screen.pixels || x<0 || y<0 || (uint32_t)x>=screen.width || (uint32_t)y>=screen.height) return;
    uint32_t value=(((rgb>>16)&255)>>(8-screen.red_bits))<<screen.red_shift |
        (((rgb>>8)&255)>>(8-screen.green_bits))<<screen.green_shift |
        ((rgb&255)>>(8-screen.blue_bits))<<screen.blue_shift;
    volatile uint8_t *pixel=screen.pixels+(uint32_t)y*screen.pitch+(uint32_t)x*screen.bytes_per_pixel;
    for(uint32_t i=0;i<screen.bytes_per_pixel;i++) pixel[i]=(uint8_t)(value>>(8*i));
}
void framebuffer_pixel(int32_t x,int32_t y,uint32_t rgb){framebuffer_blend(x,y,rgb,255);}
void framebuffer_blend(int32_t x,int32_t y,uint32_t rgb,uint8_t coverage) {
    if(x<clip_left||y<clip_top||x>=clip_right||y>=clip_bottom)return;
    coverage=(uint8_t)(((uint32_t)coverage*paint_opacity+127)/255);
    if(!screen.pixels||x<0||y<0||(uint32_t)x>=screen.width||(uint32_t)y>=screen.height||!coverage)return;
    if(coverage==255){put_pixel(x,y,rgb);return;}
    volatile uint8_t *p=screen.pixels+(uint32_t)y*screen.pitch+(uint32_t)x*screen.bytes_per_pixel;uint32_t old=0;
    for(uint32_t i=0;i<screen.bytes_per_pixel;i++)old|=(uint32_t)p[i]<<(i*8);
    uint32_t result=0;uint8_t bits[3]={screen.blue_bits,screen.green_bits,screen.red_bits},shifts[3]={screen.blue_shift,screen.green_shift,screen.red_shift};
    for(uint32_t i=0;i<3;i++){uint32_t mask=(1u<<bits[i])-1,background=((old>>shifts[i])&mask)*255/mask;
        result|=((((rgb>>(i*8))&255)*coverage+background*(255-coverage)+127)/255)<<(i*8);}
    put_pixel(x,y,result);
}
void framebuffer_rect(int32_t x,int32_t y,uint32_t width,uint32_t height,uint32_t rgb) {
    if(!screen.pixels || x>=(int32_t)screen.width || y>=(int32_t)screen.height ||
        !width || !height) return;
    if(x<0) { uint32_t clipped=(uint32_t)(-(int64_t)x); if(clipped>=width) return; width-=clipped; x=0; }
    if(y<0) { uint32_t clipped=(uint32_t)(-(int64_t)y); if(clipped>=height) return; height-=clipped; y=0; }
    if(width>screen.width-(uint32_t)x) width=screen.width-(uint32_t)x;
    if(height>screen.height-(uint32_t)y) height=screen.height-(uint32_t)y;
    for(uint32_t row=0;row<height;row++) for(uint32_t col=0;col<width;col++)
        framebuffer_pixel(x+(int32_t)col,y+(int32_t)row,rgb);
}
static void glyph(int32_t x,int32_t y,uint8_t character,uint32_t rgb) {
    const uint8_t *font=(const uint8_t *)0x6000+character*16;
    for(int32_t row=0;row<16;row++) for(int32_t col=0;col<8;col++)
        if(font[row]&(0x80>>col)) framebuffer_pixel(x+col,y+row,rgb);
}
void framebuffer_text(int32_t x,int32_t y,const char *text,uint32_t rgb) {
    int32_t start=x;
    for(uint32_t i=0;text[i] && i<4096;i++) {
        if(text[i]=='\n') { y+=18; x=start; }
        else if(text[i]!='\r') { glyph(x,y,(uint8_t)text[i],rgb); x+=8; }
    }
}
void framebuffer_text_size(int32_t x,int32_t y,const char *text,uint32_t rgb,uint32_t height) {
    if(!height||height>256)return;uint32_t width=(height+1)/2;
    for(uint32_t i=0;text[i]&&i<4096;i++,x+=(int32_t)width) {
        const uint8_t *font=(const uint8_t *)0x6000+(uint8_t)text[i]*16;
        for(uint32_t row=0;row<height;row++)for(uint32_t col=0;col<width;col++)
            if(font[row*16/height]&(0x80>>(col*8/width)))framebuffer_pixel(x+(int32_t)col,y+(int32_t)row,rgb);
    }
}
void framebuffer_console_write(const char *text) {
    if(!screen.pixels) return;
    for(;*text;text++) {
        if(*text=='\n'||console_x+8>screen.width) { console_x=8; console_y+=18; if(*text=='\n') continue; }
        if(console_y+16>screen.height) { framebuffer_rect(0,0,screen.width,screen.height,0x101820); console_x=console_y=8; }
        if(*text!='\r') { glyph((int32_t)console_x,(int32_t)console_y,(uint8_t)*text,0xe8edf2); console_x+=8; }
    }
}
