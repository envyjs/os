#include "hc.h"
#include "../../Arch/x86/io.h"

/* OHCI uses physical pointers in its schedules. The current x86 kernel is
   identity-mapped and runs below 4 GiB, so these DMA objects are addressable. */
#define OHCI_WAIT 2500000u
#define OHCI_TD_COUNT 128u
#define OHCI_SLOTS 4u
typedef struct { uint32_t control,tail,head,next; } ohci_ed;
typedef struct { uint32_t control,cbp,next,be; } ohci_td;
typedef struct {
    _Alignas(256) uint32_t hcca[64];
    _Alignas(16) ohci_ed control_ed,bulk_ed;
    _Alignas(16) ohci_td td[OHCI_TD_COUNT];
    uint32_t requested[OHCI_TD_COUNT];
    _Alignas(16) uint8_t setup[8];
} ohci_dma;
static ohci_dma dma[OHCI_SLOTS];
static volatile uint32_t *regs(usb_controller *c){return (volatile uint32_t *)(uintptr_t)c->base;}
static void pause_short(void){__asm__ volatile("pause");}
static void delay(uint32_t n){while(n--)pause_short();}
static int wait_mask(volatile uint32_t *p,uint32_t mask,uint32_t value){for(uint32_t i=0;i<OHCI_WAIT;i++){if((*p&mask)==value)return 1;pause_short();}return 0;}

uint32_t usb_ohci_ports(usb_controller *c){volatile uint32_t*r=regs(c);uint32_t connected=0,enabled=0,n=c->ports;if(n>15)n=15;
    for(uint32_t i=0;i<n;i++){uint32_t s=r[0x54/4+i];if(s&1)connected|=1u<<i;if(s&2)enabled|=1u<<i;}
    c->connected_ports=connected;c->enabled_ports=enabled;return connected;}

static void reset_port(usb_controller*c,uint32_t i){volatile uint32_t*p=&regs(c)[0x54/4+i];uint32_t s=*p;if(!(s&1))return;
    *p=1u<<4; if(!wait_mask(p,1u<<4,0))return;delay(20000);s=*p;
    if((s&1)&&!(s&2))*p=1u<<1;
}

int usb_ohci_start(usb_controller*c){if(!c||c->kind!=USB_CONTROLLER_OHCI||c->slot>=OHCI_SLOTS)return-1;
    volatile uint32_t*r=regs(c);ohci_dma*m=&dma[c->slot];
    r[0x08/4]=1u; if(!wait_mask(&r[0x08/4],1u,0))return-1;
    r[0x0c/4]=0xffffffffu;r[0x14/4]=0xffffffffu;
    r[0x3c/4]=(uint32_t)(uintptr_t)m->hcca;
    r[0x20/4]=(uint32_t)(uintptr_t)&m->control_ed;
    r[0x28/4]=(uint32_t)(uintptr_t)&m->bulk_ed;
    r[0x04/4]=2u<<6; /* operational state; schedules enabled per transfer */
    if(!wait_mask(&r[0x04/4],3u<<6,2u<<6))return-1;
    c->ready=1;for(uint32_t i=0;i<c->ports&&i<15;i++)reset_port(c,i);(void)usb_ohci_ports(c);return 0;
}

static uint32_t td_control(uint8_t direction,uint8_t toggle,uint8_t rounding){
    /* Data-toggle 0 means carry, 1 means DATA0, 2 means DATA1. */
    return (0xfu<<28)|(1u<<26)|((uint32_t)(direction&3u)<<19)|
           ((uint32_t)(toggle&3u)<<24)|(rounding?1u<<18:0);
}
static int run_list(usb_controller*c,ohci_ed*ed,uint32_t count,uint8_t control,uint32_t*actual){
    volatile uint32_t*r=regs(c);ohci_dma*m=&dma[c->slot];uint32_t listbit=control?1u<<4:1u<<5;
    m->td[count]=(ohci_td){.control=0xf0000000u};
    m->td[count-1].next=(uint32_t)(uintptr_t)&m->td[count];
    ed->head=(uint32_t)(uintptr_t)&m->td[0];ed->head&=~0x0fu;
    ed->tail=(uint32_t)(uintptr_t)&m->td[count];
    m->hcca[0]=0;__asm__ volatile("":::"memory");
    r[0x04/4]|=listbit; r[0x08/4]|=control?2u:4u;
    uint32_t loops=OHCI_WAIT;
    while(loops--){uint32_t head=ed->head;if((head&~0x0fu)==ed->tail)break;pause_short();}
    r[0x04/4]&=~listbit;
    if(!loops)return-1;
    uint32_t total=0;
    for(uint32_t i=0;i<count;i++){uint32_t cc=m->td[i].control>>28;if(cc!=0)return-1;
        uint32_t cbp=m->td[i].cbp,be=m->td[i].be;
        uint32_t remaining=cbp&&be>=cbp?be-cbp+1u:0;
        if(m->requested[i]<remaining)return-1;total+=m->requested[i]-remaining;
    }
    if(actual)*actual=total;return 0;
}

