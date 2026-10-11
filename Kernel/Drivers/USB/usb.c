#include "usb.h"
#include "hc.h"
#include "../../Arch/x86/io.h"
#include "device.h"
#include "hid.h"
#include "msc.h"
#include "../../Drivers/Input/ps2.h"

#define PCI_CONFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA 0xcfc
#define USB_MAX_DEVICES 32
#define USB_CONFIG_MAX 2048
#define USB_MAX_INTERFACES 16
typedef struct {usb_controller *hc;uint8_t address,low_speed,configured;uint16_t ep0_packet;uint32_t port;
    usb_interface_info interfaces[USB_MAX_INTERFACES];uint32_t interface_count;} usb_device_state;
typedef struct {usb_controller *hc;uint8_t address,low_speed;uint16_t in_packet,out_packet;} usb_msc_link;
typedef struct {usb_controller *hc;uint8_t address,endpoint,low_speed,mouse,interval;uint16_t packet;uint8_t report[64];} usb_hid_link;
static usb_device_state devices[USB_MAX_DEVICES];static uint32_t device_count;
static usb_msc_link msc_links[USB_MAX_STORAGE];static usb_msc_device storage[USB_MAX_STORAGE];static uint32_t storage_count;
static usb_hid_link hid_links[USB_MAX_DEVICES*2];static uint32_t hid_count;
static int msc_transfer(void *context,uint8_t endpoint,void *buffer,uint32_t bytes,uint32_t *actual);

static uint32_t pci_read(uint32_t address, uint8_t offset) {
    out32(PCI_CONFIG_ADDRESS, address | (offset & 0xfcu));
    return in32(PCI_CONFIG_DATA);
}

static uint32_t pci_address(uint8_t bus, uint8_t device, uint8_t function) {
    return 0x80000000u | ((uint32_t)bus << 16) |
           ((uint32_t)device << 11) | ((uint32_t)function << 8);
}

static void pci_enable(uint32_t address, uint16_t flags) {
    out32(PCI_CONFIG_ADDRESS, address | 4u);
    out16(PCI_CONFIG_DATA, (uint16_t)(flags | 4u)); /* bus master */
}

static uint32_t read_ports(usb_controller *controller) {
    switch(controller->kind){
    case USB_CONTROLLER_UHCI:return usb_uhci_ports(controller);
    case USB_CONTROLLER_OHCI:return usb_ohci_ports(controller);
    case USB_CONTROLLER_EHCI:return usb_ehci_ports(controller);
    case USB_CONTROLLER_XHCI:return usb_xhci_ports(controller);
    default:return 0;
    }
}

static int controller_probe(uint8_t bus, uint8_t device, uint8_t function,
                            uint32_t address, usb_controller *result) {
    uint32_t class_reg = pci_read(address, 8);
    if ((class_reg >> 24) != 0x0c || ((class_reg >> 16) & 0xffu) != 3)
        return 0;

    uint8_t programming_interface = (uint8_t)(class_reg >> 8);
    usb_controller_kind kind;
    switch (programming_interface) {
    case 0x00: kind = USB_CONTROLLER_UHCI; break;
    case 0x10: kind = USB_CONTROLLER_OHCI; break;
    case 0x20: kind = USB_CONTROLLER_EHCI; break;
    case 0x30: kind = USB_CONTROLLER_XHCI; break;
    default: return 0;
    }

    uint32_t bar = pci_read(address, kind == USB_CONTROLLER_UHCI ? 0x20 : 0x10);
    uint32_t base;
    if (kind == USB_CONTROLLER_UHCI) {
        if (!(bar & 1u)) return 0;
        base = bar & 0xfffcu;
        if (!base || base > 0xffe0u) return 0;
    } else {
        uint32_t bar_type=(bar>>1)&3u;
        if ((bar&1u)||bar_type==3u||(bar_type==2u&&pci_read(address,0x14)!=0))return 0;
        base = bar & ~15u;
        if (!base || base > 0xffffe000u) return 0;
    }

    uint16_t command = (uint16_t)pci_read(address, 4);
    pci_enable(address, (uint16_t)(command |
        (kind == USB_CONTROLLER_UHCI ? 1u : 2u)));
    *result = (usb_controller){
        .kind = kind, .bus = bus, .device = device, .function = function,
        .base = base
    };

    volatile uint32_t *mmio = (volatile uint32_t *)(uintptr_t)base;
    switch (kind) {
    case USB_CONTROLLER_UHCI:
        result->ports = 2;
        break;
    case USB_CONTROLLER_OHCI:
        if ((mmio[0] & 0xffu) != 0x10u) return 0;
        result->ports = (uint8_t)(mmio[0x48u / 4u] & 0xffu);
        break;
    case USB_CONTROLLER_EHCI: {
        uint32_t cap_length = mmio[0] & 0xffu;
        if (cap_length < 0x10u || cap_length > 0x40u) return 0;
        result->ports = (uint8_t)(mmio[1] & 0x0fu);
        break;
    }
    case USB_CONTROLLER_XHCI: {
        uint32_t cap_length = mmio[0] & 0xffu;
        if (cap_length < 0x20u || cap_length > 0x100u) return 0;
        result->ports = (uint8_t)(mmio[1] >> 24);
        break;
    }
    }
    if (result->ports > 32) result->ports = 32;
    (void)read_ports(result);
    return 1;
}

