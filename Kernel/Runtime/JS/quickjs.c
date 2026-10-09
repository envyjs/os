#define CONFIG_VERSION "2025-09-13"
#define ENVY_FREESTANDING 1
#include "vendor/quickjs/quickjs.c"

JSValue envy_qjs_globals(JSContext *ctx){return JS_DupValue(ctx,ctx->global_var_obj);}
JSValue envy_qjs_global(JSContext *ctx,const char *name){JSAtom atom=JS_NewAtom(ctx,name);JSValue value=JS_GetGlobalVar(ctx,atom,0);JS_FreeAtom(ctx,atom);return value;}

