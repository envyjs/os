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
    CHECK((input_init(800,600)&3)==3,154); /* QEMU's keyboard and mouse acknowledge setup. */
    CHECK(cursor_restores_background(0,0),155);
    const framebuffer *fb=framebuffer_get();
    CHECK(fb&&cursor_restores_background(fb->width-32,fb->height-32),156);
    return 0;
}
