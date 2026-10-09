#include <stddef.h>
#include <stdint.h>
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
