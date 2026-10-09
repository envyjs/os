#ifndef ENVY_SYS_TIME_H
#define ENVY_SYS_TIME_H
struct timeval {long tv_sec,tv_usec;};
int gettimeofday(struct timeval *,void *);
#endif
