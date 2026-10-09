#ifndef ENVY_IMAGE_MATH_H
#define ENVY_IMAGE_MATH_H
static inline float fabsf(float x){return x<0?-x:x;}
static inline double fabs(double x){return x<0?-x:x;}
static inline int isnan(double x){return x!=x;}
static inline float sqrtf(float x){float result;__asm__("fsqrt":"=t"(result):"0"(x));return result;}
static inline double sqrt(double x){double result;__asm__("fsqrt":"=t"(result):"0"(x));return result;}
static inline float sinf(float x){float result;__asm__("fsin":"=t"(result):"0"(x));return result;}
static inline float cosf(float x){float result;__asm__("fcos":"=t"(result):"0"(x));return result;}
static inline float tanf(float x){return sinf(x)/cosf(x);}
static inline float atan2f(float y,float x){float result;__asm__("fpatan":"=t"(result):"0"(x),"u"(y):"st(1)");return result;}
static inline float acosf(float x){if(x>1)x=1;if(x<-1)x=-1;return atan2f(sqrtf(1-x*x),x);}
static inline float floorf(float x){if(!(x>-2147483648.0f&&x<2147483648.0f))return x;int n=(int)x;return (float)(n-(x<(float)n));}
static inline float ceilf(float x){if(!(x>-2147483648.0f&&x<2147483648.0f))return x;int n=(int)x;return (float)(n+(x>(float)n));}
static inline float roundf(float x){return x<0?ceilf(x-0.5f):floorf(x+0.5f);}
static inline float fmodf(float x,float y){float result;unsigned short status;
    __asm__ volatile("1: fprem; fnstsw %%ax; testw $0x400, %%ax; jnz 1b":"=t"(result),"=a"(status):"0"(x),"u"(y));return result;}
/* NanoSVG calls pow only for decimal integer exponents. */
static inline double pow(double base,double exponent){
    if(exponent>308)return 1.7976931348623157e308;if(exponent<-308)return 0;
    int n=(int)exponent;double result=1;if(n<0){base=1/base;n=-n;}
    while(n){if(n&1)result*=base;n>>=1;if(n)base*=base;}return result;
}
#endif
