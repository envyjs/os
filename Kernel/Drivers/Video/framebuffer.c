#include "framebuffer.h"
#include "../../Arch/x86/io.h"
#include "../../Runtime/Image/image.h"
static framebuffer screen;
extern const uint8_t bootlogo_start[],bootlogo_end[];
static image_surface boot_logo;
static uint32_t splash_step;
static int splash_active,splash_deferred_scene;
static int32_t splash_previous_left,splash_previous_right,splash_bar_y;
static uint16_t splash_pit_last;static uint32_t splash_pit_accum;static int splash_irq_enabled;
static const uint16_t splash_positions[48]={0,0,0,1,3,5,9,14,20,27,35,45,57,69,83,98,115,132,150,169,188,209,229,250,270,291,311,332,351,370,388,405,422,437,451,463,475,485,493,500,506,511,515,517,519,520,520,520};
static uint16_t splash_pit_counter(void){out8(0x43,0);uint16_t low=in8(0x40);return low|(uint16_t)in8(0x40)<<8;}
static int32_t clip_left,clip_top,clip_right=2147483647,clip_bottom=2147483647;
static uint8_t paint_opacity=255;
void framebuffer_paint_region(int32_t x,int32_t y,uint32_t width,uint32_t height,uint8_t opacity){
    clip_left=x;clip_top=y;clip_right=x+(int32_t)width;clip_bottom=y+(int32_t)height;paint_opacity=opacity;
}
void framebuffer_paint_reset(void){clip_left=clip_top=0;clip_right=2147483647;clip_bottom=2147483647;paint_opacity=255;}
static uint8_t frame_pixels[16u*1024u*1024u];static volatile uint8_t *front_pixels;static int composing;
static int rgb32;
static uint32_t published_bytes;
static uint32_t blur_pixels;
static int damage_enabled,damage_count;static int32_t damage_left[2],damage_top[2],damage_right[2],damage_bottom[2];
static void copy_bytes(volatile uint8_t *destination,const volatile uint8_t *source,uint32_t count){
    uint32_t words=count/4;__asm__ volatile("cld; rep movsl":"+D"(destination),"+S"(source),"+c"(words)::"memory");
    for(uint32_t i=0;i<(count&3);i++)destination[i]=source[i];
}
static void copy_region(volatile uint8_t *to,const volatile uint8_t *from,int32_t left,int32_t top,int32_t right,int32_t bottom){
    if(left<0)left=0;if(top<0)top=0;if(right>(int32_t)screen.width)right=(int32_t)screen.width;if(bottom>(int32_t)screen.height)bottom=(int32_t)screen.height;
    if(left>=right||top>=bottom)return;uint32_t bytes=(uint32_t)(right-left)*screen.bytes_per_pixel;
    for(int32_t y=top;y<bottom;y++){uint32_t offset=(uint32_t)y*screen.pitch+(uint32_t)left*screen.bytes_per_pixel;copy_bytes(to+offset,from+offset,bytes);}
}
static uint32_t native_color(uint32_t rgb){
    if(rgb32)return rgb&0xffffffu;
    return (((rgb>>16)&255)>>(8-screen.red_bits))<<screen.red_shift |
        (((rgb>>8)&255)>>(8-screen.green_bits))<<screen.green_shift |
        ((rgb&255)>>(8-screen.blue_bits))<<screen.blue_shift;
}
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
static void copy_pointer_region(volatile uint8_t *to,const volatile uint8_t *from){
    if(!pointer_enabled)return;copy_region(to,from,pointer_x,pointer_y,pointer_x+POINTER_WIDTH,pointer_y+POINTER_HEIGHT);
}
static uint16_t read16(const uint8_t *p) { return p[0] | (uint16_t)p[1]<<8; }
static uint32_t read32(const uint8_t *p) { return p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
enum { VBE_DISPI_INDEX=0x1ce,VBE_DISPI_DATA=0x1cf,VBE_ID=0,VBE_XRES=1,VBE_YRES=2,VBE_BPP=3,VBE_ENABLE=4,VBE_VIRT_WIDTH=6,VBE_VIDEO_MEMORY=10 };
static int vbe_dispi;
static uint16_t vbe_read(uint16_t index){out16(VBE_DISPI_INDEX,index);return in16(VBE_DISPI_DATA);}
static void vbe_write(uint16_t index,uint16_t value){out16(VBE_DISPI_INDEX,index);out16(VBE_DISPI_DATA,value);}
static uint32_t boot_mode_count(void){uint32_t count=read16((const uint8_t *)0x54f0);return count>128?128:count;}
static int boot_mode_get(uint32_t index,uint32_t *width,uint32_t *height){
    if(index>=boot_mode_count()||!width||!height)return 0;const uint8_t *mode=(const uint8_t *)(0x5500+index*4);*width=read16(mode);*height=read16(mode+2);return *width&&*height;
}
static const uint16_t standard_modes[][2]={{640,480},{800,600},{1024,768},{1280,720},{1280,800},{1280,1024},{1366,768},{1440,900},{1600,900},{1600,1200},{1920,1080}};
static int boot_mode_contains(uint32_t width,uint32_t height){for(uint32_t i=0;i<boot_mode_count();i++){uint32_t w,h;if(boot_mode_get(i,&w,&h)&&w==width&&h==height)return 1;}return 0;}
static int mode_fits(uint32_t width,uint32_t height){
    if(!width||!height||width>4096||height>2048||!screen.bytes_per_pixel)return 0;
    uint32_t pitch=width*screen.bytes_per_pixel;
    if(pitch>16384||height>sizeof(frame_pixels)/pitch)return 0;
    if(vbe_dispi){uint32_t memory=(uint32_t)vbe_read(VBE_VIDEO_MEMORY)*65536u;
        if(memory&&(uint64_t)pitch*height>memory)return 0;}
    return 1;
}
static int resolution_at(uint32_t wanted,uint32_t *width,uint32_t *height){
    if(!width||!height||!screen.pixels)return 0;if(!vbe_dispi){if(wanted)return 0;*width=screen.width;*height=screen.height;return 1;}
    uint32_t index=0;for(uint32_t i=0;i<boot_mode_count();i++){uint32_t w,h;if(!boot_mode_get(i,&w,&h)||!mode_fits(w,h))continue;if(index++==wanted){*width=w;*height=h;return 1;}}
    for(uint32_t i=0;i<sizeof(standard_modes)/sizeof(standard_modes[0]);i++){uint32_t w=standard_modes[i][0],h=standard_modes[i][1];if(boot_mode_contains(w,h)||!mode_fits(w,h))continue;if(index++==wanted){*width=w;*height=h;return 1;}}
    int current_found=boot_mode_contains(screen.width,screen.height);for(uint32_t i=0;i<sizeof(standard_modes)/sizeof(standard_modes[0]);i++)if(standard_modes[i][0]==screen.width&&standard_modes[i][1]==screen.height)current_found=1;
    if(!current_found&&index==wanted){*width=screen.width;*height=screen.height;return 1;}return 0;
}
int framebuffer_init(void) {
    screen.pixels=0;front_pixels=0;composing=0;damage_enabled=0;
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
    vbe_dispi=vbe_read(VBE_ID)>=0xb0c0;
    rgb32=bytes==4&&screen.red_bits==8&&screen.red_shift==16&&screen.green_bits==8&&screen.green_shift==8&&screen.blue_bits==8&&screen.blue_shift==0;
    console_x=console_y=8;
    framebuffer_rect(0,0,width,height,0x000000); return 1;
}
void framebuffer_boot_splash_start(void){
    if(!screen.pixels)return;
    if(screen.width!=1024||screen.height!=768)framebuffer_set_resolution(1024,768);
    const framebuffer *fb=framebuffer_get();if(!fb)return;
    splash_active=1;splash_deferred_scene=0;splash_step=0;splash_pit_accum=0;splash_pit_last=splash_pit_counter();splash_previous_left=splash_previous_right=0;framebuffer_rect(0,0,fb->width,fb->height,0x000000);
    uint32_t bytes=(uint32_t)(bootlogo_end-bootlogo_start);
    if(!image_decode(bootlogo_start,bytes,0,0,&boot_logo)){boot_logo=(image_surface){0};return;}
    uint32_t width=boot_logo.width,height=boot_logo.height;
    if(width>560||height>300){uint32_t scale_w=560u*65536u/width,scale_h=300u*65536u/height,scale=scale_w<scale_h?scale_w:scale_h;
        width=width*scale/65536u;height=height*scale/65536u;}
    framebuffer_image(boot_logo.pixels,boot_logo.width,boot_logo.height,
        ((int32_t)fb->width-(int32_t)width)/2,((int32_t)fb->height-(int32_t)height)/2-70,width,height);
    splash_bar_y=(int32_t)(fb->height+height)/2+14;if(splash_bar_y+6>(int32_t)fb->height-20)splash_bar_y=(int32_t)fb->height-32;
    framebuffer_rect((int32_t)(fb->width-380)/2,splash_bar_y,380,6,0x282828);
    framebuffer_boot_splash_update();
}
static void splash_draw_frame(void){
    const framebuffer *fb=framebuffer_get();if(!fb)return;
    int32_t old_left=clip_left,old_top=clip_top,old_right=clip_right,old_bottom=clip_bottom;uint8_t old_opacity=paint_opacity;
    volatile uint8_t *draw_pixels=screen.pixels;if(front_pixels)screen.pixels=front_pixels;framebuffer_paint_reset();
    int32_t x=((int32_t)fb->width-380)/2,y=splash_bar_y;
    if(splash_previous_left<splash_previous_right)framebuffer_rect(splash_previous_left,y,(uint32_t)(splash_previous_right-splash_previous_left),6,0x282828);
    uint32_t phase=splash_step%144u,index=phase/3u,remainder=phase%3u;
    uint32_t first=splash_positions[index],second=index==47u?first:splash_positions[index+1u];
    uint32_t position=(first*(3u-remainder)+second*remainder)/3u;int32_t segment_x=x+(int32_t)position-140;
    int32_t left=segment_x<x?x:segment_x,right=segment_x+140>x+380?x+380:segment_x+140;
    if(left<right)framebuffer_rect(left,y,(uint32_t)(right-left),6,0x68b5ff);
    splash_previous_left=left;splash_previous_right=right;
    screen.pixels=draw_pixels;clip_left=old_left;clip_top=old_top;clip_right=old_right;clip_bottom=old_bottom;paint_opacity=old_opacity;
}
int framebuffer_boot_splash_update(void){
    if(!splash_active)return 0;if(splash_irq_enabled)return 1;const framebuffer *fb=framebuffer_get();if(!fb)return 0;
    uint16_t now=splash_pit_counter();splash_pit_accum+=(uint16_t)(splash_pit_last-now);splash_pit_last=now;
    if(splash_pit_accum<19886u)return 1;
    splash_step=(splash_step+(splash_pit_accum/19886u)*3u)%144u;splash_pit_accum%=19886u;splash_draw_frame();return 1;
}
int framebuffer_boot_splash_active(void){return splash_active;}
void framebuffer_boot_splash_irq_mode(int enabled){splash_irq_enabled=enabled!=0;}
void framebuffer_boot_splash_irq(void){if(splash_active&&splash_irq_enabled){splash_step=(splash_step+2u)%144u;splash_draw_frame();}}
void framebuffer_boot_splash_stop(void){
    if(!splash_active)return;splash_active=0;splash_irq_enabled=0;
    if(splash_deferred_scene){copy_bytes(screen.pixels,frame_pixels,screen.pitch*screen.height);splash_deferred_scene=0;}
}
const framebuffer *framebuffer_get(void) { return screen.pixels?&screen:0; }
uint32_t framebuffer_resolution_count(void){
    uint32_t count=0,width,height;while(resolution_at(count,&width,&height))count++;return count;
}
int framebuffer_resolution_get(uint32_t index,uint32_t *width,uint32_t *height){
    return index<framebuffer_resolution_count()&&resolution_at(index,width,height);
}
int framebuffer_set_resolution(uint32_t width,uint32_t height){
    if(!screen.pixels||!vbe_dispi||!mode_fits(width,height))return width==screen.width&&height==screen.height;
    if(width==screen.width&&height==screen.height)return 1;
    vbe_write(VBE_ENABLE,0);vbe_write(VBE_XRES,(uint16_t)width);vbe_write(VBE_YRES,(uint16_t)height);
    vbe_write(VBE_BPP,(uint16_t)(screen.bytes_per_pixel*8));vbe_write(VBE_VIRT_WIDTH,(uint16_t)width);vbe_write(VBE_ENABLE,0x41);
    if(vbe_read(VBE_XRES)!=width||vbe_read(VBE_YRES)!=height){
        vbe_write(VBE_ENABLE,0);vbe_write(VBE_XRES,(uint16_t)screen.width);vbe_write(VBE_YRES,(uint16_t)screen.height);
        vbe_write(VBE_BPP,(uint16_t)(screen.bytes_per_pixel*8));vbe_write(VBE_VIRT_WIDTH,(uint16_t)screen.width);vbe_write(VBE_ENABLE,0x41);return 0;
    }
    screen.width=width;screen.height=height;screen.pitch=width*screen.bytes_per_pixel;
    composing=0;front_pixels=screen.pixels;damage_enabled=0;damage_count=0;published_bytes=0;framebuffer_paint_reset();
    framebuffer_rect(0,0,width,height,0x000000);return 1;
}
static void begin_frame(int preserve){
    if(!screen.pixels||composing)return;front_pixels=screen.pixels;
    if(preserve){if(damage_enabled){for(int i=0;i<damage_count;i++)copy_region(frame_pixels,front_pixels,damage_left[i],damage_top[i],damage_right[i],damage_bottom[i]);copy_pointer_region(frame_pixels,front_pixels);}
        else copy_bytes(frame_pixels,front_pixels,screen.pitch*screen.height);}
    screen.pixels=frame_pixels;composing=1;framebuffer_cursor_restore();
}
void framebuffer_begin_frame(void){begin_frame(1);}
void framebuffer_begin_scene(void){begin_frame(0);}
void framebuffer_damage_region(int32_t x,int32_t y,uint32_t width,uint32_t height){
    framebuffer_damage_regions(x,y,width,height,0,0,0,0);
}
static void damage_set(int i,int32_t x,int32_t y,uint32_t width,uint32_t height){
    int64_t right=(int64_t)x+width,bottom=(int64_t)y+height;damage_left[i]=x;damage_top[i]=y;
    damage_right[i]=right>2147483647?2147483647:(int32_t)right;damage_bottom[i]=bottom>2147483647?2147483647:(int32_t)bottom;
    if(damage_left[i]<0)damage_left[i]=0;if(damage_top[i]<0)damage_top[i]=0;
    if(damage_right[i]<0)damage_right[i]=0;if(damage_bottom[i]<0)damage_bottom[i]=0;
    if(damage_left[i]>(int32_t)screen.width)damage_left[i]=(int32_t)screen.width;
    if(damage_top[i]>(int32_t)screen.height)damage_top[i]=(int32_t)screen.height;
    if(damage_right[i]>(int32_t)screen.width)damage_right[i]=(int32_t)screen.width;
    if(damage_bottom[i]>(int32_t)screen.height)damage_bottom[i]=(int32_t)screen.height;
}
void framebuffer_damage_regions(int32_t x1,int32_t y1,uint32_t w1,uint32_t h1,int32_t x2,int32_t y2,uint32_t w2,uint32_t h2){
    damage_set(0,x1,y1,w1,h1);damage_count=1;
    if(w2&&h2){damage_set(1,x2,y2,w2,h2);damage_count=2;}damage_enabled=1;
}
uint32_t framebuffer_published_bytes(void){return published_bytes;}
uint32_t framebuffer_blur_pixels(void){return blur_pixels;}
void framebuffer_reset_blur_pixels(void){blur_pixels=0;}
void framebuffer_end_frame(void){
    if(!composing)return;
    /* Keep the old cursor visible until the new scene, including its cursor,
       is ready. Never expose the cursor-free rendering buffer. */
    framebuffer_cursor_redraw();
    if(splash_active){
        /* Keep the splash visible until the first browser scene is complete. */
        splash_deferred_scene=1;screen.pixels=front_pixels;front_pixels=0;composing=0;
        damage_enabled=0;damage_count=0;published_bytes=0;return;
    }
    /* Copy only changed spans after the whole frame is ready. No clear is shown. */
    published_bytes=0;
    if(damage_enabled){
        for(int i=0;i<damage_count;i++){
            uint32_t bytes=(uint32_t)(damage_right[i]-damage_left[i])*screen.bytes_per_pixel;
            for(int32_t y=damage_top[i];y<damage_bottom[i];y++){
                uint32_t offset=(uint32_t)y*screen.pitch+(uint32_t)damage_left[i]*screen.bytes_per_pixel;
                copy_bytes(front_pixels+offset,frame_pixels+offset,bytes);
            }
            published_bytes+=bytes*(uint32_t)(damage_bottom[i]-damage_top[i]);
        }
        if(pointer_enabled){copy_region(front_pixels,frame_pixels,pointer_x,pointer_y,pointer_x+POINTER_WIDTH,pointer_y+POINTER_HEIGHT);
            published_bytes+=(uint32_t)POINTER_WIDTH*POINTER_HEIGHT*screen.bytes_per_pixel;}
        screen.pixels=front_pixels;front_pixels=0;composing=0;damage_enabled=0;framebuffer_cursor_redraw();return;
    }
    for(uint32_t y=0;y<screen.height;y++){uint32_t start=y*screen.pitch,end=start+screen.width*screen.bytes_per_pixel;
        while(end-start>=4&&*(const volatile uint32_t *)(front_pixels+start)==*(const uint32_t *)(frame_pixels+start))start+=4;
        while(start<end&&front_pixels[start]==frame_pixels[start])start++;
        while(end-start>=4&&*(const volatile uint32_t *)(front_pixels+end-4)==*(const uint32_t *)(frame_pixels+end-4))end-=4;
        while(end>start&&front_pixels[end-1]==frame_pixels[end-1])end--;
        published_bytes+=end-start;copy_bytes(front_pixels+start,frame_pixels+start,end-start);}
    screen.pixels=front_pixels;front_pixels=0;composing=0;damage_enabled=0;framebuffer_cursor_redraw();
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
    int32_t left=clip_left,top=clip_top,right=clip_right,bottom=clip_bottom;uint8_t opacity=paint_opacity;
    framebuffer_paint_reset();
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
    clip_left=left;clip_top=top;clip_right=right;clip_bottom=bottom;paint_opacity=opacity;
}
static void copy_pointer_area(volatile uint8_t *to,const volatile uint8_t *from,int32_t x,int32_t y){
    for(int32_t row=0;row<POINTER_HEIGHT;row++)for(int32_t col=0;col<POINTER_WIDTH;col++){
        int32_t px=x+col,py=y+row;if(px<0||py<0||(uint32_t)px>=screen.width||(uint32_t)py>=screen.height)continue;
        uint32_t offset=(uint32_t)py*screen.pitch+(uint32_t)px*screen.bytes_per_pixel;
        for(uint32_t b=0;b<screen.bytes_per_pixel;b++)to[offset+b]=from[offset+b];
    }
}
void framebuffer_cursor_move(int32_t x,int32_t y){
    if(pointer_enabled&&pointer_visible&&x==pointer_x&&y==pointer_y)return;
    if(!screen.pixels)return;
    if(composing){framebuffer_cursor_restore();pointer_x=x;pointer_y=y;pointer_enabled=1;return;}
    /* Compose only the old/new cursor rectangles before publishing either. */
    volatile uint8_t *front=screen.pixels;int32_t old_x=pointer_x,old_y=pointer_y;
    copy_pointer_area(frame_pixels,front,old_x,old_y);copy_pointer_area(frame_pixels,front,x,y);
    screen.pixels=frame_pixels;framebuffer_cursor_restore();pointer_x=x;pointer_y=y;pointer_enabled=1;framebuffer_cursor_redraw();
    copy_pointer_area(front,frame_pixels,old_x,old_y);copy_pointer_area(front,frame_pixels,x,y);screen.pixels=front;
}
static void put_pixel(int32_t x,int32_t y,uint32_t rgb) {
    if(!screen.pixels || x<0 || y<0 || (uint32_t)x>=screen.width || (uint32_t)y>=screen.height) return;
    uint32_t value=native_color(rgb);
    volatile uint8_t *pixel=screen.pixels+(uint32_t)y*screen.pitch+(uint32_t)x*screen.bytes_per_pixel;
    if(screen.bytes_per_pixel==4)*(volatile uint32_t *)pixel=value;
    else for(uint32_t i=0;i<screen.bytes_per_pixel;i++)pixel[i]=(uint8_t)(value>>(8*i));
}
void framebuffer_pixel(int32_t x,int32_t y,uint32_t rgb){framebuffer_blend(x,y,rgb,255);}
void framebuffer_blend(int32_t x,int32_t y,uint32_t rgb,uint8_t coverage) {
    if(x<clip_left||y<clip_top||x>=clip_right||y>=clip_bottom)return;
    coverage=(uint8_t)(((uint32_t)coverage*paint_opacity+127)/255);
    if(!screen.pixels||x<0||y<0||(uint32_t)x>=screen.width||(uint32_t)y>=screen.height||!coverage)return;
    if(coverage==255){put_pixel(x,y,rgb);return;}
    volatile uint8_t *p=screen.pixels+(uint32_t)y*screen.pitch+(uint32_t)x*screen.bytes_per_pixel;uint32_t old=0;
    if(rgb32){old=*(volatile uint32_t *)p;uint32_t inverse=255-coverage;
        uint32_t red=(((rgb>>16)&255)*coverage+((old>>16)&255)*inverse+127)/255;
        uint32_t green=(((rgb>>8)&255)*coverage+((old>>8)&255)*inverse+127)/255;
        uint32_t blue=((rgb&255)*coverage+(old&255)*inverse+127)/255;
        *(volatile uint32_t *)p=(red<<16)|(green<<8)|blue;return;}
    for(uint32_t i=0;i<screen.bytes_per_pixel;i++)old|=(uint32_t)p[i]<<(i*8);
    uint32_t result=0;uint8_t bits[3]={screen.blue_bits,screen.green_bits,screen.red_bits},shifts[3]={screen.blue_shift,screen.green_shift,screen.red_shift};
    for(uint32_t i=0;i<3;i++){uint32_t mask=(1u<<bits[i])-1,background=((old>>shifts[i])&mask)*255/mask;
        result|=((((rgb>>(i*8))&255)*coverage+background*(255-coverage)+127)/255)<<(i*8);}
    put_pixel(x,y,result);
}
void framebuffer_rect(int32_t x,int32_t y,uint32_t width,uint32_t height,uint32_t rgb) {
    if(!paint_opacity||!screen.pixels || x>=(int32_t)screen.width || y>=(int32_t)screen.height ||
        !width || !height) return;
    if(x<0) { uint32_t clipped=(uint32_t)(-(int64_t)x); if(clipped>=width) return; width-=clipped; x=0; }
    if(y<0) { uint32_t clipped=(uint32_t)(-(int64_t)y); if(clipped>=height) return; height-=clipped; y=0; }
    if(width>screen.width-(uint32_t)x) width=screen.width-(uint32_t)x;
    if(height>screen.height-(uint32_t)y) height=screen.height-(uint32_t)y;
    if(x<clip_left){uint32_t trim=(uint32_t)((int64_t)clip_left-x);if(trim>=width)return;x=clip_left;width-=trim;}
    if(y<clip_top){uint32_t trim=(uint32_t)((int64_t)clip_top-y);if(trim>=height)return;y=clip_top;height-=trim;}
    if(x>=clip_right||y>=clip_bottom)return;
    if(width>(uint32_t)(clip_right-x))width=(uint32_t)(clip_right-x);
    if(height>(uint32_t)(clip_bottom-y))height=(uint32_t)(clip_bottom-y);
    if(paint_opacity==255){uint32_t value=native_color(rgb);
        for(uint32_t row=0;row<height;row++){volatile uint8_t *pixel=screen.pixels+((uint32_t)y+row)*screen.pitch+(uint32_t)x*screen.bytes_per_pixel;
            if(screen.bytes_per_pixel==4){uint32_t count=width;__asm__ volatile("cld; rep stosl":"+D"(pixel),"+c"(count):"a"(value):"memory");}
            else for(uint32_t col=0;col<width;col++){pixel[0]=(uint8_t)value;pixel[1]=(uint8_t)(value>>8);pixel[2]=(uint8_t)(value>>16);pixel+=3;}}
        return;}
    for(uint32_t row=0;row<height;row++) for(uint32_t col=0;col<width;col++)
        framebuffer_pixel(x+(int32_t)col,y+(int32_t)row,rgb);
}
void framebuffer_image(const uint8_t *rgba,uint32_t source_width,uint32_t source_height,int32_t x,int32_t y,uint32_t width,uint32_t height){
    if(!screen.pixels||!rgba||!source_width||!source_height||source_width>8192||source_height>8192||!width||!height||width>8192||height>8192||!paint_opacity)return;
    int64_t end_x=(int64_t)x+width,end_y=(int64_t)y+height;
    int32_t left=x>clip_left?x:clip_left,top=y>clip_top?y:clip_top;if(left<0)left=0;if(top<0)top=0;
    int32_t right=(int32_t)screen.width,bottom=(int32_t)screen.height;if(end_x<right)right=(int32_t)end_x;if(end_y<bottom)bottom=(int32_t)end_y;
    if(right>clip_right)right=clip_right;if(bottom>clip_bottom)bottom=clip_bottom;if(left>=right||top>=bottom)return;
    uint32_t first=(uint32_t)(left-x)*source_width,step=source_width/width,remainder_step=source_width%width;
    for(int32_t py=top;py<bottom;py++){uint32_t sy=(uint32_t)(py-y)*source_height/height,sx=first/width,remainder=first%width;
        volatile uint8_t *destination=screen.pixels+(uint32_t)py*screen.pitch+(uint32_t)left*screen.bytes_per_pixel;
        for(int32_t px=left;px<right;px++){const uint8_t *pixel=rgba+(sy*source_width+sx)*4;uint32_t color=(uint32_t)pixel[0]<<16|(uint32_t)pixel[1]<<8|pixel[2];
            if(rgb32&&paint_opacity==255&&pixel[3]==255)*(volatile uint32_t *)destination=color;
            else if(pixel[3])framebuffer_blend(px,py,color,pixel[3]);
            destination+=screen.bytes_per_pixel;sx+=step;remainder+=remainder_step;if(remainder>=width){remainder-=width;sx++;}}
    }
}
/* A separable blur keeps only its source rows. Every source row is sampled
   before output touches it, avoiding feedback and a second full framebuffer. */
static uint8_t blur_rows[65][4096*3];static uint32_t blur_sums[4096*3];
static uint32_t backdrop_rgb(int32_t x,int32_t y){
    if(x<0)x=0;if(y<0)y=0;if((uint32_t)x>=screen.width)x=(int32_t)screen.width-1;if((uint32_t)y>=screen.height)y=(int32_t)screen.height-1;
    volatile uint8_t *pixel=screen.pixels+(uint32_t)y*screen.pitch+(uint32_t)x*screen.bytes_per_pixel;uint32_t raw=0,rgb=0;
    if(rgb32)return *(const volatile uint32_t *)pixel&0xffffffu;
    for(uint32_t b=0;b<screen.bytes_per_pixel;b++)raw|=(uint32_t)pixel[b]<<(b*8);
    uint8_t bits[]={screen.blue_bits,screen.green_bits,screen.red_bits},shifts[]={screen.blue_shift,screen.green_shift,screen.red_shift};
    for(uint32_t c=0;c<3;c++){uint32_t mask=(1u<<bits[c])-1,value=(raw>>shifts[c])&mask;
        if(bits[c]!=8)value=value*255/mask;rgb|=value<<(c*8);}return rgb;
}
static void backdrop_row(uint32_t slot,int32_t left,int32_t y,uint32_t width,uint32_t radius){
    uint32_t sums[3]={0},count=radius*2+1;
    for(int32_t i=-(int32_t)radius;i<=(int32_t)radius;i++){uint32_t rgb=backdrop_rgb(left+i*2,y);for(uint32_t c=0;c<3;c++)sums[c]+=(rgb>>(c*8))&255;}
    for(uint32_t x=0;x<width;x++){
        for(uint32_t c=0;c<3;c++)blur_rows[slot][x*3+c]=(uint8_t)((sums[c]+count/2)/count);
        uint32_t remove=backdrop_rgb(left+((int32_t)x-(int32_t)radius)*2,y),add=backdrop_rgb(left+((int32_t)x+(int32_t)radius+1)*2,y);
        for(uint32_t c=0;c<3;c++)sums[c]+=((add>>(c*8))&255)-((remove>>(c*8))&255);
    }
}
void framebuffer_backdrop(int32_t x,int32_t y,uint32_t width,uint32_t height,uint32_t blur,uint32_t brightness,int32_t radius){
    if(!screen.pixels||!width||!height||!paint_opacity||brightness>8000)return;if(blur>32)blur=32;
    int32_t left=x<clip_left?clip_left:x,top=y<clip_top?clip_top:y,right=x+(int32_t)width,bottom=y+(int32_t)height;
    if(left<0)left=0;if(top<0)top=0;if(right>clip_right)right=clip_right;if(bottom>clip_bottom)bottom=clip_bottom;
    if(right>(int32_t)screen.width)right=(int32_t)screen.width;if(bottom>(int32_t)screen.height)bottom=(int32_t)screen.height;
    if(right<=left||bottom<=top)return;uint32_t span=(uint32_t)(right-left),coarse_span=(span+1)/2;
    if(blur)blur=(blur+1)/2;uint32_t count=blur*2+1;
    if(radius>(int32_t)width/2)radius=(int32_t)width/2;if(radius>(int32_t)height/2)radius=(int32_t)height/2;
    if(blur){for(uint32_t b=0;b<coarse_span*3;b++)blur_sums[b]=0;
        for(uint32_t row=0;row<count;row++){backdrop_row(row,left,top+((int32_t)row-(int32_t)blur)*2,coarse_span,blur);for(uint32_t b=0;b<coarse_span*3;b++)blur_sums[b]+=blur_rows[row][b];}}
    uint32_t last_coarse_y=0;
    for(int32_t py=top;py<bottom;py++){
        uint32_t coarse_y=(uint32_t)(py-top)/2;
        if(blur&&coarse_y>last_coarse_y){uint32_t slot=(coarse_y-1)%count;for(uint32_t b=0;b<coarse_span*3;b++)blur_sums[b]-=blur_rows[slot][b];
            backdrop_row(slot,left,top+((int32_t)coarse_y+(int32_t)blur)*2,coarse_span,blur);for(uint32_t b=0;b<coarse_span*3;b++)blur_sums[b]+=blur_rows[slot][b];last_coarse_y=coarse_y;}
        for(int32_t px=left;px<right;px++){
            int32_t lx=px-x,ly=py-y,dx=lx<radius?radius-1-lx:lx>=(int32_t)width-radius?lx-((int32_t)width-radius):0;
            int32_t dy=ly<radius?radius-1-ly:ly>=(int32_t)height-radius?ly-((int32_t)height-radius):0;
            if(radius>0&&dx*dx+dy*dy>radius*radius)continue;
            uint32_t old=blur?0:backdrop_rgb(px,py),rgb=0;if(blur)blur_pixels++;
            for(uint32_t c=0;c<3;c++){uint32_t channel=blur?(blur_sums[((uint32_t)(px-left)/2)*3+c]+count/2)/count:(old>>(c*8))&255;
                channel=(channel*brightness+500)/1000;if(channel>255)channel=255;rgb|=channel<<(c*8);}
            framebuffer_blend(px,py,rgb,255);
        }
    }
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
