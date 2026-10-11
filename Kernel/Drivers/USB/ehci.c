#include "hc.h"
#include "../../Arch/x86/io.h"

#define EHCI_WAIT 3000000u
typedef struct {_Alignas(32) uint32_t w[14];} ehci_qh;
typedef struct {_Alignas(32) uint32_t w[8];} ehci_td;
typedef struct {_Alignas(4096) uint32_t frames[1024];ehci_qh head,transfer;_Alignas(32) ehci_td tds[4];uint32_t requested[4];uint8_t toggles[128][16][2];} ehci_dma;
static ehci_dma state[USB_MAX_CONTROLLERS];
static void pause_short(void){__asm__ volatile("pause");}
static volatile uint32_t*cap(usb_controller*c){return(volatile uint32_t*)(uintptr_t)c->base;}
static uint32_t caplen(usb_controller*c){return cap(c)[0]&0xffu;}
static volatile uint32_t*op(usb_controller*c){return(volatile uint32_t*)(uintptr_t)(c->base+caplen(c));}
static int wait_reg(volatile uint32_t*r,uint32_t mask,uint32_t value){for(uint32_t i=0;i<EHCI_WAIT;i++){if((*r&mask)==value)return 1;pause_short();}return 0;}
static void delay(uint32_t n){while(n--)pause_short();}
uint32_t usb_ehci_ports(usb_controller*c){volatile uint32_t*r=op(c);uint32_t a=0,b=0,n=c->ports;if(n>15)n=15;
    for(uint32_t i=0;i<n;i++){uint32_t s=r[0x44/4+i];if(s&1)a|=1u<<i;if(s&2)b|=1u<<i;}c->connected_ports=a;c->enabled_ports=b;return a;}
static void legacy_handoff(usb_controller*c){uint32_t eecp=(cap(c)[2]>>8)&0xffu;if(!eecp)return;
    for(uint32_t n=0;eecp&&n<32;n++){volatile uint32_t*p=(volatile uint32_t*)(uintptr_t)(c->base+eecp);uint32_t v=*p;
        if((v&255u)==1u){p[0]=v|(1u<<24);p[1]=0;for(uint32_t i=0;i<EHCI_WAIT;i++){if(!(p[0]&(1u<<16)))break;pause_short();}break;}eecp=(v>>8)&255u;}}
static void reset_port(usb_controller*c,uint32_t i){volatile uint32_t*r=op(c);volatile uint32_t*p=&r[0x44/4+i];uint32_t s=*p;if(!(s&1))return;
    *p=(s&~((1u<<1)|(1u<<3)|(1u<<5)))|(1u<<12);delay(10000);*p=(*p&~((1u<<1)|(1u<<3)|(1u<<5)))|(1u<<8);delay(400000);
    s=*p;*p=s&~((1u<<1)|(1u<<3)|(1u<<5)|(1u<<8));delay(100000);s=*p;
    if((s&1u)&&!(s&2u))*p=(s&~((1u<<1)|(1u<<3)|(1u<<5)))|(1u<<13);}
int usb_ehci_start(usb_controller*c){if(!c||c->slot>=USB_MAX_CONTROLLERS||c->kind!=USB_CONTROLLER_EHCI)return-1;
    legacy_handoff(c);volatile uint32_t*r=op(c);r[0]&=~1u;if(!wait_reg(&r[1],1u<<12,1u<<12))return-1;r[0]|=2;if(!wait_reg(&r[0],2,0))return-1;
    ehci_dma*m=&state[c->slot];m->head.w[0]=(uint32_t)(uintptr_t)&m->transfer|2u;m->head.w[1]=(1u<<15)|(1u<<14)|(64u<<16)|(4u<<28);m->head.w[4]=1;m->head.w[5]=1;m->head.w[6]=0;
    m->transfer.w[0]=(uint32_t)(uintptr_t)&m->head|2u;for(uint32_t i=0;i<1024;i++)m->frames[i]=(uint32_t)(uintptr_t)&m->transfer|2u;
    r[4]=0;r[5]=(uint32_t)(uintptr_t)m->frames;r[6]=(uint32_t)(uintptr_t)&m->head;r[2]=0;r[1]=0xffffffffu;r[0]=1u|(1u<<4)|(1u<<5);r[0x40/4]=1;
    if(!wait_reg(&r[4],(1u<<14)|(1u<<15),(1u<<14)|(1u<<15)))return-1;c->ready=1;
    for(uint32_t i=0;i<c->ports&&i<15;i++)reset_port(c,i);(void)usb_ehci_ports(c);return 0;}
