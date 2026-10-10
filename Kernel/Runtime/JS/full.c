#include "full.h"
enum {ENVY_UNDEFINED=JS_UNDEFINED,ENVY_NULL=JS_NULL,ENVY_UNINITIALIZED=JS_UNINITIALIZED};
#include "vendor/quickjs/quickjs.h"
#include "compat/stdlib.h"
#include "compat/string.h"
#include "compat/math.h"
typedef struct handle {struct handle *next;js_value value;JSValue object;int foreign;} handle;
typedef struct state {struct state *next;js_vm *vm;JSRuntime *rt;JSContext *ctx;handle *handles;int syncing,running,draining;} state;
static state *states;
static JSClassID object_class,function_class;
extern JSValue envy_qjs_globals(JSContext *);
extern JSValue envy_qjs_global(JSContext *,const char *);
static state *find(js_vm *vm){for(state *s=states;s;s=s->next)if(s->vm==vm)return s;return NULL;}
int js_full_active(js_vm *vm){return find(vm)!=NULL;}
static JSValue push(state *,js_value);
static js_value pull(state *,JSValueConst);
static void import_globals(state *);
static void export_globals(state *);
static void exception(state *s){
    JSValue error=JS_GetException(s->ctx),line=JS_GetPropertyStr(s->ctx,error,"lineNumber");int32_t n=0;
    JS_ToInt32(s->ctx,&n,line);JS_FreeValue(s->ctx,line);
    const char *text=JS_ToCString(s->ctx,error);s->vm->error=js_string(s->vm,text?text:"JavaScript exception",text?(uint32_t)strlen(text):20).text;
    s->vm->error_line=n>0?(uint32_t)n:1;JS_FreeCString(s->ctx,text);JS_FreeValue(s->ctx,error);
}
static handle *by_owner(state *s,uint32_t owner){for(handle *h=s->handles;h;h=h->next)if(js_property_owner(h->value)==owner)return h;return NULL;}
static int equal_value(js_value a,js_value b){return a.type==b.type&&a.number==b.number&&(a.type!=JS_STRING||(a.length==b.length&&!memcmp(a.text,b.text,a.length)))&&(a.type!=JS_NATIVE||(a.length==b.length&&a.text==b.text));}
static handle *by_value(state *s,js_value v){for(handle *h=s->handles;h;h=h->next)if(equal_value(h->value,v))return h;return NULL;}
static int own_property(JSContext *ctx,JSPropertyDescriptor *desc,JSValueConst obj,JSAtom atom){
    state *s=JS_GetContextOpaque(ctx);handle *h=JS_GetOpaque(obj,object_class);if(!h)return 0;
    const char *name=JS_AtomToCString(ctx,atom);if(!name)return -1;
    js_value value=js_get(s->vm,js_property_owner(h->value),name);JS_FreeCString(ctx,name);
    if(s->vm->error){JS_ThrowInternalError(ctx,"%s",s->vm->error);return -1;}if(value.type==ENVY_UNDEFINED)return 0;
    if(desc){desc->flags=JS_PROP_C_W_E;desc->value=push(s,value);desc->getter=desc->setter=JS_UNDEFINED;}
    return 1;
}
static int property_names(JSContext *ctx,JSPropertyEnum **out,uint32_t *length,JSValueConst obj){
    state *s=JS_GetContextOpaque(ctx);handle *h=JS_GetOpaque(obj,object_class);uint32_t owner=js_property_owner(h->value),n=0;
    for(uint32_t i=0;i<s->vm->property_count;i++)if(s->vm->properties[i].object==owner)n++;
    JSPropertyEnum *names=n?js_malloc(ctx,n*sizeof(*names)):NULL;if(n&&!names)return -1;*out=names;*length=n;n=0;
    for(uint32_t i=0;i<s->vm->property_count;i++)if(s->vm->properties[i].object==owner){names[n].is_enumerable=1;names[n++].atom=JS_NewAtom(ctx,s->vm->properties[i].name);}return 0;
}
static int set_property(JSContext *ctx,JSValueConst obj,JSAtom atom,JSValueConst value,JSValueConst receiver,int flags){
    (void)receiver;(void)flags;state *s=JS_GetContextOpaque(ctx);handle *h=JS_GetOpaque(obj,object_class);
    const char *name=JS_AtomToCString(ctx,atom);if(!name)return -1;js_value v=pull(s,value);
    int ok=js_set(s->vm,js_property_owner(h->value),name,v);JS_FreeCString(ctx,name);
    if(!ok){JS_ThrowInternalError(ctx,"%s",s->vm->error?s->vm->error:"Property assignment failed");return -1;}return 1;
}
static JSValue host_call(JSContext *ctx,JSValueConst function,JSValueConst receiver,int argc,JSValueConst *argv,int flags){
    (void)receiver;(void)flags;state *s=JS_GetContextOpaque(ctx);handle *h=JS_GetOpaque(function,function_class);js_value result={0};
    js_value *args=argc?envy_js_alloc(NULL,NULL,argc*sizeof(*args)):NULL;if(argc&&!args)return JS_ThrowOutOfMemory(ctx);
    for(int i=0;i<argc;i++)args[i]=pull(s,argv[i]);int sync=h->value.type!=JS_NATIVE||!s->vm->host_sync||s->vm->host_sync(s->vm,(int)h->value.number,args,(uint32_t)argc);if(sync)export_globals(s);int ok;
    if(h->value.type==JS_NATIVE){uint32_t saved=s->vm->native_owner;s->vm->native_owner=h->value.length;
        ok=s->vm->host&&s->vm->host(s->vm,(int)h->value.number,args,(uint32_t)argc,&result);s->vm->native_owner=saved;
    }else ok=js_call_args(s->vm,h->value,args,(uint32_t)argc,&result);
    envy_js_alloc(NULL,args,0);if(!ok||s->vm->error)return JS_ThrowInternalError(ctx,"%s",s->vm->error?s->vm->error:"Browser API call failed");
    if(sync)import_globals(s);return push(s,result);
}
static JSClassExoticMethods exotic={.get_own_property=own_property,.get_own_property_names=property_names,.set_property=set_property};
static JSValue push(state *s,js_value v){
    JSContext *ctx=s->ctx;
    switch(v.type){
    case ENVY_UNDEFINED:case ENVY_UNINITIALIZED:return JS_UNDEFINED;
    case ENVY_NULL:return JS_NULL;
    case JS_BOOLEAN:return JS_NewBool(ctx,v.number!=0);
    case JS_NUMBER:return JS_NewInt64(ctx,v.number);
    case JS_REAL:{union {int64_t bits;double number;}n={.bits=v.number};return JS_NewFloat64(ctx,n.number);}
    case JS_STRING:return JS_NewStringLen(ctx,v.text,v.length);
    }
    handle *h=by_value(s,v);if(h)return JS_DupValue(ctx,h->object);
    h=envy_js_alloc(NULL,NULL,sizeof(*h));if(!h)return JS_ThrowOutOfMemory(ctx);
    int callable=v.type==JS_NATIVE||v.type==JS_FUNCTION;
    JSValue obj=JS_NewObjectClass(ctx,callable?function_class:object_class);if(JS_IsException(obj)){envy_js_alloc(NULL,h,0);return obj;}
    *h=(handle){s->handles,v,obj,0};s->handles=h;JS_SetOpaque(obj,h);if(callable)JS_SetConstructorBit(ctx,obj,1);
    else {JSValue global=JS_GetGlobalObject(ctx),constructor=JS_GetPropertyStr(ctx,global,js_get(s->vm,(uint32_t)v.number,"length").type==JS_NUMBER?"Array":"Object"),proto=JS_GetPropertyStr(ctx,constructor,"prototype");
        JS_SetPrototype(ctx,obj,proto);JS_FreeValue(ctx,proto);JS_FreeValue(ctx,constructor);JS_FreeValue(ctx,global);}
    return JS_DupValue(ctx,obj);
}
static js_value pull(state *s,JSValueConst v){
    if(JS_IsUndefined(v))return (js_value){0};if(JS_IsNull(v))return (js_value){ENVY_NULL,0,0,0};
    if(JS_IsBool(v))return (js_value){JS_BOOLEAN,JS_ToBool(s->ctx,v),0,0};
    if(JS_IsNumber(v)){double n;JS_ToFloat64(s->ctx,&n,v);
        if(isfinite(n)&&n>=-9223372036854775808.0&&n<9223372036854775808.0&&floor(n)==n)return (js_value){JS_NUMBER,(int64_t)n,0,0};
        union {double number;int64_t bits;}a={.number=n};return (js_value){JS_REAL,a.bits,0,0};}
    if(JS_IsString(v)){size_t length;const char *text=JS_ToCStringLen(s->ctx,&length,v);if(!text)return (js_value){0};js_value value=js_string(s->vm,text,(uint32_t)length);JS_FreeCString(s->ctx,text);return value;}
    for(handle *h=s->handles;h;h=h->next)if(JS_VALUE_GET_PTR(h->object)==JS_VALUE_GET_PTR(v))return h->value;
    js_value value=js_object(s->vm);if(s->vm->error)return value;
    if(JS_IsFunction(s->ctx,v)){value.type=JS_FUNCTION;value.number|=JS_FULL_FUNCTION;}
    handle *h=envy_js_alloc(NULL,NULL,sizeof(*h));if(!h){s->vm->error="JavaScript memory exhausted";return (js_value){0};}
    *h=(handle){s->handles,value,JS_DupValue(s->ctx,v),1};s->handles=h;return value;
}
static void import_globals(state *s){
    if(s->syncing)return;s->syncing=1;JSValue global=JS_GetGlobalObject(s->ctx);
    for(uint32_t i=0;i<s->vm->property_count;i++){js_property p=s->vm->properties[i];if(p.object)continue;
        if(!strcmp(p.name,"Date"))continue;
        JSValue old=envy_qjs_global(s->ctx,p.name),value=push(s,p.value);if(!JS_SameValue(s->ctx,old,value)){
            if(p.readonly)JS_DefinePropertyValueStr(s->ctx,global,p.name,JS_DupValue(s->ctx,value),JS_PROP_ENUMERABLE);
            else JS_SetPropertyStr(s->ctx,global,p.name,JS_DupValue(s->ctx,value));}
        JS_FreeValue(s->ctx,old);JS_FreeValue(s->ctx,value);}
    JS_FreeValue(s->ctx,global);s->syncing=0;
}
static void export_object(state *s,JSValueConst obj){
    JSPropertyEnum *names;uint32_t count;if(JS_GetOwnPropertyNames(s->ctx,&names,&count,obj,JS_GPN_STRING_MASK|JS_GPN_ENUM_ONLY)<0)return;
    for(uint32_t i=0;i<count;i++){const char *name=JS_AtomToCString(s->ctx,names[i].atom);JSValue value=JS_GetProperty(s->ctx,obj,names[i].atom);int same=0;
        if(JS_IsException(value)){JSValue error=JS_GetException(s->ctx);JS_FreeValue(s->ctx,error);JS_FreeCString(s->ctx,name);JS_FreeAtom(s->ctx,names[i].atom);continue;}
        if(name){for(uint32_t j=0;j<s->vm->property_count;j++){js_property p=s->vm->properties[j];if(!p.object&&!strcmp(p.name,name)){JSValue old=push(s,p.value);same=JS_SameValue(s->ctx,old,value);JS_FreeValue(s->ctx,old);break;}}
            if(!same)js_set(s->vm,0,name,pull(s,value));}
        JS_FreeCString(s->ctx,name);JS_FreeValue(s->ctx,value);JS_FreeAtom(s->ctx,names[i].atom);}
    js_free(s->ctx,names);
}
static void export_globals(state *s){if(s->syncing)return;s->syncing=1;JSValue global=JS_GetGlobalObject(s->ctx);export_object(s,global);JS_FreeValue(s->ctx,global);
    global=envy_qjs_globals(s->ctx);export_object(s,global);JS_FreeValue(s->ctx,global);s->syncing=0;}
