#include "../../FS/CDFS/cdfs.h"
#include "../../Drivers/Storage/atapi.h"
typedef struct { uint8_t *image; uint32_t failed_lba; } test_device;
static int read_block(void *context, uint32_t lba, uint8_t *buffer) {
    test_device *device = context;
    if (lba >= 512 || lba == device->failed_lba) return -1;
    for (uint32_t i = 0; i < 2048; ++i) buffer[i] = device->image[lba * 2048 + i];
    return 0;
}
static void both32(uint8_t *p, uint32_t value) {
    for (uint32_t i = 0; i < 4; ++i) { p[i] = (uint8_t)(value >> (8*i)); p[7-i] = p[i]; }
}
/* Native reads build an isolated fixture; mutations never reach the CD. */
int cdfs_self_test(cdfs *fs, atapi_device *cd) {
    test_device device = { (uint8_t *)0x4000000, 0xffffffffu };
    for (uint32_t i = 0; i < 512; ++i)
        if (atapi_read_block(cd, i, device.image + i*2048)) return 133;
    uint8_t *pvd = device.image + 16*2048;
    uint8_t *root = device.image + 22*2048;
    cdfs_node file, node;
    uint8_t data[4097];
    uint32_t got;
#define CHECK(condition, code) do { if (!(condition)) return code; } while (0)
    CHECK(cdfs_mount(fs, read_block, &device) == CDFS_OK, 101);
    CHECK(cdfs_lookup(fs, "/", &node) == CDFS_OK && (node.flags & 2), 102);
    CHECK(cdfs_lookup(fs, "/docs/readme.txt", &file) == CDFS_OK && file.lba == 409, 103);
    CHECK(cdfs_lookup(fs, "DOCS/README.TXT;1", &node) == CDFS_OK && node.size == file.size, 104);
    CHECK(cdfs_lookup(fs, "/DOCS/MISSING.TXT", &node) == CDFS_NOT_FOUND, 105);
    CHECK(cdfs_lookup(fs, "/DOCS/README.TXT/", &node) == CDFS_NOT_FOUND, 106);
    CHECK(cdfs_lookup(fs, "/../DOCS", &node) == CDFS_UNSUPPORTED, 107);
    CHECK(cdfs_read(fs, &file, 0, data, sizeof(data), &got) == CDFS_OK && got == file.size, 108);
    for (uint32_t i = 0; i < got; ++i) CHECK(data[i] == device.image[file.lba*2048+i], 109);
    CHECK(cdfs_read(fs, &file, file.size, data, 1, &got) == CDFS_OK && got == 0, 110);
    CHECK(cdfs_read(fs, &file, 0xffffffffu, data, 1, &got) == CDFS_OK && got == 0, 111);
    CHECK(cdfs_read(fs, &fs->root, 0, data, 1, &got) == CDFS_NOT_FILE, 112);
    CHECK(cdfs_read(fs, &file, 0, 0, 1, &got) == CDFS_INVALID, 113);
    device.failed_lba = file.lba;
    CHECK(cdfs_read(fs, &file, 0, data, 1, &got) == CDFS_IO && got == 0, 114);
    device.failed_lba = 22;
    CHECK(cdfs_lookup(fs, "/DOCS", &node) == CDFS_IO, 115);
    device.failed_lba = 0xffffffffu;

    /* The next directory block must still be scanned after zero padding. */
    fs->root.size = 4096;
    CHECK(cdfs_lookup(fs, "/README.TXT", &node) == CDFS_OK, 116);
    fs->root.size = 2048;
    uint8_t saved = root[0]; root[0] = 1;
    CHECK(cdfs_lookup(fs, "/DOCS", &node) == CDFS_INVALID, 117);
    root[0] = saved;
    root[26] = 1;
    CHECK(cdfs_lookup(fs, "/DOCS", &node) == CDFS_UNSUPPORTED, 118);
    root[26] = 0;
    root[25] |= 0x80;
    CHECK(cdfs_lookup(fs, "/DOCS", &node) == CDFS_UNSUPPORTED, 119);
    root[25] &= 0x7f;
    both32(root + 2, 512);
    CHECK(cdfs_lookup(fs, "/DOCS", &node) == CDFS_INVALID, 120);
    both32(root + 2, 22);
    root[32] = 255;
    CHECK(cdfs_lookup(fs, "/DOCS", &node) == CDFS_INVALID, 121);
    root[32] = 1;

    /* Distinct bytes on both sides of each 2048-byte block boundary. */
    for (uint32_t i = 0; i < 6144; ++i) device.image[i] = (uint8_t)(i*29 + i/2048);
    node = (cdfs_node){ 0, 6144, 0 };
    CHECK(cdfs_read(fs, &node, 2047, data, sizeof(data), &got) == CDFS_OK && got == sizeof(data), 122);
    for (uint32_t i = 0; i < got; ++i) CHECK(data[i] == device.image[2047+i], 123);
    CHECK(cdfs_read(fs, &node, 6140, data, 100, &got) == CDFS_OK && got == 4, 124);
    device.failed_lba = 1;
    CHECK(cdfs_read(fs, &node, 2047, data, 100, &got) == CDFS_IO && got == 1, 125);
    device.failed_lba = 0xffffffffu;
    for (uint32_t i = 0; i < 6144; ++i) device.image[i] = 0;
    node = (cdfs_node){ 511, 4096, 0 };
    CHECK(cdfs_read(fs, &node, 0, data, 1, &got) == CDFS_INVALID, 126);

    pvd[1] = 'X';
    CHECK(cdfs_mount(fs, read_block, &device) == CDFS_INVALID, 127);
    CHECK(cdfs_lookup(fs, "/DOCS", &node) == CDFS_INVALID, 128);
    pvd[1] = 'C';
    pvd[84] ^= 1;
    CHECK(cdfs_mount(fs, read_block, &device) == CDFS_UNSUPPORTED, 129);
    pvd[84] ^= 1;
    both32(pvd + 158, 512);
    CHECK(cdfs_mount(fs, read_block, &device) == CDFS_INVALID, 130);
    both32(pvd + 158, 22);
    device.failed_lba = 16;
    CHECK(cdfs_mount(fs, read_block, &device) == CDFS_IO, 131);
    device.failed_lba = 0xffffffffu;
    CHECK(cdfs_mount(fs, read_block, &device) == CDFS_OK, 132);
    return 0;
}
