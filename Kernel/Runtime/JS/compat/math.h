#ifndef ENVY_JS_MATH_H
#define ENVY_JS_MATH_H
#include <stdint.h>
#define INFINITY (__builtin_inf())
#define NAN (__builtin_nan(""))
#define isnan(x) __builtin_isnan(x)
#define isinf(x) __builtin_isinf(x)
#define isfinite(x) __builtin_isfinite(x)
static inline int signbit(double x){union {double d;uint32_t n[2];}v={.d=x};return v.n[1]>>31;}
static inline double fabs(double x){return __builtin_fabs(x);}
double envy_floor(double),envy_ceil(double),envy_sqrt(double),envy_sin(double),envy_cos(double),envy_tan(double);
double envy_atan2(double,double),envy_atan(double),envy_asin(double),envy_acos(double);
double envy_log(double),envy_exp(double),envy_pow(double,double),envy_fmod(double,double);
#define floor envy_floor
#define ceil envy_ceil
#define sqrt envy_sqrt
#define sin envy_sin
#define cos envy_cos
#define tan envy_tan
#define atan2 envy_atan2
#define atan envy_atan
#define asin envy_asin
#define acos envy_acos
#define log envy_log
#define exp envy_exp
#define pow envy_pow
#define fmod envy_fmod
static inline double trunc(double x){return x<0?ceil(x):floor(x);}
static inline double round(double x){return x<0?ceil(x-0.5):floor(x+0.5);}
static inline long lrint(double x){long result;__asm__("fistpl %0":"=m"(result):"t"(x):"st");return result;}
static inline double cosh(double x){return (exp(x)+exp(-x))/2;}
static inline double sinh(double x){return (exp(x)-exp(-x))/2;}
static inline double tanh(double x){if(x>20)return 1;if(x< -20)return -1;double e=exp(2*x);return (e-1)/(e+1);}
static inline double acosh(double x){return log(x+sqrt(x*x-1));}
static inline double asinh(double x){return x<0?-log(-x+sqrt(x*x+1)):log(x+sqrt(x*x+1));}
static inline double atanh(double x){return log((1+x)/(1-x))/2;}
static inline double expm1(double x){return exp(x)-1;}
static inline double log1p(double x){return log(1+x);}
static inline double log2(double x){return log(x)*1.4426950408889634074;}
static inline double log10(double x){return log(x)*0.43429448190325182765;}
static inline double cbrt(double x){return x<0?-pow(-x,1.0/3):pow(x,1.0/3);}
static inline double hypot(double x,double y){x=fabs(x);y=fabs(y);if(x<y){double t=x;x=y;y=t;}if(isinf(x))return INFINITY;if(x==0)return 0;y/=x;return x*sqrt(1+y*y);}
static inline double fmin(double x,double y){if(isnan(x))return y;if(isnan(y))return x;if(x==y)return signbit(x)?x:y;return x<y?x:y;}
static inline double fmax(double x,double y){if(isnan(x))return y;if(isnan(y))return x;if(x==y)return signbit(x)?y:x;return x>y?x:y;}
#endif
