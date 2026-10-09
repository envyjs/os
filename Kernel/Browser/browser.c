#include "browser.h"
#include "../Runtime/JS/js.h"
#include "../Arch/x86/io.h"
#include "../Drivers/Video/framebuffer.h"
#include "html.h"
#include "css.h"
#include "font.h"
#include "images.h"
#define SOURCE_MEMORY (2u*1024u*1024u)
typedef struct {
    js_vm vm; cdfs *fs; void (*write)(const char *); void (*fallback_write)(const char *);
    char sources[SOURCE_MEMORY]; uint32_t source_used;
    const char *load_error;
    const char *document_html;
    uint32_t body, head,document,window,event; uint32_t script_failures;
    struct {uint32_t target;js_value type,callback;int active,once;} listeners[64];
    struct {js_value callback;uint32_t interval,due;int active,repeat;} timers[32];
    uint32_t seconds,pit_cycles;uint16_t last_pit;uint8_t last_second;
} browser;
static browser page;
static char errors[4096];
static uint32_t error_used;
static int recording_error, console_open;
static uint8_t control_keys, extended_key, end_down, pause_bytes;
static uint8_t pointer_buttons;
/* Save the bottom 96 rows, including pitch padding, before drawing the panel. */
static uint8_t panel_saved[16384u*96u];
static void remember(char c) {
    if(error_used==sizeof(errors)-1) {
        uint32_t remove=0; while(remove<error_used&&errors[remove++]!='\n') {}
        for(uint32_t i=remove;i<error_used;i++) errors[i-remove]=errors[i];
        error_used-=remove;
    }
    errors[error_used++]=c; errors[error_used]=0;
}
static void console_toggle(void) {
    const framebuffer *fb=framebuffer_get();
    framebuffer_cursor_restore();
    console_open=!console_open;
    if(!fb) return;
    uint32_t rows=fb->height<96?fb->height:96, offset=(fb->height-rows)*fb->pitch;
    for(uint32_t i=0;i<rows*fb->pitch;i++) {
        if(console_open) panel_saved[i]=fb->pixels[offset+i];
        else fb->pixels[offset+i]=panel_saved[i];
    }
    if(!console_open){framebuffer_cursor_redraw();return;}
    int32_t top=(int32_t)(fb->height-rows);
    framebuffer_rect(0,top,fb->width,rows,0x101820);
    framebuffer_text(8,top+4,"JavaScript errors - Ctrl+End to close",0xe8edf2);
    uint32_t columns=fb->width>16?(fb->width-16)/8:1;
    uint32_t lines=0, column=0;
    for(uint32_t i=0;i<error_used;i++) {
        if(errors[i]=='\n') {lines++;column=0;}
        else if(++column>=columns) {lines++;column=0;}
    }
    uint32_t skip=lines>3?lines-3:0, begin=0; column=0;
    while(begin<error_used&&skip) {
        char c=errors[begin++];
        if(c=='\n'||++column>=columns) {skip--;column=0;}
    }
    int32_t x=8,y=top+26;
    if(!error_used) framebuffer_text(x,y,"No JavaScript errors.",0xe8edf2);
    for(uint32_t i=begin;i<error_used&&y+16<=(int32_t)fb->height;i++) {
        char c=errors[i];
        if(c=='\n'||x+8>(int32_t)fb->width-8) {x=8;y+=18;if(c=='\n') continue;}
        char glyph[2]={c,0}; framebuffer_text(x,y,glyph,0xff9090);x+=8;
    }
    framebuffer_cursor_redraw();
}
int browser_key(uint8_t code) {
    if(pause_bytes) {pause_bytes--;return 0;}
    if(code==0xe1) {pause_bytes=5;extended_key=0;return 0;}
    if(code==0xe0) {extended_key=1;return 0;}
    uint8_t extended=extended_key;extended_key=0;
    if((code&0x7f)==0x1d) {
        uint8_t bit=extended?2:1;
        if(code&0x80) control_keys&=(uint8_t)~bit; else control_keys|=bit;
    }
    if(extended&&(code&0x7f)==0x4f) {
        if(code&0x80) end_down=0;
        else if(!end_down) {end_down=1;if(control_keys) {console_toggle();return 1;}}
    }
    return 0;
}
static void trace(const char *text) {
    if(recording_error) for(const char *p=text;*p;p++) remember(*p);
    if(!framebuffer_get()) {page.fallback_write(text);return;}
    while(*text) out8(0xe9,(uint8_t)*text++);
}
enum { CREATE=1, APPEND, INSERT, LOG, CONSOLE_ERROR, FETCH, GET_BY_ID,
       DATE, SET_INTERVAL, CLEAR_TIMER, SET_TIMEOUT, QUERY_SELECTOR, MEDIA_PLAY, MEDIA_PAUSE, PREVENT_DEFAULT,
       THEN_BASE=1000, CATCH_BASE=2000, APPEND_BASE=3000, INSERT_BASE=4000, DATE_BASE=6000, ADD_EVENT_BASE=8000, REMOVE_EVENT_BASE=9000 };
