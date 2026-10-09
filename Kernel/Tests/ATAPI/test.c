#include "../../Drivers/Storage/atapi.h"
#include "../../FS/CDFS/cdfs.h"
int atapi_self_test(atapi_device *device, cdfs *fs) {
    uint8_t block[2048];
    if (device->blocks != 512 || !device->model[0]) return 141;
    if (atapi_read_block(device, 16, block)) return 142;
    if (block[0] != 1 || block[1] != 'C' || block[2] != 'D' || block[6] != 1) return 143;
    if (atapi_read_block(device, 511, block)) return 144;
    for (uint32_t i = 0; i < 2048; ++i) if (block[i]) return 145;
    if (atapi_read_block(device, 512, block) != ATAPI_RANGE) return 146;
    if (atapi_read_block(device, 0xffffffffu, block) != ATAPI_RANGE) return 147;
    if (atapi_read_block(device, 16, 0) != ATAPI_RANGE) return 148;
    if (atapi_media_ready(device)) return 149;
    if (cdfs_mount(fs, atapi_read_block, device)) return 150;
    cdfs_node file;
    if (cdfs_lookup(fs, "/DOCS/README.TXT", &file)) return 151;
    uint32_t got;
    if (cdfs_read(fs, &file, 0, block, 2048, &got) || got != file.size || block[0] != 'W') return 152;
    if(atapi_media_ready(device))return 157;
    uint32_t commands=atapi_read_command_count(),hits=atapi_cache_hit_count();
    if(atapi_read_block(device,16,block)||block[0]!=1)return 158;
    if(atapi_read_block(device,17,block)||block[0]!=0)return 159;
    if(atapi_read_command_count()!=commands+1||atapi_cache_hit_count()!=hits+1)return 160;
    if(atapi_media_ready(device)||atapi_read_block(device,16,block)||atapi_read_command_count()!=commands+2)return 465;
    return 0;
}
