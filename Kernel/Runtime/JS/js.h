#ifndef ENVY_JS_H
#define ENVY_JS_H
#include <stdint.h>
enum { JS_UNDEFINED, JS_NUMBER, JS_STRING, JS_OBJECT, JS_FUNCTION, JS_NATIVE, JS_NULL, JS_UNINITIALIZED, JS_BOOLEAN, JS_REAL };
typedef struct { int type; int64_t number; const char *text; uint32_t length; } js_value;
typedef struct js_vm js_vm;
typedef int (*js_host_call)(js_vm *, int id, const js_value *, uint32_t, js_value *);
typedef struct { uint32_t object; const char *name; js_value value; int readonly; uint32_t next; } js_property;
typedef struct js_function {
    const char *source; uint32_t length, scope, parameter_count, line;
    const char *defaults[8]; uint32_t default_lengths[8];
    int expression_body;
    char name[64], parameters[8][64];
    struct js_function *next; uint32_t index;
} js_function;
struct js_vm {
    js_property *properties; uint32_t property_count, property_capacity, objects;
    uint32_t property_buckets[2048];
    _Alignas(js_function) _Alignas(js_property) char strings[1048576]; uint32_t used, arena_end;
    js_host_call host; void *context;
    int (*host_sync)(js_vm *,int,const js_value *,uint32_t);
    int (*read_property)(js_vm *,uint32_t,const char *,js_value *);
    void (*changed_property)(js_vm *,uint32_t,const char *);
    int (*module_source)(js_vm *,const char *,const char **,uint32_t *);
    const char *source_name;
    uint32_t native_owner; /* Bound receiver during a host call; restored on return. */
    const char *error; uint32_t error_line, depth;
    js_function *functions; uint32_t function_count;
    uint32_t scope, environment_count, parents[4096];
};
void js_init(js_vm *vm, js_host_call host, void *context);
js_value js_object(js_vm *vm);
js_value js_string(js_vm *vm, const char *text, uint32_t length);
js_value js_native(int id);
/* JS_NATIVE.length carries the bound object's identity. */
js_value js_native_bound(int id, uint32_t owner);
uint32_t js_property_owner(js_value value);
js_value js_number_string(js_vm *vm, int64_t number);
js_value js_get(js_vm *vm, uint32_t object, const char *name);
int js_set(js_vm *vm, uint32_t object, const char *name, js_value value);
/* Function bodies reference source: keep it alive while its functions are usable. */
int js_eval(js_vm *vm, const char *source, uint32_t length);
int js_call(js_vm *vm, js_value function);
int js_call_args(js_vm *vm, js_value function, const js_value *args, uint32_t count, js_value *result);
int js_json_parse(js_vm *vm,const char *text,uint32_t length,js_value *result);
#endif