static uint32_t qh_char(uint8_t a,uint8_t ep,uint16_t mps,uint8_t low,uint8_t control){return a|((uint32_t)(ep&15)<<8)|((uint32_t)(low?1:2)<<12)|(control?0u:1u<<14)|((control&&low)?1u<<27:0u)|((uint32_t)mps<<16)|(4u<<28);}
static void build_td(ehci_dma*m,uint32_t index,uint32_t next,uint8_t pid,void*buffer,uint32_t len,uint8_t toggle,uint8_t ioc){ehci_td*t=&m->tds[index];uintptr_t p=(uintptr_t)buffer;t->w[0]=next;t->w[1]=1;m->requested[index]=len;
    t->w[2]=(1u<<7)|(3u<<10)|((uint32_t)pid<<8)|((len&0x7fff)<<16)|((uint32_t)toggle<<31)|(ioc?1u<<15:0);t->w[3]=(uint32_t)p;
    uintptr_t page=p&~(uintptr_t)4095;for(uint32_t i=1;i<5;i++)t->w[3+i]=(uint32_t)(page+i*4096u);}
static int run_qh(usb_controller*c,uint8_t type,uint32_t count,uint32_t*actual){ehci_dma*m=&state[c->slot];volatile uint32_t*r=op(c);
    for(uint32_t i=0;i<count;i++)m->tds[i].w[0]=i+1<count?(uint32_t)(uintptr_t)&m->tds[i+1]:1u;
    if(type==2){m->head.w[0]=(uint32_t)(uintptr_t)&m->head|2u;m->transfer.w[0]=(uint32_t)(uintptr_t)&m->transfer|2u;}
    else{m->head.w[0]=(uint32_t)(uintptr_t)&m->transfer|2u;m->transfer.w[0]=(uint32_t)(uintptr_t)&m->head|2u;}
    m->transfer.w[4]=(uint32_t)(uintptr_t)&m->tds[0];m->transfer.w[5]=1;m->transfer.w[6]=0;m->transfer.w[7]=0;
    uint32_t cmd=r[0];if(type==2){cmd&=~(1u<<5);cmd|=1u<<4;}else{cmd&=~(1u<<4);cmd|=1u<<5;}r[0]=cmd;
    uint32_t bit=type==2?1u<<14:1u<<15;if(!wait_reg(&r[1],bit,bit))return-1;__asm__ volatile("":::"memory");uint32_t total=0;
    for(uint32_t i=0;i<count;i++){uint32_t loops=type==2?100000u:EHCI_WAIT,t;do{t=m->tds[i].w[2];if(!(t&(1u<<7)))break;pause_short();}while(--loops);
        if(t&(1u<<7)){r[0]&=~(type==2?1u<<4:1u<<5);(void)wait_reg(&r[1],bit,0);m->transfer.w[4]=1;return type==2?-2:-1;}
        if(t&0x7cu){m->transfer.w[4]=1;return-1;}uint32_t remain=(t>>16)&0x7fffu;total+=m->requested[i]-remain;}
    m->transfer.w[4]=1;r[0]&=~bit;if(!wait_reg(&r[1],bit,0))return-1;if(actual)*actual=total;return 0;}
int usb_ehci_transfer(usb_controller*c,uint8_t type,uint8_t a,uint8_t endpoint,uint16_t packet,uint8_t low,const uint8_t setup[8],void*data,uint32_t len,uint32_t*actual){
    if(!c||!c->ready||c->kind!=USB_CONTROLLER_EHCI||!packet||len>16384)return-1;ehci_dma*m=&state[c->slot];uint32_t n=0;uint8_t ep=endpoint&15;
    if(type==0){if(!setup)return-1;build_td(m,n++,1,2,(void*)setup,8,0,0);
        if(len){build_td(m,n++,1,(setup[0]&0x80)?1:0,data,len,1,0);m->tds[1].w[1]=(uint32_t)(uintptr_t)&m->tds[2];}
        build_td(m,n++,1,(len&&!(setup[0]&0x80))?1:0,0,0,1,1);m->transfer.w[1]=qh_char(a,endpoint,packet,low,1);m->transfer.w[2]=1u<<30;
    }else{if(low)return-1;uint8_t in=(endpoint&0x80)!=0;uint8_t*p=data;uint32_t left=len;
        uint8_t start_toggle=m->toggles[a][ep][in],toggle=start_toggle;
        if(!left)build_td(m,n++,1,in?1:0,0,0,toggle,1);
        while(left){if(n>=4)return-1;uint32_t chunk=left>16384?16384:left;build_td(m,n++,1,in?1:0,p,chunk,toggle,1);toggle^=(uint8_t)(((chunk+packet-1)/packet)&1);p+=chunk;left-=chunk;}
        m->transfer.w[1]=qh_char(a,ep,packet,0,0);m->transfer.w[2]=type==2?1u:0u;}
    uint32_t done=0;int rc=run_qh(c,type,n,&done);
    if(type==0&&done>=8)done-=8;if(actual)*actual=done;
    if(type!=0&&rc==0){uint8_t in=(endpoint&0x80)!=0;uint32_t packets=done?(done+packet-1u)/packet:1u;m->toggles[a][ep][in]^=(uint8_t)(packets&1u);}return rc;}
