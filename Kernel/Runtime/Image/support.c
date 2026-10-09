#include "compat/stdlib.h"
#include "compat/string.h"
#include "compat/stdio.h"
#include "support.h"
#include <stdint.h>
#include <stdarg.h>
/* Recycle decoder scratch buffers while keeping the returned pixels alive. */
static _Alignas(16) uint8_t heap[184u*1024u*1024u];static int failed;static uint32_t generation;
typedef struct {uint32_t size,next,available,reserved;} block;
void image_heap_reset(void){*(block *)heap=(block){sizeof(heap)-sizeof(block),0,1,0};failed=0;generation++;}
int image_heap_failed(void){return failed;}
unsigned int image_heap_generation(void){return generation;}
void *image_alloc(size_t count){
    if(count>sizeof(heap)-32){failed=1;return 0;}uint32_t size=((uint32_t)count+15)&~15u;if(!size)size=16;
    uint32_t offset=0;
    do{block *b=(block *)(heap+offset);if(b->available&&b->size>=size){
            if(b->size-size>=32){uint32_t next=offset+sizeof(block)+size;*(block *)(heap+next)=(block){b->size-size-sizeof(block),b->next,1,0};b->next=next;b->size=size;}
            b->available=0;return heap+offset+sizeof(block);}
        offset=b->next;
    }while(offset);failed=1;return 0;
}
void image_free(void *pointer){
    if(!pointer)return;block *freed=(block *)((uint8_t *)pointer-sizeof(block));freed->available=1;uint32_t offset=0;
    do{block *b=(block *)(heap+offset);while(b->available&&b->next&&((block *)(heap+b->next))->available){block *next=(block *)(heap+b->next);b->size+=sizeof(block)+next->size;b->next=next->next;}
        offset=b->next;
    }while(offset);
}
void *image_realloc(void *pointer,size_t count){
    if(pointer&&count<=((block *)((uint8_t *)pointer-sizeof(block)))->size)return pointer;
    void *result=image_alloc(count);if(!result)return 0;
    if(pointer){uint32_t old=((block *)((uint8_t *)pointer-sizeof(block)))->size;memcpy(result,pointer,old<count?old:count);image_free(pointer);}return result;
}
int image_memcmp(const void *a,const void *b,size_t count){const uint8_t *x=a,*y=b;for(size_t i=0;i<count;i++)if(x[i]!=y[i])return x[i]-y[i];return 0;}
size_t image_strlen(const char *p){size_t n=0;while(p[n])n++;return n;}
int image_strcmp(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return (uint8_t)*a-(uint8_t)*b;}
int image_strncmp(const char *a,const char *b,size_t n){for(size_t i=0;i<n;i++){if(a[i]!=b[i])return (uint8_t)a[i]-(uint8_t)b[i];if(!a[i])return 0;}return 0;}
char *image_strncpy(char *to,const char *from,size_t n){size_t i=0;while(i<n&&from[i]){to[i]=from[i];i++;}while(i<n)to[i++]=0;return to;}
char *image_strchr(const char *p,int c){do{if((uint8_t)*p==(uint8_t)c)return (char *)p;}while(*p++);return 0;}
char *image_strstr(const char *p,const char *needle){size_t n=image_strlen(needle);for(;*p;p++)if(!image_strncmp(p,needle,n))return (char *)p;return n?0:(char *)p;}
static int space(char c){return c==' '||c=='\n'||c=='\r'||c=='\t'||c=='\f'||c=='\v';}
long long image_strtoll(const char *text,char **end,int base){
    const char *p=text;while(space(*p))p++;int negative=0;if(*p=='-'||*p=='+')negative=*p++=='-';
    if(!base)base=10;uint64_t value=0;const char *start=p;
    while(*p){int digit=*p>='0'&&*p<='9'?*p-'0':*p>='a'&&*p<='f'?*p-'a'+10:*p>='A'&&*p<='F'?*p-'A'+10:99;
        if(digit>=base)break;/* Saturate before multiplication, avoiding 64-bit division helpers. */
        if(value>0x0cccccccccccccccULL||(value==0x0cccccccccccccccULL&&(uint32_t)digit>7))value=0x7fffffffffffffffULL;
        else value=value*(uint32_t)base+(uint32_t)digit;p++;}
    if(end)*end=(char *)(p==start?text:p);return negative?-(long long)value:(long long)value;
}
long image_strtol(const char *p,char **end,int base){long long n=image_strtoll(p,end,base);return n>2147483647?2147483647:n<(-2147483647LL-1)?(-2147483647-1):(long)n;}
int image_sscanf(const char *p,const char *format,...){
    va_list args;va_start(args,format);int count=0;
    while(*format){if(space(*format)){while(space(*format))format++;while(space(*p))p++;continue;}
        if(*format!='%'){if(*format++!=*p++)break;continue;}format++;uint32_t limit=0;
        while(*format>='0'&&*format<='9')limit=limit*10+(uint32_t)(*format++-'0');if(!limit)limit=0xffffffffu;
        char type=*format++;if(type!='x'&&type!='u')break;while(space(*p))p++;uint32_t value=0,n=0;
        while(n<limit){int digit=*p>='0'&&*p<='9'?*p-'0':*p>='a'&&*p<='f'?*p-'a'+10:*p>='A'&&*p<='F'?*p-'A'+10:99;
            if(digit>=(type=='x'?16:10))break;value=value*(type=='x'?16u:10u)+(uint32_t)digit;p++;n++;}
        if(!n)break;*va_arg(args,unsigned int *)=value;count++;
    }
    va_end(args);return count;
}
static void swap(uint8_t *a,uint8_t *b,size_t size){for(size_t i=0;i<size;i++){uint8_t x=a[i];a[i]=b[i];b[i]=x;}}
static void sift(uint8_t *data,size_t start,size_t count,size_t size,int (*compare)(const void *,const void *)){
    while(start<count/2){size_t child=start*2+1;if(child+1<count&&compare(data+child*size,data+(child+1)*size)<0)child++;
        if(compare(data+start*size,data+child*size)>=0)return;swap(data+start*size,data+child*size,size);start=child;}}
void image_qsort(void *base,size_t count,size_t size,int (*compare)(const void *,const void *)){
    if(count<2||!size)return;uint8_t *data=base;for(size_t i=count/2;i;i--)sift(data,i-1,count,size,compare);
    for(size_t i=count-1;i;i--){swap(data,data+i*size,size);sift(data,0,i,size,compare);}}
