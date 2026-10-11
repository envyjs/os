#ifndef KERNEL_CDFS_H
#define KERNEL_CDFS_H
#include <stdint.h>
#define CDFS_BLOCK_SIZE 2048u
enum { CDFS_OK = 0, CDFS_IO = -1, CDFS_INVALID = -2,
       CDFS_NOT_FOUND = -3, CDFS_UNSUPPORTED = -4, CDFS_NOT_FILE = -5 };
/* Read exactly one 2048-byte block. Return zero on success. */
typedef int (*cdfs_read_block)(void *context, uint32_t lba, uint8_t *buffer);
typedef struct { uint32_t lba, size; uint8_t flags; } cdfs_node;
typedef struct {
    cdfs_read_block read;
    void *context;
    uint32_t blocks;
    cdfs_node root;
    uint8_t scratch[CDFS_BLOCK_SIZE];
} cdfs;
int cdfs_mount(cdfs *fs, cdfs_read_block read, void *context);
int cdfs_lookup(cdfs *fs, const char *path, cdfs_node *node);
int cdfs_read(cdfs *fs, const cdfs_node *node, uint32_t offset,
              void *buffer, uint32_t count, uint32_t *bytes_read);
/* Visits immediate children in on-disc order. Names omit ISO ;version suffixes.
   Return nonzero to continue, zero to stop successfully. */
typedef int (*cdfs_visit)(void *context, const char *name, const cdfs_node *node);
int cdfs_list(cdfs *fs, const cdfs_node *directory, cdfs_visit visit, void *context);
#endif
