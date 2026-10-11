#include "hc.h"
#include "../../Arch/x86/io.h"

#define XHCI_WAIT 3000000u
#define XHCI_SLOTS 8u
#define XHCI_CONTROLLERS 4u
#define XHCI_RING_TRBS 16u
typedef struct {uint32_t d[4];} xhci_trb;
typedef struct {_Alignas(64) uint64_t address;uint32_t size,reserved;} xhci_erst;
typedef struct {
    _Alignas(64) uint64_t dcbaa[256];
    _Alignas(64) xhci_trb command[256];
    _Alignas(64) xhci_trb event[256];
    _Alignas(64) xhci_erst erst;
    _Alignas(64) uint32_t output[XHCI_SLOTS][32][8];
    _Alignas(64) uint32_t input[XHCI_SLOTS][34][8];
    _Alignas(64) xhci_trb rings[XHCI_SLOTS][32][XHCI_RING_TRBS];
    _Alignas(4096) uint8_t scratch_pages[4][4096];
    _Alignas(64) uint64_t scratch_array[4];
    uint16_t cmd_index,event_index,ring_index[XHCI_SLOTS][32];
    uint8_t cmd_cycle,event_cycle,ring_cycle[XHCI_SLOTS][32],enabled_slots;
    uint8_t speed[XHCI_SLOTS+1],address[XHCI_SLOTS+1];uint16_t mps[XHCI_SLOTS+1];
    uint32_t endpoint_mask[XHCI_SLOTS+1];
    uint8_t slot_for_address[128];
    uint8_t pending[XHCI_SLOTS+1][32],completed[XHCI_SLOTS+1][32];
    int8_t transfer_status[XHCI_SLOTS+1][32];uint32_t transfer_actual[XHCI_SLOTS+1][32],transfer_length[XHCI_SLOTS+1][32];
} xhci_state;
static xhci_state state[XHCI_CONTROLLERS];

static void pause_short(void){__asm__ volatile("pause");}
static volatile uint32_t*cap(usb_controller*c){return(volatile uint32_t*)(uintptr_t)c->base;}
static uint32_t caplen(usb_controller*c){return cap(c)[0]&0xffu;}
static volatile uint32_t*op(usb_controller*c){return(volatile uint32_t*)(uintptr_t)(c->base+caplen(c));}
static volatile uint32_t*runtime(usb_controller*c){return(volatile uint32_t*)(uintptr_t)(c->base+(cap(c)[6]&~0x1fu));}
static volatile uint32_t*doorbells(usb_controller*c){return(volatile uint32_t*)(uintptr_t)(c->base+(cap(c)[5]&~3u));}
static int wait_mask(volatile uint32_t*r,uint32_t mask,uint32_t value){for(uint32_t i=0;i<XHCI_WAIT;i++){if((*r&mask)==value)return 1;pause_short();}return 0;}
static uint32_t*input_context(xhci_state*s,uint8_t slot,uint8_t ctx){return s->input[slot-1][ctx];}
static uint32_t*output_context(xhci_state*s,uint8_t slot,uint8_t ctx){return s->output[slot-1][ctx];}
static uint8_t xhci_interval(uint8_t speed,uint8_t interval){if(!interval)interval=1;
    if(speed>=3)return interval>16?15:(uint8_t)(interval-1);uint8_t lg=0,v=1;while(v<interval&&lg<10){v<<=1;lg++;}return(uint8_t)(lg+3);}

uint32_t usb_xhci_ports(usb_controller*c){volatile uint32_t*r=op(c);uint32_t a=0,b=0,n=c->ports;if(n>32)n=32;
    for(uint32_t i=0;i<n;i++){uint32_t s=r[0x400/4+i*4];if(s&1)a|=1u<<i;if(s&2)b|=1u<<i;}c->connected_ports=a;c->enabled_ports=b;return a;}

