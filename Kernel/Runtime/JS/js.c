#include "js.h"
#include "full.h"
typedef struct {
    js_vm *vm; const char *start, *p, *end, *token;
    uint32_t length, line, token_line, previous_line; int kind;
} parser;
extern void *envy_js_alloc(void *,void *,int);
static int template_append(parser *p,char **buffer,uint32_t *capacity,uint32_t *used,const char *text,uint32_t count){
    if(count>0x7fffffffu-*used-1){p->vm->error="JavaScript memory exhausted";return 0;}
    uint32_t needed=*used+count+1;if(needed>*capacity){uint32_t size=*capacity?*capacity:256;
        while(size<needed){if(size>0x3fffffffu){size=needed;break;}size*=2;}
        char *grown=envy_js_alloc(0,*buffer,(int)size);if(!grown){p->vm->error="JavaScript memory exhausted";return 0;}*buffer=grown;*capacity=size;}
    for(uint32_t i=0;i<count;i++)(*buffer)[(*used)++]=text[i];return 1;
}
enum { IDENTIFIER=256, STRING, NUMBER, TEMPLATE, STRICT_EQUAL, STRICT_NOT_EQUAL, LOGICAL_OR, LOGICAL_AND, INCREMENT, DECREMENT };
static int same(const char *a, const char *b) {
    while (*a && *a==*b) { ++a; ++b; } return *a==*b;
}
static uint32_t length(const char *s) { uint32_t n=0; while(s[n]) ++n; return n; }
static js_value undefined(void) { return (js_value){0,0,0,0}; }
js_value js_number_string(js_vm *vm,int64_t number) {
    char digits[21];uint32_t position=20;uint64_t n=number<0?0ull-(uint64_t)number:(uint64_t)number;
    digits[position]=0;
    do{uint64_t quotient=0;uint32_t remainder=0;
        /* Freestanding x86 has no compiler runtime for 64-bit division. */
        for(int bit=63;bit>=0;bit--){remainder=(remainder<<1)|(uint32_t)(n>>63);n<<=1;quotient<<=1;
            if(remainder>=10){remainder-=10;quotient|=1;}}
        digits[--position]=(char)('0'+remainder);n=quotient;
    }while(n);
    if(number<0)digits[--position]='-';return js_string(vm,digits+position,20-position);
}
#define number_string js_number_string
static int error(parser *p,const char *message) {
    if (!p->vm->error) { p->vm->error=message; p->vm->error_line=p->token_line; } return 0;
}
js_value js_string(js_vm *vm,const char *text,uint32_t count) {
    if (count>=vm->arena_end-vm->used) { vm->error="JavaScript memory exhausted"; return undefined(); }
    char *copy=vm->strings+vm->used;
    for(uint32_t i=0;i<count;i++) copy[i]=text[i]; copy[count]=0; vm->used+=count+1;
    return (js_value){JS_STRING,0,copy,count};
}
js_value js_object(js_vm *vm) {
    /* Reserve one arena byte per identity; object counts are memory bounded. */
    if(vm->used>=vm->arena_end || vm->objects>=0x7fffffffu) { vm->error="JavaScript memory exhausted"; return undefined(); }
    vm->used++;
    return (js_value){JS_OBJECT,(int32_t)++vm->objects,0,0};
}
js_value js_native(int id) { return (js_value){JS_NATIVE,id,0,0}; }
js_value js_native_bound(int id,uint32_t owner) { return (js_value){JS_NATIVE,id,0,owner}; }
uint32_t js_property_owner(js_value value){
    if(value.type==JS_OBJECT)return (uint32_t)value.number;
    if(value.type==JS_FUNCTION)return 0x40000000u|(uint32_t)value.number;
    if(value.type==JS_NATIVE)return 0x60000000u|((uint32_t)value.number&0x1fffffffu);
    return 0;
}
static uint32_t property_hash(uint32_t object,const char *name){uint32_t hash=object*2654435761u;for(;*name;name++)hash=(hash^(uint8_t)*name)*16777619u;return hash&2047u;}
static void property_index(js_vm *vm){for(uint32_t i=0;i<2048;i++)vm->property_buckets[i]=0;for(uint32_t i=0;i<vm->property_count;i++){uint32_t bucket=property_hash(vm->properties[i].object,vm->properties[i].name);vm->properties[i].next=vm->property_buckets[bucket];vm->property_buckets[bucket]=i+1;}}
js_value js_get(js_vm *vm,uint32_t object,const char *name) {
    js_value dynamic;if(vm->read_property&&vm->read_property(vm,object,name,&dynamic))return dynamic;
    for(uint32_t slot=vm->property_buckets[property_hash(object,name)];slot;slot=vm->properties[slot-1].next){uint32_t i=slot-1;
        if(vm->properties[i].object==object && same(vm->properties[i].name,name)) return vm->properties[i].value;
    }
    js_value value;if(js_full_get(vm,object,name,&value))return value;
    return undefined();
}
int js_set(js_vm *vm,uint32_t object,const char *name,js_value value) {
    if(object){int full=js_full_set(vm,object,name,value);if(full>=0)return full;}
    uint32_t bucket=property_hash(object,name);
    for(uint32_t slot=vm->property_buckets[bucket];slot;slot=vm->properties[slot-1].next){uint32_t i=slot-1;if(vm->properties[i].object==object && same(vm->properties[i].name,name)) {
        if(vm->properties[i].readonly) {vm->error="Assignment to constant binding";return 0;}
        vm->properties[i].value=value;if(vm->changed_property)vm->changed_property(vm,object,name);return 1;
    }
    }
    if(vm->property_count==vm->property_capacity) {
        uint32_t capacity=vm->property_capacity?vm->property_capacity*2:16;
        uint32_t end=vm->arena_end;
        /* Reuse the old table's space when no function was allocated below it. */
        if(vm->properties&&(char *)vm->properties==vm->strings+end)
            end+=vm->property_capacity*sizeof(js_property);
        if(capacity>end/sizeof(js_property)) {vm->error="JavaScript memory exhausted";return 0;}
        uint32_t offset=(end-capacity*sizeof(js_property))&~((uint32_t)_Alignof(js_property)-1u);
        if(offset<vm->used) {vm->error="JavaScript memory exhausted";return 0;}
        js_property *properties=(js_property *)(vm->strings+offset);
        for(uint32_t i=0;i<vm->property_count;i++) properties[i]=vm->properties[i];
        vm->properties=properties;vm->property_capacity=capacity;vm->arena_end=offset;
    }
    js_value key=js_string(vm,name,length(name)); if(vm->error) return 0;
    vm->properties[vm->property_count++]=(js_property){object,key.text,value,0,vm->property_buckets[bucket]};vm->property_buckets[bucket]=vm->property_count;if(vm->changed_property)vm->changed_property(vm,object,name);return 1;
}
void js_init(js_vm *vm,js_host_call host,void *context) {
    js_full_reset(vm);
    vm->read_property=0;vm->changed_property=0;vm->module_source=0;vm->source_name=0;vm->host_sync=0;
    vm->property_count=vm->objects=vm->used=vm->depth=0; vm->host=host; vm->context=context;
    vm->error=0; vm->error_line=0; vm->native_owner=0;
    vm->function_count=vm->scope=vm->environment_count=0;
    vm->functions=0;vm->arena_end=sizeof(vm->strings);
    vm->properties=0;vm->property_capacity=0;
    property_index(vm);
}
static int alpha(char c) { return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'||c=='$'; }
static int digit(char c) { return c>='0'&&c<='9'; }
static void next(parser *p) {
    p->previous_line=p->token_line;
    for(;;) {
        while(p->p<p->end && (unsigned char)*p->p<=32) { if(*p->p=='\n') p->line++; p->p++; }
        if(p->end-p->p>=2 && p->p[0]=='/' && p->p[1]=='/') {
            while(p->p<p->end && *p->p!='\n') p->p++; continue;
        }
        if(p->end-p->p>=2 && p->p[0]=='/' && p->p[1]=='*') {
            p->p+=2;
            while(p->end-p->p>=2 && !(p->p[0]=='*'&&p->p[1]=='/')) { if(*p->p++=='\n') p->line++; }
            if(p->end-p->p<2) { error(p,"Unterminated comment"); p->kind=0; return; }
            p->p+=2; continue;
        }
        break;
    }
    p->token=p->p; p->token_line=p->line; p->length=0;
    if(p->p==p->end) { p->kind=0; return; }
    char c=*p->p++;
    if((c=='='||c=='!')&&p->end-p->p>=2&&p->p[0]=='='&&p->p[1]=='=') {
        p->p+=2;p->kind=c=='='?STRICT_EQUAL:STRICT_NOT_EQUAL;
    }
    else if(p->p<p->end&&p->p[0]==c&&(c=='|'||c=='&'||c=='+'||c=='-')){
        p->p++;p->kind=c=='|'?LOGICAL_OR:c=='&'?LOGICAL_AND:c=='+'?INCREMENT:DECREMENT;
    }
    else if(alpha(c)) { while(p->p<p->end && (alpha(*p->p)||digit(*p->p))) p->p++; p->kind=IDENTIFIER; }
    else if(digit(c)) { while(p->p<p->end && digit(*p->p)) p->p++; p->kind=NUMBER; }
    else if(c=='`') {
        p->token=p->p;
        while(p->p<p->end&&*p->p!='`') {
            if(*p->p=='\n') p->line++;
            if(*p->p=='\\'&&p->p+1<p->end) p->p++;
            p->p++;
        }
        if(p->p==p->end) {error(p,"Unterminated template literal");p->kind=0;return;}
        p->length=(uint32_t)(p->p-p->token);p->p++;p->kind=TEMPLATE;return;
    }
    else if(c=='\''||c=='"') {
        p->token=p->p;
        while(p->p<p->end && *p->p!=c) {
            if(*p->p=='\n') { error(p,"Newline in string literal"); break; }
            if(*p->p=='\\') { p->p++; if(p->p==p->end) break; }
            p->p++;
        }
        if(p->p==p->end || *p->p!=c) { error(p,"Unterminated string literal"); p->kind=0; return; }
        p->length=(uint32_t)(p->p-p->token); p->p++; p->kind=STRING; return;
    } else p->kind=(unsigned char)c;
    p->length=(uint32_t)(p->p-p->token);
}
static int word(parser *p,const char *text) {
    if(p->kind!=IDENTIFIER || p->length!=length(text)) return 0;
    for(uint32_t i=0;i<p->length;i++) if(p->token[i]!=text[i]) return 0;
    return 1;
}
static int expect(parser *p,int kind) {
    if(p->kind!=kind) return error(p,"Unexpected token"); next(p); return !p->vm->error;
}
static int identifier(parser *p,char *name) {
    if(p->kind!=IDENTIFIER || p->length>63) return error(p,"Identifier expected (maximum 63 bytes)");
    for(uint32_t i=0;i<p->length;i++) name[i]=p->token[i]; name[p->length]=0; next(p); return 1;
}
static const char *token_source(parser *p){return p->kind==STRING||p->kind==TEMPLATE?p->token-1:p->token;}
typedef struct { js_value value; uint32_t owner; char name[64]; int reference; } expression;
static expression expr(parser *p,uint32_t depth);
static int execute(js_vm *vm,const char *source,uint32_t count,int function_body,js_value *result,int *returned,uint32_t line);
static uint32_t binding(js_vm *vm,const char *name,int *found) {
    uint32_t scope=vm->scope; *found=0;
    for(;;) {
        for(uint32_t i=0;i<vm->property_count;i++) if(vm->properties[i].object==scope&&same(vm->properties[i].name,name)) {
            *found=1; return scope;
        }
        if(!scope) break; scope=vm->parents[(scope&0xffffu)-1];
    }
    return 0;
}
static js_value function(parser *p,int declaration,int arrow) {
    js_value value=undefined(); js_function fn={0}; fn.scope=p->vm->scope;
    if(!arrow) {
        next(p);
        if(p->kind==IDENTIFIER) { if(!identifier(p,fn.name)) return value; }
        else if(declaration) {error(p,"Function declaration requires a name");return value;}
    }
    if(arrow!=2&&!expect(p,'(')) return value;
    while(p->kind!=')'&&!p->vm->error) {
        if(fn.parameter_count==8) {error(p,"Function parameter limit reached");return value;}
        if(!identifier(p,fn.parameters[fn.parameter_count++])) return value;
        if(arrow==2) break;
        if(p->kind=='=') {
            next(p); uint32_t index=fn.parameter_count-1, nesting=0;
            fn.defaults[index]=token_source(p);
            while(p->kind&&!p->vm->error) {
                if(!nesting&&(p->kind==','||p->kind==')')) break;
                if(p->kind=='('||p->kind=='{'||p->kind=='[') nesting++;
                else if(p->kind==')'||p->kind=='}'||p->kind==']') {
                    if(!nesting) {error(p,"Invalid default parameter expression");return value;} nesting--;
                }
                next(p);
            }
            fn.default_lengths[index]=(uint32_t)(p->token-fn.defaults[index]);
            if(!fn.default_lengths[index]||nesting) {error(p,"Default parameter expression expected");return value;}
        }
        if(p->kind!=',') break; next(p);
        if(p->kind==')') {error(p,"Trailing parameter comma is not supported");return value;}
    }
    if(arrow!=2&&!expect(p,')')) return value;
    if(arrow&&(!expect(p,'=')||!expect(p,'>'))) return value;
    if(arrow&&p->kind!='{') {
        fn.expression_body=1;fn.source=token_source(p);fn.line=p->token_line;uint32_t nesting=0;
        while(p->kind&&!p->vm->error) {
            if(!nesting&&(p->kind==','||p->kind==')'||p->kind==';'||p->kind=='}')) break;
            if(p->kind=='('||p->kind=='{'||p->kind=='[') nesting++;
            else if(p->kind==')'||p->kind=='}'||p->kind==']') {if(!nesting) break;nesting--;}
            next(p);
        }
        fn.length=(uint32_t)(p->token-fn.source);
        if(!fn.length||nesting) {error(p,"Arrow expression expected");return value;}
    } else {
    if(p->kind!='{') {error(p,"Function body expected");return value;}
    fn.source=p->p;fn.line=p->token_line;uint32_t level=1; next(p);
    while(p->kind&&level&&!p->vm->error) {
        if(p->kind=='{') level++; else if(p->kind=='}') level--;
        if(level) next(p);
    }
    if(level||p->vm->error) {error(p,"Unterminated function body");return value;}
    fn.length=(uint32_t)(p->token-fn.source);next(p);
    }
    js_vm *vm=p->vm;js_function *stored=vm->functions;
    while(stored&&(stored->source!=fn.source||stored->scope!=fn.scope)) stored=stored->next;
    if(!stored) {
        if(vm->arena_end<sizeof(fn)) {error(p,"JavaScript memory exhausted");return value;}
        uint32_t offset=(vm->arena_end-sizeof(fn))&~((uint32_t)_Alignof(js_function)-1u);
        if(offset<vm->used) {error(p,"JavaScript memory exhausted");return value;}
        stored=(js_function *)(vm->strings+offset);vm->arena_end=offset;
        fn.index=vm->function_count++;fn.next=vm->functions;
        *stored=fn;vm->functions=stored;
    }
    value=(js_value){JS_FUNCTION,(int32_t)stored->index,fn.source,fn.length};
    if(declaration) js_set(p->vm,p->vm->scope,fn.name,value);
    return value;
}
static int invoke(js_vm *vm,js_value function,const js_value *args,uint32_t argc,js_value *result) {
    if(function.type==JS_FUNCTION&&((uint32_t)function.number&JS_FULL_FUNCTION))return js_full_call(vm,function,args,argc,result);
    if(function.type!=JS_FUNCTION||function.number<0||(uint32_t)function.number>=vm->function_count) {vm->error="Function expected";return 0;}
    if(vm->depth>=32||vm->environment_count==4096) {vm->error="JavaScript call/environment limit reached";return 0;}
    js_function *fn=vm->functions;
    while(fn&&fn->index!=(uint32_t)function.number) fn=fn->next;
    if(!fn) {vm->error="Function expected";return 0;}
    *result=undefined();uint32_t first_object=vm->objects+1;
    uint32_t saved_scope=vm->scope, environment=vm->environment_count++;
    vm->parents[environment]=fn->scope; vm->scope=0x80000000u | (environment+1);
    if(fn->name[0]) js_set(vm,vm->scope,fn->name,function);
    for(uint32_t i=0;i<fn->parameter_count;i++)
        js_set(vm,vm->scope,fn->parameters[i],(js_value){JS_UNINITIALIZED,0,0,0});
    /* Initializers run at call time, left to right, before body declarations. */
    vm->depth++;
    for(uint32_t i=0;i<fn->parameter_count&&!vm->error;i++) {
        js_value value=i<argc?args[i]:undefined();
        if(value.type==JS_UNDEFINED&&fn->defaults[i]) {
            parser p={.vm=vm,.p=fn->defaults[i],.end=fn->defaults[i]+fn->default_lengths[i],.line=1};
            next(&p);value=expr(&p,0).value;
            if(p.kind&&!vm->error) error(&p,"Invalid default parameter expression");
        }
        if(!vm->error) js_set(vm,vm->scope,fn->parameters[i],value);
    }
    vm->depth--;
    int returned=0,ok=0;
    if(!vm->error&&fn->expression_body) {
        vm->depth++;
        parser p={.vm=vm,.p=fn->source,.end=fn->source+fn->length,.line=fn->line};next(&p);
        *result=expr(&p,0).value;
        if(p.kind&&!vm->error) error(&p,"Invalid arrow expression");
        vm->depth--;ok=!vm->error;
    } else if(!vm->error) ok=execute(vm,fn->source,fn->length,1,result,&returned,fn->line);
    vm->scope=saved_scope;
    /* Recycle calls that created no captured environments. Keep escaped objects. */
    int captured=0;for(js_function *f=vm->functions;f;f=f->next)if(f->scope==(0x80000000u | (environment+1))){captured=1;break;}
    if(!captured&&vm->environment_count==environment+1) {
        uint32_t scope=0x80000000u | (environment+1);
        int escaped=js_full_active(vm)||(result->type==JS_OBJECT&&(uint32_t)result->number>=first_object)||
            (result->type==JS_NATIVE&&!result->text&&(result->number>=0||result->number==-3)&&result->length>=first_object);
        for(uint32_t i=0;i<vm->property_count&&!escaped;i++) {
            js_property *p=&vm->properties[i];js_value value=p->value;
            if(p->object==scope||(p->object>=first_object&&p->object<=vm->objects))continue;
            if((value.type==JS_OBJECT&&(uint32_t)value.number>=first_object)||
               (value.type==JS_NATIVE&&(value.number>=0||value.number==-3)&&!value.text&&value.length>=first_object))escaped=1;
        }
        uint32_t kept=0;
        for(uint32_t i=0;i<vm->property_count;i++) {
            js_property property=vm->properties[i];
            if(property.object==scope||(!escaped&&property.object>=first_object&&property.object<=vm->objects))continue;
            vm->properties[kept++]=property;
        }
        vm->property_count=kept;property_index(vm);if(!escaped)vm->objects=first_object-1;vm->environment_count--;
    }
    return ok;
}
static expression primary(parser *p,uint32_t depth) {
    expression e={.value={0,0,0,0}};
    if(depth>32) { error(p,"JavaScript expression nesting limit"); return e; }
    int arrow=0;
    if(p->kind==IDENTIFIER||p->kind=='(') {
        parser look=*p;
        if(p->kind==IDENTIFIER) {next(&look);arrow=2;}
        else {
            uint32_t nesting=1;next(&look);
            while(look.kind&&nesting&&!p->vm->error) {
                if(look.kind=='(') nesting++;else if(look.kind==')') nesting--;
                next(&look);
            }
            arrow=1;
        }
        if(look.kind!='=') arrow=0;
        else {next(&look);if(look.kind!='>') arrow=0;}
    }
    if(word(p,"new")) {next(p);e=primary(p,depth+1);}
    else if(arrow) e.value=function(p,0,arrow);
    else if(p->kind=='[') {
        e.value=js_object(p->vm);next(p);uint32_t count=0;
        while(p->kind!=']'&&!p->vm->error) {
            js_value key=number_string(p->vm,(int32_t)count++),value=expr(p,depth+1).value;
            if(!p->vm->error)js_set(p->vm,e.value.number,key.text,value);
            if(p->kind!=',')break;next(p);
        }
        expect(p,']');if(!p->vm->error)js_set(p->vm,e.value.number,"length",(js_value){JS_NUMBER,(int32_t)count,0,0});
    }
    else if(p->kind=='{') {
        e.value=js_object(p->vm);next(p);
        while(p->kind!='}'&&!p->vm->error) {
            char key[64];
            if(p->kind==STRING&&p->length<64) {
                for(uint32_t i=0;i<p->length;i++) key[i]=p->token[i];key[p->length]=0;next(p);
            } else if(!identifier(p,key)) break;
            if(!expect(p,':')) break;
            js_value value=expr(p,depth+1).value;
            if(!p->vm->error) js_set(p->vm,e.value.number,key,value);
            if(p->kind!=',') break;next(p);
        }
        expect(p,'}');
    }
    else if(p->kind==TEMPLATE) {
        char *buffer=0;uint32_t capacity=0,used=0;
        for(uint32_t i=0;i<p->length&&!p->vm->error;i++) {
            char c=p->token[i];
            if(c=='$'&&i+1<p->length&&p->token[i+1]=='{') {
                parser sub={.vm=p->vm,.p=p->token+i+2,.end=p->token+p->length,.line=p->token_line};
                next(&sub);js_value value=expr(&sub,depth+1).value;
                if(sub.kind!='}') {error(p,"Template interpolation closing brace expected");break;}
                i=(uint32_t)(sub.token-p->token);
                const char *text=value.text; uint32_t size=value.length;
                if(value.type==JS_NUMBER) {
                    js_value formatted=number_string(p->vm,value.number);text=formatted.text;size=formatted.length;
                    if(p->vm->error)break;
                } else if(value.type==JS_BOOLEAN) {text=value.number?"true":"false";size=value.number?4:5;}
                else if(value.type==JS_UNDEFINED) {text="undefined";size=9;}
                else if(value.type==JS_NULL) {text="null";size=4;}
                else if(value.type!=JS_STRING) {error(p,"Unsupported template interpolation value");break;}
                if(!template_append(p,&buffer,&capacity,&used,text,size))break;
                continue;
            }
            if(c=='\\'&&i+1<p->length) {
                c=p->token[++i];if(c=='n') c='\n';else if(c=='r') c='\r';else if(c=='t') c='\t';
            }
            if(!template_append(p,&buffer,&capacity,&used,&c,1))break;
        }
        if(!p->vm->error)e.value=js_string(p->vm,buffer?buffer:"",used);envy_js_alloc(0,buffer,0);next(p);
    } else if(p->kind==STRING) {
        if(p->vm->used>=p->vm->arena_end) {error(p,"JavaScript memory exhausted");return e;}
        uint32_t start=p->vm->used;
        for(uint32_t i=0;i<p->length;i++) {
            char c=p->token[i];
            if(c=='\\') {
                c=p->token[++i];
                if(c=='n') c='\n'; else if(c=='r') c='\r'; else if(c=='t') c='\t';
                else if(c!='\\'&&c!='\''&&c!='"') { error(p,"Unsupported string escape"); return e; }
            }
            if(p->vm->used+1>=p->vm->arena_end) { error(p,"JavaScript memory exhausted"); return e; }
            p->vm->strings[p->vm->used++]=c;
        }
        p->vm->strings[p->vm->used++]=0;
        e.value=(js_value){JS_STRING,0,p->vm->strings+start,p->vm->used-start-1}; next(p);
    } else if(p->kind==NUMBER) {
        uint64_t n=0;
        for(uint32_t i=0;i<p->length;i++) { if(n>922337203685477580ull || (n==922337203685477580ull && p->token[i]>'7')) { error(p,"Integer literal out of range"); return e; } n=n*10+(p->token[i]-'0'); }
        e.value=(js_value){JS_NUMBER,(int64_t)n,0,0}; next(p);
    } else if(word(p,"function")) {
        e.value=function(p,0,0);
    } else if(p->kind=='(') { next(p); e=expr(p,depth+1); expect(p,')'); }
    else if(p->kind==IDENTIFIER) {
        if(word(p,"true")||word(p,"false")) { e.value=(js_value){JS_BOOLEAN,word(p,"true"),0,0}; next(p); }
        else if(word(p,"null")||word(p,"undefined")) {if(word(p,"null")) e.value.type=JS_NULL;next(p);}
        else { identifier(p,e.name); int found=0; e.owner=binding(p->vm,e.name,&found);
            e.reference=1; e.value=js_get(p->vm,e.owner,e.name);
            if(e.value.type==JS_UNINITIALIZED) error(p,"Parameter accessed before initialization");
            if(!found&&e.value.type==JS_UNDEFINED) error(p,"Unknown JavaScript identifier"); }
    } else { error(p,"Unsupported JavaScript expression"); return e; }
    while(!p->vm->error && (p->kind=='.'||p->kind=='('||p->kind=='[')) {
        if(p->kind=='[') {
            if(e.value.type!=JS_OBJECT&&e.value.type!=JS_FUNCTION&&e.value.type!=JS_NATIVE){error(p,"Indexing requires an object");break;}
            uint32_t object=js_property_owner(e.value);next(p);js_value key=expr(p,depth+1).value;
            if(!expect(p,']'))break;
            if(key.type==JS_NUMBER)key=number_string(p->vm,key.number);
            if(key.type!=JS_STRING||key.length>63){error(p,"Unsupported property key");break;}
            e.owner=object;for(uint32_t i=0;i<=key.length;i++)e.name[i]=key.text[i];
            e.reference=1;e.value=js_get(p->vm,object,key.text);
        }
        else if(p->kind=='.') {
            if(e.value.type==JS_NUMBER||e.value.type==JS_STRING) {
                js_value receiver=e.value;next(p);if(!identifier(p,e.name))break;
                if(receiver.type==JS_NUMBER&&same(e.name,"toString")){js_value text=number_string(p->vm,receiver.number);e.value=(js_value){JS_NATIVE,-1,text.text,text.length};}
                else if(receiver.type==JS_STRING&&same(e.name,"padStart"))e.value=(js_value){JS_NATIVE,-2,receiver.text,receiver.length};
                else if(receiver.type==JS_STRING&&same(e.name,"includes"))e.value=(js_value){JS_NATIVE,-4,receiver.text,receiver.length};
                else {error(p,"Unsupported primitive property");break;}
                e.reference=0;continue;
            }
            if(e.value.type!=JS_OBJECT&&e.value.type!=JS_FUNCTION&&e.value.type!=JS_NATIVE) { error(p,"Property access requires an object"); break; }
            int object=e.value.type==JS_OBJECT;e.owner=js_property_owner(e.value);next(p);if(!identifier(p,e.name))break;
            e.reference=1; e.value=js_get(p->vm,e.owner,e.name);
            if(e.value.type==JS_UNDEFINED&&same(e.name,"forEach")&&js_get(p->vm,e.owner,"length").type==JS_NUMBER){e.value=js_native(-3);e.value.length=e.owner;e.reference=0;}
            if(object&&e.value.type==JS_UNDEFINED&&same(e.name,"classList")){
                js_value list=js_object(p->vm),add=js_native_bound(12000,e.owner);
                js_set(p->vm,list.number,"add",add);js_set(p->vm,e.owner,"classList",list);e.value=list;
            }
        } else {
            next(p); js_value args[8]; uint32_t count=0;
            while(p->kind!=')'&&!p->vm->error) {
                if(count==8) { error(p,"Call argument limit reached"); break; }
                args[count++]=expr(p,depth+1).value;
                if(p->kind!=',') break; next(p);
            }
            if(!expect(p,')')) break;
            js_value result=undefined();
            if(e.value.type==JS_NATIVE&&e.value.number==-4){
                if(!count||args[0].type!=JS_STRING){error(p,"String.includes requires a string");break;}
                uint32_t begin=count>1&&args[1].type==JS_NUMBER&&args[1].number>0?(uint32_t)args[1].number:0;int found=0;
                for(uint32_t i=begin;i<=e.value.length&&args[0].length<=e.value.length-i;i++){uint32_t j=0;while(j<args[0].length&&e.value.text[i+j]==args[0].text[j])j++;if(j==args[0].length){found=1;break;}}
                result=(js_value){JS_BOOLEAN,found,0,0};
            }else if(e.value.type==JS_NATIVE&&e.value.number==-1) {
                if(count)error(p,"Number.toString radix is not supported yet");
                else result=(js_value){JS_STRING,0,e.value.text,e.value.length};
            } else if(e.value.type==JS_NATIVE&&e.value.number==-2) {
                if(count<1||args[0].type!=JS_NUMBER||args[0].number<0) {error(p,"Invalid padStart length");break;}
                uint32_t size=(uint32_t)args[0].number;
                js_value fill=count>1?args[1]:(js_value){JS_STRING,0," ",1};
                if(fill.type!=JS_STRING) {error(p,"padStart fill must be a string");break;}
                if(size<=e.value.length||!fill.length)result=(js_value){JS_STRING,0,e.value.text,e.value.length};
                else {
                    if(size>=p->vm->arena_end-p->vm->used) {error(p,"JavaScript memory exhausted");break;}
                    char *text=p->vm->strings+p->vm->used;uint32_t padding=size-e.value.length;
                    for(uint32_t i=0;i<padding;i++)text[i]=fill.text[i%fill.length];
                    for(uint32_t i=0;i<e.value.length;i++)text[padding+i]=e.value.text[i];
                    text[size]=0;p->vm->used+=size+1;result=(js_value){JS_STRING,0,text,size};
                }
            } else if(e.value.type==JS_NATIVE&&e.value.number==-3){
                if(!count||args[0].type!=JS_FUNCTION){error(p,"Array.forEach requires a function");break;}
                uint32_t owner=e.value.length;js_value array={JS_OBJECT,(int32_t)owner,0,0},size=js_get(p->vm,owner,"length");
                if(size.number<0){error(p,"Invalid array length");break;}
                for(int64_t i=0;i<size.number&&!p->vm->error;i++){js_value key=number_string(p->vm,i),arguments[3]={js_get(p->vm,owner,key.text),{JS_NUMBER,i,0,0},array},ignored;invoke(p->vm,args[0],arguments,3,&ignored);}
            } else if(e.value.type==JS_NATIVE) {
                uint32_t saved_owner=p->vm->native_owner;p->vm->native_owner=e.value.length;
                int called=p->vm->host&&p->vm->host(p->vm,e.value.number,args,count,&result);
                p->vm->native_owner=saved_owner;
                if(!called&&!p->vm->error)error(p,"Browser API call failed");
            } else if(e.value.type==JS_FUNCTION) invoke(p->vm,e.value,args,count,&result);
            else error(p,"Value is not a supported callable");
            e.value=result; e.reference=0;
        }
    }
    if((p->kind==INCREMENT||p->kind==DECREMENT)&&!p->vm->error){
        if(!e.reference||e.value.type!=JS_NUMBER)error(p,"Increment requires a numeric reference");
        else{js_value updated=e.value;int decrement=p->kind==DECREMENT;
            if((!decrement&&updated.number==INT64_MAX)||(decrement&&updated.number==INT64_MIN))error(p,"Integer increment out of range");
            else{updated.number+=decrement?-1:1;js_set(p->vm,e.owner,e.name,updated);}}
        next(p);e.reference=0;
    }
    return e;
}
static int truth(js_value value);
static expression unary(parser *p,uint32_t depth){
    if(p->kind!='!'&&p->kind!='-'&&p->kind!='+')return primary(p,depth);
    expression e={.value={0,0,0,0}};if(depth>32){error(p,"JavaScript expression nesting limit");return e;}
    int operation=p->kind;next(p);e=unary(p,depth+1);e.reference=0;if(p->vm->error)return e;
    if(operation=='!')e.value=(js_value){JS_BOOLEAN,!truth(e.value),0,0};
    else if(e.value.type!=JS_NUMBER)error(p,"Unary numeric operator requires a number");
    else if(operation=='-'){if(e.value.number==INT64_MIN)error(p,"Integer negation out of range");else e.value.number=-e.value.number;}
    return e;
}
static expression addition(parser *p,uint32_t depth) {
    expression e=unary(p,depth);
    while((p->kind=='+'||p->kind=='-')&&!p->vm->error) {
        int subtract=p->kind=='-';next(p); expression right=unary(p,depth+1);
        if(e.value.type==JS_NUMBER && right.value.type==JS_NUMBER) {
            if(subtract){if(right.value.number==INT64_MIN){error(p,"Integer subtraction out of range");break;}right.value.number=-right.value.number;}
            if((right.value.number>0&&e.value.number>INT64_MAX-right.value.number)||
               (right.value.number<0&&e.value.number<INT64_MIN-right.value.number)) { error(p,"Integer addition out of range"); break; }
            e.value.number+=right.value.number;
        } else if(!subtract&&e.value.type==JS_STRING && right.value.type==JS_STRING) {
            uint32_t count=e.value.length+right.value.length;
            if(count>=p->vm->arena_end-p->vm->used) { error(p,"JavaScript memory exhausted"); break; }
            char *dst=p->vm->strings+p->vm->used;
            for(uint32_t i=0;i<e.value.length;i++) dst[i]=e.value.text[i];
            for(uint32_t i=0;i<right.value.length;i++) dst[e.value.length+i]=right.value.text[i];
            dst[count]=0; p->vm->used+=count+1; e.value=(js_value){JS_STRING,0,dst,count};
        } else error(p,"Unsupported operands for +");
        e.reference=0;
    }
    return e;
}
static expression equality(parser *p,uint32_t depth) {
    expression e=addition(p,depth);
    while((p->kind==STRICT_EQUAL||p->kind==STRICT_NOT_EQUAL)&&!p->vm->error) {
        int unequal=p->kind==STRICT_NOT_EQUAL;next(p);
        expression right=addition(p,depth+1);js_value a=e.value,b=right.value;
        int equal=a.type==b.type;
        if(equal&&a.type==JS_STRING) {
            equal=a.length==b.length;
            for(uint32_t i=0;equal&&i<a.length;i++) equal=a.text[i]==b.text[i];
        } else if(equal) equal=a.number==b.number;
        e.value=(js_value){JS_BOOLEAN,equal!=unequal,0,0};e.reference=0;
    }
    return e;
}
/* Consume an unchosen logical operand without invoking functions or allocating
   strings/objects. Balanced delimiters keep nested calls and literals together. */
static int skip_operand(parser *p,int stop_and){
    int closers[64];uint32_t nesting=0;int consumed=0;
    while(p->kind&&!p->vm->error){int kind=p->kind;
        if(!nesting&&(kind==LOGICAL_OR||(stop_and&&kind==LOGICAL_AND)||kind==','||kind==';'||kind==')'||kind==']'||kind=='}'||kind=='?'||kind==':'))break;
        if(kind=='('||kind=='['||kind=='{'){if(nesting==64)return error(p,"Expression nesting limit");closers[nesting++]=kind=='('?')':kind=='['?']':'}';}
        else if(kind==')'||kind==']'||kind=='}'){if(!nesting||closers[--nesting]!=kind)return error(p,"Mismatched expression delimiter");}
        consumed=1;next(p);
    }
    if(nesting||!consumed)return error(p,"Logical operand expected");return 1;
}
static int truth(js_value value);
static expression logical_and(parser *p,uint32_t depth){
    expression e=equality(p,depth);
    while(p->kind==LOGICAL_AND&&!p->vm->error){next(p);if(truth(e.value))e=equality(p,depth+1);else skip_operand(p,1);e.reference=0;}
    return e;
}
static expression expr(parser *p,uint32_t depth){
    expression e=logical_and(p,depth);
    while(p->kind==LOGICAL_OR&&!p->vm->error){next(p);if(!truth(e.value))e=logical_and(p,depth+1);else skip_operand(p,0);e.reference=0;}
    return e;
}
static int truth(js_value value) {
    return value.type!=JS_UNDEFINED&&value.type!=JS_NULL&&((value.type!=JS_NUMBER&&value.type!=JS_BOOLEAN)||value.number!=0)&&(value.type!=JS_STRING||value.length!=0);
}
/* Capture a braced branch without executing its expressions. */
static int block(parser *p,const char **source,uint32_t *count,uint32_t *line) {
    if(p->kind!='{') return error(p,"Braced conditional body expected");
    *source=p->p;*line=p->token_line;uint32_t nesting=1;next(p);
    while(p->kind&&nesting&&!p->vm->error) {
        if(p->kind=='{') nesting++;else if(p->kind=='}') nesting--;
        if(nesting) next(p);
    }
    if(nesting) return error(p,"Unterminated conditional body");
    *count=(uint32_t)(p->token-*source);next(p);return !p->vm->error;
}
static int execute(js_vm *vm,const char *source,uint32_t count,int function_body,js_value *result,int *returned,uint32_t line) {
    if(!vm || !source) return 0;
    if(vm->depth>=32) { vm->error="JavaScript call depth exceeded"; return 0; }
    vm->depth++; vm->error=0; vm->error_line=0;
    *result=undefined();
    /* Hoist declarations at this lexical body's top level; nested bodies are skipped. */
    parser scan={.vm=vm,.start=source,.p=source,.end=source+count,.line=line};next(&scan);
    uint32_t braces=0, parentheses=0; int statement_start=1;
    while(scan.kind&&!vm->error) {
        if(!braces&&!parentheses&&statement_start&&word(&scan,"function")) {
            parser look=scan;next(&look);
            if(look.kind==IDENTIFIER) {function(&scan,1,0);continue;}
        }
        if(scan.kind=='(') parentheses++; else if(scan.kind==')'&&parentheses) parentheses--;
        if(scan.kind=='{') braces++; else if(scan.kind=='}') {if(!braces){error(&scan,"Unexpected closing brace");break;}braces--;}
        statement_start=!braces&&!parentheses&&scan.kind==';';
        next(&scan);
        if(!braces&&!parentheses&&scan.token_line>scan.previous_line) statement_start=1;
    }
    if(vm->error) {vm->depth--;return 0;}
    parser p={.vm=vm,.start=source,.p=source,.end=source+count,.line=line}; next(&p);
    while(p.kind && !vm->error) {
        if(p.kind==';') { next(&p); continue; }
        if(word(&p,"class")) {error(&p,"JavaScript classes are not supported yet");break;}
        if(word(&p,"async")) {error(&p,"JavaScript async functions are not supported yet");break;}
        if(word(&p,"if")) {
            next(&p);if(!expect(&p,'(')) break;
            int chosen=truth(expr(&p,0).value);if(!expect(&p,')')) break;
            const char *yes,*no=0;uint32_t yes_count,no_count=0,yes_line,no_line=0;
            if(!block(&p,&yes,&yes_count,&yes_line)) break;
            if(word(&p,"else")) {next(&p);if(!block(&p,&no,&no_count,&no_line)) break;}
            const char *body=chosen?yes:no;uint32_t size=chosen?yes_count:no_count;
            if(body&&!vm->error) execute(vm,body,size,function_body,result,returned,chosen?yes_line:no_line);
            if(*returned) break;
            continue;
        }
        if(word(&p,"function")) {
            parser look=p;next(&look);
            if(look.kind!=IDENTIFIER) {error(&p,"Function declaration requires a name");break;}
            function(&p,0,0);continue;
        }
        if(word(&p,"return")) {
            if(!function_body) {error(&p,"return outside a function");break;}
            next(&p);
            if(p.kind&&p.kind!=';'&&p.token_line==p.previous_line) *result=expr(&p,0).value;
            if(p.kind&&p.kind!=';'&&p.token_line<=p.previous_line&&!vm->error) error(&p,"Invalid return expression");
            *returned=1;
            break;
        }
        if(word(&p,"var")||word(&p,"let")||word(&p,"const")) {
            int constant=word(&p,"const");
            char name[64]; next(&p); if(!identifier(&p,name)) break;
            js_value value=undefined();
            if(p.kind=='=') { next(&p); value=expr(&p,0).value; }
            else if(constant) {error(&p,"Constant initializer expected");break;}
            if(!vm->error) js_set(vm,vm->scope,name,value);
            if(constant&&!vm->error) for(uint32_t i=0;i<vm->property_count;i++)
                if(vm->properties[i].object==vm->scope&&same(vm->properties[i].name,name)) vm->properties[i].readonly=1;
        } else {
            expression e=expr(&p,0);
            if(p.kind=='='&&!vm->error) {
                if(!e.reference) { error(&p,"Invalid assignment target"); break; }
                next(&p); js_value value=expr(&p,0).value;
                if(!vm->error) js_set(vm,e.owner,e.name,value);
            }
        }
        if(p.kind==';') next(&p);
        else if(p.kind && p.token_line<=p.previous_line && !vm->error) error(&p,"Statement separator expected");
    }
    vm->depth--; return !vm->error;
}
int js_eval(js_vm *vm,const char *source,uint32_t count) {
    if(!vm) return 0;
    /* Select the complete ES5 interpreter before execution; never replay a
       partially executed script after an error. Modern syntax stays local. */
    vm->error=0;vm->error_line=0;
    if(js_full_active(vm))return js_full_eval_named(vm,source,count,vm->source_name?vm->source_name:"<script>");
    parser scan={.vm=vm,.p=source,.end=source+count,.line=1};next(&scan);int extended=js_full_active(vm);
    while(scan.kind&&!vm->error){
        if(word(&scan,"class")||word(&scan,"async")||word(&scan,"await")||word(&scan,"export")||word(&scan,"import")||word(&scan,"typeof")||word(&scan,"this")||word(&scan,"for")||word(&scan,"while")||word(&scan,"do")||word(&scan,"try")||word(&scan,"throw")||word(&scan,"delete")||word(&scan,"instanceof")||scan.kind=='?'||scan.kind=='~'||scan.kind=='*'||scan.kind=='/'||scan.kind=='%'||scan.kind=='^'||scan.kind=='<'||scan.kind=='>')extended=1;
        if((scan.kind==NUMBER&&scan.p<scan.end&&(*scan.p=='.'||*scan.p=='x'||*scan.p=='X'||*scan.p=='e'||*scan.p=='E'))||
           ((scan.kind=='='||scan.kind=='!')&&scan.p<scan.end&&*scan.p=='='))extended=1;
        if(word(&scan,"push")||word(&scan,"pop")||word(&scan,"slice")||word(&scan,"splice")||word(&scan,"join")||word(&scan,"charAt")||word(&scan,"charCodeAt")||word(&scan,"substr")||word(&scan,"substring")||word(&scan,"split")||word(&scan,"parseInt")||word(&scan,"Object")||word(&scan,"Math")||word(&scan,"arguments")||word(&scan,"call")||word(&scan,"apply"))extended=1;
        if(extended)break;next(&scan);
    }
    if(vm->error)return 0;
    if(extended){int full=js_full_eval_named(vm,source,count,vm->source_name?vm->source_name:"<script>");if(full>=0)return full;}
    uint32_t scope=vm->scope; vm->scope=0;
    js_value result;int returned=0;int ok=execute(vm,source,count,0,&result,&returned,1);vm->scope=scope;return ok;
}
int js_call(js_vm *vm,js_value function) {
    js_value result;return js_call_args(vm,function,0,0,&result);
}
int js_call_args(js_vm *vm,js_value function,const js_value *args,uint32_t count,js_value *result) {
    if(!vm) return 0;
    if(!result||(count>8&&!((uint32_t)function.number&JS_FULL_FUNCTION))||(count&&!args)) {vm->error="Invalid function call arguments";return 0;}
    if(!vm->depth) {vm->error=0;vm->error_line=0;}
    return invoke(vm,function,args,count,result);
}
