#include "browser.h"
#include "../Runtime/JS/js.h"
#include "../Runtime/JS/full.h"
#include "../Arch/x86/io.h"
#include "../Drivers/Video/framebuffer.h"
#include "html.h"
#include "css.h"
#include "font.h"
#include "images.h"
#define SOURCE_MEMORY (2u*1024u*1024u)
extern void *envy_js_alloc(void *,void *,int);
typedef struct {uint32_t target;js_value type,callback;int active,once;} event_listener;
typedef struct {
    js_vm vm; cdfs *fs; void (*write)(const char *); void (*fallback_write)(const char *);
    char sources[SOURCE_MEMORY]; uint32_t source_used;
    const char *load_error;
    const char *document_html;
    char dom_markup[262144];uint32_t dom_used,dom_paint;int dom_dirty;
    struct {char path[256];cdfs_node node;char *source;uint32_t length;int status;} files[256];
    uint32_t file_count,source_reads,source_cache_hits;
    uint32_t body, head,document,window,event,focused; uint32_t script_failures;
    int64_t clock_ms;uint32_t clock_remainder;uint16_t clock_counter;
    event_listener *listeners;uint32_t listener_capacity;
    struct {js_value callback;int64_t interval,due;int active,repeat;} timers[32];
    uint32_t seconds,pit_cycles;uint16_t last_pit;uint8_t last_second;
} browser;
static int dispatch_event(browser *b,const char *type,const input_event *input);
static browser page;
static char errors[4096];
static uint32_t error_used;
static int recording_error, console_open;
static int console_dirty;
static uint8_t control_keys, extended_key, end_down, pause_bytes;
static uint8_t pointer_buttons;
static int32_t pointer_screen_x,pointer_screen_y;
/* Save the bottom 96 rows, including pitch padding, before drawing the panel. */
static uint8_t panel_saved[16384u*96u];
static void remember(char c) {
    if(error_used==sizeof(errors)-1) {
        uint32_t remove=0; while(remove<error_used&&errors[remove++]!='\n') {}
        for(uint32_t i=remove;i<error_used;i++) errors[i-remove]=errors[i];
        error_used-=remove;
    }
    errors[error_used++]=c; errors[error_used]=0; console_dirty=1;
}
static uint32_t console_first,console_paint;
static int console_follow=1;
static uint32_t console_columns(const framebuffer *fb){return fb->width>28?(fb->width-28)/8:1;}
static uint32_t console_visible(const framebuffer *fb){uint32_t rows=fb->height<96?fb->height:96;return rows>=42?1+(rows-42)/18:1;}
static uint32_t console_next(uint32_t begin,uint32_t columns){
    uint32_t n=0;while(begin<error_used&&errors[begin]!='\n'&&n<columns){begin++;n++;}
    if(begin<error_used&&errors[begin]=='\n')begin++;return begin;
}
static uint32_t console_lines(uint32_t columns){uint32_t lines=0;for(uint32_t i=0;i<error_used;lines++)i=console_next(i,columns);return lines;}
static void console_save(const framebuffer *fb){
    uint32_t rows=fb->height<96?fb->height:96,offset=(fb->height-rows)*fb->pitch;
    for(uint32_t i=0;i<rows*fb->pitch;i++)panel_saved[i]=fb->pixels[offset+i];console_paint=html_view_paint_count();
}
static void console_draw(void){
    const framebuffer *fb=framebuffer_get();if(!fb||!console_open)return;framebuffer_cursor_restore();
    uint32_t rows=fb->height<96?fb->height:96,columns=console_columns(fb),visible=console_visible(fb),lines=console_lines(columns);
    uint32_t maximum=lines>visible?lines-visible:0;if(console_follow||console_first>maximum)console_first=maximum;
    int32_t top=(int32_t)(fb->height-rows);framebuffer_rect(0,top,fb->width,rows,0x101820);
    framebuffer_text(8,top+4,"JS errors - Wheel / Up / Down / PgUp / PgDn / Home / End - Ctrl+End close",0xe8edf2);
    uint32_t begin=0;for(uint32_t line=0;line<console_first;line++)begin=console_next(begin,columns);
    int32_t y=top+26;if(!error_used)framebuffer_text(8,y,"No JavaScript errors.",0xe8edf2);
    for(uint32_t line=0;line<visible&&begin<error_used&&y+16<=(int32_t)fb->height;line++,y+=18){
        uint32_t end=console_next(begin,columns),column=0;
        for(uint32_t i=begin;i<end&&errors[i]!='\n';i++){char glyph[2]={errors[i],0};framebuffer_text(8+(int32_t)column*8,y,glyph,0xff9090);column++;}begin=end;
    }
    if(maximum&&fb->width>=8&&rows>30){uint32_t track=rows-30,thumb=track*visible/lines;if(thumb<4)thumb=4;
        framebuffer_rect((int32_t)fb->width-7,top+26,4,track,0x354350);
        framebuffer_rect((int32_t)fb->width-7,top+26+(int32_t)((track-thumb)*console_first/maximum),4,thumb,0x9baebf);}
    console_dirty=0;framebuffer_cursor_redraw();
}
static void console_sync(void){
    const framebuffer *fb=framebuffer_get();if(!console_open||!fb)return;
    if(console_paint!=html_view_paint_count()){framebuffer_cursor_restore();console_save(fb);console_dirty=1;}
    if(console_dirty)console_draw();
}
static void console_scroll(int32_t delta){
    const framebuffer *fb=framebuffer_get();if(!console_open||!fb)return;
    uint32_t lines=console_lines(console_columns(fb)),visible=console_visible(fb),maximum=lines>visible?lines-visible:0;
    int64_t next=(int64_t)console_first+delta;if(next<0)next=0;if(next>maximum)next=maximum;
    console_follow=(uint32_t)next==maximum;if(console_first!=(uint32_t)next){console_first=(uint32_t)next;console_draw();}
}
static void console_toggle(void){
    const framebuffer *fb=framebuffer_get();framebuffer_cursor_restore();console_open=!console_open;if(!fb)return;
    if(console_open){console_follow=1;console_save(fb);console_draw();return;}
    uint32_t rows=fb->height<96?fb->height:96,offset=(fb->height-rows)*fb->pitch;
    for(uint32_t i=0;i<rows*fb->pitch;i++)fb->pixels[offset+i]=panel_saved[i];framebuffer_cursor_redraw();
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
    if(console_open&&extended&&!(code&0x80)){
        const framebuffer *fb=framebuffer_get();int32_t step=fb?(int32_t)console_visible(fb):4;
        if(code==0x48)console_scroll(-1);else if(code==0x50)console_scroll(1);
        else if(code==0x49)console_scroll(-step);else if(code==0x51)console_scroll(step);
        else if(code==0x47)console_scroll(-2147483647);else if(code==0x4f)console_scroll(2147483647);
    }
    return 0;
}
static void trace(const char *text) {
    if(recording_error) for(const char *p=text;*p;p++) remember(*p);
    if(!framebuffer_get()) {page.fallback_write(text);return;}
    while(*text) out8(0xe9,(uint8_t)*text++);
}
enum { CREATE=1, APPEND, INSERT, LOG, CONSOLE_ERROR, FETCH, GET_BY_ID,
       DATE, SET_INTERVAL, CLEAR_TIMER, SET_TIMEOUT, QUERY_SELECTOR, MEDIA_PLAY, MEDIA_PAUSE, PREVENT_DEFAULT, TEST_NOOP, DATE_NOW, FOCUS, QUERY_ALL,
       THEN_BASE=1000, CATCH_BASE=2000, APPEND_BASE=3000, INSERT_BASE=4000, DATE_BASE=6000, ADD_EVENT_BASE=8000, REMOVE_EVENT_BASE=9000,
       RESPONSE_TEXT_BASE=10000, RESPONSE_JSON_BASE=11000, CLASS_ADD_BASE=12000, CLASS_REMOVE, CLASS_TOGGLE, CLASS_CONTAINS, NODE_REMOVE, STYLE_SET_PROPERTY, SET_ATTRIBUTE, REMOVE_ATTRIBUTE };
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
static int64_t clock_epoch(void){
    uint8_t values[5];
    for(uint32_t attempt=0;attempt<8;attempt++){
        uint8_t second=rtc_read(0),year=rtc_read(9),century=rtc_read(0x32),mode=rtc_read(11);
        if(!clock_snapshot(values)||(rtc_read(10)&128)||second!=rtc_read(0)||year!=rtc_read(9))continue;
        if(!(mode&4)){second=(uint8_t)((second>>4)*10+(second&15));year=(uint8_t)((year>>4)*10+(year&15));century=(uint8_t)((century>>4)*10+(century&15));}
        uint32_t full_year=(century>=19&&century<=99?century:20)*100u+year;
        if(full_year<1970||values[3]>=12||!values[2]||values[2]>31||second>59)return -1;
        uint32_t days=0;for(uint32_t y=1970;y<full_year;y++)days+=365+(y%4==0&&(y%100!=0||y%400==0));
        static const uint8_t months[]={31,28,31,30,31,30,31,31,30,31,30,31};
        for(uint32_t m=0;m<values[3];m++)days+=months[m]+(m==1&&full_year%4==0&&(full_year%100!=0||full_year%400==0));
        days+=values[2]-1;
        return ((int64_t)days*86400+values[0]*3600u+values[1]*60u+second)*1000;
    }
    return -1;
}
static void clock_tick(browser *b,uint16_t counter){
    uint32_t scaled=(uint16_t)(b->clock_counter-counter)*1000u+b->clock_remainder;
    b->clock_counter=counter;b->clock_ms+=scaled/1193182u;b->clock_remainder=scaled%1193182u;
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
static int cached_lookup(browser *b,const char *path,cdfs_node *node){
    for(uint32_t i=0;i<b->file_count;i++)if(equal(b->files[i].path,path)){*node=b->files[i].node;return b->files[i].status;}
    int status=cdfs_lookup(b->fs,path,node);uint32_t size=count(path);
    if((status==CDFS_OK||status==CDFS_NOT_FOUND)&&size<256&&b->file_count<256){uint32_t i=b->file_count++;
        for(uint32_t j=0;j<=size;j++)b->files[i].path[j]=path[j];b->files[i].status=status;b->files[i].node=status?(cdfs_node){0}:*node;b->files[i].source=0;b->files[i].length=0;}
    return status;
}
static int load(browser *b,const char *path,char **source,uint32_t *length) {
    cdfs_node node;
    b->load_error=0;
    for(uint32_t i=0;i<b->file_count;i++)if(b->files[i].source&&equal(b->files[i].path,path)){
        *source=b->files[i].source;*length=b->files[i].length;b->source_cache_hits++;return 1;}
    if(cached_lookup(b,path,&node)) {b->load_error="Local script not found";return 0;}
    if(node.flags&2) {b->load_error="Script path is a directory";return 0;}
    uint32_t saved=b->source_used,got;char *buffer=source_buffer(b,node.size);
    if(!buffer) return 0;
    b->source_reads++;
    if(cdfs_read(b->fs,&node,0,buffer,node.size,&got)||got!=node.size) {
        b->source_used=saved;b->load_error="Cannot read local script";return 0;
    }
    buffer[got]=0; *source=buffer; *length=got;
    for(uint32_t i=0;i<b->file_count;i++)if(equal(b->files[i].path,path)){b->files[i].source=buffer;b->files[i].length=got;break;}return 1;
}
static js_value element(js_vm *vm,const char *tag) {
    js_value node=js_object(vm), style=js_object(vm);
    js_set(vm,node.number,"tagName",js_string(vm,tag,count(tag)));
    js_set(vm,node.number,"style",style);
    js_set(vm,style.number,"setProperty",js_native_bound(STYLE_SET_PROPERTY,(uint32_t)style.number));
    js_set(vm,node.number,"setAttribute",js_native_bound(SET_ATTRIBUTE,(uint32_t)node.number));
    js_set(vm,node.number,"removeAttribute",js_native_bound(REMOVE_ATTRIBUTE,(uint32_t)node.number));
    js_set(vm,style.number,"__element",node);js_set(vm,node.number,"__created",(js_value){JS_BOOLEAN,1,0,0});
    js_value list=js_object(vm);js_set(vm,node.number,"classList",list);
    js_set(vm,list.number,"add",js_native_bound(CLASS_ADD_BASE,(uint32_t)node.number));
    js_set(vm,list.number,"remove",js_native_bound(CLASS_REMOVE,(uint32_t)node.number));
    js_set(vm,list.number,"toggle",js_native_bound(CLASS_TOGGLE,(uint32_t)node.number));
    js_set(vm,list.number,"contains",js_native_bound(CLASS_CONTAINS,(uint32_t)node.number));
    js_set(vm,node.number,"remove",js_native_bound(NODE_REMOVE,(uint32_t)node.number));
    js_set(vm,node.number,"dataset",js_object(vm));js_set(vm,node.number,"focus",js_native_bound(FOCUS,(uint32_t)node.number));
    js_set(vm,node.number,"querySelector",js_native_bound(QUERY_SELECTOR,(uint32_t)node.number));
    js_set(vm,node.number,"querySelectorAll",js_native_bound(QUERY_ALL,(uint32_t)node.number));
    if(equal(tag,"input")||equal(tag,"textarea"))js_set(vm,node.number,"value",js_string(vm,"",0));
    js_value append=js_native_bound(APPEND_BASE,(uint32_t)node.number),insert=js_native_bound(INSERT_BASE,(uint32_t)node.number);
    js_set(vm,node.number,"appendChild",append);
    js_set(vm,node.number,"insertBefore",insert);
    js_value listen=js_native_bound(ADD_EVENT_BASE,(uint32_t)node.number),remove=js_native_bound(REMOVE_EVENT_BASE,(uint32_t)node.number);
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
    js_set(vm,node.number,"__created",(js_value){JS_BOOLEAN,0,0,0});
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
static uint32_t live_node(browser *b,uint32_t node,const char *id){
    if(node)return node;if(!id||!*id)return 0;
    for(uint32_t i=0;i<b->vm.property_count;i++){js_property *p=&b->vm.properties[i];if(equal(p->name,"id")&&p->value.type==JS_STRING&&equal(p->value.text,id)&&connected(b,p->object))return p->object;}return 0;
}
typedef struct {browser *b;uint32_t root,scope,count;js_value array;} query_context;
static int query_contains(browser *b,uint32_t scope,uint32_t node){
    if(!scope||scope==b->document)return connected(b,node);
    for(uint32_t depth=0;node&&depth<=b->vm.objects;depth++){
        if(node==scope)return 1;js_value parent=js_get(&b->vm,node,"parentNode");node=parent.type==JS_OBJECT?(uint32_t)parent.number:0;}
    return 0;
}
static void query_selected(void *context,uint32_t node){
    query_context *q=context;js_vm *vm=&q->b->vm;if(!node||vm->error)return;
    if(q->scope!=q->b->document&&q->scope&&node==q->scope)return;
    for(uint32_t i=0;i<vm->property_count;i++)if(vm->properties[i].object==(uint32_t)q->array.number&&vm->properties[i].value.type==JS_OBJECT&&vm->properties[i].value.number==node)return;
    js_value key=js_number_string(vm,q->count++);js_set(vm,q->array.number,key.text,(js_value){JS_OBJECT,node,0,0});
}
static uint32_t query_materialize(void *context,uint32_t index,uint32_t parent,const char *tag,const char *id,const char *classes,const char *value){
    query_context *q=context;js_vm *vm=&q->b->vm;if(!index)return q->root;if(vm->error)return 0;
    char key[32]="__query";js_value index_text=js_number_string(vm,index);if(vm->error)return 0;
    for(uint32_t i=0;i<=index_text.length;i++)key[7+i]=index_text.text[i];
    js_value existing=js_get(vm,q->root,key);if(existing.type==JS_OBJECT)return (uint32_t)existing.number;
    uint32_t known=*id?live_node(q->b,0,id):0;js_value node=known?(js_value){JS_OBJECT,known,0,0}:element(vm,tag);
    if(!known)js_set(vm,node.number,"__created",(js_value){JS_BOOLEAN,0,0,0});
    if(!known){if(*id)js_set(vm,node.number,"id",js_string(vm,id,count(id)));
        if(*classes)js_set(vm,node.number,"className",js_string(vm,classes,count(classes)));
        if(equal(tag,"input"))js_set(vm,node.number,"value",js_string(vm,value,count(value)));
        js_set(vm,node.number,"parentNode",(js_value){JS_OBJECT,parent,0,0});link_child(vm,parent,node);}
    js_set(vm,q->root,key,node);return vm->error?0:(uint32_t)node.number;
}
static css_element *query_element(browser *b,uint32_t node,css_element *elements){
    uint32_t n=0;while(node&&n<32){js_value tag=js_get(&b->vm,node,"tagName"),id=js_get(&b->vm,node,"id"),classes=js_get(&b->vm,node,"className");
        elements[n]=(css_element){tag.type==JS_STRING?tag.text:"body",id.type==JS_STRING?id.text:"",classes.type==JS_STRING?classes.text:"",0};
        if(n)elements[n-1].parent=&elements[n];n++;js_value parent=js_get(&b->vm,node,"parentNode");node=parent.type==JS_OBJECT?(uint32_t)parent.number:0;}
    return elements;
}
static void query_markup(query_context *q,uint32_t owner,const char *markup,const char *selector){
    js_vm *vm=&q->b->vm;js_value previous=js_get(vm,owner,"__querySource");
    if(previous.type==JS_STRING&&previous.text!=markup){
        for(uint32_t i=0;i<vm->property_count;i++){js_property p=vm->properties[i];
            if(p.object!=owner||p.value.type!=JS_OBJECT)continue;
            const char *name=p.name,*prefix="__query";while(*prefix&&*name==*prefix){name++;prefix++;}
            if(!*prefix){js_set(vm,p.value.number,"parentNode",(js_value){JS_UNDEFINED,0,0,0});js_set(vm,owner,p.name,(js_value){JS_UNDEFINED,0,0,0});}
        }
    }
    js_set(vm,owner,"__querySource",(js_value){JS_STRING,0,markup,count(markup)});
    q->root=owner;css_element ancestors[32];html_query_selector(markup,selector,query_element(q->b,owner,ancestors),q,query_materialize,query_selected);
}
static js_value query_all(browser *b,uint32_t scope,const char *selector){
    const char *attribute=find(selector,"[");
    if(attribute){
        char base[256],name[64],expected[256];uint32_t used=(uint32_t)(attribute-selector);if(used>=sizeof(base))return (js_value){0};
        for(uint32_t i=0;i<used;i++)base[i]=selector[i];base[used]=0;if(!used){base[0]='*';base[1]=0;}
        const char *p=attribute+1;used=0;while(*p&&*p!='='&&*p!=']'&&used+1<sizeof(name))name[used++]=*p++;name[used]=0;
        int equality=*p=='=';if(equality)p++;char quote=*p=='\''||*p=='\"'?*p++:0;used=0;
        while(*p&&*p!=']'&&(!quote||*p!=quote)&&used+1<sizeof(expected))expected[used++]=*p++;expected[used]=0;
        if(quote&&*p==quote)p++;if(*p!=']'||p[1])return (js_value){0};
        js_value source=query_all(b,scope,base),size=js_get(&b->vm,source.number,"length"),result=js_object(&b->vm);uint32_t selected=0;
        for(int64_t i=0;i<size.number;i++){js_value key=js_number_string(&b->vm,i),node=js_get(&b->vm,source.number,key.text),value;
            if(name[0]=='d'&&name[1]=='a'&&name[2]=='t'&&name[3]=='a'&&name[4]=='-'){js_value data=js_get(&b->vm,node.number,"dataset");value=js_get(&b->vm,data.number,name+5);}
            else value=js_get(&b->vm,node.number,equal(name,"class")?"className":name);
            if(value.type!=JS_UNDEFINED&&(!equality||(value.type==JS_STRING&&equal(value.text,expected)))){key=js_number_string(&b->vm,selected++);js_set(&b->vm,result.number,key.text,node);}}
        js_set(&b->vm,result.number,"length",(js_value){JS_NUMBER,selected,0,0});return result;
    }
    js_vm *vm=&b->vm;query_context q={.b=b,.scope=scope,.array=js_object(vm)};
    uint32_t properties=vm->property_count;
    if((!scope||scope==b->document)&&b->document_html)query_markup(&q,b->body,b->document_html,selector);
    for(uint32_t i=0;i<properties&&!vm->error;i++){js_property p=vm->properties[i];
        if(p.value.type!=JS_STRING||!query_contains(b,scope,p.object))continue;
        if(equal(p.name,"innerHTML"))query_markup(&q,p.object,p.value.text,selector);
        else if(equal(p.name,"tagName")){css_element ancestors[32];if(css_selector_matches(query_element(b,p.object,ancestors),selector))query_selected(&q,p.object);}
    }
    js_set(vm,q.array.number,"length",(js_value){JS_NUMBER,q.count,0,0});return q.array;
}
static int equal_prefix(const char *s,const char *prefix){while(*prefix)if(*s++!=*prefix++)return 0;return 1;}
static void dom_write(browser *b,const char *text,int escaped){
    while(text&&*text&&b->dom_used+8<sizeof(b->dom_markup)){char c=*text++;
        const char *replacement=escaped?(c=='&'?"&amp;":c=='<'?"&lt;":c=='>'?"&gt;":c=='\"'?"&quot;":0):0;
        if(replacement)while(*replacement)b->dom_markup[b->dom_used++]=*replacement++;else b->dom_markup[b->dom_used++]=c;
    }
}
static void dom_contents(browser *,uint32_t,uint32_t);
static void dom_element(browser *b,uint32_t node,uint32_t depth){
    if(depth>32)return;js_vm *vm=&b->vm;js_value tag=js_get(vm,node,"tagName");if(tag.type!=JS_STRING)return;
    if(equal(tag.text,"script")||equal(tag.text,"style")||equal(tag.text,"link"))return;
    dom_write(b,"<",0);dom_write(b,tag.text,0);dom_write(b," data-envy-node=\"",0);
    char digits[12];uint32_t pos=11,n=node;digits[pos]=0;do{digits[--pos]=(char)('0'+n%10);n/=10;}while(n);dom_write(b,digits+pos,0);dom_write(b,"\"",0);
    const char *properties[]={"id","className","src","value"},*attributes[]={"id","class","src","value"};
    for(uint32_t i=0;i<4;i++){js_value v=js_get(vm,node,properties[i]);if(v.type!=JS_STRING)continue;dom_write(b," ",0);dom_write(b,attributes[i],0);dom_write(b,"=\"",0);dom_write(b,v.text,1);dom_write(b,"\"",0);}
    dom_write(b,">",0);dom_contents(b,node,depth+1);dom_write(b,"</",0);dom_write(b,tag.text,0);dom_write(b,">",0);
}
static void dom_contents(browser *b,uint32_t node,uint32_t depth){
    js_vm *vm=&b->vm;js_value html=js_get(vm,node,"innerHTML"),text=js_get(vm,node,"innerText");
    if(text.type!=JS_STRING)text=js_get(vm,node,"textContent");
    if(text.type==JS_STRING)dom_write(b,text.text,1);else if(html.type==JS_STRING)dom_write(b,html.text,0);
    uint32_t properties=vm->property_count;
    for(uint32_t i=0;i<properties;i++){js_property p=vm->properties[i];if(p.object!=node||p.value.type!=JS_OBJECT||!equal_prefix(p.name,"__child"))continue;
        uint32_t child=(uint32_t)p.value.number;js_value parent=js_get(vm,child,"parentNode");
        if(parent.type==JS_OBJECT&&parent.number==node&&js_get(vm,child,"__created").number)dom_element(b,child,depth);
    }
}
static const char *live_markup(void *context,const char *id){
    browser *b=context;uint32_t paint=html_view_paint_count();if(b->dom_paint!=paint){b->dom_paint=paint;b->dom_used=0;b->dom_dirty=0;}
    uint32_t node=live_node(b,0,id);if(!node)return 0;uint32_t start=b->dom_used;
    dom_contents(b,node,0);if(b->dom_used==start){js_value html=js_get(&b->vm,node,"innerHTML");return html.type==JS_STRING?html.text:0;}
    b->dom_markup[b->dom_used++]=0;return b->dom_markup+start;
}
static void property_changed(js_vm *vm,uint32_t owner,const char *name){
    if(!owner||owner>=0x40000000u)return;browser *b=vm->context;
    if(equal(name,"innerHTML")||equal(name,"textContent")||equal(name,"innerText")){
        for(uint32_t i=0;i<vm->property_count;i++){js_property p=vm->properties[i];
            if(p.object!=owner||p.value.type!=JS_OBJECT||!equal_prefix(p.name,"__child"))continue;
            js_value parent=js_get(vm,(uint32_t)p.value.number,"parentNode");
            if(parent.type==JS_OBJECT&&(uint32_t)parent.number==owner)js_set(vm,(uint32_t)p.value.number,"parentNode",(js_value){JS_NULL,0,0,0});}
    }
    if(equal(name,"parentNode")||equal(name,"className")||equal(name,"id")||equal(name,"innerHTML")||equal(name,"textContent")||equal(name,"innerText")||equal(name,"value")||equal(name,"src")||js_get(vm,owner,"__element").type==JS_OBJECT)b->dom_dirty=1;
}
static int property_read(js_vm *vm,uint32_t owner,const char *name,js_value *out){
    if(!equal(name,"offsetLeft")&&!equal(name,"offsetTop")&&!equal(name,"offsetWidth")&&!equal(name,"offsetHeight"))return 0;
    js_value id=js_get(vm,owner,"id");int32_t x=0,y=0,w=0,h=0;html_view_geometry(owner,id.type==JS_STRING?id.text:"",&x,&y,&w,&h);
    js_value parent=js_get(vm,owner,"parentNode");if(parent.type==JS_OBJECT){js_value parent_id=js_get(vm,parent.number,"id");int32_t px=0,py=0,pw,ph;
        if(html_view_geometry((uint32_t)parent.number,parent_id.type==JS_STRING?parent_id.text:"",&px,&py,&pw,&ph)){x-=px;y-=py;}}
    *out=(js_value){JS_NUMBER,equal(name,"offsetLeft")?x:equal(name,"offsetTop")?y:equal(name,"offsetWidth")?w:h,0,0};return 1;
}
static void flush_dom(browser *b){if(b->dom_dirty){b->dom_dirty=0;b->dom_used=0;html_view_repaint();}}
static void live_style(void *context,uint32_t node,const char *id,char *out,uint32_t capacity){
    browser *b=context;if(!capacity)return;out[0]=0;node=live_node(b,node,id);if(!node)return;
    js_value style=js_get(&b->vm,node,"style");if(style.type!=JS_OBJECT)return;uint32_t used=0;
    js_value text=js_get(&b->vm,style.number,"cssText");if(text.type==JS_STRING&&text.length+2<capacity){for(uint32_t i=0;i<text.length;i++)out[used++]=text.text[i];out[used++]=';';}
    for(uint32_t i=0;i<b->vm.property_count;i++){js_property *p=&b->vm.properties[i];if(p->object!=(uint32_t)style.number||equal(p->name,"cssText"))continue;
        if(p->value.type!=JS_STRING&&p->value.type!=JS_NUMBER)continue;char name[128],value[16];uint32_t name_used=0;
        for(const char *q=p->name;*q&&name_used+2<sizeof(name);q++){if(*q>='A'&&*q<='Z')name[name_used++]='-';name[name_used++]=lower(*q);}name[name_used]=0;
        const char *v=p->value.text;uint32_t size=p->value.length;
        if(p->value.type==JS_NUMBER){uint32_t n=p->value.number<0?0u-(uint32_t)p->value.number:(uint32_t)p->value.number,pos=15;value[pos]=0;do{value[--pos]=(char)('0'+n%10);n/=10;}while(n);if(p->value.number<0)value[--pos]='-';v=value+pos;size=15-pos;}
        if(used+name_used+size+3>=capacity)continue;for(uint32_t j=0;j<name_used;j++)out[used++]=name[j];out[used++]=':';for(uint32_t j=0;j<size;j++)out[used++]=v[j];out[used++]=';';}
    out[used]=0;
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
    if(js_full_active(vm)){js_value result={0};js_full_promise(vm,value,rejected,&result);return result;}
    js_value object=js_object(vm);
    js_set(vm,object.number,"value",value);
    js_set(vm,object.number,"rejected",(js_value){JS_NUMBER,rejected,0,0});
    js_set(vm,object.number,"then",js_native_bound(THEN_BASE,(uint32_t)object.number));
    js_set(vm,object.number,"catch",js_native_bound(CATCH_BASE,(uint32_t)object.number));
    js_set(vm,object.number,"__promise",(js_value){JS_BOOLEAN,1,0,0});
    return object;
}
static int local_module(js_vm *vm,const char *name,const char **source,uint32_t *size){
    char path[256],*text;if(!local_path(name,path)||!load(vm->context,path,&text,size))return 0;*source=text;return 1;
}
static int eval_file(js_vm *vm,const char *source,uint32_t size,const char *path){
    const char *previous=vm->source_name;vm->source_name=path;int ok=js_eval(vm,source,size);vm->source_name=previous;return ok;
}
int64_t envy_js_now_ms(void){
    browser *b=&page;int64_t epoch=clock_epoch();if(!b->clock_ms)return epoch;
    out8(0x43,0);uint16_t counter=in8(0x40);counter|=(uint16_t)in8(0x40)<<8;clock_tick(b,counter);
    if(epoch>b->clock_ms){b->clock_ms=epoch;b->clock_remainder=0;}return b->clock_ms;
}
static int api(js_vm *vm,int id,const js_value *args,uint32_t argc,js_value *result) {
    browser *b=vm->context;uint32_t owner=vm->native_owner;
    if(id==STYLE_SET_PROPERTY){if(argc<2||args[0].type!=JS_STRING||args[1].type!=JS_STRING)return 0;return js_set(vm,owner,args[0].text,args[1]);}
    if(id==SET_ATTRIBUTE||id==REMOVE_ATTRIBUTE){
        if(!argc||args[0].type!=JS_STRING||(id==SET_ATTRIBUTE&&argc<2))return 0;const char *name=args[0].text;
        js_value value=id==SET_ATTRIBUTE?args[1]:(js_value){0};
        if(equal_prefix(name,"data-")){js_value dataset=js_get(vm,owner,"dataset");return js_set(vm,dataset.number,name+5,value);}
        return js_set(vm,owner,equal(name,"class")?"className":name,value);
    }
    if(id==DATE_NOW){
        int64_t epoch=clock_epoch();if(epoch<0){vm->error="Cannot read PC real-time clock";return 0;}
        out8(0x43,0);uint16_t counter=in8(0x40);counter|=(uint16_t)in8(0x40)<<8;clock_tick(b,counter);
        if(epoch>b->clock_ms){b->clock_ms=epoch;b->clock_remainder=0;}
        *result=(js_value){JS_NUMBER,b->clock_ms,0,0};return 1;
    }
    if(id==FOCUS){if(!connected(b,owner)){vm->error="Cannot focus a detached element";return 0;}
        if(b->focused==owner)return 1;input_event input={.type=INPUT_KEY};
        if(b->focused)dispatch_event(b,"blur",&input);b->focused=owner;
        if(!js_set(vm,b->document,"activeElement",(js_value){JS_OBJECT,owner,0,0}))return 0;
        dispatch_event(b,"focus",&input);return !vm->error;}
#ifdef BOOT_IMAGE_TESTS
    if(id==TEST_NOOP)return 1;
#endif
    if(id==RESPONSE_TEXT_BASE||id==RESPONSE_JSON_BASE){
        int json=id==RESPONSE_JSON_BASE;
        if(js_get(vm,owner,"bodyUsed").number){const char *message="Response body has already been consumed";*result=promise(vm,js_string(vm,message,count(message)),1);return !vm->error;}
        js_set(vm,owner,"bodyUsed",(js_value){JS_BOOLEAN,1,0,0});js_value body=js_get(vm,owner,"__body"),value=body;
        int rejected=0;if(json&&!js_json_parse(vm,body.text,body.length,&value)){const char *message=vm->error?vm->error:"Invalid JSON response";vm->error=0;value=js_string(vm,message,count(message));rejected=1;}
        *result=promise(vm,value,rejected);return !vm->error;
    }
    if(id==NODE_REMOVE){js_set(vm,owner,"parentNode",(js_value){JS_NULL,0,0,0});return !vm->error;}
    if(id==CLASS_ADD_BASE||id==CLASS_REMOVE||id==CLASS_TOGGLE||id==CLASS_CONTAINS){
        js_value classes=js_get(vm,owner,"className");char buffer[512];uint32_t used=0;
        if(classes.type==JS_STRING){if(classes.length>=sizeof(buffer))return 0;for(uint32_t i=0;i<classes.length;i++)buffer[used++]=classes.text[i];}buffer[used]=0;
        uint32_t tokens=id==CLASS_TOGGLE?1:argc;
        for(uint32_t i=0;i<tokens;i++){
            if(i>=argc||args[i].type!=JS_STRING||!args[i].length)return 0;
            int found=0;for(uint32_t at=0;at<used;){while(at<used&&buffer[at]==' ')at++;uint32_t begin=at;while(at<used&&buffer[at]!=' ')at++;
                int match=at-begin==args[i].length;for(uint32_t j=0;match&&j<args[i].length;j++)if(buffer[begin+j]!=args[i].text[j])match=0;if(match)found=1;}
            if(id==CLASS_CONTAINS){*result=(js_value){JS_BOOLEAN,found,0,0};return 1;}
            int want=id==CLASS_ADD_BASE||(id==CLASS_TOGGLE&&(argc>1?args[1].number!=0:!found));uint32_t keep=0;
            for(uint32_t at=0;at<used;){while(at<used&&buffer[at]==' ')at++;uint32_t begin=at;while(at<used&&buffer[at]!=' ')at++;uint32_t size=at-begin;
                int match=size==args[i].length;for(uint32_t j=0;match&&j<size;j++)if(buffer[begin+j]!=args[i].text[j])match=0;
                if(size&&(want||!match)){if(keep)buffer[keep++]=' ';for(uint32_t j=0;j<size;j++)buffer[keep++]=buffer[begin+j];}}
            used=keep;if(want&&!found){if(used+args[i].length+2>=sizeof(buffer))return 0;if(used)buffer[used++]=' ';for(uint32_t j=0;j<args[i].length;j++)buffer[used++]=args[i].text[j];}
            buffer[used]=0;if(id==CLASS_TOGGLE)*result=(js_value){JS_BOOLEAN,want,0,0};
        }
        return js_set(vm,owner,"className",js_string(vm,buffer,used));
    }
    if(id==ADD_EVENT_BASE||id==REMOVE_EVENT_BASE){
        int remove=id==REMOVE_EVENT_BASE;uint32_t target=owner;
        if(argc<2||args[0].type!=JS_STRING||args[1].type!=JS_FUNCTION){vm->error="Event listener requires an event name and function";return 0;}
        uint32_t free_slot=b->listener_capacity;
        for(uint32_t i=0;i<b->listener_capacity;i++){if(!b->listeners[i].active){if(free_slot==b->listener_capacity)free_slot=i;continue;}
            if(b->listeners[i].target==target&&equal(b->listeners[i].type.text,args[0].text)&&b->listeners[i].callback.number==args[1].number){if(remove)b->listeners[i].active=0;return 1;}}
        if(remove)return 1;if(free_slot==b->listener_capacity){
            uint32_t capacity=b->listener_capacity?b->listener_capacity*2:64;
            if(capacity>0x100000u){vm->error="Event listener memory exhausted";return 0;}
            event_listener *listeners=envy_js_alloc(0,b->listeners,(int)(capacity*sizeof(event_listener)));
            if(!listeners){vm->error="Event listener memory exhausted";return 0;}
            b->listeners=listeners;for(uint32_t i=b->listener_capacity;i<capacity;i++)listeners[i].active=0;b->listener_capacity=capacity;}
        b->listeners[free_slot].target=target;b->listeners[free_slot].type=args[0];b->listeners[free_slot].callback=args[1];b->listeners[free_slot].active=1;b->listeners[free_slot].once=0;
        if(argc>2&&args[2].type==JS_OBJECT)b->listeners[free_slot].once=js_get(vm,args[2].number,"once").number!=0;return 1;
    }
    if(id==PREVENT_DEFAULT){if(b->event)js_set(vm,b->event,"defaultPrevented",(js_value){JS_BOOLEAN,1,0,0});return 1;}
    if(id==MEDIA_PAUSE)return 1;
    if(id==MEDIA_PLAY){const char *message="Audio playback is not available in the native kernel";*result=promise(vm,js_string(vm,message,count(message)),1);return !vm->error;}
    if(id==QUERY_SELECTOR||id==QUERY_ALL){
        if(argc!=1||args[0].type!=JS_STRING)return 0;js_value nodes=query_all(b,owner?owner:b->document,args[0].text);
        *result=id==QUERY_ALL?nodes:js_get(vm,nodes.number,"0");if(id==QUERY_SELECTOR&&result->type==JS_UNDEFINED)*result=(js_value){JS_NULL,0,0,0};return !vm->error;
    }
    if(id==SET_INTERVAL||id==SET_TIMEOUT) {
        if(argc<2||args[0].type!=JS_FUNCTION||args[1].type!=JS_NUMBER||args[1].number<0)return 0;
        uint32_t i=0;while(i<32&&b->timers[i].active)i++;
        if(i==32){vm->error="Timer storage exhausted";return 0;}
        int64_t interval=args[1].number?args[1].number:1;
        b->timers[i].callback=args[0];b->timers[i].interval=interval;b->timers[i].due=b->clock_ms+interval;b->timers[i].active=1;b->timers[i].repeat=id==SET_INTERVAL;
        *result=(js_value){JS_NUMBER,(int32_t)i+1,0,0};return 1;
    }
    if(id==CLEAR_TIMER){if(argc&&args[0].type==JS_NUMBER&&args[0].number>0&&args[0].number<=32)b->timers[args[0].number-1].active=0;return 1;}
    const char *date_methods[]={"getHours","getMinutes","getDate","getMonth","getDay"};
    if(id==DATE) {
        if(argc){vm->error="Date arguments are not supported yet";return 0;}
        uint8_t values[5];if(!clock_snapshot(values)){vm->error="Cannot read PC real-time clock";return 0;}
        *result=js_object(vm);
        for(uint32_t i=0;i<5;i++) {
            js_value method=js_native_bound(DATE_BASE+(int)i,(uint32_t)result->number);
            js_set(vm,result->number,date_methods[i],method);
            char key[2]={(char)('0'+i),0};js_set(vm,result->number,key,(js_value){JS_NUMBER,values[i],0,0});
        }
        return !vm->error;
    }
    if(id>=DATE_BASE&&id<DATE_BASE+5) {
        uint32_t method=(uint32_t)(id-DATE_BASE);
        char key[2]={(char)('0'+method),0};*result=js_get(vm,owner,key);return 1;
    }
    if(id==GET_BY_ID) {
        if(argc!=1||args[0].type!=JS_STRING) return 0;
        *result=get_by_id(b,args[0].text);return !vm->error;
    }
    uint32_t parent=0;
    if(id==INSERT_BASE) {parent=owner;id=INSERT;}
    else if(id==APPEND_BASE) {parent=owner;id=APPEND;}
    /* Local CDFS requests complete synchronously; callbacks retain lexical scope. */
    if(id==FETCH) {
        if(!argc||args[0].type!=JS_STRING) return 0;
        char path[256];cdfs_node node;
        if(!local_path(args[0].text,path)) {
            const char *message="Only local CDFS fetch is supported";
            *result=promise(vm,js_string(vm,message,count(message)),1);return !vm->error;
        }
        int head=0;
        if(argc>1) {
            if(args[1].type!=JS_OBJECT) return 0;
            js_value method=js_get(vm,args[1].number,"method");
            if(method.type==JS_STRING&&equal(method.text,"HEAD"))head=1;
            else if(method.type!=JS_UNDEFINED&&(method.type!=JS_STRING||!equal(method.text,"GET"))){const char *message="Local fetch supports GET and HEAD requests";*result=promise(vm,js_string(vm,message,count(message)),1);return !vm->error;}
        }
        int exists=!cached_lookup(b,path,&node)&&!(node.flags&2);
        js_value response=js_object(vm);
        js_set(vm,response.number,"ok",(js_value){JS_BOOLEAN,exists,0,0});
        js_set(vm,response.number,"status",(js_value){JS_NUMBER,exists?200:404,0,0});
        char *body="";uint32_t body_size=0;
        if(exists&&!head&&!load(b,path,&body,&body_size)){const char *message=b->load_error?b->load_error:"Cannot read local response";*result=promise(vm,js_string(vm,message,count(message)),1);return !vm->error;}
        js_set(vm,response.number,"__body",(js_value){JS_STRING,0,body,body_size});js_set(vm,response.number,"bodyUsed",(js_value){JS_BOOLEAN,0,0,0});
        js_value text=js_native_bound(RESPONSE_TEXT_BASE,(uint32_t)response.number),json=js_native_bound(RESPONSE_JSON_BASE,(uint32_t)response.number);
        js_set(vm,response.number,"text",text);js_set(vm,response.number,"json",json);
        *result=promise(vm,response,0);return !vm->error;
    }
    if(id==THEN_BASE||id==CATCH_BASE) {
        uint32_t object=owner;
        int rejected=js_get(vm,object,"rejected").number;
        js_value value=js_get(vm,object,"value");
        uint32_t callback=rejected&&id==THEN_BASE&&argc>1?1:0;
        if(argc>callback&&args[callback].type==JS_FUNCTION&&(rejected==(id==CATCH_BASE)||callback==1)) {
            js_value returned;
            if(js_call_args(vm,args[callback],&value,1,&returned)) {value=returned;rejected=0;
                if(returned.type==JS_OBJECT&&js_get(vm,returned.number,"__promise").number){value=js_get(vm,returned.number,"value");rejected=js_get(vm,returned.number,"rejected").number;}}
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
            else if(args[i].type==JS_NUMBER) {js_value text=js_number_string(vm,args[i].number);if(vm->error)return 0;b->write(text.text);}
            else b->write("undefined");
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
            success=eval_file(vm,source,length,path);
            if(!success) diagnostic(b,path,vm->error,vm->error_line);
        } else diagnostic(b,path[0]?path:"(script source)",b->load_error,0);
        if(!success) b->script_failures++;
        /* Script exceptions dispatch the page's onerror handler, not fake success. */
        js_value handler=js_get(vm,node,success?"onload":"onerror"); vm->error=0;
        if(handler.type==JS_FUNCTION && !js_call(vm,handler)) return 0;
    } else {
        js_value text=js_get(vm,node,"textContent");
        js_value markup=js_get(vm,node,"innerHTML");
        if(parent==b->body&&connected(b,node)&&tag.type==JS_STRING) {
            js_value style=js_get(vm,node,"style"), css={0,0,0,0};
            if(style.type==JS_OBJECT) css=js_get(vm,style.number,"cssText");
            js_value id_value=js_get(vm,node,"id"),classes=js_get(vm,node,"className");
            html_view_node(node,tag.text,id_value.type==JS_STRING?id_value.text:0,classes.type==JS_STRING?classes.text:0,
                text.type==JS_STRING?text.text:0,markup.type==JS_STRING?markup.text:0,css.type==JS_STRING?css.text:0);
            js_value owner=js_get(vm,node,"parentNode"),owner_id=owner.type==JS_OBJECT?js_get(vm,owner.number,"id"):(js_value){0,0,0,0};
            if(owner_id.type==JS_STRING&&owner_id.length)html_view_parent(node,owner_id.text);
            if(equal(tag.text,"img")){
                js_value src=js_get(vm,node,"src"),width=js_get(vm,node,"width"),height=js_get(vm,node,"height");
                html_view_image(node,src.type==JS_STRING?src.text:0,width.type==JS_NUMBER&&width.number>0?(uint32_t)width.number:0,height.type==JS_NUMBER&&height.number>0?(uint32_t)height.number:0);
            }
        }
    }
    if(connected(b,node))html_view_repaint();
    return !vm->error;
}
static void initialize(cdfs *fs,void (*write)(const char *)) {
    browser *b=&page; b->fs=fs; b->fallback_write=write; b->write=trace; b->source_used=b->script_failures=0;
    b->document_html=0;
    b->file_count=b->source_reads=b->source_cache_hits=0;
    css_init();
    font_init(fs);
    images_init(fs);
    html_view_text_lookup(live_text,b);
    html_view_style_lookup(live_style,b);html_view_markup_lookup(live_markup,b);
    error_used=0;errors[0]=0;recording_error=console_open=0;
    control_keys=extended_key=end_down=pause_bytes=0;
    js_init(&b->vm,api,b);
    b->dom_dirty=0;b->dom_used=0;b->dom_paint=0;b->vm.read_property=property_read;b->vm.changed_property=property_changed;b->vm.module_source=local_module;
    js_value document=js_object(&b->vm), body=element(&b->vm,"body"), head=element(&b->vm,"head"), console=js_object(&b->vm);
    js_value window=js_object(&b->vm);b->document=(uint32_t)document.number;b->window=(uint32_t)window.number;
    b->event=b->focused=0;
    js_set(&b->vm,0,"window",window);js_set(&b->vm,window.number,"document",document);
    js_set(&b->vm,document.number,"addEventListener",js_native_bound(ADD_EVENT_BASE,(uint32_t)document.number));js_set(&b->vm,document.number,"removeEventListener",js_native_bound(REMOVE_EVENT_BASE,(uint32_t)document.number));
    js_set(&b->vm,window.number,"addEventListener",js_native_bound(ADD_EVENT_BASE,(uint32_t)window.number));js_set(&b->vm,window.number,"removeEventListener",js_native_bound(REMOVE_EVENT_BASE,(uint32_t)window.number));
    for(uint32_t i=0;i<b->listener_capacity;i++)b->listeners[i].active=0;
    b->body=(uint32_t)body.number; b->head=(uint32_t)head.number;
    js_set(&b->vm,0,"document",document); js_set(&b->vm,0,"console",console);
    js_set(&b->vm,0,"fetch",js_native(FETCH));
    js_set(&b->vm,0,"Date",js_native(DATE));
    js_set(&b->vm,js_property_owner(js_native(DATE)),"now",js_native(DATE_NOW));
    js_set(&b->vm,0,"setInterval",js_native(SET_INTERVAL));js_set(&b->vm,0,"setTimeout",js_native(SET_TIMEOUT));
    js_set(&b->vm,0,"clearInterval",js_native(CLEAR_TIMER));js_set(&b->vm,0,"clearTimeout",js_native(CLEAR_TIMER));
    for(uint32_t i=0;i<32;i++)b->timers[i].active=0;
    b->seconds=b->pit_cycles=0;b->last_second=rtc_read(0);
    out8(0x43,0);b->last_pit=in8(0x40);b->last_pit|=(uint16_t)in8(0x40)<<8;
    b->clock_counter=b->last_pit;b->clock_remainder=0;b->clock_ms=clock_epoch();
    js_set(&b->vm,document.number,"body",body); js_set(&b->vm,document.number,"head",head);
    js_set(&b->vm,document.number,"activeElement",body);
    js_set(&b->vm,document.number,"createElement",js_native(CREATE));
    js_set(&b->vm,document.number,"getElementById",js_native(GET_BY_ID));
    js_set(&b->vm,document.number,"querySelector",js_native_bound(QUERY_SELECTOR,(uint32_t)document.number));
    js_set(&b->vm,document.number,"querySelectorAll",js_native_bound(QUERY_ALL,(uint32_t)document.number));
    js_set(&b->vm,console.number,"log",js_native(LOG)); js_set(&b->vm,console.number,"error",js_native(CONSOLE_ERROR));
}
int browser_start(cdfs *fs,void (*write)(const char *)) {
    initialize(fs,write);browser *b=&page;
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
    html_view_batch_begin();
    while((script=find(script,"<script"))) {
        const char *open=find(script,">"); if(!open) { write("Malformed script tag\n");html_view_batch_end();return 0; }
        const char *close=find(open+1,"</script>"); if(!close) { write("Missing script closing tag\n");html_view_batch_end();return 0; }
        if(!js_eval(&b->vm,open+1,(uint32_t)(close-open-1))) {
            diagnostic(b,"/Envy/index.html",b->vm.error,b->vm.error_line);html_view_batch_end();return 0;
        }
        script=close+9;
    }
    if(b->script_failures) b->write("Browser increment: WebUI compatibility incomplete.\n");
    else {b->write("Document scripts completed.\n");html_view_status("Local page loaded");}
    html_view_repaint();html_view_batch_end();return b->script_failures==0;
}
#ifdef BOOT_IMAGE_TESTS
static uint32_t visual_signature(browser *b){
    uint32_t hash=2166136261u;
    /* Visit visible properties and connected styles without indexing by ID. */
    for(uint32_t pass=0;pass<=b->vm.property_count;pass++) {
        uint32_t style=0;
        if(pass){js_property *link=&b->vm.properties[pass-1];
            if(!equal(link->name,"style")||link->value.type!=JS_OBJECT||!connected(b,link->object))continue;
            style=(uint32_t)link->value.number;}
        for(uint32_t i=0;i<b->vm.property_count;i++){js_property *p=&b->vm.properties[i];
            if(style){if(p->object!=style)continue;}
            else if(!(equal(p->name,"innerText")||equal(p->name,"textContent")||equal(p->name,"innerHTML")||equal(p->name,"className")||equal(p->name,"src")||equal(p->name,"width")||equal(p->name,"height"))||!connected(b,p->object))continue;
            hash=(hash^p->object)*16777619u;for(const char *key=p->name;*key;key++)hash=(hash^(uint8_t)*key)*16777619u;
            hash=(hash^(uint32_t)p->value.type)*16777619u;
            if(p->value.type==JS_STRING){for(uint32_t n=0;n<p->value.length;n++)hash=(hash^(uint8_t)p->value.text[n])*16777619u;}
            else hash=(hash^(uint32_t)p->value.number)*16777619u;
        }
    }
    return hash;
}
#endif
static void run_timers(browser *b){
    html_view_batch_begin();
    for(uint32_t i=0;i<32;i++)if(b->timers[i].active&&b->clock_ms>=b->timers[i].due) {
        if(b->timers[i].repeat)b->timers[i].due=b->clock_ms+b->timers[i].interval;else b->timers[i].active=0;
        if(!js_call(&b->vm,b->timers[i].callback)){diagnostic(b,"(timer)",b->vm.error,b->vm.error_line);b->timers[i].active=0;}
    }
    if(!js_full_jobs(&b->vm))diagnostic(b,"(async)",b->vm.error,b->vm.error_line);
    flush_dom(b);html_view_batch_end();console_sync();
}
void browser_poll(void) {
    browser *b=&page;out8(0x43,0);uint16_t counter=in8(0x40);counter|=(uint16_t)in8(0x40)<<8;
    clock_tick(b,counter);
    b->pit_cycles+=(uint16_t)(b->last_pit-counter);b->last_pit=counter;
    if(b->pit_cycles<11932)return;b->pit_cycles=0;
    uint8_t second=rtc_read(0);
    uint8_t old=b->last_second;b->last_second=second;
    if(!(rtc_read(11)&4)){old=(uint8_t)((old>>4)*10+(old&15));second=(uint8_t)((second>>4)*10+(second&15));}
    b->seconds+=(second+60u-old)%60u;
    run_timers(b);
}
static int dispatch_event(browser *b,const char *type,const input_event *input){
    uint32_t interested=0;for(uint32_t i=0;i<b->listener_capacity;i++)if(b->listeners[i].active&&equal(b->listeners[i].type.text,type))interested++;
    if(!interested)return 0;js_vm *vm=&b->vm;vm->error=0;
    if(!b->event){js_value event=js_object(vm);if(vm->error){diagnostic(b,"(event)",vm->error,0);vm->error=0;return 0;}b->event=(uint32_t)event.number;js_set(vm,b->event,"preventDefault",js_native(PREVENT_DEFAULT));}
    js_value event={JS_OBJECT,(int32_t)b->event,0,0};js_set(vm,b->event,"type",(js_value){JS_STRING,0,type,count(type)});
    uint32_t target=input->type==INPUT_KEY&&b->focused?b->focused:b->body;const char *target_id=input->type==INPUT_POINTER?html_view_target(input->x,input->y):"";
    uint32_t rendered=input->type==INPUT_POINTER?html_view_target_node(input->x,input->y):0;
    if(rendered)target=rendered;
    else if(*target_id){js_value found=get_by_id(b,target_id);if(found.type==JS_OBJECT)target=(uint32_t)found.number;}
    js_set(vm,b->event,"target",(js_value){JS_OBJECT,(int32_t)target,0,0});js_set(vm,b->event,"defaultPrevented",(js_value){JS_BOOLEAN,0,0,0});
    js_set(vm,b->event,"clientX",(js_value){JS_NUMBER,input->x,0,0});js_set(vm,b->event,"clientY",(js_value){JS_NUMBER,input->y,0,0});
    js_set(vm,b->event,"buttons",(js_value){JS_NUMBER,input->buttons,0,0});js_set(vm,b->event,"keyCode",(js_value){JS_NUMBER,input->key,0,0});
    js_set(vm,b->event,"deltaX",(js_value){JS_NUMBER,input->modifiers&INPUT_SHIFT?input->wheel*48:0,0,0});
    js_set(vm,b->event,"deltaY",(js_value){JS_NUMBER,input->modifiers&INPUT_SHIFT?0:input->wheel*48,0,0});
    js_set(vm,b->event,"deltaMode",(js_value){JS_NUMBER,0,0,0});
    event_listener *callbacks=envy_js_alloc(0,0,(int)(interested*sizeof(event_listener)));if(!callbacks)return 0;uint32_t count_callbacks=0;
    for(uint32_t i=0;i<b->listener_capacity;i++)if(b->listeners[i].active&&equal(b->listeners[i].type.text,type)){
        int bubbles=b->listeners[i].target==b->document||b->listeners[i].target==b->window;uint32_t ancestor=target;
        for(uint32_t depth=0;ancestor&&depth<=vm->objects;depth++){if(ancestor==b->listeners[i].target){bubbles=1;break;}js_value parent=js_get(vm,ancestor,"parentNode");ancestor=parent.type==JS_OBJECT?(uint32_t)parent.number:0;}
        if(!bubbles)continue;
        callbacks[count_callbacks++]=b->listeners[i];if(b->listeners[i].once)b->listeners[i].active=0;}
    for(uint32_t i=0;i<count_callbacks;i++){js_value result;js_set(vm,b->event,"currentTarget",(js_value){JS_OBJECT,(int32_t)callbacks[i].target,0,0});
        if(!js_call_args(vm,callbacks[i].callback,&event,1,&result)){diagnostic(b,"(event listener)",vm->error,vm->error_line);vm->error=0;}}
    envy_js_alloc(0,callbacks,0);
    return js_get(vm,b->event,"defaultPrevented").number!=0;
}
void browser_input(const input_event *event) {
    html_view_batch_begin();
    if(event->type==INPUT_KEY) {
        browser_key(event->modifiers&INPUT_CONTROL?0x1d:0x9d);
        if(event->key&0x100)browser_key(0xe0);browser_key(event->scancode);
        if(!console_open){int prevented=dispatch_event(&page,event->pressed?"keydown":"keyup",event);
            if(event->pressed&&!prevented&&!(event->modifiers&(INPUT_CONTROL|INPUT_ALT))){
                int32_t dx=0,dy=0;const framebuffer *fb=framebuffer_get();int32_t step=fb?(int32_t)fb->height-48:480;
                if(event->key==0x150)dy=48;else if(event->key==0x148)dy=-48;
                else if(event->key==0x14d)dx=48;else if(event->key==0x14b)dx=-48;
                else if(event->key==0x151)dy=step;else if(event->key==0x149)dy=-step;
                else if(event->key==0x147)dy=-2147483647;else if(event->key==0x14f)dy=2147483647;
                if(dx||dy)html_view_scroll(pointer_screen_x,pointer_screen_y,dx,dy);
            }}
    } else if(event->type==INPUT_POINTER) {
        pointer_screen_x=event->x;pointer_screen_y=event->y;
        if(!console_open)html_view_pointer(event->x,event->y,event->buttons);
        framebuffer_cursor_move(event->x,event->y);
        if(event->wheel&&console_open)console_scroll((int32_t)event->wheel*3);
        if(event->wheel&&!console_open&&!dispatch_event(&page,"wheel",event))
            html_view_scroll(event->x,event->y,event->modifiers&INPUT_SHIFT?event->wheel*48:0,event->modifiers&INPUT_SHIFT?0:event->wheel*48);
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
    flush_dom(&page);html_view_batch_end();console_sync();
}
#ifdef BOOT_IMAGE_TESTS
static void test_write(const char *text){(void)text;}
static uint32_t test_pixel(int32_t x,int32_t y){const framebuffer *fb=framebuffer_get();uint32_t raw=0;
    const volatile uint8_t *p=fb->pixels+y*fb->pitch+x*fb->bytes_per_pixel;for(uint32_t i=0;i<fb->bytes_per_pixel;i++)raw|=(uint32_t)p[i]<<(i*8);
    return (((raw>>fb->red_shift)&255)<<16)|(((raw>>fb->green_shift)&255)<<8)|((raw>>fb->blue_shift)&255);}
int browser_self_test(cdfs *fs){
#define CHECK_BROWSER(condition,code) do{if(!(condition))return code;}while(0)
    initialize(fs,test_write);browser *b=&page;js_vm *vm=&b->vm;
    /* Exercise promises beyond every former method-ID range. */
    for(uint32_t i=0;i<65537;i++)CHECK_BROWSER(js_object(vm).type==JS_OBJECT,470);
    b->document_html="<body><div id='userland'></div></body>";
    html_view_begin("Desktop tests","/Envy/index.html");html_view_render(b->document_html);
    const char *test="var appCount=0;var appName='';var error='';fetch('./UI/desktopapps.json').then(response=>response.json()).then(data=>{appCount=data.length;appName=data[0].name;}).catch(reason=>{error=reason;});";
    CHECK_BROWSER(js_eval(vm,test,count(test)),450);
    CHECK_BROWSER(js_get(vm,0,"appCount").number==10&&equal(js_get(vm,0,"appName").text,"Calculator (Legacy)")&&js_get(vm,0,"error").length==0,451);
    test="var status=0;var body='x';fetch('./UI/missing.json').then(response=>{status=response.status;return response.text();}).then(text=>{body=text;});";
    CHECK_BROWSER(js_eval(vm,test,count(test))&&js_get(vm,0,"status").number==404&&js_get(vm,0,"body").length==0,452);
    test="var headOK=false;fetch('./UI/desktopapps.json',{method:'HEAD'}).then(response=>{headOK=response.ok;return response.text();}).then(text=>{body=text;});";
    CHECK_BROWSER(js_eval(vm,test,count(test))&&js_get(vm,0,"headOK").number&&js_get(vm,0,"body").length==0,453);
    uint32_t reads=b->source_reads;test="fetch('./UI/desktopapps.json').then(response=>response.text()).then(text=>{body=text;});";
    CHECK_BROWSER(js_eval(vm,test,count(test))&&b->source_reads==reads&&b->source_cache_hits>0&&js_get(vm,0,"body").length>0,466);
    test="var caught=false;fetch('./UI/desktopapps.json',{method:'POST'}).catch(reason=>{caught=true;});";
    CHECK_BROWSER(js_eval(vm,test,count(test))&&js_get(vm,0,"caught").number,475);
    /* Reset before exercising the unmodified desktop script and its app list. */
    initialize(fs,test_write);b->document_html="<body><div id='userland'></div></body>";
    html_view_begin("Desktop tests","/Envy/index.html");html_view_render(b->document_html);
    char *source;uint32_t size;
    const char *sheets[]={"/Envy/Styles/base.css","/Envy/Styles/cherrytree.css","/Envy/Styles/ewinbox.css"};
    for(uint32_t i=0;i<3;i++)CHECK_BROWSER(load(b,sheets[i],&source,&size)&&css_add_file(source,size,sheets[i]),457);
    js_set(vm,0,"loadScript",js_native(TEST_NOOP));
    CHECK_BROWSER(load(b,"/Envy/UI/desktop.js",&source,&size),454);
    reads=b->source_reads;char *cached_source;uint32_t cached_size;
    CHECK_BROWSER(load(b,"/Envy/UI/desktop.js",&cached_source,&cached_size)&&cached_source==source&&cached_size==size&&b->source_reads==reads,467);
    uint32_t paints=html_view_paint_count();html_view_batch_begin();int desktop_ok=js_eval(vm,source,size);html_view_batch_end();CHECK_BROWSER(desktop_ok,454);
    CHECK_BROWSER(html_view_paint_count()==paints+1,468);
    CHECK_BROWSER(js_get(vm,js_get(vm,0,"dappsData").number,"length").number==10,455);
    uint32_t rasterizations=font_rasterization_count();html_view_repaint();CHECK_BROWSER(font_rasterization_count()==rasterizations,469);
    input_event click={.type=INPUT_POINTER,.buttons=1,.x=300,.y=200};browser_input(&click);click.buttons=0;browser_input(&click);
    CHECK_BROWSER(js_get(vm,0,"hasClicked").number==1&&!vm->error,456);
    initialize(fs,test_write);b->document_html="<body><div id='sem' style='position:absolute;left:10px;top:10px;width:40px;height:40px;background:red'></div></body>";
    for(uint32_t i=0;i<65537;i++)CHECK_BROWSER(js_object(vm).type==JS_OBJECT,471);
    html_view_begin("Live CSS tests","/Envy/index.html");html_view_render(b->document_html);CHECK_BROWSER(test_pixel(20,20)==0xff0000,458);
    test="var tile=document.getElementById('sem');";
    CHECK_BROWSER(js_eval(vm,test,count(test)),459);uint32_t signature=visual_signature(b);
    test="tile.style.opacity='0';";
    CHECK_BROWSER(js_eval(vm,test,count(test))&&visual_signature(b)!=signature,476);
    test="tile.classList.add('high-id');var date=new Date();var hours=date.getHours();var child=document.createElement('span');tile.appendChild(child);function boundFactory(){var holder=document.createElement('div');return holder.appendChild;}var append=boundFactory();append(document.createElement('span'));";
    CHECK_BROWSER(js_eval(vm,test,count(test)),472);
    CHECK_BROWSER(js_get(vm,0,"hours").type==JS_NUMBER&&equal(js_get(vm,js_get(vm,0,"tile").number,"className").text,"high-id"),473);
    CHECK_BROWSER(js_get(vm,js_get(vm,0,"child").number,"parentNode").number==js_get(vm,0,"tile").number&&visual_signature(b)!=signature,474);
    html_view_repaint();CHECK_BROWSER(test_pixel(20,20)==0xffffff,460);
    test="tile.style.opacity='1';tile.style.backgroundColor='blue';";
    CHECK_BROWSER(js_eval(vm,test,count(test)),461);html_view_repaint();CHECK_BROWSER(test_pixel(20,20)==0x0000ff,462);
    test="var elementClicks=0;function clicked(e){elementClicks=elementClicks+1;e.preventDefault();}tile.addEventListener('click',clicked,{once:true});";
    CHECK_BROWSER(js_eval(vm,test,count(test)),463);click.x=click.y=20;click.buttons=1;browser_input(&click);click.buttons=0;browser_input(&click);click.buttons=1;browser_input(&click);click.buttons=0;browser_input(&click);
    CHECK_BROWSER(js_get(vm,0,"elementClicks").number==1&&!vm->error,464);
    initialize(fs,test_write);b->document_html="<body><div id='scroll' style='position:absolute;left:10px;top:10px;width:80px;height:40px;overflow:auto'><div style='height:40px;background:red'></div><div style='height:40px;background:blue'></div></div></body>";
    html_view_begin("Wheel events","/Envy/index.html");html_view_render(b->document_html);
    test="var wheelDelta=0;document.addEventListener('wheel',e=>{wheelDelta=e.deltaY;e.preventDefault();},{once:true});";
    CHECK_BROWSER(js_eval(vm,test,count(test)),490);
    input_event wheel={.type=INPUT_POINTER,.x=20,.y=20,.wheel=1};browser_input(&wheel);
    CHECK_BROWSER(js_get(vm,0,"wheelDelta").number==48&&test_pixel(50,20)==0xff0000,491);
    browser_input(&wheel);CHECK_BROWSER(test_pixel(50,20)==0x0000ff,492);
    input_event up={.type=INPUT_KEY,.key=0x149,.scancode=0x49,.pressed=1};browser_input(&up);
    CHECK_BROWSER(test_pixel(50,20)==0xff0000,493);
    test="var currentTime=Date.now();var timeText=`${currentTime}`;var constructorWorks=new Date();";
    CHECK_BROWSER(js_eval(vm,test,count(test))&&js_get(vm,0,"currentTime").number>1700000000000ll&&js_get(vm,0,"timeText").length==13&&js_get(vm,0,"constructorWorks").type==JS_OBJECT,503);
    test="var field=document.createElement('input');document.body.appendChild(field);field.focus();field.dataset.role='editor';var focused=document.activeElement===field;";
    CHECK_BROWSER(js_eval(vm,test,count(test))&&js_get(vm,0,"focused").number&&equal(js_get(vm,js_get(vm,js_get(vm,0,"field").number,"dataset").number,"role").text,"editor"),504);
    /* Exercise console navigation through the same keyboard and wheel input. */
    for(uint32_t i=0;i<20;i++)for(const char *p="A diagnostic line\n";*p;p++)remember(*p);
    console_toggle();CHECK_BROWSER(console_open&&console_first>0,550);
    uint32_t bottom=console_first;wheel.wheel=-1;browser_input(&wheel);
    CHECK_BROWSER(console_first==bottom-3&&!console_follow,551);
    input_event home={.type=INPUT_KEY,.key=0x147,.scancode=0x47,.pressed=1};browser_input(&home);
    CHECK_BROWSER(console_first==0,552);remember('X');remember('\n');console_sync();CHECK_BROWSER(console_first==0,553);
    input_event end={.type=INPUT_KEY,.key=0x14f,.scancode=0x4f,.pressed=1};browser_input(&end);
    CHECK_BROWSER(console_follow&&console_first>bottom,554);
    const framebuffer *fb=framebuffer_get();uint32_t panel_y=fb->height-96;
    b->document_html="<body><div style='position:fixed;left:0;top:0;width:100%;height:100%;background:#123456'></div></body>";html_view_render(b->document_html);console_sync();
    CHECK_BROWSER(test_pixel(0,(int32_t)panel_y)==0x101820,555);
    console_toggle();CHECK_BROWSER(!console_open&&test_pixel(0,(int32_t)panel_y)==0x123456,556);
    return 0;
}
int browser_test_scripts(cdfs *fs,const char *html,const char *const *paths,uint32_t path_count,const char *verification){
    initialize(fs,test_write);browser *b=&page;b->document_html=html;
    html_view_begin("Script integration tests","/Envy/index.html");html_view_render(html);
    html_view_batch_begin();int failure=0;
    for(uint32_t i=0;i<path_count;i++){char *source;uint32_t size;
        if(!load(b,paths[i],&source,&size)||!(count(paths[i])>4&&equal(paths[i]+count(paths[i])-4,".css")?css_add_file(source,size,paths[i]):eval_file(&b->vm,source,size,paths[i]))){
            diagnostic(b,paths[i],b->vm.error?b->vm.error:b->load_error,b->vm.error_line);failure=(int)i+1;break;}}
    flush_dom(b);html_view_batch_end();html_view_batch_begin();
    if(!failure&&(!js_eval(&b->vm,verification,count(verification))||!js_get(&b->vm,0,"__testPassed").number)){
        if(b->vm.error)diagnostic(b,"(verification)",b->vm.error,b->vm.error_line);failure=(int)path_count+1;}
    flush_dom(b);html_view_batch_end();return failure;
}
int browser_test_verify(const char *source){browser *b=&page;html_view_batch_begin();int ok=js_eval(&b->vm,source,count(source))&&js_get(&b->vm,0,"__testPassed").number;
    if(!ok&&b->vm.error)diagnostic(b,"(verification)",b->vm.error,b->vm.error_line);flush_dom(b);html_view_batch_end();return ok;}
void browser_test_advance(uint32_t ms){page.clock_ms+=ms;run_timers(&page);}
int browser_test_bounds(const char *selector,int32_t *x,int32_t *y,int32_t *w,int32_t *h){js_value nodes=query_all(&page,page.document,selector),node=js_get(&page.vm,nodes.number,"0");
    if(node.type!=JS_OBJECT)return 0;js_value id=js_get(&page.vm,node.number,"id");return html_view_bounds((uint32_t)node.number,id.type==JS_STRING?id.text:"",x,y,w,h);}
#endif