static int try_event(usb_controller*c,xhci_trb*out){xhci_state*s=&state[c->slot];volatile uint32_t*ir=runtime(c)+0x20/4;
    __asm__ volatile("":::"memory");xhci_trb*e=&s->event[s->event_index];if((e->d[3]&1u)!=s->event_cycle)return 0;
    *out=*e;s->event_index++;if(s->event_index==256){s->event_index=0;s->event_cycle^=1;}
    uintptr_t dequeue=(uintptr_t)&s->event[s->event_index];ir[6]=(uint32_t)(dequeue|8u);ir[7]=0;return 1;
}
static void pump_events(usb_controller*c){xhci_trb e;xhci_state*s=&state[c->slot];while(try_event(c,&e)){
    if(((e.d[3]>>10)&0x3fu)==32u){uint8_t slot=(uint8_t)(e.d[3]>>24),ep=(uint8_t)((e.d[3]>>16)&31u);
        if(slot<=XHCI_SLOTS&&ep<32&&s->pending[slot][ep]){uint8_t code=(uint8_t)(e.d[2]>>24);uint32_t residual=e.d[2]&0xffffffu;
            s->transfer_status[slot][ep]=(code==1||code==13)?0:-1;s->transfer_actual[slot][ep]=residual<=s->transfer_length[slot][ep]?s->transfer_length[slot][ep]-residual:0;s->completed[slot][ep]=1;}}
}}
static int next_event(usb_controller*c,xhci_trb*out){for(uint32_t n=0;n<XHCI_WAIT;n++){if(try_event(c,out))return 1;pause_short();}return 0;
}
static int command(usb_controller*c,uint8_t type,uint32_t p0,uint32_t p1,uint32_t p2,uint32_t control,uint8_t*slot){
    xhci_state*s=&state[c->slot];uint16_t i=s->cmd_index;xhci_trb*t=&s->command[i];uint32_t cycle=s->cmd_cycle;
    t->d[0]=p0;t->d[1]=p1;t->d[2]=p2;t->d[3]=(control&~1u)|((uint32_t)type<<10)|cycle;
    __asm__ volatile("":::"memory");i++;if(i==255){xhci_trb*l=&s->command[255];l->d[0]=(uint32_t)(uintptr_t)&s->command[0];l->d[1]=0;l->d[2]=0;l->d[3]=(6u<<10)|(1u<<1)|cycle;s->cmd_index=0;s->cmd_cycle^=1;}else s->cmd_index=i;
    doorbells(c)[0]=0;xhci_trb e;if(!next_event(c,&e))return-1;
    if(((e.d[3]>>10)&0x3fu)!=33u||((e.d[2]>>24)&0xffu)!=1u)return-1;
    if(slot)*slot=(uint8_t)(e.d[3]>>24);return 0;
}
static void init_ring(xhci_state*s,uint8_t slot,uint8_t epid){xhci_trb*r=s->rings[slot-1][epid-1];
    s->ring_index[slot-1][epid-1]=0;s->ring_cycle[slot-1][epid-1]=1;
    r[15].d[0]=(uint32_t)(uintptr_t)&r[0];r[15].d[1]=0;r[15].d[2]=0;r[15].d[3]=(6u<<10)|(1u<<1)|1u;
}
static void push_ring(xhci_state*s,uint8_t slot,uint8_t epid,uint32_t d0,uint32_t d1,uint32_t d2,uint32_t control){
    uint16_t*i=&s->ring_index[slot-1][epid-1];uint8_t*cycle=&s->ring_cycle[slot-1][epid-1];xhci_trb*r=s->rings[slot-1][epid-1];
    r[*i].d[0]=d0;r[*i].d[1]=d1;r[*i].d[2]=d2;r[*i].d[3]=(control&~1u)|*cycle;__asm__ volatile("":::"memory");
    (*i)++;if(*i==15){r[15].d[3]=(6u<<10)|(1u<<1)|*cycle;*i=0;*cycle^=1;}
}
static void write_endpoint_context(xhci_state*s,uint8_t slot,uint8_t epid,uint8_t type,uint16_t packet,uint8_t interval){
    uint32_t*ctx=input_context(s,slot,(uint8_t)(epid+1));for(uint32_t i=0;i<8;i++)ctx[i]=0;
    ctx[0]=(uint32_t)interval<<16;ctx[1]=(3u<<1)|((uint32_t)type<<3)|((uint32_t)packet<<16);
    uint64_t ring=(uint32_t)(uintptr_t)s->rings[slot-1][epid-1];ctx[2]=(uint32_t)ring|1u;ctx[3]=(uint32_t)(ring>>32);ctx[4]=8;
}
static int configure_endpoint(usb_controller*c,uint8_t slot,uint8_t epid,uint8_t type,uint16_t packet,uint8_t interval){
    xhci_state*s=&state[c->slot];uint32_t*ctrl=input_context(s,slot,0);for(uint32_t i=0;i<8;i++)ctrl[i]=0;
    ctrl[1]=1u|(1u<<epid);uint32_t*sc=input_context(s,slot,1);uint32_t*old=output_context(s,slot,1);
    for(uint32_t i=0;i<8;i++)sc[i]=old[i];uint32_t entries=old[0]>>27;if(epid>entries)sc[0]=(sc[0]&~(31u<<27))|((uint32_t)epid<<27);
    write_endpoint_context(s,slot,epid,type,packet,interval);
    uint64_t input=(uint32_t)(uintptr_t)s->input[slot-1];if(command(c,12,(uint32_t)input,(uint32_t)(input>>32),0,(uint32_t)slot<<24,0))return-1;
    s->endpoint_mask[slot]|=1u<<epid;return 0;
}
static int evaluate_ep0(usb_controller*c,uint8_t slot,uint16_t packet){xhci_state*s=&state[c->slot];uint32_t*ctrl=input_context(s,slot,0);for(uint32_t i=0;i<8;i++)ctrl[i]=0;
    ctrl[1]=1u<<1;uint32_t*ep=input_context(s,slot,2);uint32_t*out=output_context(s,slot,2);for(uint32_t i=0;i<8;i++)ep[i]=out[i];ep[1]=(ep[1]&0xffffu)|((uint32_t)packet<<16);
    uint64_t input=(uint32_t)(uintptr_t)s->input[slot-1];if(command(c,13,(uint32_t)input,(uint32_t)(input>>32),0,(uint32_t)slot<<24,0))return-1;s->mps[slot]=packet;return 0;}

