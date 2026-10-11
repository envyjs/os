#include "msc.h"

#define CBW_SIGNATURE 0x43425355u
#define CSW_SIGNATURE 0x53425355u

static void put32le(uint8_t *p,uint32_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);}
static uint32_t get32le(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint32_t get32be(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}

static int transfer(usb_msc_device *device,uint8_t endpoint,void *buffer,
                    uint32_t bytes,uint32_t expected){
    uint32_t actual=0;
    if(device->transfer(device->controller,endpoint,buffer,bytes,&actual)||actual!=expected)return -1;
    return 0;
}

static int command(usb_msc_device *device,const uint8_t *cdb,uint8_t cdb_length,
                   void *data,uint32_t data_length,int data_in){
    if(!device||!device->transfer||!cdb||!cdb_length||cdb_length>16)return -1;
    uint8_t cbw[31]={0};put32le(cbw,CBW_SIGNATURE);put32le(cbw+4,++device->tag);
    put32le(cbw+8,data_length);cbw[12]=(uint8_t)(data_in?0x80:0);cbw[13]=device->lun;cbw[14]=cdb_length;
    for(uint32_t i=0;i<cdb_length;i++)cbw[15+i]=cdb[i];
    if(transfer(device,device->bulk_out,cbw,sizeof(cbw),sizeof(cbw)))return -1;
    if(data_length&&transfer(device,data_in?device->bulk_in:device->bulk_out,data,data_length,data_length))return -1;
    uint8_t csw[13]={0};if(transfer(device,device->bulk_in,csw,sizeof(csw),sizeof(csw)))return -1;
    if(get32le(csw)!=CSW_SIGNATURE||get32le(csw+4)!=device->tag||get32le(csw+8)>data_length||csw[12])return -1;
    return 0;
}

int usb_msc_probe(usb_msc_device *device,void *controller,
                  usb_bulk_transfer_fn transfer_fn,const usb_interface_info *interface){
    if(!device||!controller||!transfer_fn||!interface||interface->class_code!=USB_CLASS_MASS_STORAGE||
       interface->subclass!=USB_MSC_SCSI||interface->protocol!=USB_MSC_BULK_ONLY||
       !interface->bulk_in.address||!interface->bulk_out.address)return -1;
    *device=(usb_msc_device){.controller=controller,.transfer=transfer_fn,
        .bulk_in=interface->bulk_in.address,.bulk_out=interface->bulk_out.address,.tag=0};
    uint8_t inquiry[6]={0x12,0,0,0,36,0},response[36]={0};
    if(command(device,inquiry,sizeof(inquiry),response,sizeof(response),1))return -1;
    if((response[0]&0x1fu)==0x1fu)return -1;
    uint8_t capacity_cmd[10]={0x25},capacity[8]={0};
    if(command(device,capacity_cmd,sizeof(capacity_cmd),capacity,sizeof(capacity),1))return -1;
    uint32_t last_lba=get32be(capacity),sector_size=get32be(capacity+4);
    if(last_lba==0xffffffffu||sector_size!=512)return -1;
    device->sectors=(uint64_t)last_lba+1;device->sector_size=sector_size;device->ready=1;return 0;
}

int usb_msc_read(void *context,uint64_t lba,uint32_t sectors,void *buffer){
    usb_msc_device *device=context;if(!device||!device->ready||(!buffer&&sectors)||
        lba>device->sectors||sectors>device->sectors-lba)return -1;
    uint8_t *bytes=buffer;
    while(sectors){uint16_t count=sectors>65535u?65535u:(uint16_t)sectors;
        if(lba>0xffffffffu)return -1;uint8_t cdb[10]={0x28,0,(uint8_t)(lba>>24),(uint8_t)(lba>>16),(uint8_t)(lba>>8),(uint8_t)lba,
            0,(uint8_t)(count>>8),(uint8_t)count,0};uint32_t size=(uint32_t)count*512u;
        if(command(device,cdb,sizeof(cdb),bytes,size,1))return -1;
        bytes+=size;lba+=count;sectors-=count;
    }return 0;
}

int usb_msc_write(void *context,uint64_t lba,uint32_t sectors,const void *buffer){
    usb_msc_device *device=context;if(!device||!device->ready||(!buffer&&sectors)||
        lba>device->sectors||sectors>device->sectors-lba)return -1;
    const uint8_t *bytes=buffer;
    while(sectors){uint16_t count=sectors>65535u?65535u:(uint16_t)sectors;
        if(lba>0xffffffffu)return -1;uint8_t cdb[10]={0x2a,0,(uint8_t)(lba>>24),(uint8_t)(lba>>16),(uint8_t)(lba>>8),(uint8_t)lba,
            0,(uint8_t)(count>>8),(uint8_t)count,0};uint32_t size=(uint32_t)count*512u;
        if(command(device,cdb,sizeof(cdb),(void *)bytes,size,0))return -1;
        bytes+=size;lba+=count;sectors-=count;
    }return 0;
}
int usb_msc_flush(void *context){usb_msc_device*d=context;if(!d||!d->ready)return -1;uint8_t cdb[10]={0x35};return command(d,cdb,sizeof(cdb),0,0,0);}
