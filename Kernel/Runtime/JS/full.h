#ifndef ENVY_JS_FULL_H
#define ENVY_JS_FULL_H
#include "js.h"
void js_full_reset(js_vm *);
int js_full_eval(js_vm *,const char *,uint32_t);
int js_full_eval_named(js_vm *,const char *,uint32_t,const char *);
int js_full_jobs(js_vm *);
int js_full_promise(js_vm *,js_value,int,js_value *);
int js_full_get(js_vm *,uint32_t,const char *,js_value *);
int js_full_set(js_vm *,uint32_t,const char *,js_value);
int js_full_call(js_vm *,js_value,const js_value *,uint32_t,js_value *);
#define JS_FULL_FUNCTION 0x10000000u
int js_full_active(js_vm *);
#endif