static JSModuleDef *module_loader(JSContext *ctx,const char *name,void *opaque){
    state *s=opaque;const char *source;uint32_t count;
    if(!s->vm->module_source||!s->vm->module_source(s->vm,name,&source,&count)){JS_ThrowReferenceError(ctx,"Cannot load local module '%s'",name);return NULL;}
    JSValue compiled=JS_Eval(ctx,source,count,name,JS_EVAL_TYPE_MODULE|JS_EVAL_FLAG_COMPILE_ONLY);
    if(JS_IsException(compiled))return NULL;JSModuleDef *module=JS_VALUE_GET_PTR(compiled);JS_FreeValue(ctx,compiled);return module;
}
void js_full_reset(js_vm *vm){state **link=&states;while(*link){state *s=*link;if(s->vm!=vm){link=&s->next;continue;}
    while(s->handles){handle *h=s->handles;s->handles=h->next;JS_FreeValue(s->ctx,h->object);envy_js_alloc(NULL,h,0);}
    JS_FreeContext(s->ctx);JS_FreeRuntime(s->rt);*link=s->next;envy_js_alloc(NULL,s,0);break;}}
static state *create(js_vm *vm){
    state *s=find(vm);if(s)return s;s=envy_js_alloc(NULL,NULL,sizeof(*s));if(!s){vm->error="JavaScript memory exhausted";return NULL;}
    *s=(state){.next=states,.vm=vm};s->rt=JS_NewRuntime();if(!s->rt){envy_js_alloc(NULL,s,0);vm->error="JavaScript memory exhausted";return NULL;}
    JS_SetMaxStackSize(s->rt,98304);s->ctx=JS_NewContext(s->rt);if(!s->ctx){JS_FreeRuntime(s->rt);envy_js_alloc(NULL,s,0);vm->error="JavaScript memory exhausted";return NULL;}
    if(!object_class)JS_NewClassID(&object_class);if(!function_class)JS_NewClassID(&function_class);
    JSClassDef object={.class_name="EnvyObject",.exotic=&exotic},function={.class_name="EnvyFunction",.call=host_call};
    JS_NewClass(s->rt,object_class,&object);JS_NewClass(s->rt,function_class,&function);
    JSValue global=JS_GetGlobalObject(s->ctx),constructor=JS_GetPropertyStr(s->ctx,global,"Function"),proto=JS_GetPropertyStr(s->ctx,constructor,"prototype");JS_SetClassProto(s->ctx,function_class,proto);JS_FreeValue(s->ctx,constructor);JS_FreeValue(s->ctx,global);
    JS_SetContextOpaque(s->ctx,s);
    const char *dom_types="globalThis.HTMLElement=function HTMLElement(){throw new TypeError('Use document.createElement');};Object.defineProperty(HTMLElement,Symbol.hasInstance,{value:function(value){return !!value&&typeof value.tagName==='string';}});";
    JSValue setup=JS_Eval(s->ctx,dom_types,strlen(dom_types),"<DOM>",JS_EVAL_TYPE_GLOBAL);JS_FreeValue(s->ctx,setup);
    JS_SetModuleLoaderFunc(s->rt,NULL,module_loader,s);states=s;return s;
}
int js_full_jobs(js_vm *vm){state *s=find(vm);if(!s||s->running||s->draining)return 1;s->draining=1;JSContext *ctx;int result;
    while((result=JS_ExecutePendingJob(s->rt,&ctx))>0){}if(result<0)exception(s);export_globals(s);s->draining=0;return !vm->error;}
