#ifndef ENVY_FAT32_H
#define ENVY_FAT32_H
#include <stdint.h>
enum { FAT32_OK=0,FAT32_IO=-1,FAT32_INVALID=-2,FAT32_NOT_FOUND=-3,FAT32_UNSUPPORTED=-4,FAT32_NOT_FILE=-5,FAT32_READ_ONLY=-6,FAT32_NO_SPACE=-7,FAT32_EXISTS=-8 };
typedef int (*fat32_read_sectors)(void *,uint64_t,uint32_t,void *);
typedef int (*fat32_write_sectors)(void *,uint64_t,uint32_t,const void *);
typedef int (*fat32_flush_device)(void *);
typedef struct {uint32_t cluster,size,entry_sector;uint16_t entry_offset;uint8_t attributes;char short_name[13];} fat32_node;
typedef struct {
    fat32_read_sectors read;fat32_write_sectors write;fat32_flush_device flush;void *context;
    uint64_t device_sectors,start;uint32_t sectors,fat_start,data_start,fat_sectors,clusters,root,next_free,info_sector,backup_info;
    uint8_t sectors_per_cluster,fats,active_fat,mirrored;uint8_t scratch[512],fat_cache[512];uint32_t cached_fat;
} fat32;
/* 512-byte-sector FAT32, superfloppy or primary MBR partition. No automatic writes. */
int fat32_mount(fat32 *,fat32_read_sectors,fat32_write_sectors,fat32_flush_device,void *,uint64_t);
int fat32_lookup(fat32 *,const char *,fat32_node *);
int fat32_read(fat32 *,const fat32_node *,uint32_t,void *,uint32_t,uint32_t *);
int fat32_write(fat32 *,fat32_node *,uint32_t,const void *,uint32_t,uint32_t *);
/* Creates a regular file with an 8.3 name in an existing directory. */
int fat32_create(fat32 *,const char *,fat32_node *);
typedef int (*fat32_visit)(void *,const char *,const fat32_node *);
int fat32_list(fat32 *,const fat32_node *,fat32_visit,void *);
#endif
