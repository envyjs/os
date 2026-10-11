#include "../../Drivers/Storage/sata.h"
#include "../../FS/FAT32/fat32.h"
static sata_device devices[SATA_MAX_DEVICES];static fat32 fs,other;static uint8_t data[70*512],copy[70*512];
static int same(const char *a,const char *b,uint32_t n){for(uint32_t i=0;i<n;i++)if(a[i]!=b[i])return 0;return 1;}
static int visit(void *context,const char *name,const fat32_node *node){(void)name;(void)node;(*(uint32_t *)context)++;return 0;}
static int modified_read(void *context,uint64_t lba,uint32_t count,void *buffer){int r=sata_read(context,lba,count,buffer);if(r)return r;uint8_t *p=buffer;if(lba==fs.start){p[40]=129;p[41]=0;}return 0;}
static int corrupt_read(void *context,uint64_t lba,uint32_t count,void *buffer){int r=sata_read(context,lba,count,buffer);if(!r&&lba==fs.start)((uint8_t *)buffer)[13]=3;return r;}
static int cycle_read(void *context,uint64_t lba,uint32_t count,void *buffer){int r=sata_read(context,lba,count,buffer);if(!r&&lba==fs.start+fs.fat_start){uint8_t *p=buffer;p[12]=3;p[13]=p[14]=p[15]=0;}return r;}
int storage_self_test(void){
#define CHECK(condition,code) do{if(!(condition))return code;}while(0)
    uint32_t count=sata_probe(devices,SATA_MAX_DEVICES);CHECK(count==1,700);sata_device *disk=&devices[0];CHECK(disk->sectors>=131072&&disk->model[0],701);
    CHECK(!sata_read(disk,0,1,data),702);CHECK(data[510]==0x55&&data[511]==0xaa,703);CHECK(sata_read(disk,disk->sectors,1,data)==SATA_RANGE,704);
    CHECK(!fat32_mount(&fs,sata_read,sata_write,sata_flush,disk,disk->sectors),705);fat32_node file,dir;uint32_t done;
    CHECK(!fat32_lookup(&fs,"/readme.txt",&file)&&file.size==700,706);CHECK(!fat32_read(&fs,&file,0,copy,700,&done)&&done==700,707);for(uint32_t i=0;i<700;i++)CHECK(copy[i]==i%251,708);
    CHECK(!fat32_read(&fs,&file,499,copy,125,&done)&&done==125,709);for(uint32_t i=0;i<125;i++)CHECK(copy[i]==(i+499)%251,710);
    CHECK(!fat32_lookup(&fs,"/DIR/NESTED.TXT",&file)&&!fat32_read(&fs,&file,0,copy,20,&done)&&done==12&&same((char *)copy,"nested works",12),711);
    CHECK(!fat32_lookup(&fs,"/Long file name.txt",&file)&&!fat32_read(&fs,&file,0,copy,20,&done)&&same((char *)copy,"long-name-ok",12),712);
    CHECK(!fat32_lookup(&fs,"/LONGFI~1.TXT",&file)&&file.size==12,725);
    CHECK(fat32_lookup(&fs,"/missing.txt",&file)==FAT32_NOT_FOUND,713);CHECK(!fat32_lookup(&fs,"/",&dir),714);count=0;CHECK(!fat32_list(&fs,&dir,visit,&count)&&count==3,715);
    CHECK(!fat32_mount(&other,sata_read,0,0,disk,disk->sectors)&&fat32_create(&other,"/NO.TXT",&file)==FAT32_READ_ONLY,716);
    CHECK(fat32_mount(&other,corrupt_read,0,0,disk,disk->sectors)==FAT32_INVALID,717);
    CHECK(!fat32_mount(&other,cycle_read,0,0,disk,disk->sectors)&&!fat32_lookup(&other,"/README.TXT",&file)&&fat32_read(&other,&file,0,copy,700,&done)==FAT32_INVALID,726);
    CHECK(!fat32_create(&fs,"/DIR/NEW.TXT",&file),718);CHECK(fat32_create(&fs,"/DIR/NEW.TXT",&dir)==FAT32_EXISTS,719);
    for(uint32_t i=0;i<1300;i++)data[i]=(uint8_t)(i*7);CHECK(!fat32_write(&fs,&file,0,data,1300,&done)&&done==1300&&file.size==1300,720);
    CHECK(!fat32_mount(&other,modified_read,sata_write,sata_flush,disk,disk->sectors)&&!other.mirrored&&other.active_fat==1,721);CHECK(!fat32_lookup(&other,"/DIR/NEW.TXT",&file)&&!fat32_read(&other,&file,0,copy,1300,&done)&&done==1300&&same((char *)data,(char *)copy,1300),722);
    CHECK(!fat32_lookup(&fs,"/README.TXT",&file)&&!fat32_write(&fs,&file,700,data,1300,&done)&&done==1300&&file.size==2000,727);CHECK(!fat32_lookup(&fs,"/README.TXT",&file)&&!fat32_read(&fs,&file,700,copy,1300,&done)&&same((char *)data,(char *)copy,1300),728);
    for(int i=0;i<20;i++){char path[]="/FILE00.TXT";path[5]=(char)('0'+i/10);path[6]=(char)('0'+i%10);CHECK(!fat32_create(&fs,path,&file),729);}CHECK(!fat32_lookup(&fs,"/",&dir),730);count=0;CHECK(!fat32_list(&fs,&dir,visit,&count)&&count==23,731);
    CHECK(!sata_read(disk,fs.start+1,1,copy)&&copy[488]==255&&copy[489]==255&&copy[490]==255&&copy[491]==255,732);
    for(uint32_t i=0;i<70*512;i++)data[i]=(uint8_t)(i*13);CHECK(!sata_write(disk,disk->sectors-70,70,data)&&!sata_flush(disk),723);CHECK(!sata_read(disk,disk->sectors-70,70,copy)&&same((char *)data,(char *)copy,70*512),724);
    return 0;
}
