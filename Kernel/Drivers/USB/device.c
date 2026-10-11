#include "device.h"

static uint16_t le16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}

uint32_t usb_parse_configuration(const uint8_t *bytes,uint32_t length,
                                 usb_interface_info *interfaces,uint32_t capacity){
    if(!bytes||!interfaces||capacity==0||length<9||bytes[0]<9||bytes[1]!=2)return 0;
    uint32_t total=le16(bytes+2);if(total<9||total>length)return 0;
    uint8_t configuration=bytes[5];uint32_t count=0;usb_interface_info *active=0;
    for(uint32_t offset=0;offset<total;){
        if(total-offset<2)return 0;uint8_t size=bytes[offset],type=bytes[offset+1];
        if(size<2||size>total-offset)return 0;
        const uint8_t *d=bytes+offset;
        if(type==4){
            active=0;
            if(size<9)return 0;
            if(d[3]!=0) { offset+=size;continue; }
            if(count>=capacity)return 0;
            active=&interfaces[count++];
            *active=(usb_interface_info){.number=d[2],.alternate=d[3],
                .class_code=d[5],.subclass=d[6],.protocol=d[7],
                .configuration=configuration};
        } else if(type==5&&active){
            if(size<7)return 0;uint8_t address=d[2],transfer=d[3]&3u;
            uint16_t packet=le16(d+4)&0x07ffu;
            if((address&15u)==0||packet==0)return 0;
            usb_endpoint_info endpoint={.address=address,.attributes=d[3],
                .interval=d[6],.max_packet=packet};
            if(transfer==3&&(address&0x80u)&&!active->interrupt_in.address)
                active->interrupt_in=endpoint;
            if(transfer==2){
                if(address&0x80u){if(!active->bulk_in.address)active->bulk_in=endpoint;}
                else if(!active->bulk_out.address)active->bulk_out=endpoint;
            }
        }
        offset+=size;
    }
    return count;
}
