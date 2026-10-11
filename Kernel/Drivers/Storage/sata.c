#include "sata.h"
#include "../../Arch/x86/io.h"
#define WAIT_LIMIT 4000000u
typedef struct {_Alignas(1024) uint32_t headers[256];_Alignas(256) uint8_t fis[256];_Alignas(128) uint8_t table[256];_Alignas(4) uint8_t data[32768];} dma_port;
static dma_port memory[SATA_MAX_DEVICES];
static uint32_t pci_read(uint32_t address,uint32_t offset){out32(0xcf8,address|offset);return in32(0xcfc);}
static void zero(void *address,uint32_t bytes){uint8_t *p=address;while(bytes--)*p++=0;}
static int wait_clear(volatile uint32_t *p,uint32_t bits){for(uint32_t i=0;i<WAIT_LIMIT;i++){if(!(*p&bits))return 1;__asm__ volatile("pause");}return 0;}
static int stop(volatile uint32_t *p){p[6]&=~1u;if(!wait_clear(p+6,1u<<15))return 0;p[6]&=~16u;return wait_clear(p+6,1u<<14);}
static int command(sata_device *d,uint8_t opcode,uint64_t lba,uint32_t sectors,int write,uint32_t bytes){
    if(d->failed)return SATA_IO;volatile uint32_t *p=d->port;dma_port *m=&memory[d->slot];
    if(p[14]||p[13]||!wait_clear(p+8,0x88))return SATA_TIMEOUT;
    zero(m->table,sizeof(m->table));m->table[0]=0x27;m->table[1]=0x80;m->table[2]=opcode;m->table[7]=0x40;
    for(uint32_t i=0;i<3;i++){m->table[4+i]=(uint8_t)(lba>>(8*i));m->table[8+i]=(uint8_t)(lba>>(24+8*i));}
    m->table[12]=(uint8_t)sectors;m->table[13]=(uint8_t)(sectors>>8);
    m->headers[0]=5u|(write?64u:0u)|(bytes?1u<<16:0u);m->headers[1]=0;m->headers[2]=(uint32_t)(uintptr_t)m->table;m->headers[3]=0;
    if(bytes){uint32_t *prd=(uint32_t *)(m->table+128);prd[0]=(uint32_t)(uintptr_t)m->data;prd[1]=prd[2]=0;prd[3]=bytes-1;}
    p[4]=0xffffffffu;p[12]=0xffffffffu;__asm__ volatile("":::"memory");p[14]=1;
    for(uint32_t i=0;i<WAIT_LIMIT;i++){uint32_t status=p[4];if(!(p[14]&1)){__asm__ volatile("":::"memory");if((status&0x7d000000u)||(p[8]&1))return SATA_IO;if(bytes&&m->headers[1]!=bytes)return SATA_IO;return SATA_OK;}__asm__ volatile("pause");}
    /* Never reuse DMA storage after an uncompleted command. */
    d->failed=1;(void)stop(p);return SATA_TIMEOUT;
}
uint32_t sata_probe(sata_device *devices,uint32_t capacity){
    uint32_t count=0;if(capacity>SATA_MAX_DEVICES)capacity=SATA_MAX_DEVICES;
    for(uint32_t bus=0;bus<256&&count<capacity;bus++)for(uint32_t dev=0;dev<32&&count<capacity;dev++){
        uint32_t base=0x80000000u|(bus<<16)|(dev<<11);if((pci_read(base,0)&65535)==65535)continue;
        uint32_t functions=pci_read(base,12)&0x800000?8:1;
        for(uint32_t f=0;f<functions&&count<capacity;f++){uint32_t address=base|(f<<8);if((pci_read(address,0)&65535)==65535||(pci_read(address,8)>>8)!=0x010601)continue;
            uint32_t bar=pci_read(address,0x24);if(!bar||(bar&7))continue;bar&=~15u;if(!bar||bar>0xffffe000u)continue;
            uint16_t flags=(uint16_t)pci_read(address,4);out32(0xcf8,address|4);out16(0xcfc,flags|6);
            volatile uint32_t *h=(volatile uint32_t *)(uintptr_t)bar;
            if(h[9]&1){h[10]|=2;for(uint32_t i=0;i<WAIT_LIMIT&&(h[10]&17);i++)__asm__ volatile("pause");if(h[10]&17)continue;}
            h[1]=(h[1]|0x80000000u)&~2u;uint32_t implemented=h[3];
            for(uint32_t port=0;port<32&&count<capacity;port++){if(!(implemented&(1u<<port)))continue;volatile uint32_t *p=h+64+port*32;
                if((p[10]&15)!=3||((p[10]>>8)&15)!=1||p[9]!=0x00000101u||!stop(p))continue;
                dma_port *m=&memory[count];zero(m,sizeof(*m));p[0]=(uint32_t)(uintptr_t)m->headers;p[1]=0;p[2]=(uint32_t)(uintptr_t)m->fis;p[3]=0;p[5]=0;p[4]=p[12]=0xffffffffu;p[6]|=17;
                sata_device *d=&devices[count];*d=(sata_device){.port=p,.slot=count};
                if(command(d,0xec,0,0,0,512)){if(!stop(p))return count;continue;}uint16_t *id=(uint16_t *)m->data;
                if(!(id[83]&(1u<<10))||((id[106]&0xc000)==0x4000&&(id[106]&(1u<<12))&&((uint32_t)id[117]|((uint32_t)id[118]<<16))!=256)){(void)stop(p);continue;}
                d->sectors=(uint64_t)id[100]|((uint64_t)id[101]<<16)|((uint64_t)id[102]<<32)|((uint64_t)id[103]<<48);if(!d->sectors||d->sectors>(1ull<<48)){(void)stop(p);continue;}
                for(uint32_t i=0;i<20;i++){d->model[i*2]=(char)(id[27+i]>>8);d->model[i*2+1]=(char)id[27+i];}d->model[40]=0;for(int i=39;i>=0&&d->model[i]==' ';i--)d->model[i]=0;count++;
            }
        }
    }
    return count;
}
static int transfer(sata_device *d,uint64_t lba,uint32_t count,void *buffer,int write){
    if(!d||(!buffer&&count)||lba>d->sectors||count>d->sectors-lba)return SATA_RANGE;uint8_t *p=buffer;
    while(count){uint32_t n=count>64?64:count,bytes=n*512;dma_port *m=&memory[d->slot];if(write)for(uint32_t i=0;i<bytes;i++)m->data[i]=p[i];
        int result=command(d,write?0x35:0x25,lba,n,write,bytes);if(result)return result;if(!write)for(uint32_t i=0;i<bytes;i++)p[i]=m->data[i];p+=bytes;lba+=n;count-=n;}return SATA_OK;
}
int sata_read(void *device,uint64_t lba,uint32_t sectors,void *buffer){return transfer(device,lba,sectors,buffer,0);}
int sata_write(void *device,uint64_t lba,uint32_t sectors,const void *buffer){return transfer(device,lba,sectors,(void *)buffer,1);}
int sata_flush(void *device){if(!device)return SATA_RANGE;return command(device,0xea,0,0,0,0);}
