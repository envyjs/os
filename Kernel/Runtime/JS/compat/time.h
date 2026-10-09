#ifndef ENVY_JS_TIME_H
#define ENVY_JS_TIME_H
typedef long time_t;
struct tm {int tm_sec,tm_min,tm_hour,tm_mday,tm_mon,tm_year,tm_wday,tm_yday,tm_isdst;long tm_gmtoff;};
time_t envy_js_time(time_t *);
/* The freestanding browser uses UTC. */
static inline struct tm *gmtime(const time_t *t){static struct tm v;(void)t;return &v;}
static inline struct tm *localtime(const time_t *t){return gmtime(t);}
static inline time_t mktime(struct tm *t){(void)t;return 0;}
#define time envy_js_time
static inline struct tm *localtime_r(const time_t *t,struct tm *out){*out=*localtime(t);return out;}
#endif
