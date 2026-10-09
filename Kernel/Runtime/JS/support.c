#include "compat/stdlib.h"
#include "compat/string.h"
#include "compat/stdio.h"
#include "compat/math.h"
#include "compat/time.h"
#include <stdint.h>
/* Reclaimable allocator for interpreter bytecode, objects, and GC scratch. */
static _Alignas(16) uint8_t heap[8u*1024u*1024u];
typedef struct {uint32_t size,next,free,previous;} block;
static int initialized;
static uint32_t cursor;
int envy_js_errno;
void *envy_js_alloc(void *context,void *pointer,int count){
    (void)context;
    if(!initialized){*(block *)heap=(block){sizeof(heap)-sizeof(block),0,1,0};initialized=1;}
    if(count<0||(uint32_t)count>sizeof(heap)-32)return NULL;
    if(!count){
        if(!pointer)return NULL;
        block *b=(block *)pointer-1;uint32_t off=(uint32_t)((uint8_t *)b-heap);b->free=1;
        if(off){block *previous=(block *)(heap+b->previous);
            if(previous->free){previous->size+=sizeof(block)+b->size;previous->next=b->next;off=b->previous;b=previous;}}
        if(b->next){block *next=(block *)(heap+b->next);
            if(next->free){b->size+=sizeof(block)+next->size;b->next=next->next;}}
        if(b->next)((block *)(heap+b->next))->previous=off;
        cursor=off;
        return NULL;
    }
    uint32_t size=((uint32_t)count+15)&~15u;
    if(pointer&&((block *)pointer-1)->size>=size)return pointer;
    uint32_t start=cursor,off=start;
    do{block *b=(block *)(heap+off);
        if(b->free&&b->size>=size){
            if(b->size-size>=32){uint32_t next=off+sizeof(block)+size;
                *(block *)(heap+next)=(block){b->size-size-sizeof(block),b->next,1,off};
                if(b->next)((block *)(heap+b->next))->previous=next;b->next=next;b->size=size;}
            b->free=0;cursor=b->next;void *result=b+1;
            if(pointer){memcpy(result,pointer,((block *)pointer-1)->size);envy_js_alloc(context,pointer,0);}return result;
        }
        off=b->next;
    }while(off!=start);
    return NULL;
}
void envy_js_abort(void){for(;;)__asm__ volatile("cli; hlt");}
char *envy_strcpy(char *d,const char *s){char *result=d;while((*d++=*s++)){}return result;}
char *envy_strcat(char *d,const char *s){envy_strcpy(d+strlen(d),s);return d;}
char *envy_strrchr(const char *s,int c){const char *found=NULL;do{if(*s==c)found=s;}while(*s++);return (char *)found;}
int envy_atoi(const char *s){int n=0,negative=0;while(*s&&*s<=32)s++;if(*s=='-'||*s=='+')negative=*s++=='-';while(*s>='0'&&*s<='9')n=n*10+*s++-'0';return negative?-n:n;}
static void emit(char *out,size_t size,size_t *used,char c){if(*used+1<size)out[*used]=c;(*used)++;}
int envy_vsnprintf(char *out,size_t size,const char *format,va_list args){
    size_t used=0;
    while(*format){if(*format!='%'){emit(out,size,&used,*format++);continue;}format++;
        if(*format=='%'){emit(out,size,&used,*format++);continue;}
        int pad=0;char padding=' ';if(*format=='0'){padding='0';format++;}
        while(*format>='0'&&*format<='9')pad=pad*10+*format++-'0';
        if(*format=='.'){format++;while(*format>='0'&&*format<='9')format++;}
        int wide=0;if(*format=='l'||*format=='z'){wide=1;format++;if(*format=='l')format++;}
        char type=*format++;(void)wide;
        if(type=='s'){const char *s=va_arg(args,const char *);if(!s)s="(null)";while(*s)emit(out,size,&used,*s++);}
        else if(type=='c')emit(out,size,&used,(char)va_arg(args,int));
        else if(type=='d'||type=='i'||type=='u'||type=='x'||type=='p'){
            uint32_t n=va_arg(args,uint32_t);int negative=(type=='d'||type=='i')&&(int32_t)n<0;
            if(negative)n=0-n;char digits[12];int count=0;uint32_t base=type=='x'||type=='p'?16:10;
            do{digits[count++]="0123456789abcdef"[n%base];n/=base;}while(n);
            if(negative)emit(out,size,&used,'-');while(pad>count){emit(out,size,&used,padding);pad--;}
            while(count)emit(out,size,&used,digits[--count]);
        }else if(type=='g'||type=='f'){double n=va_arg(args,double);(void)n;emit(out,size,&used,'?');}
    }
    if(size)out[used<size?used:size-1]=0;return (int)used;
}
int envy_snprintf(char *out,size_t size,const char *format,...){va_list a;va_start(a,format);int n=envy_vsnprintf(out,size,format,a);va_end(a);return n;}
int envy_sprintf(char *out,const char *format,...){va_list a;va_start(a,format);int n=envy_vsnprintf(out,0x7fffffff,format,a);va_end(a);return n;}
int envy_fputc(int c,FILE *f){(void)f;__asm__ volatile("outb %0,$0xe9"::"a"((uint8_t)c));return c;}
int envy_fputs(const char *s,FILE *f){while(*s)envy_fputc(*s++,f);return 0;}
int envy_printf(const char *format,...){char out[512];va_list a;va_start(a,format);int n=envy_vsnprintf(out,sizeof(out),format,a);va_end(a);envy_fputs(out,NULL);return n;}
time_t envy_js_time(time_t *out){uint32_t lo,hi;__asm__ volatile("rdtsc":"=a"(lo),"=d"(hi));time_t t=(time_t)(lo>>20);(void)hi;if(out)*out=t;return t;}
static double round_mode(double x,unsigned short mode){unsigned short old,changed;double r;
    __asm__ volatile("fnstcw %0":"=m"(old));changed=(old&~0xc00u)|mode;
    __asm__ volatile("fldcw %1; frndint; fldcw %2":"=t"(r):"m"(changed),"m"(old),"0"(x));return r;}
