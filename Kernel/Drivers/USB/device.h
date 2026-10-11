#ifndef ENVY_USB_DEVICE_H
#define ENVY_USB_DEVICE_H
#include <stdint.h>

enum { USB_CLASS_HID=3, USB_CLASS_MASS_STORAGE=8 };
enum { USB_HID_BOOT=1, USB_HID_KEYBOARD=1, USB_HID_MOUSE=2 };
enum { USB_MSC_SCSI=6, USB_MSC_BULK_ONLY=0x50 };

typedef struct {
    uint8_t address, attributes, interval;
    uint16_t max_packet;
} usb_endpoint_info;

typedef struct {
    uint8_t number, alternate, class_code, subclass, protocol;
    uint8_t configuration;
    usb_endpoint_info interrupt_in, bulk_in, bulk_out;
} usb_interface_info;

/* Parse one complete configuration descriptor into the interfaces it exposes.
   Returns interface count, or zero for malformed/unsupported input. */
uint32_t usb_parse_configuration(const uint8_t *bytes,uint32_t length,
                                 usb_interface_info *interfaces,uint32_t capacity);

#endif
