#ifndef ENVY_USB_MSC_H
#define ENVY_USB_MSC_H
#include <stdint.h>
#include "device.h"

typedef int (*usb_bulk_transfer_fn)(void *controller,uint8_t endpoint,
                                    void *buffer,uint32_t bytes,uint32_t *actual);

typedef struct {
    void *controller;
    usb_bulk_transfer_fn transfer;
    uint8_t bulk_in,bulk_out,lun,ready;
    uint32_t tag;
    uint64_t sectors;
    uint32_t sector_size;
} usb_msc_device;

/* Probe one SCSI transparent/Bulk-Only interface and read its capacity. */
int usb_msc_probe(usb_msc_device *device,void *controller,
                  usb_bulk_transfer_fn transfer,const usb_interface_info *interface);
int usb_msc_read(void *device,uint64_t lba,uint32_t sectors,void *buffer);
int usb_msc_write(void *device,uint64_t lba,uint32_t sectors,const void *buffer);
int usb_msc_flush(void *device);

#endif
