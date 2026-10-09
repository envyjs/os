#ifndef ENVY_IMAGE_H
#define ENVY_IMAGE_H
#include <stdint.h>
typedef struct {uint8_t *pixels;uint32_t width,height,natural_width,natural_height;} image_surface;
/* Decode output remains valid until the next image_decode / image_reset. SVG input
   is copied because its parser modifies the XML. PNG and JPEG bytes are immutable. */
int image_decode(const void *data,uint32_t size,uint32_t width,uint32_t height,image_surface *result);
void image_reset(void);
uint32_t image_generation(void);
#endif
