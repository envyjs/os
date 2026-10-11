#include "hc.h"
#include "../../Arch/x86/io.h"

#define UHCI_WAIT 2500000u
#define UHCI_TD_MAX 260u
#define TD_ACTIVE (1u<<23)
#define TD_LOW_SPEED (1u<<26)
#define TD_SPD (1u<<29)
#define TD_ERRORS ((1u<<17)|(1u<<18)|(1u<<20)|(1u<<21)|(1u<<22))
typedef struct {
    _Alignas(4096) uint32_t frames[1024];
    _Alignas(16) uint32_t queue[2];
    _Alignas(16) uint32_t td[UHCI_TD_MAX][4];
    uint8_t toggle[128][16][2];
} uhci_dma;
static uhci_dma dma[USB_MAX_CONTROLLERS];

static void pause_short(void){__asm__ volatile("pause");}
static uint16_t port_read(usb_controller *c,uint32_t port){return in16((uint16_t)(c->base+0x10u+port*2u));}
static void port_write(usb_controller *c,uint32_t port,uint16_t value){out16((uint16_t)(c->base+0x10u+port*2u),value);}

uint32_t usb_uhci_ports(usb_controller *controller){
    uint32_t connected=0,enabled=0,count=controller->ports;if(count>2)count=2;
    for(uint32_t i=0;i<count;i++){uint16_t s=port_read(controller,i);if(s&1)connected|=1u<<i;if(s&2)enabled|=1u<<i;}
    controller->connected_ports=connected;controller->enabled_ports=enabled;return connected;
}
static int wait_halted(usb_controller *c){for(uint32_t i=0;i<UHCI_WAIT;i++){if(in16((uint16_t)(c->base+2))&(1u<<5))return 1;pause_short();}return 0;}
static void delay(uint32_t loops){while(loops--)pause_short();}
static void reset_port(usb_controller *c,uint32_t i){
    uint16_t p=port_read(c,i);if(!(p&1))return;
    /* Port change bits are write-one-to-clear. */
    port_write(c,i,(uint16_t)((p&~((1u<<1)|(1u<<3)))|(1u<<9)));delay(400000);
    p=port_read(c,i);port_write(c,i,(uint16_t)(p&~((1u<<1)|(1u<<3)|(1u<<9))));delay(100000);
    p=port_read(c,i);if(p&1)port_write(c,i,(uint16_t)((p&~((1u<<1)|(1u<<3)))|(1u<<2)));
}
int usb_uhci_start(usb_controller *c){
    if(!c||c->slot>=USB_MAX_CONTROLLERS||c->kind!=USB_CONTROLLER_UHCI)return -1;
    uhci_dma *m=&dma[c->slot];for(uint32_t i=0;i<1024;i++)m->frames[i]=(uint32_t)(uintptr_t)m->queue|2u;
    m->queue[0]=1;m->queue[1]=1;
    uint16_t io=(uint16_t)c->base;out16(io,0);if(!wait_halted(c))return -1;
    out16((uint16_t)(io+2),0xffff);out16((uint16_t)(io+4),0);
    out16((uint16_t)(io+6),0);out32((uint16_t)(io+8),(uint32_t)(uintptr_t)m->frames);
    out8((uint16_t)(io+0x0c),0x40);
    out16(io,0x00c1);delay(10000);
    if(!(in16((uint16_t)(io+2))&(1u<<5))){c->ready=1;for(uint32_t i=0;i<c->ports&&i<2;i++)reset_port(c,i);return 0;}
    c->ready=0;return -1;
}