uint32_t usb_init(usb_controller *controllers, uint32_t capacity) {
    if (!controllers || !capacity) return 0;
    if (capacity > USB_MAX_CONTROLLERS) capacity = USB_MAX_CONTROLLERS;
    uint32_t count = 0;
    device_count=storage_count=hid_count=0;usb_hid_reset();
    uint32_t xhci_count=0,ohci_count=0;
    for (uint32_t bus = 0; bus < 256 && count < capacity; ++bus) {
        for (uint32_t device = 0; device < 32 && count < capacity; ++device) {
            uint32_t first = pci_address((uint8_t)bus, (uint8_t)device, 0);
            if ((pci_read(first, 0) & 0xffffu) == 0xffffu) continue;
            uint32_t functions = (pci_read(first, 0x0c) & 0x00800000u) ? 8 : 1;
            for (uint32_t function = 0; function < functions && count < capacity; ++function) {
                uint32_t address = pci_address((uint8_t)bus, (uint8_t)device,
                                               (uint8_t)function);
                if ((pci_read(address, 0) & 0xffffu) == 0xffffu) continue;
                if (controller_probe((uint8_t)bus, (uint8_t)device,
                                     (uint8_t)function, address,
                                     &controllers[count])) {
                    controllers[count].slot=controllers[count].kind==USB_CONTROLLER_XHCI?
                        (uint8_t)(xhci_count++):controllers[count].kind==USB_CONTROLLER_OHCI?
                        (uint8_t)(ohci_count++):(uint8_t)count;
                    switch(controllers[count].kind){
                    case USB_CONTROLLER_UHCI:(void)usb_uhci_start(&controllers[count]);break;
                    case USB_CONTROLLER_OHCI:(void)usb_ohci_start(&controllers[count]);break;
                    case USB_CONTROLLER_EHCI:(void)usb_ehci_start(&controllers[count]);break;
                    case USB_CONTROLLER_XHCI:(void)usb_xhci_start(&controllers[count]);break;
                    default:break;
                    }
                    ++count;
                }
            }
        }
    }
    for(uint32_t i=0;i<count;i++)if(controllers[i].ready){
        for(uint32_t port=0;port<controllers[i].ports&&port<32;port++){
            if(!(controllers[i].connected_ports&(1u<<port))||device_count>=USB_MAX_DEVICES)continue;
            uint8_t address=0,low=0;uint16_t packet=8;
            if(controllers[i].kind==USB_CONTROLLER_XHCI){
                if(usb_xhci_prepare_port(&controllers[i],port,&address,&packet,&low))continue;
            }else if(controllers[i].kind==USB_CONTROLLER_UHCI){
                low=(uint8_t)((in16((uint16_t)(controllers[i].base+0x10u+port*2u))>>8)&1u);
            }else if(controllers[i].kind==USB_CONTROLLER_EHCI){
                volatile uint32_t *mmio=(volatile uint32_t *)(uintptr_t)controllers[i].base;
                volatile uint32_t *r=(volatile uint32_t *)(uintptr_t)(controllers[i].base+(mmio[0]&0xffu));
                uint32_t status=r[0x44/4+port];if(!(status&2u))continue;
            }
            usb_device_state *d=&devices[device_count];*d=(usb_device_state){.hc=&controllers[i],.address=address,
                .low_speed=low,.ep0_packet=packet,.port=port};
            uint8_t descriptor[18]={0},setup[8]={0x80,6,0,1,0,0,8,0};
            if(usb_control_transfer(d->hc,address,0,packet,low,setup,descriptor,8))continue;
            uint16_t max_packet=descriptor[7];if(controllers[i].kind==USB_CONTROLLER_XHCI&&max_packet==9)max_packet=512;
            if(max_packet!=8&&max_packet!=16&&max_packet!=32&&max_packet!=64&&max_packet!=512)continue;
            if(low&&max_packet!=8)continue;
            d->ep0_packet=max_packet;
            setup[6]=18;if(usb_control_transfer(d->hc,address,0,max_packet,low,setup,descriptor,18))continue;
            if(controllers[i].kind!=USB_CONTROLLER_XHCI){
                uint8_t new_address=(uint8_t)(1+(device_count%120));uint8_t set_address[8]={0,5,new_address,0,0,0,0,0};
                if(usb_control_transfer(d->hc,address,0,max_packet,low,set_address,0,0))continue;
                d->address=new_address;for(uint32_t wait=0;wait<50000;wait++)__asm__ volatile("pause");
            }
            uint8_t config_head[9]={0};setup[0]=0x80;setup[1]=6;setup[2]=0;setup[3]=2;setup[4]=0;setup[5]=0;setup[6]=9;setup[7]=0;
            if(usb_control_transfer(d->hc,d->address,0,max_packet,low,setup,config_head,9))continue;
            uint16_t config_length=(uint16_t)(config_head[2]|((uint16_t)config_head[3]<<8));
            if(config_length<9||config_length>USB_CONFIG_MAX)continue;
            uint8_t config[USB_CONFIG_MAX];setup[6]=(uint8_t)config_length;setup[7]=(uint8_t)(config_length>>8);
            if(usb_control_transfer(d->hc,d->address,0,max_packet,low,setup,config,config_length))continue;
            uint32_t interfaces=usb_parse_configuration(config,config_length,d->interfaces,USB_MAX_INTERFACES);
            if(!interfaces)continue;d->interface_count=interfaces;
            uint8_t set_config[8]={0,9,config[5],0,0,0,0,0};
            if(usb_control_transfer(d->hc,d->address,0,max_packet,low,set_config,0,0))continue;
            d->configured=1;
            for(uint32_t j=0;j<interfaces;j++){
                usb_interface_info *in=&d->interfaces[j];
                if(in->class_code==USB_CLASS_HID&&in->subclass==USB_HID_BOOT&&
                   (in->protocol==USB_HID_KEYBOARD||in->protocol==USB_HID_MOUSE)&&in->interrupt_in.address&&in->interrupt_in.max_packet<=64&&hid_count<USB_MAX_DEVICES*2){
                    uint8_t request[8]={0x21,0x0b,0,0,in->number,0,0,0};
                    (void)usb_control_transfer(d->hc,d->address,0,max_packet,low,request,0,0);
                    request[1]=0x0a;request[2]=0;request[3]=0;(void)usb_control_transfer(d->hc,d->address,0,max_packet,low,request,0,0);
                    hid_links[hid_count++]=(usb_hid_link){.hc=d->hc,.address=d->address,.endpoint=in->interrupt_in.address,
                        .low_speed=low,.mouse=(uint8_t)(in->protocol==USB_HID_MOUSE),.interval=in->interrupt_in.interval,.packet=in->interrupt_in.max_packet};
                }else if(in->class_code==USB_CLASS_MASS_STORAGE&&storage_count<USB_MAX_STORAGE){
                    usb_msc_link *link=&msc_links[storage_count];*link=(usb_msc_link){.hc=d->hc,.address=d->address,.low_speed=low,
                        .in_packet=in->bulk_in.max_packet,.out_packet=in->bulk_out.max_packet};
                    if(!usb_msc_probe(&storage[storage_count],link,msc_transfer,in))storage_count++;
                }
            }
            ++device_count;
        }
    }
    return count;
}

