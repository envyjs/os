#ifndef ENVY_PS2_H
#define ENVY_PS2_H
#include <stdint.h>
enum { INPUT_KEY=1, INPUT_POINTER, INPUT_SHIFT=1, INPUT_CONTROL=2, INPUT_ALT=4, INPUT_CAPS=8, INPUT_SUPER=16 };
typedef struct {
    uint32_t type;uint16_t key;uint8_t scancode,character,modifiers,pressed,repeat,buttons;
    int32_t x,y,dx,dy,wheel;
} input_event;
void input_reset(uint32_t width,uint32_t height);
int input_init(uint32_t width,uint32_t height);
/* Decode controller bytes independently of port I/O for guest regression tests. */
void input_feed(uint8_t status,uint8_t byte);
/* Select the packet format after querying the mouse's PS/2 device ID. */
void input_mouse_id(uint8_t id);
/* Submit translated events from non-PS/2 devices such as USB HID. */
void input_submit(const input_event *event);
void input_pointer_delta(int32_t dx, int32_t dy, uint8_t buttons, int32_t wheel);
int input_next(input_event *event);
void input_poll(void);
#endif
