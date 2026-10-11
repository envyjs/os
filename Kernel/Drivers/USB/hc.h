#ifndef ENVY_USB_HC_INTERNAL_H
#define ENVY_USB_HC_INTERNAL_H
#include "usb.h"
uint32_t usb_uhci_ports(usb_controller *controller);
uint32_t usb_ohci_ports(usb_controller *controller);
uint32_t usb_ehci_ports(usb_controller *controller);
uint32_t usb_xhci_ports(usb_controller *controller);
int usb_uhci_start(usb_controller *controller);
int usb_ohci_start(usb_controller *controller);
int usb_ohci_transfer(usb_controller *controller,uint8_t type,uint8_t address,
                     uint8_t endpoint,uint16_t max_packet,uint8_t low_speed,
                     const uint8_t setup[8],void *data,uint32_t length,uint32_t *actual);
int usb_uhci_transfer(usb_controller *controller,uint8_t type,uint8_t address,
                     uint8_t endpoint,uint16_t max_packet,uint8_t low_speed,
                     const uint8_t setup[8],void *data,uint32_t length,uint32_t *actual);
int usb_ehci_start(usb_controller *controller);
int usb_ehci_transfer(usb_controller *controller,uint8_t type,uint8_t address,
                      uint8_t endpoint,uint16_t max_packet,uint8_t low_speed,
                      const uint8_t setup[8],void *data,uint32_t length,uint32_t *actual);
int usb_xhci_start(usb_controller *controller);
int usb_xhci_transfer(usb_controller *controller,uint8_t type,uint8_t address,
                      uint8_t endpoint,uint16_t max_packet,uint8_t low_speed,uint8_t interval,
                      const uint8_t setup[8],void *data,uint32_t length,uint32_t *actual);
int usb_xhci_prepare_port(usb_controller *controller,uint32_t port,uint8_t *address,
                          uint16_t *ep0_packet,uint8_t *low_speed);
#endif
