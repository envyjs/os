#ifndef ENVY_USB_HID_H
#define ENVY_USB_HID_H
#include <stdint.h>

/* Decode USB HID boot-protocol keyboard and mouse input reports. */
void usb_hid_reset(void);
int usb_hid_keyboard_report(const uint8_t *report, uint32_t length);
int usb_hid_mouse_report(const uint8_t *report, uint32_t length);

#endif
