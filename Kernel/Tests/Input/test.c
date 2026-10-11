#include "../../Drivers/Input/ps2.h"
#include "../../Drivers/Video/framebuffer.h"
static int cursor_restores_background(int32_t x,int32_t y) {
    const framebuffer *fb=framebuffer_get();uint8_t saved[32*32*4];
    if(!fb)return 0;
    framebuffer_cursor_restore();
    for(uint32_t row=0;row<32;row++)for(uint32_t col=0;col<32;col++)
        framebuffer_pixel(x+col,y+row,0x386da4+row*257+col);
    for(uint32_t row=0;row<32;row++)for(uint32_t b=0;b<32*fb->bytes_per_pixel;b++)
        saved[row*32*fb->bytes_per_pixel+b]=fb->pixels[(y+row)*fb->pitch+x*fb->bytes_per_pixel+b];
    framebuffer_cursor_move(x+12,y+12);
    framebuffer_cursor_move(x+20,y+20);
    framebuffer_cursor_restore();
    for(uint32_t row=0;row<32;row++)for(uint32_t b=0;b<32*fb->bytes_per_pixel;b++)
        if(saved[row*32*fb->bytes_per_pixel+b]!=fb->pixels[(y+row)*fb->pitch+x*fb->bytes_per_pixel+b])return 0;
    return 1;
}
int input_self_test(void) {
#define CHECK(condition,code) do{if(!(condition))return code;}while(0)
    input_event e;input_reset(100,80);
    input_feed(1,0x2a);CHECK(input_next(&e)&&e.pressed&&(e.modifiers&INPUT_SHIFT),90);
    input_feed(1,0x1e);CHECK(input_next(&e)&&e.character=='A'&&!e.repeat,91);
    input_feed(1,0x1e);CHECK(input_next(&e)&&e.repeat,92);
    input_feed(1,0x9e);CHECK(input_next(&e)&&!e.pressed,93);
    input_feed(1,0xaa);CHECK(input_next(&e)&&!(e.modifiers&INPUT_SHIFT),94);
    input_feed(1,0xe0);input_feed(1,0x1d);CHECK(input_next(&e)&&e.key==0x11d&&(e.modifiers&INPUT_CONTROL),95);
    input_feed(1,0xe0);input_feed(1,0x4f);CHECK(input_next(&e)&&e.key==0x14f&&e.pressed,96);
    input_feed(1,0xe0);input_feed(1,0x9d);CHECK(input_next(&e)&&!(e.modifiers&INPUT_CONTROL),97);
    input_feed(0x21,8);input_feed(0x21,10);input_feed(0x21,5);CHECK(input_next(&e)&&e.type==INPUT_POINTER&&e.x==59&&e.y==34,98);
    input_feed(0x21,0x39);input_feed(0x21,251);input_feed(0x21,246);CHECK(input_next(&e)&&e.dx==-5&&e.dy==10&&e.buttons==1&&e.x==54&&e.y==44,99);
    input_feed(0x21,0xc8);input_feed(0x21,255);input_feed(0x21,255);CHECK(input_next(&e)&&e.dx==0&&e.dy==0,150);
    input_feed(0x21,8);input_feed(0xe1,0);input_feed(0x21,0);CHECK(!input_next(&e),151);
    input_feed(0x21,8);input_feed(0x21,127);input_feed(0x21,127);CHECK(input_next(&e)&&e.x==99&&e.y==0,152);
    input_reset(1,1);for(uint32_t i=0;i<100;i++)input_feed(1,0x1e);
    uint32_t count=0;while(input_next(&e))count++;CHECK(count==63,153);
    input_reset(100,80);input_mouse_id(3);
    input_feed(0x21,8);input_feed(0x21,2);input_feed(0x21,0);CHECK(!input_next(&e),157);
    input_feed(0x21,255);CHECK(input_next(&e)&&e.wheel==-1&&e.dx==2&&e.x==51,158);
    input_feed(0x21,8);input_feed(0x21,0);input_feed(0x21,0);input_feed(0x21,1);
    CHECK(input_next(&e)&&e.wheel==1&&e.dx==0,159);
    input_mouse_id(4);input_feed(0x21,8);input_feed(0x21,0);input_feed(0x21,0);input_feed(0x21,0x1f);
    CHECK(input_next(&e)&&e.wheel==-1&&e.buttons==8,160);
    input_reset(100,80);
    for(uint32_t i=0;i<3;i++){input_feed(0x21,8);input_feed(0x21,2);input_feed(0x21,0);}
    CHECK(input_next(&e)&&e.dx==6&&e.x==55&&!input_next(&e),306);
    CHECK((input_init(800,600)&3)==3,154); /* QEMU's keyboard and mouse acknowledge setup. */
    input_feed(0x21,8);input_feed(0x21,0);input_feed(0x21,0);input_feed(0x21,1);
    CHECK(input_next(&e)&&e.wheel==1&&!input_next(&e),301);
    CHECK(cursor_restores_background(0,0),155);
    const framebuffer *fb=framebuffer_get();
    CHECK(fb&&cursor_restores_background(fb->width-32,fb->height-32),156);
    framebuffer_paint_reset();framebuffer_rect(60,60,60,60,0x386da4);
    framebuffer_paint_region(0,0,1,1,0);framebuffer_cursor_move(80,80);
    volatile uint8_t *front=fb->pixels;uint32_t offset=80*fb->pitch+80*fb->bytes_per_pixel;
    uint8_t ink[4];for(uint32_t b=0;b<fb->bytes_per_pixel;b++)ink[b]=front[offset+b];
    framebuffer_paint_reset();framebuffer_cursor_restore();
    int different=0;for(uint32_t b=0;b<fb->bytes_per_pixel;b++)if(front[offset+b]!=ink[b])different=1;
    CHECK(different,302);framebuffer_cursor_redraw();
    framebuffer_begin_frame();framebuffer_rect(60,60,60,60,0xff0000);
    for(uint32_t b=0;b<fb->bytes_per_pixel;b++)CHECK(front[offset+b]==ink[b],303);
    framebuffer_end_frame();for(uint32_t b=0;b<fb->bytes_per_pixel;b++)CHECK(front[offset+b]==ink[b],304);
    framebuffer_cursor_restore();uint32_t reference=60*fb->pitch+60*fb->bytes_per_pixel;
    for(uint32_t b=0;b<fb->bytes_per_pixel;b++)CHECK(front[offset+b]==front[reference+b],305);
    framebuffer_begin_scene();framebuffer_rect(0,0,fb->width,fb->height,0x123456);framebuffer_end_frame();
    framebuffer_begin_scene();framebuffer_rect(0,0,fb->width,fb->height,0x123456);framebuffer_end_frame();
    CHECK(framebuffer_published_bytes()==0,573);
    if(fb->width>120&&fb->height>120){uint32_t inside=100*fb->pitch+100*fb->bytes_per_pixel,outside=99*fb->pitch+100*fb->bytes_per_pixel;uint8_t before_inside[4],before_outside[4];
        for(uint32_t b=0;b<fb->bytes_per_pixel;b++){before_inside[b]=front[inside+b];before_outside[b]=front[outside+b];}
        framebuffer_damage_region(100,100,4,4);framebuffer_begin_frame();framebuffer_rect(100,100,4,4,0xaabbcc);framebuffer_end_frame();
        int changed=0;for(uint32_t b=0;b<fb->bytes_per_pixel;b++){if(front[inside+b]!=before_inside[b])changed=1;CHECK(front[outside+b]==before_outside[b],575);}CHECK(changed,576);
        CHECK(framebuffer_published_bytes()==(16u+18u*26u)*fb->bytes_per_pixel,577);
    }
    if(fb->width>140&&fb->height>120){uint32_t gap=110*fb->pitch+101*fb->bytes_per_pixel;uint8_t before_gap[4];
        for(uint32_t b=0;b<fb->bytes_per_pixel;b++)before_gap[b]=front[gap+b];
        framebuffer_damage_regions(100,100,4,4,120,100,4,4);framebuffer_begin_frame();
        framebuffer_rect(100,100,4,4,0x22bb44);framebuffer_rect(120,100,4,4,0xbb4422);framebuffer_end_frame();
        for(uint32_t b=0;b<fb->bytes_per_pixel;b++)CHECK(front[gap+b]==before_gap[b],578);
        CHECK(framebuffer_published_bytes()==(32u+18u*26u)*fb->bytes_per_pixel,579);
    }
    framebuffer_cursor_restore();framebuffer_rect(10,10,10,10,0x0000ff);
    framebuffer_paint_region(12,12,3,3,255);framebuffer_rect(10,10,10,10,0xff0000);framebuffer_paint_reset();
    uint32_t clipped=12*fb->pitch+12*fb->bytes_per_pixel,outside=11*fb->pitch+11*fb->bytes_per_pixel;
    CHECK(fb->pixels[clipped+fb->red_shift/8]==255&&fb->pixels[outside+fb->blue_shift/8]==255,574);
    return 0;
}
