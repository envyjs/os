#ifndef ENVY_FRAMEBUFFER_H
#define ENVY_FRAMEBUFFER_H
#include <stdint.h>
typedef struct {
    volatile uint8_t *pixels;
    uint32_t width, height, pitch, bytes_per_pixel;
    uint8_t red_bits, red_shift, green_bits, green_shift, blue_bits, blue_shift;
} framebuffer;
int framebuffer_init(void);
const framebuffer *framebuffer_get(void);
void framebuffer_begin_frame(void);
/* For callers which paint every visible pixel, without reading the old front. */
void framebuffer_begin_scene(void);
uint32_t framebuffer_published_bytes(void);
void framebuffer_end_frame(void);
void framebuffer_pixel(int32_t x, int32_t y, uint32_t rgb);
void framebuffer_blend(int32_t x,int32_t y,uint32_t rgb,uint8_t coverage);
void framebuffer_paint_region(int32_t x,int32_t y,uint32_t width,uint32_t height,uint8_t opacity);
void framebuffer_paint_reset(void);
void framebuffer_rect(int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t rgb);
void framebuffer_image(const uint8_t *rgba,uint32_t source_width,uint32_t source_height,int32_t x,int32_t y,uint32_t width,uint32_t height);
void framebuffer_backdrop(int32_t x,int32_t y,uint32_t width,uint32_t height,uint32_t blur,uint32_t brightness,int32_t radius);
void framebuffer_text(int32_t x, int32_t y, const char *text, uint32_t rgb);
void framebuffer_text_size(int32_t x, int32_t y, const char *text, uint32_t rgb, uint32_t height);
void framebuffer_console_write(const char *text);
void framebuffer_cursor_move(int32_t x, int32_t y);
void framebuffer_cursor_restore(void);
void framebuffer_cursor_redraw(void);
#endif