static uint32_t make_td(uhci_dma *m,uint32_t index,uint32_t next,uint8_t pid,
                        uint8_t address,uint8_t endpoint,uint8_t toggle,
                        void *buffer,uint32_t bytes,uint8_t low_speed){
    uint32_t max_length=bytes?bytes-1u:0x7ffu;
    uint32_t token=(uint32_t)pid|((uint32_t)address<<8)|((uint32_t)endpoint<<15)|
        ((uint32_t)(toggle&1u)<<19)|(max_length<<21);
    uint32_t status=0x7ffu|(3u<<27)|TD_ACTIVE|(pid==0x69?TD_SPD:0)|(low_speed?TD_LOW_SPEED:0);
    m->td[index][0]=next;m->td[index][1]=status;m->td[index][2]=token;
    m->td[index][3]=(uint32_t)(uintptr_t)buffer;
    return (uint32_t)(uintptr_t)&m->td[index][0];
}
static int queue_transfer(usb_controller *c,uint32_t count,uint32_t *actual,uint8_t type){
    uhci_dma *m=&dma[c->slot];for(uint32_t i=0;i<count;i++)m->td[i][0]=i+1<count?(uint32_t)(uintptr_t)&m->td[i+1][0]:1u;
    m->queue[1]=(uint32_t)(uintptr_t)&m->td[0][0];__asm__ volatile("":::"memory");
    uint32_t total=0;
    for(uint32_t i=0;i<count;i++){
        uint32_t loops=UHCI_WAIT,status;
        do{status=m->td[i][1];if(!(status&TD_ACTIVE))break;if(type==2&&(status&(1u<<19))){m->queue[1]=1;delay(2000);return -2;}pause_short();}while(--loops);
        if(status&TD_ACTIVE){m->queue[1]=1;out16((uint16_t)c->base,0);(void)wait_halted(c);out16((uint16_t)c->base,0x00c1);return -1;}
        if(status&TD_ERRORS){m->queue[1]=1;return -1;}
        uint32_t got=status&0x7ffu;if(got!=0x7ffu)total+=got+1u;
    }
    m->queue[1]=1;__asm__ volatile("":::"memory");if(actual)*actual=total;return 0;
}

static int control(usb_controller *c,uint8_t address,uint8_t ep,uint16_t mps,
                   uint8_t low,const uint8_t setup[8],void *data,uint32_t length,uint32_t *actual){
    if(!setup||length>16384u||mps==0||mps>64)return -1;
    uhci_dma *m=&dma[c->slot];uint32_t count=0;uint8_t data_in=setup[0]&0x80u?1:0,toggle=1;
    make_td(m,count++,1,0x2d,address,ep,0,(void *)setup,8,low);
    uint8_t *p=data;uint32_t left=length;
    while(left){if(count>=UHCI_TD_MAX-1)return -1;uint32_t chunk=left;if(chunk>mps)chunk=mps;
        uint32_t page=4096u-((uint32_t)(uintptr_t)p&4095u);if(chunk>page)chunk=page;
        make_td(m,count++,1,data_in?0x69:0xe1,address,ep,toggle,p,chunk,low);
        p+=chunk;left-=chunk;toggle^=1;
    }
    make_td(m,count++,1,data_in?0xe1:0x69,address,ep,1,0,0,low);
    uint32_t transferred=0;int result=queue_transfer(c,count,&transferred,0);if(actual)*actual=transferred>=8?transferred-8:0;return result;
}
static int data_transfer(usb_controller *c,uint8_t type,uint8_t address,uint8_t ep,
                         uint16_t mps,uint8_t low,void *data,uint32_t length,uint32_t *actual){
    if(length>16384u||mps==0||mps>64)return -1;uhci_dma *m=&dma[c->slot];uint8_t in=(ep&0x80u)!=0;
    uint8_t endpoint=ep&15u,*p=data;uint8_t *toggle=&m->toggle[address][endpoint][in];uint8_t next_toggle=*toggle;uint32_t count=0,left=length;
    while(left){if(count>=UHCI_TD_MAX)return -1;uint32_t chunk=left;if(chunk>mps)chunk=mps;
        uint32_t page=4096u-((uint32_t)(uintptr_t)p&4095u);if(chunk>page)chunk=page;
        make_td(m,count++,1,in?0x69:0xe1,address,endpoint,next_toggle,p,chunk,low);next_toggle^=1;p+=chunk;left-=chunk;
    }
    if(!count)make_td(m,count++,1,in?0x69:0xe1,address,endpoint,next_toggle,0,0,low);
    int result=queue_transfer(c,count,actual,type);if(!result)*toggle=next_toggle;return result;
}
int usb_uhci_transfer(usb_controller *c,uint8_t type,uint8_t address,uint8_t ep,
                     uint16_t mps,uint8_t low,const uint8_t setup[8],void *data,
                     uint32_t length,uint32_t *actual){
    if(!c||!c->ready||c->kind!=USB_CONTROLLER_UHCI)return -1;
    if(type==0)return control(c,address,ep,mps,low,setup,data,length,actual);
    return data_transfer(c,type,address,ep,mps,low,data,length,actual);
}