static uint8_t rtc_read(uint8_t reg) {out8(0x70,reg);return in8(0x71);}
static int clock_snapshot(uint8_t *values) {
    const uint8_t registers[]={4,2,7,8,6};
    for(uint32_t attempt=0;attempt<8;attempt++) {
        uint32_t budget=100000;while((rtc_read(10)&128)&&--budget){}
        if(!budget)return 0;uint8_t seconds=rtc_read(0),mode=rtc_read(11);
        for(uint32_t i=0;i<5;i++)values[i]=rtc_read(registers[i]);
        if((rtc_read(10)&128)||seconds!=rtc_read(0))continue;
        uint8_t pm=values[0]&128;values[0]&=127;
        if(!(mode&4))for(uint32_t i=0;i<5;i++)values[i]=(uint8_t)((values[i]>>4)*10+(values[i]&15));
        if(!(mode&2))values[0]=(uint8_t)(values[0]%12+(pm?12:0));
        values[3]--;values[4]=(uint8_t)((values[4]+6)%7);return 1;
    }
    return 0;
}
static uint32_t count(const char *s) { uint32_t n=0; while(s[n]) ++n; return n; }
static int equal(const char *a,const char *b) { while(*a&&*a==*b) {a++;b++;} return *a==*b; }
static char lower(char c) { return c>='A'&&c<='Z'?(char)(c-'A'+'a'):c; }
static const char *find(const char *s,const char *needle) {
    for(;*s;s++) { uint32_t i=0; while(needle[i]&&s[i]&&lower(s[i])==lower(needle[i])) i++; if(!needle[i]) return s; } return 0;
}
static void number(browser *b,uint32_t n) {
    char digits[11]; uint32_t used=0;
    do { digits[used++]=(char)('0'+n%10); n/=10; } while(n);
    char result[11]; for(uint32_t i=0;i<used;i++) result[i]=digits[used-i-1]; result[used]=0; b->write(result);
}
static char *source_buffer(browser *b,uint32_t size) {
    /* One shared arena: no separate per-file or script-count limit. */
    if(size>=sizeof(b->sources)-b->source_used) {
        b->load_error="Not enough source memory to load script";return 0;
    }
    char *buffer=b->sources+b->source_used;b->source_used+=size+1;return buffer;
}
static int load(browser *b,const char *path,char **source,uint32_t *length) {
    cdfs_node node;
    b->load_error=0;
    if(cdfs_lookup(b->fs,path,&node)) {b->load_error="Local script not found";return 0;}
    if(node.flags&2) {b->load_error="Script path is a directory";return 0;}
    uint32_t saved=b->source_used,got;char *buffer=source_buffer(b,node.size);
    if(!buffer) return 0;
    if(cdfs_read(b->fs,&node,0,buffer,node.size,&got)||got!=node.size) {
        b->source_used=saved;b->load_error="Cannot read local script";return 0;
    }
    buffer[got]=0; *source=buffer; *length=got; return 1;
}
static js_value element(js_vm *vm,const char *tag) {
    js_value node=js_object(vm), style=js_object(vm);
    js_set(vm,node.number,"tagName",js_string(vm,tag,count(tag)));
    js_set(vm,node.number,"style",style);
    js_value append=js_native(APPEND_BASE+node.number),insert=js_native(INSERT_BASE+node.number);append.length=insert.length=(uint32_t)node.number;
    js_set(vm,node.number,"appendChild",append);
    js_set(vm,node.number,"insertBefore",insert);
    js_value listen=js_native(ADD_EVENT_BASE+node.number),remove=js_native(REMOVE_EVENT_BASE+node.number);listen.length=remove.length=(uint32_t)node.number;
    js_set(vm,node.number,"addEventListener",listen);js_set(vm,node.number,"removeEventListener",remove);
    if(equal(tag,"audio")||equal(tag,"video")){js_set(vm,node.number,"play",js_native(MEDIA_PLAY));js_set(vm,node.number,"pause",js_native(MEDIA_PAUSE));}
    return node;
}
static int connected(browser *b,uint32_t node) {
    for(uint32_t depth=0;node&&depth<=b->vm.objects;depth++) {
        if(node==b->body||node==b->head) return 1;
        js_value parent=js_get(&b->vm,node,"parentNode");
        if(parent.type!=JS_OBJECT) return 0;node=(uint32_t)parent.number;
    }
    return 0;
}
static void link_child(js_vm *vm,uint32_t parent,js_value child) {
    char key[20]="__child",digits[10];uint32_t n=(uint32_t)child.number,used=0;
    do{digits[used++]=(char)('0'+n%10);n/=10;}while(n);
    for(uint32_t i=0;i<used;i++)key[7+i]=digits[used-i-1];key[7+used]=0;
    js_set(vm,parent,key,child);
}
static js_value get_by_id(browser *b,const char *id) {
    js_vm *vm=&b->vm;
    if(!*id) return (js_value){JS_NULL,0,0,0};
    for(uint32_t i=0;i<vm->property_count;i++) {
        js_property *property=&vm->properties[i];
        if(equal(property->name,"id")&&property->value.type==JS_STRING&&
           equal(property->value.text,id)&&connected(b,property->object))
            return (js_value){JS_OBJECT,(int32_t)property->object,0,0};
    }
    char tag[64];uint32_t parent=0;
    if(b->document_html&&html_find_element(b->document_html,id,tag,sizeof(tag))) parent=b->body;
    else for(uint32_t i=0;i<vm->property_count;i++) {
        js_property *property=&vm->properties[i];
        if(equal(property->name,"innerHTML")&&property->value.type==JS_STRING&&connected(b,property->object)&&
           html_find_element(property->value.text,id,tag,sizeof(tag))) {parent=property->object;break;}
    }
    if(!parent) return (js_value){JS_NULL,0,0,0};
    js_value node=element(vm,tag);
    js_set(vm,node.number,"id",js_string(vm,id,count(id)));
    js_set(vm,node.number,"parentNode",(js_value){JS_OBJECT,(int32_t)parent,0,0});
    link_child(vm,parent,node);
    return node;
}
static const char *live_text(void *context,const char *id) {
    browser *b=context;js_vm *vm=&b->vm;
    for(uint32_t i=0;i<vm->property_count;i++) {
        js_property *p=&vm->properties[i];
        if(equal(p->name,"id")&&p->value.type==JS_STRING&&equal(p->value.text,id)&&connected(b,p->object)) {
            js_value text=js_get(vm,p->object,"innerText");
            if(text.type!=JS_STRING)text=js_get(vm,p->object,"textContent");
            return text.type==JS_STRING?text.text:0;
        }
    }
    return 0;
}
static void diagnostic(browser *b,const char *path,const char *error,uint32_t line) {
    recording_error=1;
    b->write("Script stopped: "); b->write(path); b->write("\n  line "); number(b,line);
    b->write(": "); b->write(error?error:"unsupported browser API"); b->write("\n");
    recording_error=0;
    html_view_status(error?error:"Unsupported browser API");
}
static int local_path(const char *src,char *path) {
    const char *prefix="/Envy/"; uint32_t used=0;
    while(src[0]=='.'&&src[1]=='/') src+=2;
    if(*src=='/') {
        for(uint32_t i=0;i<6;i++) if(!src[i]||lower(src[i])!=lower(prefix[i])) return 0;
    } else while(prefix[used]) { path[used]=prefix[used]; used++; }
    for(;*src;src++) {
        if(*src==':'||*src=='\\'||used>=255) return 0;
        if(src[0]=='.'&&src[1]=='.'&&(src[2]=='/'||!src[2])) return 0;
        path[used++]=*src;
    }
    path[used]=0; return 1;
}
static js_value promise(js_vm *vm,js_value value,int rejected) {
    js_value object=js_object(vm);
    js_set(vm,object.number,"value",value);
    js_set(vm,object.number,"rejected",(js_value){JS_NUMBER,rejected,0,0});
    js_set(vm,object.number,"then",js_native(THEN_BASE+object.number));
    js_set(vm,object.number,"catch",js_native(CATCH_BASE+object.number));
    return object;
}
static int api(js_vm *vm,int id,const js_value *args,uint32_t argc,js_value *result) {
    browser *b=vm->context;
    if((id>ADD_EVENT_BASE&&id<=ADD_EVENT_BASE+128)||(id>REMOVE_EVENT_BASE&&id<=REMOVE_EVENT_BASE+128)){
        int remove=id>REMOVE_EVENT_BASE;uint32_t target=(uint32_t)(id-(remove?REMOVE_EVENT_BASE:ADD_EVENT_BASE));
        if(argc<2||args[0].type!=JS_STRING||args[1].type!=JS_FUNCTION){vm->error="Event listener requires an event name and function";return 0;}
        uint32_t free_slot=64;
        for(uint32_t i=0;i<64;i++){if(!b->listeners[i].active){if(free_slot==64)free_slot=i;continue;}
            if(b->listeners[i].target==target&&equal(b->listeners[i].type.text,args[0].text)&&b->listeners[i].callback.number==args[1].number){if(remove)b->listeners[i].active=0;return 1;}}
        if(remove)return 1;if(free_slot==64){vm->error="Event listener storage exhausted";return 0;}
        b->listeners[free_slot].target=target;b->listeners[free_slot].type=args[0];b->listeners[free_slot].callback=args[1];b->listeners[free_slot].active=1;b->listeners[free_slot].once=0;
        if(argc>2&&args[2].type==JS_OBJECT)b->listeners[free_slot].once=js_get(vm,args[2].number,"once").number!=0;return 1;
    }
    if(id==PREVENT_DEFAULT){if(b->event)js_set(vm,b->event,"defaultPrevented",(js_value){JS_BOOLEAN,1,0,0});return 1;}
    if(id==MEDIA_PAUSE)return 1;
    if(id==MEDIA_PLAY){const char *message="Audio playback is not available in the native kernel";*result=promise(vm,js_string(vm,message,count(message)),1);return !vm->error;}
    if(id==QUERY_SELECTOR){
        if(argc!=1||args[0].type!=JS_STRING)return 0;const char *selector=args[0].text;
        if(*selector=='#'){*result=get_by_id(b,selector+1);return !vm->error;}
        *result=(js_value){JS_NULL,0,0,0};
        for(uint32_t i=0;i<vm->property_count;i++){js_property *p=&vm->properties[i];if(equal(p->name,"tagName")&&p->value.type==JS_STRING&&equal(p->value.text,selector)&&connected(b,p->object)){*result=(js_value){JS_OBJECT,(int32_t)p->object,0,0};return 1;}}
        uint32_t parent=0;if(b->document_html&&html_has_tag(b->document_html,selector))parent=b->body;
        else for(uint32_t i=0;i<vm->property_count;i++){js_property *p=&vm->properties[i];if(equal(p->name,"innerHTML")&&p->value.type==JS_STRING&&connected(b,p->object)&&html_has_tag(p->value.text,selector)){parent=p->object;break;}}
        if(parent){*result=element(vm,selector);js_set(vm,result->number,"parentNode",(js_value){JS_OBJECT,(int32_t)parent,0,0});link_child(vm,parent,*result);}return !vm->error;
    }
    if(id==SET_INTERVAL||id==SET_TIMEOUT) {
        if(argc<2||args[0].type!=JS_FUNCTION||args[1].type!=JS_NUMBER||args[1].number<0)return 0;
        uint32_t i=0;while(i<32&&b->timers[i].active)i++;
        if(i==32){vm->error="Timer storage exhausted";return 0;}
        uint32_t interval=((uint32_t)args[1].number+999)/1000;if(!interval)interval=1;
        b->timers[i].callback=args[0];b->timers[i].interval=interval;b->timers[i].due=b->seconds+interval;b->timers[i].active=1;b->timers[i].repeat=id==SET_INTERVAL;
        *result=(js_value){JS_NUMBER,(int32_t)i+1,0,0};return 1;
    }
    if(id==CLEAR_TIMER){if(argc&&args[0].type==JS_NUMBER&&args[0].number>0&&args[0].number<=32)b->timers[args[0].number-1].active=0;return 1;}
    const char *date_methods[]={"getHours","getMinutes","getDate","getMonth","getDay"};
    if(id==DATE) {
        if(argc){vm->error="Date arguments are not supported yet";return 0;}
        uint8_t values[5];if(!clock_snapshot(values)){vm->error="Cannot read PC real-time clock";return 0;}
        *result=js_object(vm);
        for(uint32_t i=0;i<5;i++) {
            js_value method=js_native(DATE_BASE+result->number*8+(int)i);method.length=(uint32_t)result->number;
            js_set(vm,result->number,date_methods[i],method);
            char key[2]={(char)('0'+i),0};js_set(vm,result->number,key,(js_value){JS_NUMBER,values[i],0,0});
        }
        return !vm->error;
    }
    if(id>=DATE_BASE&&id<DATE_BASE+129*8) {
        uint32_t encoded=(uint32_t)(id-DATE_BASE);if(encoded%8>=5)return 0;
        char key[2]={(char)('0'+encoded%8),0};*result=js_get(vm,encoded/8,key);return 1;
    }
    if(id==GET_BY_ID) {
        if(argc!=1||args[0].type!=JS_STRING) return 0;
        *result=get_by_id(b,args[0].text);return !vm->error;
    }
    uint32_t parent=0;
    if(id>INSERT_BASE&&id<=INSERT_BASE+128) {parent=(uint32_t)(id-INSERT_BASE);id=INSERT;}
    else if(id>APPEND_BASE&&id<=APPEND_BASE+128) {parent=(uint32_t)(id-APPEND_BASE);id=APPEND;}
    /* Local CDFS requests complete synchronously; callbacks retain lexical scope. */
    if(id==FETCH) {
        if(!argc||args[0].type!=JS_STRING) return 0;
        char path[256];cdfs_node node;
        if(!local_path(args[0].text,path)) {
            const char *message="Only local CDFS fetch is supported";
            *result=promise(vm,js_string(vm,message,count(message)),1);return !vm->error;
        }
        if(argc>1) {
            if(args[1].type!=JS_OBJECT) return 0;
            js_value method=js_get(vm,args[1].number,"method");
            if(method.type!=JS_STRING||!equal(method.text,"HEAD")) {
                vm->error="Local fetch currently supports HEAD requests only";return 0;
            }
        } else {vm->error="Local fetch currently supports HEAD requests only";return 0;}
        int exists=!cdfs_lookup(b->fs,path,&node)&&!(node.flags&2);
        js_value response=js_object(vm);
        js_set(vm,response.number,"ok",(js_value){JS_NUMBER,exists,0,0});
        js_set(vm,response.number,"status",(js_value){JS_NUMBER,exists?200:404,0,0});
        *result=promise(vm,response,0);return !vm->error;
    }
    if((id>THEN_BASE&&id<=THEN_BASE+128)||(id>CATCH_BASE&&id<=CATCH_BASE+128)) {
        uint32_t object=(uint32_t)(id>=CATCH_BASE?id-CATCH_BASE:id-THEN_BASE);
        int rejected=js_get(vm,object,"rejected").number;
        js_value value=js_get(vm,object,"value");
        if(argc&&args[0].type==JS_FUNCTION&&rejected==(id>=CATCH_BASE)) {
            js_value returned;
            if(js_call_args(vm,args[0],&value,1,&returned)) {value=returned;rejected=0;}
            else {
                const char *message=vm->error?vm->error:"Promise callback failed";
                diagnostic(b,"(fetch callback)",message,vm->error_line);vm->error=0;
                value=js_string(vm,message,count(message));rejected=1;
            }
        }
        *result=promise(vm,value,rejected);return !vm->error;
    }
    if(id==CREATE) {
        if(argc!=1||args[0].type!=JS_STRING) return 0;
        *result=element(vm,args[0].text); return !vm->error;
    }
    if(id==LOG||id==CONSOLE_ERROR) {
        recording_error=id==CONSOLE_ERROR;
        for(uint32_t i=0;i<argc;i++) {
            if(i) b->write(" ");
            if(args[i].type==JS_STRING) b->write(args[i].text);
            else if(args[i].type==JS_NUMBER) {
                if(args[i].number<0) b->write("-");
                number(b,args[i].number<0?0u-(uint32_t)args[i].number:(uint32_t)args[i].number);
            } else b->write("undefined");
        }
        b->write("\n");recording_error=0;return 1;
    }
    if(id!=APPEND&&id!=INSERT) return 0;
    if(!argc||args[0].type!=JS_OBJECT) return 0;
    uint32_t node=(uint32_t)args[0].number;
    if(parent) {
        uint32_t ancestor=parent;
        for(uint32_t depth=0;ancestor&&depth<=vm->objects;depth++) {
            if(ancestor==node) {vm->error="Cannot append an ancestor to its child";return 0;}
            js_value owner=js_get(vm,ancestor,"parentNode");ancestor=owner.type==JS_OBJECT?(uint32_t)owner.number:0;
        }
        js_set(vm,node,"parentNode",(js_value){JS_OBJECT,(int32_t)parent,0,0});
        link_child(vm,parent,args[0]);
    }
    js_value tag=js_get(vm,node,"tagName"); *result=args[0];
    if(tag.type==JS_STRING&&equal(tag.text,"link")) {
        js_value rel=js_get(vm,node,"rel"),href=js_get(vm,node,"href");
        if(rel.type==JS_STRING&&equal(rel.text,"stylesheet")&&href.type==JS_STRING) {
            char path[256],*source;uint32_t size;
            if(!local_path(href.text,path)||!load(b,path,&source,&size)) {vm->error="Cannot load local stylesheet";return 0;}
            if(!css_add_file(source,size,path)) {vm->error="Cannot parse stylesheet or CSS rule storage exhausted";return 0;}
            b->write("Stylesheet loaded: ");b->write(path);b->write("\n");html_view_repaint();
        }
    } else if(tag.type==JS_STRING&&equal(tag.text,"style")) {
        js_value text=js_get(vm,node,"textContent");
        if(text.type==JS_STRING&&!css_add(text.text,text.length)) {vm->error="Cannot parse style element";return 0;}
        html_view_repaint();
    } else if(tag.type==JS_STRING&&equal(tag.text,"script")) {
        js_value src=js_get(vm,node,"src"); char path[256]={0}, *source; uint32_t length;
        b->load_error="Unsupported script source path";
        int success=src.type==JS_STRING&&local_path(src.text,path)&&load(b,path,&source,&length);
        if(success) {
            b->write("Loading "); b->write(path); b->write("\n");
            success=js_eval(vm,source,length);
            if(!success) diagnostic(b,path,vm->error,vm->error_line);
        } else diagnostic(b,path[0]?path:"(script source)",b->load_error,0);
        if(!success) b->script_failures++;
        /* Script exceptions dispatch the page's onerror handler, not fake success. */
        js_value handler=js_get(vm,node,success?"onload":"onerror"); vm->error=0;
        if(handler.type==JS_FUNCTION && !js_call(vm,handler)) return 0;
    } else {
        js_value text=js_get(vm,node,"textContent");
        js_value markup=js_get(vm,node,"innerHTML");
        if(connected(b,node)&&tag.type==JS_STRING) {
            js_value style=js_get(vm,node,"style"), css={0,0,0,0};
            if(style.type==JS_OBJECT) css=js_get(vm,style.number,"cssText");
            js_value id_value=js_get(vm,node,"id"),classes=js_get(vm,node,"className");
            html_view_node(node,tag.text,id_value.type==JS_STRING?id_value.text:0,classes.type==JS_STRING?classes.text:0,
                text.type==JS_STRING?text.text:0,markup.type==JS_STRING?markup.text:0,css.type==JS_STRING?css.text:0);
            if(equal(tag.text,"img")){
                js_value src=js_get(vm,node,"src"),width=js_get(vm,node,"width"),height=js_get(vm,node,"height");
                html_view_image(node,src.type==JS_STRING?src.text:0,width.type==JS_NUMBER&&width.number>0?(uint32_t)width.number:0,height.type==JS_NUMBER&&height.number>0?(uint32_t)height.number:0);
            }
        }
    }
    return !vm->error;
}
int browser_start(cdfs *fs,void (*write)(const char *)) {
    browser *b=&page; b->fs=fs; b->fallback_write=write; b->write=trace; b->source_used=b->script_failures=0;
    b->document_html=0;
    css_init();
    font_init(fs);
    images_init(fs);
    html_view_text_lookup(live_text,b);
    error_used=0;errors[0]=0;recording_error=console_open=0;
    control_keys=extended_key=end_down=pause_bytes=0;
    js_init(&b->vm,api,b);
    js_value document=js_object(&b->vm), body=element(&b->vm,"body"), head=element(&b->vm,"head"), console=js_object(&b->vm);
    js_value window=js_object(&b->vm);b->document=(uint32_t)document.number;b->window=(uint32_t)window.number;
    b->event=0;
    js_set(&b->vm,0,"window",window);js_set(&b->vm,window.number,"document",document);
    js_set(&b->vm,document.number,"addEventListener",js_native(ADD_EVENT_BASE+document.number));js_set(&b->vm,document.number,"removeEventListener",js_native(REMOVE_EVENT_BASE+document.number));
    js_set(&b->vm,window.number,"addEventListener",js_native(ADD_EVENT_BASE+window.number));js_set(&b->vm,window.number,"removeEventListener",js_native(REMOVE_EVENT_BASE+window.number));
    for(uint32_t i=0;i<64;i++)b->listeners[i].active=0;
    b->body=(uint32_t)body.number; b->head=(uint32_t)head.number;
    js_set(&b->vm,0,"document",document); js_set(&b->vm,0,"console",console);
    js_set(&b->vm,0,"fetch",js_native(FETCH));
    js_set(&b->vm,0,"Date",js_native(DATE));
    js_set(&b->vm,0,"setInterval",js_native(SET_INTERVAL));js_set(&b->vm,0,"setTimeout",js_native(SET_TIMEOUT));
    js_set(&b->vm,0,"clearInterval",js_native(CLEAR_TIMER));js_set(&b->vm,0,"clearTimeout",js_native(CLEAR_TIMER));
    for(uint32_t i=0;i<32;i++)b->timers[i].active=0;
    b->seconds=b->pit_cycles=0;b->last_second=rtc_read(0);
    out8(0x43,0);b->last_pit=in8(0x40);b->last_pit|=(uint16_t)in8(0x40)<<8;
    js_set(&b->vm,document.number,"body",body); js_set(&b->vm,document.number,"head",head);
    js_set(&b->vm,document.number,"createElement",js_native(CREATE));
    js_set(&b->vm,document.number,"getElementById",js_native(GET_BY_ID));
    js_set(&b->vm,document.number,"querySelector",js_native(QUERY_SELECTOR));
    js_set(&b->vm,console.number,"log",js_native(LOG)); js_set(&b->vm,console.number,"error",js_native(CONSOLE_ERROR));
    char *html; uint32_t size;
    b->write("Envy browser v0: /Envy/index.html\n");
    if(!load(b,"/Envy/index.html",&html,&size)) { write("Cannot read browser entry document\n"); return 0; }
    b->document_html=html;
    (void)size;
    const char *title=find(html,"<title>"), *end=title?find(title+7,"</title>"):0;
    const char *page_title="Envy";
    if(end) { js_value text=js_string(&b->vm,title+7,(uint32_t)(end-title-7)); page_title=text.text; }
    html_view_begin(page_title,"/Envy/index.html"); html_view_render(html);
    const char *script=html;
    while((script=find(script,"<script"))) {
        const char *open=find(script,">"); if(!open) { write("Malformed script tag\n"); return 0; }
        const char *close=find(open+1,"</script>"); if(!close) { write("Missing script closing tag\n"); return 0; }
        if(!js_eval(&b->vm,open+1,(uint32_t)(close-open-1))) {
            diagnostic(b,"/Envy/index.html",b->vm.error,b->vm.error_line); return 0;
        }
        script=close+9;
    }
    if(b->script_failures) b->write("Browser increment: WebUI compatibility incomplete.\n");
    else {b->write("Document scripts completed.\n");html_view_status("Local page loaded");}
    html_view_repaint();return b->script_failures==0;
}
static uint32_t visual_signature(browser *b){
    uint32_t hash=2166136261u;
    for(uint32_t i=0;i<b->vm.property_count;i++){js_property *p=&b->vm.properties[i];
        if(!(equal(p->name,"innerText")||equal(p->name,"textContent")||equal(p->name,"innerHTML")||equal(p->name,"className")||equal(p->name,"src")||equal(p->name,"width")||equal(p->name,"height"))||!connected(b,p->object))continue;
        hash=(hash^p->object)*16777619u;for(const char *key=p->name;*key;key++)hash=(hash^(uint8_t)*key)*16777619u;
        hash=(hash^(uint32_t)p->value.type)*16777619u;
        if(p->value.type==JS_STRING){for(uint32_t n=0;n<p->value.length;n++)hash=(hash^(uint8_t)p->value.text[n])*16777619u;}
        else hash=(hash^(uint32_t)p->value.number)*16777619u;
    }
    return hash;
}
void browser_poll(void) {
    browser *b=&page;out8(0x43,0);uint16_t counter=in8(0x40);counter|=(uint16_t)in8(0x40)<<8;
    b->pit_cycles+=(uint16_t)(b->last_pit-counter);b->last_pit=counter;
    if(b->pit_cycles<119318)return;b->pit_cycles=0;
    uint8_t second=rtc_read(0);if(second==b->last_second)return;
    uint8_t old=b->last_second;b->last_second=second;
    if(!(rtc_read(11)&4)){old=(uint8_t)((old>>4)*10+(old&15));second=(uint8_t)((second>>4)*10+(second&15));}
    b->seconds+=(second+60u-old)%60u;int changed=0;uint32_t before=visual_signature(b);
    for(uint32_t i=0;i<32;i++)if(b->timers[i].active&&b->seconds>=b->timers[i].due) {
        if(b->timers[i].repeat)b->timers[i].due=b->seconds+b->timers[i].interval;else b->timers[i].active=0;
        if(!js_call(&b->vm,b->timers[i].callback)){diagnostic(b,"(timer)",b->vm.error,b->vm.error_line);b->timers[i].active=0;}
        changed=1;
    }
    if(changed&&!console_open&&before!=visual_signature(b))html_view_repaint();
}
static void dispatch_event(browser *b,const char *type,const input_event *input){
    int interested=0;for(uint32_t i=0;i<64;i++)if(b->listeners[i].active&&equal(b->listeners[i].type.text,type))interested=1;
    if(!interested)return;js_vm *vm=&b->vm;vm->error=0;
    if(!b->event){js_value event=js_object(vm);if(vm->error){diagnostic(b,"(event)",vm->error,0);vm->error=0;return;}b->event=(uint32_t)event.number;js_set(vm,b->event,"preventDefault",js_native(PREVENT_DEFAULT));}
    js_value event={JS_OBJECT,(int32_t)b->event,0,0};js_set(vm,b->event,"type",js_string(vm,type,count(type)));
    js_set(vm,b->event,"target",(js_value){JS_OBJECT,(int32_t)b->body,0,0});js_set(vm,b->event,"defaultPrevented",(js_value){JS_BOOLEAN,0,0,0});
    js_set(vm,b->event,"clientX",(js_value){JS_NUMBER,input->x,0,0});js_set(vm,b->event,"clientY",(js_value){JS_NUMBER,input->y,0,0});
    js_set(vm,b->event,"buttons",(js_value){JS_NUMBER,input->buttons,0,0});js_set(vm,b->event,"keyCode",(js_value){JS_NUMBER,input->key,0,0});
    js_value callbacks[64];uint32_t targets[64];uint32_t count_callbacks=0;
    for(uint32_t i=0;i<64;i++)if(b->listeners[i].active&&equal(b->listeners[i].type.text,type)&&
       (b->listeners[i].target==b->document||b->listeners[i].target==b->window||b->listeners[i].target==b->body)){
        callbacks[count_callbacks]=b->listeners[i].callback;targets[count_callbacks++]=b->listeners[i].target;if(b->listeners[i].once)b->listeners[i].active=0;}
    for(uint32_t i=0;i<count_callbacks;i++){js_value result;js_set(vm,b->event,"currentTarget",(js_value){JS_OBJECT,(int32_t)targets[i],0,0});
        if(!js_call_args(vm,callbacks[i],&event,1,&result)){diagnostic(b,"(event listener)",vm->error,vm->error_line);vm->error=0;}}
}
void browser_input(const input_event *event) {
    if(event->type==INPUT_KEY) {
        browser_key(event->modifiers&INPUT_CONTROL?0x1d:0x9d);
        if(event->key&0x100)browser_key(0xe0);browser_key(event->scancode);
        if(!console_open)dispatch_event(&page,event->pressed?"keydown":"keyup",event);
    } else if(event->type==INPUT_POINTER) {
        framebuffer_cursor_move(event->x,event->y);
        if(!console_open){if(event->dx||event->dy)dispatch_event(&page,"mousemove",event);
            if((event->buttons&1)&&!(pointer_buttons&1))dispatch_event(&page,"mousedown",event);
            if(!(event->buttons&1)&&(pointer_buttons&1)){dispatch_event(&page,"mouseup",event);dispatch_event(&page,"click",event);html_view_repaint();}}
        if((event->buttons&1)&&!(pointer_buttons&1)&&!console_open) {
            char handler[256];if(html_view_hit_test(event->x,event->y,handler,sizeof(handler))) {
                if(!js_eval(&page.vm,handler,count(handler)))diagnostic(&page,"(click handler)",page.vm.error,page.vm.error_line);
                html_view_repaint();
            }
        }
        pointer_buttons=event->buttons;
    }
}