static void legacy_handoff(usb_controller*c){uint32_t xecp=cap(c)[3]>>16;if(!xecp)return;uint32_t offset=xecp*4u;
    for(uint32_t n=0;offset&&n<64;n++){volatile uint32_t*p=(volatile uint32_t*)(uintptr_t)(c->base+offset);uint32_t v=*p;
        if((v&255u)==1u){p[0]=v|(1u<<24);p[1]=0;for(uint32_t i=0;i<XHCI_WAIT;i++){if(!(p[0]&(1u<<16)))break;pause_short();}break;}offset=((v>>8)&255u)*4u;}}
int usb_xhci_start(usb_controller*c){if(!c||c->slot>=XHCI_CONTROLLERS||c->kind!=USB_CONTROLLER_XHCI)return-1;
    if(cap(c)[3]&(1u<<2))return-1;legacy_handoff(c);
    xhci_state*s=&state[c->slot];volatile uint32_t*o=op(c);volatile uint32_t*sts=o+1;
    o[0]&=~1u;if(!wait_mask(sts,1u,1u))return-1;o[0]|=2u;if(!wait_mask(&o[0],2u,0))return-1;if(!wait_mask(sts,1u<<11,0))return-1;
    uint32_t maxslots=cap(c)[1]&0xffu;if(maxslots>XHCI_SLOTS)maxslots=XHCI_SLOTS;if(!maxslots)return-1;
    s->cmd_index=s->event_index=0;s->cmd_cycle=s->event_cycle=1;
    s->command[255].d[0]=(uint32_t)(uintptr_t)&s->command[0];s->command[255].d[1]=0;s->command[255].d[2]=0;s->command[255].d[3]=(6u<<10)|(1u<<1)|1u;
    s->erst.address=(uintptr_t)s->event;s->erst.size=256;s->erst.reserved=0;
    volatile uint32_t*rt=runtime(c);volatile uint32_t*ir=rt+0x20/4;ir[0]=0;ir[1]=0;ir[2]=1;ir[4]=(uint32_t)(uintptr_t)&s->erst;ir[5]=0;
    ir[6]=(uint32_t)(uintptr_t)&s->event[0]|8u;ir[7]=0;
    uint32_t scratch_count=(((cap(c)[2]>>21)&31u)<<5)|((cap(c)[2]>>27)&31u);
    if(scratch_count>4)return-1;if(scratch_count){for(uint32_t i=0;i<scratch_count;i++)s->scratch_array[i]=(uintptr_t)s->scratch_pages[i];s->dcbaa[0]=(uintptr_t)s->scratch_array;}
    o[2]=1;o[6]=(uint32_t)(uintptr_t)s->command|1u;o[7]=0;o[12]=(uint32_t)(uintptr_t)s->dcbaa;o[13]=0;o[14]=maxslots;
    __asm__ volatile("":::"memory");o[0]|=1u;if(!wait_mask(sts,1u,0))return-1;
    c->ready=1;(void)usb_xhci_ports(c);return 0;
}

