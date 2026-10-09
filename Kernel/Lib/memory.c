#include <stddef.h>
#include <stdint.h>
uint64_t __udivdi3(uint64_t n,uint64_t d){
    if(!d)return 0;uint64_t q=0,r=0;
    for(int bit=0;bit<64;bit++){uint64_t high=r>>63;r=(r<<1)|(n>>63);n<<=1;q<<=1;if(high||r>=d){r-=d;q|=1;}}return q;
}
uint64_t __umoddi3(uint64_t n,uint64_t d){return d?n-__udivdi3(n,d)*d:0;}
uint64_t __ashldi3(uint64_t n,int count){
    union {uint64_t n;uint32_t words[2];}v={.n=n};unsigned int shift=(unsigned int)count;
    if(shift>=64)return 0;
    if(shift>=32){v.words[1]=v.words[0]<<(shift-32);v.words[0]=0;}
    else if(shift){v.words[1]=(v.words[1]<<shift)|(v.words[0]>>(32-shift));v.words[0]<<=shift;}return v.n;
}
uint64_t __lshrdi3(uint64_t n,int count){
    union {uint64_t n;uint32_t words[2];}v={.n=n};unsigned int shift=(unsigned int)count;
    if(shift>=64)return 0;
    if(shift>=32){v.words[0]=v.words[1]>>(shift-32);v.words[1]=0;}
    else if(shift){v.words[0]=(v.words[0]>>shift)|(v.words[1]<<(32-shift));v.words[1]>>=shift;}return v.n;
}
/* Compiler runtime for overflow-safe CSS fixed-point arithmetic on i686. */
int64_t __divdi3(int64_t numerator,int64_t denominator) {
    if(!denominator)return 0;
    int negative=(numerator<0)!=(denominator<0);
    uint64_t n=numerator<0?0-(uint64_t)numerator:(uint64_t)numerator;
    uint64_t d=denominator<0?0-(uint64_t)denominator:(uint64_t)denominator;
    uint64_t quotient=0,remainder=0;
    for(int bit=0;bit<64;bit++){uint64_t high=remainder>>63;remainder=(remainder<<1)|(n>>63);n<<=1;quotient<<=1;
        if(high||remainder>=d){remainder-=d;quotient|=1;}}
    return (int64_t)(negative?0-quotient:quotient);
}
void *memmove(void *destination,const void *source,size_t count) {
    unsigned char *out=destination;const unsigned char *in=source;
    if((uintptr_t)out<(uintptr_t)in){for(size_t i=0;i<count;i++)out[i]=in[i];}
    else if(out!=in){while(count){count--;out[count]=in[count];}}
    return destination;
}
void *memcpy(void *destination, const void *source, size_t count) {
    unsigned char *dst = destination;
    const unsigned char *src = source;
    for (size_t i = 0; i < count; ++i) dst[i] = src[i];
    return destination;
}
void *memset(void *destination, int value, size_t count) {
    unsigned char *dst = destination;
    for (size_t i = 0; i < count; ++i) dst[i] = (unsigned char)value;
    return destination;
}

int64_t __moddi3(int64_t n,int64_t d){if(!d)return 0;uint64_t a=n<0?0-(uint64_t)n:(uint64_t)n,b=d<0?0-(uint64_t)d:(uint64_t)d;uint64_t r=__umoddi3(a,b);return (int64_t)(n<0?0-r:r);}
int64_t __ashrdi3(int64_t n,int count){union {int64_t n;uint32_t w[2];}v={.n=n};if(count>=64){v.w[0]=v.w[1]=(int32_t)v.w[1]<0?0xffffffffu:0;}else if(count>=32){v.w[0]=(uint32_t)((int32_t)v.w[1]>>(count-32));v.w[1]=(int32_t)v.w[1]<0?0xffffffffu:0;}else if(count>0){v.w[0]=(v.w[0]>>count)|(v.w[1]<<(32-count));v.w[1]=(uint32_t)((int32_t)v.w[1]>>count);}return v.n;}

