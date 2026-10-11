#ifndef ENVY_SATA_H
#define ENVY_SATA_H
#include <stdint.h>
#define SATA_MAX_DEVICES 16u
#define SATA_SECTOR_SIZE 512u
enum { SATA_OK=0,SATA_TIMEOUT=-1,SATA_IO=-2,SATA_UNSUPPORTED=-3,SATA_RANGE=-4 };
typedef struct {volatile uint32_t *port;uint64_t sectors;uint32_t slot;uint8_t failed;char model[41];} sata_device;
/* Polling AHCI DMA, identity-mapped physical memory; serialize calls per device. */
uint32_t sata_probe(sata_device *devices,uint32_t capacity);
int sata_read(void *device,uint64_t lba,uint32_t sectors,void *buffer);
int sata_write(void *device,uint64_t lba,uint32_t sectors,const void *buffer);
int sata_flush(void *device);
#endif