int usb_ohci_transfer(usb_controller*c,uint8_t type,uint8_t address,uint8_t endpoint,
                      uint16_t packet,uint8_t low,const uint8_t setup[8],void*data,
                      uint32_t length,uint32_t*actual){
    if(!c||!c->ready||c->kind!=USB_CONTROLLER_OHCI||c->slot>=OHCI_SLOTS||!packet||length>8192)return-1;
    ohci_dma*m=&dma[c->slot];ohci_ed*ed=type==0?&m->control_ed:&m->bulk_ed;
    uint32_t count=0,limit=OHCI_TD_COUNT-1;uint8_t in=(endpoint&0x80u)!=0;
    ed->control=(uint32_t)address|((uint32_t)(endpoint&15u)<<7)|((uint32_t)low<<13)|((uint32_t)packet<<16);
    if(type==0){if(!setup)return-1;for(uint32_t i=0;i<8;i++)m->setup[i]=setup[i];
        m->td[count]=(ohci_td){td_control(0,1,0), (uint32_t)(uintptr_t)m->setup,
                                 (uint32_t)(uintptr_t)&m->td[1],(uint32_t)(uintptr_t)(m->setup+7)};
        m->requested[count++]=8;
        uint8_t data_in=(setup[0]&0x80u)!=0;uint8_t*p=data;uint32_t left=length;uint8_t toggle=2;
        while(left){if(count>=limit)return-1;uint32_t n=left>packet?packet:left;uint8_t*end=p+n-1;
            m->td[count]=(ohci_td){td_control(data_in?2:1,toggle,data_in), (uint32_t)(uintptr_t)p,
                (uint32_t)(uintptr_t)&m->td[count+1],(uint32_t)(uintptr_t)end};m->requested[count++]=n;p+=n;left-=n;toggle=toggle==2?1:2;}
        uint8_t status_in=length&&!data_in?2:1;
        m->td[count]=(ohci_td){td_control(status_in,2,1),0,0,0};m->requested[count++]=0;
        for(uint32_t i=0;i+1<count;i++)m->td[i].next=(uint32_t)(uintptr_t)&m->td[i+1];
        uint32_t done=0;int rc=run_list(c,ed,count,1,&done);if(actual)*actual=rc?0:done;return rc;
    }
    if(type!=1)return-1;ed->control|=(uint32_t)(in?2u:1u)<<11;
    uint8_t*p=data;uint32_t left=length;uint8_t toggle=2;
    if(!left){m->td[count]=(ohci_td){td_control(in?2:1,toggle,1),0,0,0};m->requested[count++]=0;}
    while(left){if(count>=limit)return-1;uint32_t n=left>packet?packet:left;uint8_t*end=p+n-1;
        m->td[count]=(ohci_td){td_control(in?2:1,toggle,in), (uint32_t)(uintptr_t)p,
            (uint32_t)(uintptr_t)&m->td[count+1],(uint32_t)(uintptr_t)end};m->requested[count++]=n;p+=n;left-=n;
        if(n==packet)toggle=toggle==2?1:2;
    }
    for(uint32_t i=0;i+1<count;i++)m->td[i].next=(uint32_t)(uintptr_t)&m->td[i+1];
    uint32_t done=0;int rc=run_list(c,ed,count,0,&done);if(actual)*actual=rc?0:done;return rc;
}