int usb_xhci_prepare_port(usb_controller*c,uint32_t port,uint8_t*address,uint16_t*packet,uint8_t*low){
    if(!c||!c->ready||port>=c->ports||!address||!packet||!low)return-1;xhci_state*s=&state[c->slot];volatile uint32_t*o=op(c);
    volatile uint32_t*p=&o[0x400/4+port*4];uint32_t ps=*p;if(!(ps&1u))return-1;uint8_t speed=(uint8_t)((ps>>10)&15u);
    *p=(ps&~((1u<<17)|(1u<<18)|(1u<<19)|(1u<<20)|(1u<<21)|(1u<<22)|(1u<<23)))|(1u<<9);
    if(speed>=4)*p|=1u<<31;else *p|=1u<<4;
    if(!wait_mask(p,speed>=4?1u<<19:1u<<21,speed>=4?1u<<19:1u<<21))return-1;
    ps=*p;*p=ps&~((1u<<17)|(1u<<18)|(1u<<19)|(1u<<20)|(1u<<21)|(1u<<22)|(1u<<23));
    if(!(*p&2u))return-1;uint8_t slot=0;if(command(c,9,0,0,0,0,&slot)||!slot||slot>XHCI_SLOTS)return-1;
    s->enabled_slots++;s->speed[slot]=speed;s->mps[slot]=(speed==2)?8:(speed==3?64:(speed>=4?512:8));init_ring(s,slot,1);
    uint32_t*ctrl=input_context(s,slot,0);for(uint32_t i=0;i<8;i++)ctrl[i]=0;ctrl[1]=3;
    uint32_t*sc=input_context(s,slot,1);for(uint32_t i=0;i<8;i++)sc[i]=0;sc[0]=((uint32_t)speed<<20)|(1u<<27);sc[1]=(port+1u)<<16;
    uint32_t*ep=input_context(s,slot,2);for(uint32_t i=0;i<8;i++)ep[i]=0;ep[1]=(3u<<1)|(4u<<3)|((uint32_t)s->mps[slot]<<16);
    uint64_t ring=(uint32_t)(uintptr_t)s->rings[slot-1][0];ep[2]=(uint32_t)ring|1u;ep[3]=(uint32_t)(ring>>32);ep[4]=8;
    uintptr_t out=(uintptr_t)s->output[slot-1];s->dcbaa[slot]=out;
    uint64_t input=(uint32_t)(uintptr_t)s->input[slot-1];if(command(c,11,(uint32_t)input,(uint32_t)(input>>32),0,(uint32_t)slot<<24,0))return-1;
    uint8_t addr=(uint8_t)(output_context(s,slot,1)[3]&0xffu);if(!addr)return-1;s->address[slot]=addr;s->slot_for_address[addr]=slot;
    *address=addr;*packet=s->mps[slot];*low=(uint8_t)(speed==2);return 0;
}

