#include "js.h"
typedef struct {js_vm *vm;const char *p,*end;} json_parser;
static void skip(json_parser *p){while(p->p<p->end&&(*p->p==' '||*p->p=='\t'||*p->p=='\r'||*p->p=='\n'))p->p++;}
static int fail(json_parser *p){if(!p->vm->error)p->vm->error="Invalid JSON response";return 0;}
static int append(json_parser *p,uint8_t byte){js_vm *vm=p->vm;if(vm->used+1>=vm->arena_end){vm->error="JavaScript memory exhausted";return 0;}vm->strings[vm->used++]=(char)byte;return 1;}
static int hex(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
static int unicode(json_parser *p,uint32_t *value){*value=0;for(int i=0;i<4;i++){if(p->p==p->end||hex(*p->p)<0)return fail(p);*value=*value*16+(uint32_t)hex(*p->p++);}return 1;}
static int string(json_parser *p,js_value *out){
    if(p->p==p->end||*p->p++!='"')return fail(p);uint32_t start=p->vm->used;
    while(p->p<p->end){uint8_t c=(uint8_t)*p->p++;if(c=='"'){uint32_t size=p->vm->used-start;if(!append(p,0))return 0;*out=(js_value){JS_STRING,0,p->vm->strings+start,size};return 1;}
        if(c<32)return fail(p);if(c=='\\'){if(p->p==p->end)return fail(p);c=(uint8_t)*p->p++;
            if(c=='u'){uint32_t cp;if(!unicode(p,&cp))return 0;
                if(cp>=0xd800&&cp<=0xdbff){if(p->end-p->p<2||p->p[0]!='\\'||p->p[1]!='u')return fail(p);p->p+=2;uint32_t low;if(!unicode(p,&low)||low<0xdc00||low>0xdfff)return fail(p);cp=0x10000+((cp-0xd800)<<10)+low-0xdc00;}
                else if(cp>=0xdc00&&cp<=0xdfff)return fail(p);
                if(cp<128){if(!append(p,(uint8_t)cp))return 0;}
                else if(cp<2048){if(!append(p,(uint8_t)(0xc0|(cp>>6)))||!append(p,(uint8_t)(0x80|(cp&63))))return 0;}
                else if(cp<65536){if(!append(p,(uint8_t)(0xe0|(cp>>12)))||!append(p,(uint8_t)(0x80|((cp>>6)&63)))||!append(p,(uint8_t)(0x80|(cp&63))))return 0;}
                else if(!append(p,(uint8_t)(0xf0|(cp>>18)))||!append(p,(uint8_t)(0x80|((cp>>12)&63)))||!append(p,(uint8_t)(0x80|((cp>>6)&63)))||!append(p,(uint8_t)(0x80|(cp&63))))return 0;
                continue;
            }else if(c=='n')c='\n';else if(c=='r')c='\r';else if(c=='t')c='\t';else if(c=='b')c='\b';else if(c=='f')c='\f';else if(c!='"'&&c!='\\'&&c!='/')return fail(p);}
        if(!append(p,c))return 0;
    }return fail(p);
}
static int literal(json_parser *p,const char *text){const char *q=p->p;while(*text){if(q==p->end||*q++!=*text++)return 0;}p->p=q;return 1;}
static int value(json_parser *p,js_value *out,uint32_t depth){
    skip(p);if(depth>32||p->p==p->end)return fail(p);char c=*p->p;
    if(c=='"')return string(p,out);
    if(c=='{'||c=='['){int array=c=='[';p->p++;*out=js_object(p->vm);if(p->vm->error)return 0;uint32_t count=0;skip(p);char close=array?']':'}';
        if(p->p<p->end&&*p->p==close){p->p++;if(array)js_set(p->vm,out->number,"length",(js_value){JS_NUMBER,0,0,0});return !p->vm->error;}
        for(;;){js_value key,item;char digits[11];
            if(array){uint32_t n=count++,used=0;char reverse[10];do{reverse[used++]=(char)('0'+n%10);n/=10;}while(n);for(uint32_t i=0;i<used;i++)digits[i]=reverse[used-i-1];digits[used]=0;key=js_string(p->vm,digits,used);}
            else{skip(p);if(!string(p,&key))return 0;skip(p);if(p->p==p->end||*p->p++!=':')return fail(p);}
            if(p->vm->error||!value(p,&item,depth+1)||!js_set(p->vm,out->number,key.text,item))return 0;
            skip(p);if(p->p==p->end)return fail(p);if(*p->p==close){p->p++;break;}if(*p->p++!=',')return fail(p);}
        if(array)js_set(p->vm,out->number,"length",(js_value){JS_NUMBER,(int32_t)count,0,0});return !p->vm->error;
    }
    if(literal(p,"true")){*out=(js_value){JS_BOOLEAN,1,0,0};return 1;}
    if(literal(p,"false")){*out=(js_value){JS_BOOLEAN,0,0,0};return 1;}
    if(literal(p,"null")){*out=(js_value){JS_NULL,0,0,0};return 1;}
    int negative=0;if(c=='-'){negative=1;p->p++;}if(p->p==p->end||*p->p<'0'||*p->p>'9')return fail(p);
    uint32_t number=0,limit=negative?2147483648u:2147483647u;int leading_zero=*p->p=='0';uint32_t digits=0;
    while(p->p<p->end&&*p->p>='0'&&*p->p<='9'){uint32_t digit=(uint32_t)(*p->p++-'0');if(number>(limit-digit)/10)return fail(p);number=number*10+digit;digits++;}
    if(leading_zero&&digits>1)return fail(p);
    /* This VM stores integer numbers; never silently truncate JSON fractions. */
    if(p->p<p->end&&(*p->p=='.'||*p->p=='e'||*p->p=='E')){p->vm->error="JSON fractional numbers are not supported by the integer runtime";return 0;}
    *out=(js_value){JS_NUMBER,(int32_t)(negative?0u-number:number),0,0};return 1;
}
int js_json_parse(js_vm *vm,const char *text,uint32_t length,js_value *result){
    if(!vm||!text||!result)return 0;json_parser parser={vm,text,text+length};if(!value(&parser,result,0))return 0;skip(&parser);return parser.p==parser.end?1:fail(&parser);
}