const char *usb_controller_name(usb_controller_kind kind) {
    switch (kind) {
    case USB_CONTROLLER_UHCI: return "UHCI (USB 1.x)";
    case USB_CONTROLLER_OHCI: return "OHCI (USB 1.x)";
    case USB_CONTROLLER_EHCI: return "EHCI (USB 2.0)";
    case USB_CONTROLLER_XHCI: return "xHCI (USB 1.x/2.0/3.x)";
    default: return "unknown USB controller";
    }
}

static int transfer(usb_controller *c,uint8_t type,uint8_t address,uint8_t endpoint,
                    uint16_t packet,uint8_t low,uint8_t interval,const uint8_t setup[8],void *data,
                    uint32_t length,uint32_t *actual){
    if(!c||!c->ready)return -1;
    switch(c->kind){
    case USB_CONTROLLER_UHCI:return usb_uhci_transfer(c,type,address,endpoint,packet,low,setup,data,length,actual);
    case USB_CONTROLLER_OHCI:return usb_ohci_transfer(c,type,address,endpoint,packet,low,setup,data,length,actual);
    case USB_CONTROLLER_EHCI:return usb_ehci_transfer(c,type,address,endpoint,packet,low,setup,data,length,actual);
    case USB_CONTROLLER_XHCI:return usb_xhci_transfer(c,type,address,endpoint,packet,low,interval,setup,data,length,actual);
    default:return -1;
    }
}
int usb_control_transfer(usb_controller *c,uint8_t address,uint8_t endpoint,uint16_t packet,
                         uint8_t low,const uint8_t setup[8],void *data,uint16_t length){
    uint32_t actual=0;int result=transfer(c,0,address,endpoint,packet,low,0,setup,data,length,&actual);
    return result||actual!=length?-1:0;
}
int usb_bulk_transfer(usb_controller *c,uint8_t address,uint8_t endpoint,uint16_t packet,
                      uint8_t low,void *data,uint32_t length,uint32_t *actual){
    return transfer(c,1,address,endpoint,packet,low,0,0,data,length,actual);
}
int usb_interrupt_transfer(usb_controller *c,uint8_t address,uint8_t endpoint,uint16_t packet,
                           uint8_t low,uint8_t interval,void *data,uint32_t length,uint32_t *actual){
    return transfer(c,2,address,endpoint,packet,low,interval,0,data,length,actual);
}
static int msc_transfer(void *context,uint8_t endpoint,void *buffer,uint32_t bytes,uint32_t *actual){
    usb_msc_link *link=context;if(!link)return -1;uint16_t packet=(endpoint&0x80u)?link->in_packet:link->out_packet;
    if(!packet)return -1;uint32_t total=0;uint8_t *p=buffer;
    if(!bytes){uint32_t done=0;return usb_bulk_transfer(link->hc,link->address,endpoint,packet,link->low_speed,buffer,0,&done);}
    while(total<bytes){uint32_t chunk=bytes-total;if(chunk>4096)chunk=4096;uint32_t done=0;
        if(usb_bulk_transfer(link->hc,link->address,endpoint,packet,link->low_speed,p+total,chunk,&done)||done!=chunk)return -1;
        total+=done;
    }if(actual)*actual=total;return 0;
}
uint32_t usb_storage_count(void){return storage_count;}
usb_msc_device *usb_storage_get(uint32_t index){return index<storage_count?&storage[index]:0;}
void usb_poll(void){
    for(uint32_t i=0;i<hid_count;i++){usb_hid_link *h=&hid_links[i];uint32_t actual=0;
        if(!usb_interrupt_transfer(h->hc,h->address,h->endpoint,h->packet,h->low_speed,h->interval,h->report,h->packet,&actual)&&actual){
            if(h->mouse)(void)usb_hid_mouse_report(h->report,actual);else(void)usb_hid_keyboard_report(h->report,actual);
        }
    }
}
