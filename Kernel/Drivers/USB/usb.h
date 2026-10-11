#ifndef ENVY_USB_H
#define ENVY_USB_H
#include <stdint.h>
#include "msc.h"

typedef enum {
    USB_CONTROLLER_UHCI = 1,
    USB_CONTROLLER_OHCI,
    USB_CONTROLLER_EHCI,
    USB_CONTROLLER_XHCI
} usb_controller_kind;

typedef struct {
    usb_controller_kind kind;
    uint8_t bus, device, function;
    uint8_t ports;
    uint8_t slot;
    uint8_t ready;
    uint32_t base;
    uint32_t connected_ports;
    uint32_t enabled_ports;
} usb_controller;

#define USB_MAX_CONTROLLERS 16
#define USB_MAX_STORAGE 8

/* Discover PCI USB host controllers and snapshot their root-port status. */
uint32_t usb_init(usb_controller *controllers, uint32_t capacity);
void usb_poll(void);
const char *usb_controller_name(usb_controller_kind kind);
uint32_t usb_storage_count(void);
usb_msc_device *usb_storage_get(uint32_t index);

int usb_control_transfer(usb_controller *controller,uint8_t address,uint8_t endpoint,
                         uint16_t max_packet,uint8_t low_speed,const uint8_t setup[8],
                         void *data,uint16_t length);
int usb_bulk_transfer(usb_controller *controller,uint8_t address,uint8_t endpoint,
                      uint16_t max_packet,uint8_t low_speed,void *data,uint32_t length,
                      uint32_t *actual);
int usb_interrupt_transfer(usb_controller *controller,uint8_t address,uint8_t endpoint,
                           uint16_t max_packet,uint8_t low_speed,uint8_t interval,void *data,uint32_t length,
                           uint32_t *actual);

#endif