int js_full_promise(js_vm *vm,js_value value,int rejected,js_value *out){
    state *s=find(vm);if(!s)return 0;JSValue functions[2],promise=JS_NewPromiseCapability(s->ctx,functions);
    if(JS_IsException(promise)){exception(s);return 0;}JSValue argument=push(s,value),result=JS_Call(s->ctx,functions[rejected!=0],JS_UNDEFINED,1,&argument);
    JS_FreeValue(s->ctx,argument);JS_FreeValue(s->ctx,functions[0]);JS_FreeValue(s->ctx,functions[1]);
    if(JS_IsException(result)){exception(s);JS_FreeValue(s->ctx,promise);return 0;}JS_FreeValue(s->ctx,result);*out=pull(s,promise);JS_FreeValue(s->ctx,promise);return !vm->error;
}
int js_full_eval_named(js_vm *vm,const char *source,uint32_t count,const char *name){
    state *s=create(vm);if(!s)return 0;char *copy=envy_js_alloc(NULL,NULL,(int)count+1);if(!copy){vm->error="JavaScript memory exhausted";return 0;}memcpy(copy,source,count);copy[count]=0;
    import_globals(s);int type=JS_DetectModule(copy,count)?JS_EVAL_TYPE_MODULE:JS_EVAL_TYPE_GLOBAL;s->running++;
    JSValue result=JS_Eval(s->ctx,copy,count,name,type);envy_js_alloc(NULL,copy,0);s->running--;
    if(JS_IsException(result)){exception(s);return 0;}JS_FreeValue(s->ctx,result);export_globals(s);return js_full_jobs(vm);
}
int js_full_eval(js_vm *vm,const char *source,uint32_t count){return js_full_eval_named(vm,source,count,"<script>");}
int js_full_get(js_vm *vm,uint32_t owner,const char *name,js_value *out){
    state *s=find(vm);if(!s)return 0;handle *h=owner?by_owner(s,owner):NULL;if(owner&&(!h||!h->foreign))return 0;
    JSValue value=owner?JS_GetPropertyStr(s->ctx,h->object,name):envy_qjs_global(s->ctx,name);
    if(JS_IsException(value)){exception(s);*out=(js_value){0};return 1;}*out=pull(s,value);JS_FreeValue(s->ctx,value);
    if(out->type==JS_FUNCTION&&((uint32_t)out->number&JS_FULL_FUNCTION))out->length=owner;return 1;
}
int js_full_set(js_vm *vm,uint32_t owner,const char *name,js_value value){state *s=find(vm);handle *h=s?by_owner(s,owner):NULL;if(!h||!h->foreign)return -1;
    if(JS_SetPropertyStr(s->ctx,h->object,name,push(s,value))<0){exception(s);return 0;}return 1;}
int js_full_call(js_vm *vm,js_value function,const js_value *args,uint32_t count,js_value *out){
    state *s=find(vm);handle *h=s?by_owner(s,js_property_owner(function)):NULL;if(!h||!h->foreign){vm->error="Function expected";return 0;}
    JSValue *argv=count?envy_js_alloc(NULL,NULL,(int)(count*sizeof(*argv))):NULL;if(count&&!argv){vm->error="JavaScript memory exhausted";return 0;}
    for(uint32_t i=0;i<count;i++)argv[i]=push(s,args[i]);import_globals(s);handle *receiver=function.length?by_owner(s,function.length):NULL;
    s->running++;JSValue result=JS_Call(s->ctx,h->object,receiver?receiver->object:JS_UNDEFINED,(int)count,argv);s->running--;
    for(uint32_t i=0;i<count;i++)JS_FreeValue(s->ctx,argv[i]);envy_js_alloc(NULL,argv,0);
    if(JS_IsException(result)){exception(s);return 0;}*out=pull(s,result);JS_FreeValue(s->ctx,result);export_globals(s);return js_full_jobs(vm);
}