double envy_floor(double x){return round_mode(x,0x400);}
double envy_ceil(double x){return round_mode(x,0x800);}
double envy_sqrt(double x){double r;__asm__("fsqrt":"=t"(r):"0"(x));return r;}
double envy_sin(double x){double r;__asm__("fsin":"=t"(r):"0"(x));return r;}
double envy_cos(double x){double r;__asm__("fcos":"=t"(r):"0"(x));return r;}
double envy_tan(double x){return envy_sin(x)/envy_cos(x);}
double envy_atan2(double y,double x){double r;__asm__("fpatan":"=t"(r):"0"(x),"u"(y):"st(1)");return r;}
double envy_atan(double x){return envy_atan2(x,1);}
double envy_asin(double x){return envy_atan2(x,envy_sqrt(1-x*x));}
double envy_acos(double x){return envy_atan2(envy_sqrt(1-x*x),x);}
double envy_fmod(double x,double y){double r;unsigned short status;
    __asm__ volatile("1: fprem; fnstsw %%ax; testw $0x400,%%ax; jnz 1b":"=t"(r),"=a"(status):"0"(x),"u"(y));return r;}
static double log2_value(double x){double r;__asm__("fyl2x":"=t"(r):"0"(x),"u"(1.0):"st(1)");return r;}
double envy_log(double x){return log2_value(x)*0.69314718055994530942;}
static double exp2_value(double x){if(isnan(x))return x;if(x>1024)return INFINITY;if(x<-1075)return 0;
    double whole=envy_floor(x),fraction=x-whole,r;
    __asm__("f2xm1; fld1; faddp":"=t"(r):"0"(fraction));
    __asm__("fscale":"=t"(r):"0"(r),"u"(whole));return r;}
double envy_exp(double x){return exp2_value(x*1.4426950408889634074);}
double envy_pow(double x,double y){
    if(y==0)return 1;if(isnan(y)||isnan(x))return NAN;
    if(x==0)return y<0?INFINITY:0;
    if(x<0){if(!isfinite(y)||envy_floor(y)!=y)return NAN;double n=exp2_value(log2_value(-x)*y);return envy_fmod(y,2)!=0?-n:n;}
    return exp2_value(log2_value(x)*y);
}

int envy_fprintf(FILE *f,const char *format,...){char out[512];va_list args;va_start(args,format);int n=envy_vsnprintf(out,sizeof(out),format,args);va_end(args);envy_fputs(out,f);return n;}
#include "compat/sys/time.h"
extern int64_t envy_js_now_ms(void);
int gettimeofday(struct timeval *tv,void *zone){(void)zone;int64_t ms=envy_js_now_ms();tv->tv_sec=(long)(ms/1000);tv->tv_usec=(long)((ms%1000)*1000);return 0;}

