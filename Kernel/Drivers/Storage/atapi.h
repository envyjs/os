#ifndef KERNEL_ATAPI_H
#define KERNEL_ATAPI_H
#include <stdint.h>
#define ATAPI_BLOCK_SIZE 2048u
#define ATAPI_MAX_DEVICES 16u
enum { ATAPI_OK = 0, ATAPI_TIMEOUT = -1, ATAPI_DEVICE_ERROR = -2,
       ATAPI_PROTOCOL = -3, ATAPI_NO_MEDIA = -4, ATAPI_UNSUPPORTED = -5,
       ATAPI_RANGE = -6 };
typedef struct {
    uint16_t io, control;
    uint8_t drive, packet_bytes;
    uint8_t sense_key, asc, ascq;
    uint32_t blocks;
    char model[41];
} atapi_device;
/* Bootstrap polling driver. Serialize all operations sharing an IDE channel. */
uint32_t atapi_probe(atapi_device *devices, uint32_t capacity);
int atapi_media_ready(atapi_device *device);
/* CDFS-compatible callback; reads one block using ATAPI READ(12). */
int atapi_read_block(void *device, uint32_t lba, uint8_t *buffer);
#endif
