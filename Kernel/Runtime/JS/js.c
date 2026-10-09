#include "js.h"
typedef struct {
    js_vm *vm; const char *start, *p, *end, *token;
    uint32_t length, line, token_line, previous_line, steps; int kind;
} parser;
enum { IDENTIFIER=256, STRING, NUMBER, TEMPLATE, STRICT_EQUAL, STRICT_NOT_EQUAL };
static int same(const char *a, const char *b) {
    while (*a && *a==*b) { ++a; ++b; } return *a==*b;
}
static uint32_t length(const char *s) { uint32_t n=0; while(s[n]) ++n; return n; }
static js_value undefined(void) { return (js_value){0,0,0,0}; }
static js_value number_string(js_vm *vm,int32_t number) {
    char digits[12];uint32_t position=11,n=number<0?0u-(uint32_t)number:(uint32_t)number;
    digits[position]=0;do{digits[--position]=(char)('0'+n%10);n/=10;}while(n);
    if(number<0)digits[--position]='-';return js_string(vm,digits+position,11-position);
}
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
    if(vm->objects>=128) { vm->error="JavaScript object limit reached"; return undefined(); }
    return (js_value){JS_OBJECT,(int32_t)++vm->objects,0,0};
}
js_value js_native(int id) { return (js_value){JS_NATIVE,id,0,0}; }
js_value js_get(js_vm *vm,uint32_t object,const char *name) {
    for(uint32_t i=0;i<vm->property_count;i++)
        if(vm->properties[i].object==object && same(vm->properties[i].name,name)) return vm->properties[i].value;
    return undefined();
}
int js_set(js_vm *vm,uint32_t object,const char *name,js_value value) {
    for(uint32_t i=0;i<vm->property_count;i++) if(vm->properties[i].object==object && same(vm->properties[i].name,name)) {
        if(vm->properties[i].readonly) {vm->error="Assignment to constant binding";return 0;}
        vm->properties[i].value=value; return 1;
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
    vm->properties[vm->property_count++]=(js_property){object,key.text,value,0}; return 1;
}
void js_init(js_vm *vm,js_host_call host,void *context) {
    vm->property_count=vm->objects=vm->used=vm->depth=0; vm->host=host; vm->context=context;
    vm->error=0; vm->error_line=0;
    vm->function_count=vm->scope=vm->environment_count=0;
    vm->functions=0;vm->arena_end=sizeof(vm->strings);
    vm->properties=0;vm->property_capacity=0;
}
static int alpha(char c) { return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'||c=='$'; }
static int digit(char c) { return c>='0'&&c<='9'; }
static void next(parser *p) {
    p->previous_line=p->token_line;
    if(++p->steps>20000) { error(p,"JavaScript instruction budget exceeded"); p->kind=0; return; }
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
            fn.defaults[index]=p->token;
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
        fn.expression_body=1;fn.source=p->token;fn.line=p->token_line;uint32_t nesting=0;
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
    if(function.type!=JS_FUNCTION||function.number<0||(uint32_t)function.number>=vm->function_count) {vm->error="Function expected";return 0;}
    if(vm->depth>=16||vm->environment_count==128) {vm->error="JavaScript call/environment limit reached";return 0;}
    js_function *fn=vm->functions;
    while(fn&&fn->index!=(uint32_t)function.number) fn=fn->next;
    if(!fn) {vm->error="Function expected";return 0;}
    *result=undefined();uint32_t first_object=vm->objects+1;
    uint32_t saved_scope=vm->scope, environment=vm->environment_count++;
    vm->parents[environment]=fn->scope; vm->scope=0x10000u+environment+1;
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
    int captured=0;for(js_function *f=vm->functions;f;f=f->next)if(f->scope==0x10000u+environment+1){captured=1;break;}
    if(!captured&&vm->environment_count==environment+1) {
        uint32_t scope=0x10000u+environment+1;
        int escaped=result->type==JS_OBJECT&&(uint32_t)result->number>=first_object;
        for(uint32_t i=0;i<vm->property_count&&!escaped;i++) {
            js_property *p=&vm->properties[i];js_value value=p->value;
            if(p->object==scope||(p->object>=first_object&&p->object<=vm->objects))continue;
            if((value.type==JS_OBJECT&&(uint32_t)value.number>=first_object)||
               (value.type==JS_NATIVE&&value.number>=0&&!value.text&&value.length>=first_object))escaped=1;
        }
        uint32_t kept=0;
        for(uint32_t i=0;i<vm->property_count;i++) {
            js_property property=vm->properties[i];
            if(property.object==scope||(!escaped&&property.object>=first_object&&property.object<=vm->objects))continue;
            vm->properties[kept++]=property;
        }
        vm->property_count=kept;if(!escaped)vm->objects=first_object-1;vm->environment_count--;
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
        char buffer[4096]; uint32_t used=0;
        for(uint32_t i=0;i<p->length&&!p->vm->error;i++) {
            char c=p->token[i];
            if(c=='$'&&i+1<p->length&&p->token[i+1]=='{') {
                parser sub={.vm=p->vm,.p=p->token+i+2,.end=p->token+p->length,.line=p->token_line};
                next(&sub);js_value value=expr(&sub,depth+1).value;
                if(sub.kind!='}') {error(p,"Template interpolation closing brace expected");break;}
                i=(uint32_t)(sub.token-p->token);
                char digits[12]; const char *text=value.text; uint32_t size=value.length;
                if(value.type==JS_NUMBER) {
                    uint32_t n=value.number<0?0u-(uint32_t)value.number:(uint32_t)value.number, pos=11;
                    digits[pos]=0;do {digits[--pos]=(char)('0'+n%10);n/=10;} while(n);
                    if(value.number<0) digits[--pos]='-';text=digits+pos;size=11-pos;
                } else if(value.type==JS_BOOLEAN) {text=value.number?"true":"false";size=value.number?4:5;}
                else if(value.type==JS_UNDEFINED) {text="undefined";size=9;}
                else if(value.type==JS_NULL) {text="null";size=4;}
                else if(value.type!=JS_STRING) {error(p,"Unsupported template interpolation value");break;}
                if(size>=sizeof(buffer)-used) {error(p,"Template literal limit reached");break;}
                for(uint32_t j=0;j<size;j++) buffer[used++]=text[j];
                continue;
            }
            if(c=='\\'&&i+1<p->length) {
                c=p->token[++i];if(c=='n') c='\n';else if(c=='r') c='\r';else if(c=='t') c='\t';
            }
            if(used==sizeof(buffer)-1) {error(p,"Template literal limit reached");break;}
            buffer[used++]=c;
        }
        if(!p->vm->error) e.value=js_string(p->vm,buffer,used);next(p);
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
        uint32_t n=0;
        for(uint32_t i=0;i<p->length;i++) { if(n>214748364u || (n==214748364u && p->token[i]>'7')) { error(p,"Integer literal out of range"); return e; } n=n*10+(p->token[i]-'0'); }
        e.value=(js_value){JS_NUMBER,(int32_t)n,0,0}; next(p);
    } else if(word(p,"function")) {
        e.value=function(p,0,0);
    } else if(p->kind=='(') { next(p); e=expr(p,depth+1); expect(p,')'); }
    else if(p->kind==IDENTIFIER) {
        if(word(p,"true")||word(p,"false")) { e.value=(js_value){JS_BOOLEAN,word(p,"true"),0,0}; next(p); }
        else if(word(p,"null")||word(p,"undefined")) {if(word(p,"null")) e.value.type=JS_NULL;next(p);}
        else { identifier(p,e.name); int found=0; e.owner=binding(p->vm,e.name,&found);
            e.reference=1; e.value=js_get(p->vm,e.owner,e.name);
            if(e.value.type==JS_UNINITIALIZED) error(p,"Parameter accessed before initialization");
            if(!found) error(p,"Unknown JavaScript identifier"); }
    } else { error(p,"Unsupported JavaScript expression"); return e; }
    while(!p->vm->error && (p->kind=='.'||p->kind=='('||p->kind=='[')) {
        if(p->kind=='[') {
            if(e.value.type!=JS_OBJECT){error(p,"Indexing requires an object");break;}
            uint32_t object=(uint32_t)e.value.number;next(p);js_value key=expr(p,depth+1).value;
            if(!expect(p,']'))break;
            if(key.type==JS_NUMBER)key=number_string(p->vm,key.number);
            if(key.type!=JS_STRING||key.length>63){error(p,"Unsupported property key");break;}
            e.owner=object;for(uint32_t i=0;i<=key.length;i++)e.name[i]=key.text[i];
            e.reference=1;e.value=js_get(p->vm,object,key.text);
        }
        else if(p->kind=='.') {
            if(e.value.type==JS_NUMBER||e.value.type==JS_STRING) {
                js_value receiver=e.value;next(p);if(!identifier(p,e.name))break;
                if(receiver.type==JS_NUMBER&&same(e.name,"toString"))e.value=(js_value){JS_NATIVE,-1,0,(uint32_t)receiver.number};
                else if(receiver.type==JS_STRING&&same(e.name,"padStart"))e.value=(js_value){JS_NATIVE,-2,receiver.text,receiver.length};
                else {error(p,"Unsupported primitive property");break;}
                e.reference=0;continue;
            }
            if(e.value.type!=JS_OBJECT) { error(p,"Property access requires an object"); break; }
            e.owner=(uint32_t)e.value.number; next(p); if(!identifier(p,e.name)) break;
            e.reference=1; e.value=js_get(p->vm,e.owner,e.name);
        } else {
            next(p); js_value args[8]; uint32_t count=0;
            while(p->kind!=')'&&!p->vm->error) {
                if(count==8) { error(p,"Call argument limit reached"); break; }
                args[count++]=expr(p,depth+1).value;
                if(p->kind!=',') break; next(p);
            }
            if(!expect(p,')')) break;
            js_value result=undefined();
            if(e.value.type==JS_NATIVE&&e.value.number==-1) {
                if(count)error(p,"Number.toString radix is not supported yet");
                else result=number_string(p->vm,(int32_t)e.value.length);
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
            } else if(e.value.type==JS_NATIVE) {
                if((!p->vm->host || !p->vm->host(p->vm,e.value.number,args,count,&result)) && !p->vm->error) error(p,"Browser API call failed");
            } else if(e.value.type==JS_FUNCTION) invoke(p->vm,e.value,args,count,&result);
            else error(p,"Value is not a supported callable");
            e.value=result; e.reference=0;
        }
    }
    return e;
}
static expression addition(parser *p,uint32_t depth) {
    expression e=primary(p,depth);
    while(p->kind=='+'&&!p->vm->error) {
        next(p); expression right=primary(p,depth+1);
        if(e.value.type==JS_NUMBER && right.value.type==JS_NUMBER) {
            if(right.value.number>0 && e.value.number>2147483647-right.value.number) { error(p,"Integer addition out of range"); break; }
            e.value.number=(int32_t)((uint32_t)e.value.number+(uint32_t)right.value.number);
        } else if(e.value.type==JS_STRING && right.value.type==JS_STRING) {
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
static expression expr(parser *p,uint32_t depth) {
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
    if(vm->depth>=16) { vm->error="JavaScript call depth exceeded"; return 0; }
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
    uint32_t scope=vm->scope; vm->scope=0;
    js_value result;int returned=0;int ok=execute(vm,source,count,0,&result,&returned,1);vm->scope=scope;return ok;
}
int js_call(js_vm *vm,js_value function) {
    js_value result;return js_call_args(vm,function,0,0,&result);
}
int js_call_args(js_vm *vm,js_value function,const js_value *args,uint32_t count,js_value *result) {
    if(!vm) return 0;
    if(!result||count>8||(count&&!args)) {vm->error="Invalid function call arguments";return 0;}
    if(!vm->depth) {vm->error=0;vm->error_line=0;}
    return invoke(vm,function,args,count,result);
}