int usb_xhci_transfer(usb_controller*c,uint8_t type,uint8_t address,uint8_t endpoint,uint16_t packet,uint8_t low,
                      uint8_t interval,const uint8_t setup[8],void*data,uint32_t length,uint32_t*actual){
    if(!c||!c->ready||!address||length>16384)return-1;xhci_state*s=&state[c->slot];uint8_t slot=s->slot_for_address[address],epid=1;
    if(!slot||slot>XHCI_SLOTS||s->address[slot]!=address)return-1;
    if(type==0){if(packet!=s->mps[slot]&&evaluate_ep0(c,slot,packet))return-1;
        uint32_t setup_value=(uint32_t)setup[0]|((uint32_t)setup[1]<<8)|((uint32_t)setup[2]<<16)|((uint32_t)setup[3]<<24);
        uint32_t setup_index=(uint32_t)setup[4]|((uint32_t)setup[5]<<8)|((uint32_t)setup[6]<<16)|((uint32_t)setup[7]<<24);
        uint32_t trt=length?((setup[0]&0x80u)?3u:2u):0u;push_ring(s,slot,epid,setup_value,setup_index,8,(2u<<10)|(1u<<6)|(trt<<16)|(1u<<4));
        if(length){push_ring(s,slot,epid,(uint32_t)(uintptr_t)data,0,length,(3u<<10)|(1u<<4)|((setup[0]&0x80u)?1u<<16:0));}
        uint32_t dir=(!length||!(setup[0]&0x80u))?1u:0u;push_ring(s,slot,epid,0,0,0,(4u<<10)|(1u<<5)|(dir<<16));
    }else{
        epid=(uint8_t)((endpoint&15u)*2u+((endpoint&0x80u)?1u:0u));if(!epid||epid>31)return-1;
        if(!(s->endpoint_mask[slot]&(1u<<epid))){uint8_t eptype=type==1?((endpoint&0x80u)?6:2):((endpoint&0x80u)?7:3);
            if(configure_endpoint(c,slot,epid,eptype,packet,type==2?xhci_interval(s->speed[slot],interval):0))return-1;}
        if(type==2){pump_events(c);if(s->pending[slot][epid]){if(!s->completed[slot][epid])return-2;
                int result=s->transfer_status[slot][epid];if(actual)*actual=s->transfer_actual[slot][epid];s->pending[slot][epid]=s->completed[slot][epid]=0;return result;}
            push_ring(s,slot,epid,(uint32_t)(uintptr_t)data,0,length,(1u<<10)|(1u<<5));s->pending[slot][epid]=1;s->completed[slot][epid]=0;s->transfer_length[slot][epid]=length;
            doorbells(c)[slot]=epid;return-2;}
        if(!length){push_ring(s,slot,epid,0,0,0,(1u<<10)|(1u<<5));}
        else push_ring(s,slot,epid,(uint32_t)(uintptr_t)data,0,length,(1u<<10)|(1u<<5));
    }
    doorbells(c)[slot]=epid;xhci_trb event;if(!next_event(c,&event))return-1;
    uint8_t etype=(uint8_t)((event.d[3]>>10)&0x3f),code=(uint8_t)(event.d[2]>>24);
    if(etype!=32||(code!=1&&code!=13))return-1;uint32_t residual=event.d[2]&0xffffffu;
    if(actual)*actual=length>=residual?length-residual:0;(void)low;return 0;
}
