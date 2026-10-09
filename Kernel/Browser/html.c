#include "html.h"
#include "css.h"
#include "font.h"
#include "images.h"
#include "../Runtime/Font/ttf.h"
#include "../Drivers/Video/framebuffer.h"
static int32_t cursor_y;
static uint32_t batch_depth,paints;static int paint_pending;
void html_view_batch_begin(void){batch_depth++;}
void html_view_batch_end(void){if(!batch_depth)return;if(!--batch_depth&&paint_pending){paint_pending=0;html_view_repaint();}}
uint32_t html_view_paint_count(void){return paints;}
static uint32_t viewport_width, viewport_height;
static struct {char id[128];uint32_t serial;int32_t x,y,max_x,max_y;} scrolls[2048];
static uint32_t scroll_count;
static int32_t page_x,page_y,page_max_x,page_max_y;
static int page_allow_x,page_allow_y;
static char lower(char c) { return c>='A'&&c<='Z'?(char)(c-'A'+'a'):c; }
static int same(const char *a,const char *b) { while(*a&&lower(*a)==lower(*b)){a++;b++;} return *a==*b; }
static int starts(const char *s,const char *prefix) {
    while(*prefix) { if(!*s||lower(*s++)!=lower(*prefix++)) return 0; } return 1;
}
static const char *find(const char *s,const char *needle) {
    for(;*s;s++) if(starts(s,needle)) return s; return 0;
}
static int space(char c) { return c==' '||c=='\t'||c=='\r'||c=='\n'; }
typedef struct {
    uint32_t foreground, background, margin, padding, border; int hidden;
    css_style computed;int32_t x,y,width,height,translate_y,z;int font_size,line_height,align,positioned;
} style;
enum { PAINT_RECT, PAINT_BACKGROUND, PAINT_GLYPH, PAINT_IMAGE, PAINT_BOX };
typedef struct {int type,font;int32_t x,y,z,left,top,right,bottom;uint32_t width,height,color,codepoint;uint8_t opacity;const css_style *background;const char *source;} paint_command;
/* Record a scene before painting it. Stable z-order keeps later wallpaper behind
   earlier overlays, including glyph coverage at the edges of the watermark. */
static paint_command commands[32768];static uint32_t order[32768],command_count;
static css_style paint_backgrounds[512];static char paint_sources[512][256];
static uint32_t background_count,source_count;static int collecting;static int32_t paint_z;
static int32_t paint_left,paint_top,paint_right=2147483647,paint_bottom=2147483647;static uint8_t paint_opacity=255;
static paint_command *command(int type){if(command_count==32768)return 0;paint_command *c=&commands[command_count++];*c=(paint_command){.type=type,.z=paint_z,.left=paint_left,.top=paint_top,.right=paint_right,.bottom=paint_bottom,.opacity=paint_opacity};return c;}
static void paint_box(const css_style *s,int32_t x,int32_t y,uint32_t width,uint32_t height){
    if(!width||!height||background_count==512||(!s->background_image[0]&&!s->gradient_count&&(!s->value[CSS_BACKGROUND_ALPHA]||s->value[CSS_BACKGROUND]<0)&&!s->value[CSS_BORDER]&&!s->value[CSS_BACKDROP_BLUR]&&s->value[CSS_BACKDROP_BRIGHTNESS]==1000))return;paint_command *c=command(PAINT_BOX);if(!c)return;
    paint_backgrounds[background_count]=*s;c->background=&paint_backgrounds[background_count++];c->x=x;c->y=y;c->width=width;c->height=height;
}
static int rounded_inside(int32_t x,int32_t y,int32_t w,int32_t h,int32_t r){
    if(x<0||y<0||x>=w||y>=h)return 0;if(r<=0)return 1;
    if(r>w/2)r=w/2;if(r>h/2)r=h/2;
    int32_t dx=x<r?r-1-x:x>=w-r?x-(w-r):0,dy=y<r?r-1-y:y>=h-r?y-(h-r):0;
    return dx*dx+dy*dy<=r*r;
}
static void render_box(const paint_command *c){
    const css_style *s=c->background;int32_t w=(int32_t)c->width,h=(int32_t)c->height;
    int32_t radius=css_resolve(s->value[CSS_RADIUS],w<h?w:h),border=s->value[CSS_BORDER];
    if(radius<0)radius=0;if(radius>1024)radius=1024;if(border<0)border=0;
    if(s->value[CSS_BACKDROP_BLUR]||s->value[CSS_BACKDROP_BRIGHTNESS]!=1000)
        framebuffer_backdrop(c->x,c->y,c->width,c->height,(uint32_t)s->value[CSS_BACKDROP_BLUR],(uint32_t)s->value[CSS_BACKDROP_BRIGHTNESS],radius);
    int32_t left=c->x<0?0:c->x,top=c->y<0?0:c->y,right=c->x+w,bottom=c->y+h;
    if(left<c->left)left=c->left;if(top<c->top)top=c->top;if(right>c->right)right=c->right;if(bottom>c->bottom)bottom=c->bottom;
    if(right>(int32_t)viewport_width)right=(int32_t)viewport_width;if(bottom>(int32_t)viewport_height)bottom=(int32_t)viewport_height;
    if(!radius&&!border&&!s->gradient_count&&s->value[CSS_BACKGROUND]>=0&&s->value[CSS_BACKGROUND_ALPHA]==255)framebuffer_rect(c->x,c->y,c->width,c->height,(uint32_t)s->value[CSS_BACKGROUND]);
    else if(border||s->gradient_count||(s->value[CSS_BACKGROUND]>=0&&s->value[CSS_BACKGROUND_ALPHA]))for(int32_t y=top;y<bottom;y++)for(int32_t x=left;x<right;x++){
        int32_t lx=x-c->x,ly=y-c->y;if(!rounded_inside(lx,ly,w,h,radius))continue;
        if(border&&!rounded_inside(lx-border,ly-border,w-border*2,h-border*2,radius-border)){
            framebuffer_blend(x,y,(uint32_t)s->value[CSS_BORDER_COLOR],(uint8_t)s->value[CSS_BORDER_ALPHA]);continue;}
        if(s->value[CSS_BACKGROUND]>=0)framebuffer_blend(x,y,(uint32_t)s->value[CSS_BACKGROUND],(uint8_t)s->value[CSS_BACKGROUND_ALPHA]);
        if(s->gradient_count){int horizontal=s->gradient_angle==90||s->gradient_angle==270;int32_t basis=horizontal?w:h,pos=horizontal?lx:ly;
            if(s->gradient_angle==0||s->gradient_angle==270)pos=basis-1-pos;
            int stop=0;while(stop+1<s->gradient_count&&pos>=css_resolve(s->gradient_position[stop+1],basis))stop++;
            int next=stop+1<s->gradient_count?stop+1:stop;int32_t a=css_resolve(s->gradient_position[stop],basis),b=css_resolve(s->gradient_position[next],basis);
            int32_t t=b>a?(pos-a)*255/(b-a):0;if(t<0)t=0;if(t>255)t=255;uint32_t rgb=0;
            for(int channel=0;channel<3;channel++){int32_t v=(s->gradient_color[stop]>>(channel*8))&255,u=(s->gradient_color[next]>>(channel*8))&255;rgb|=(uint32_t)(v+(u-v)*t/255)<<(channel*8);}
            framebuffer_blend(x,y,rgb,(uint8_t)(s->gradient_alpha[stop]+(s->gradient_alpha[next]-s->gradient_alpha[stop])*t/255));}
    }
    if(s->background_image[0]&&w>border*2&&h>border*2)images_background(s,c->x+border,c->y+border,(uint32_t)(w-border*2),(uint32_t)(h-border*2));
}
static void paint_rect(int32_t x,int32_t y,uint32_t width,uint32_t height,uint32_t color){
    if(!collecting){framebuffer_rect(x,y,width,height,color);return;}paint_command *c=command(PAINT_RECT);if(c){c->x=x;c->y=y;c->width=width;c->height=height;c->color=color;}}
static void paint_background(const css_style *s,int32_t x,int32_t y,uint32_t width,uint32_t height){
    if(!s->background_image[0]||!width||!height)return;
    if(!collecting){images_background(s,x,y,width,height);return;}if(background_count==512)return;paint_command *c=command(PAINT_BACKGROUND);
    if(c){paint_backgrounds[background_count]=*s;c->background=&paint_backgrounds[background_count++];c->x=x;c->y=y;c->width=width;c->height=height;}}
static void paint_glyph(int font,int32_t x,int32_t y,uint32_t cp,uint32_t size,uint32_t color){
    if(!collecting){font_draw(font,x,y,cp,size,color);return;}paint_command *c=command(PAINT_GLYPH);
    if(c){c->font=font;c->x=x;c->y=y;c->codepoint=cp;c->height=size;c->color=color;}}
static void paint_image(const char *source,int32_t x,int32_t y,uint32_t width,uint32_t height){
    if(!collecting){image_surface image;if(images_load(source,width,height,&image))images_draw(&image,x,y,width,height);return;}
    if(!source||source_count==512)return;paint_command *c=command(PAINT_IMAGE);if(!c)return;
    uint32_t n=0;while(source[n]&&n<255){paint_sources[source_count][n]=source[n];n++;}paint_sources[source_count][n]=0;
    c->source=paint_sources[source_count++];c->x=x;c->y=y;c->width=width;c->height=height;
}
static int after(uint32_t a,uint32_t b){return commands[a].z>commands[b].z||(commands[a].z==commands[b].z&&a>b);}
static void sift_order(uint32_t root,uint32_t count){
    while(root<count/2){uint32_t child=root*2+1;if(child+1<count&&after(order[child+1],order[child]))child++;
        if(!after(order[child],order[root]))return;uint32_t old=order[root];order[root]=order[child];order[child]=old;root=child;}}
static void paint_scene(void){
    for(uint32_t i=0;i<command_count;i++)order[i]=i;
    for(uint32_t i=command_count/2;i;i--)sift_order(i-1,command_count);
    for(uint32_t i=command_count;i>1;i--){uint32_t old=order[0];order[0]=order[i-1];order[i-1]=old;sift_order(0,i-1);}
    for(uint32_t i=0;i<command_count;i++){paint_command *c=&commands[order[i]];
        framebuffer_paint_region(c->left,c->top,(uint32_t)(c->right-c->left),(uint32_t)(c->bottom-c->top),c->opacity);
        if(c->type==PAINT_RECT)framebuffer_rect(c->x,c->y,c->width,c->height,c->color);
        else if(c->type==PAINT_BACKGROUND)images_background(c->background,c->x,c->y,c->width,c->height);
        else if(c->type==PAINT_GLYPH)font_draw(c->font,c->x,c->y,c->codepoint,c->height,c->color);
        else if(c->type==PAINT_BOX)render_box(c);
        else{image_surface image;if(images_load(c->source,c->width,c->height,&image))images_draw(&image,c->x,c->y,c->width,c->height);}
    }
    framebuffer_paint_reset();
}
typedef struct {uint32_t node;const char *tag,*id,*classes,*text,*markup,*inline_style,*image_source,*parent_id;uint32_t image_width,image_height;} view_node;
static view_node views[128];static uint32_t view_count;
static const char *document_html;
static style body_style;
static struct {int32_t x,y,width,height;char handler[256];} hits[128];static uint32_t hit_count;
int html_view_hit_test(int32_t x,int32_t y,char *handler,uint32_t capacity) {
    for(uint32_t i=hit_count;i;i--){uint32_t n=i-1;
        if(x<hits[n].x||y<hits[n].y||x>=hits[n].x+hits[n].width||y>=hits[n].y+hits[n].height)continue;
        uint32_t j=0;while(hits[n].handler[j]&&j+1<capacity){handler[j]=hits[n].handler[j];j++;}if(capacity)handler[j]=0;return 1;}
    return 0;
}
static const char *(*text_lookup)(void *,const char *);static void *text_context;
void html_view_text_lookup(const char *(*lookup)(void *,const char *),void *context){text_lookup=lookup;text_context=context;}
static void (*style_lookup)(void *,uint32_t,const char *,char *,uint32_t);static void *style_context;
static const char *(*markup_lookup)(void *,const char *);static void *markup_context;
void html_view_style_lookup(void (*lookup)(void *,uint32_t,const char *,char *,uint32_t),void *context){style_lookup=lookup;style_context=context;}
void html_view_markup_lookup(const char *(*lookup)(void *,const char *),void *context){markup_lookup=lookup;markup_context=context;}
static void append_live_style(uint32_t node,const char *id,char *css,uint32_t capacity){
    if(!style_lookup)return;uint32_t used=0;while(css[used])used++;if(used+2>=capacity)return;if(used)css[used++]=';';css[used]=0;
    style_lookup(style_context,node,id,css+used,capacity-used);
}
static uint32_t nonnegative(int32_t n){return n>0?(uint32_t)n:0;}
static void computed(style *s,const style *parent,const css_element *element,const char *inline_style) {
    css_compute(element,parent?&parent->computed:0,inline_style,viewport_width,viewport_height,&s->computed);
    int32_t *v=s->computed.value;int32_t width=parent?parent->width:(int32_t)viewport_width;
    s->foreground=(uint32_t)v[CSS_COLOR];s->background=(uint32_t)v[CSS_BACKGROUND];s->hidden=s->computed.hidden;
    s->margin=nonnegative(css_resolve(v[CSS_MARGIN_TOP],width));s->padding=nonnegative(css_resolve(v[CSS_PADDING_LEFT],width));s->border=nonnegative(v[CSS_BORDER]);
    s->font_size=v[CSS_FONT_SIZE];s->line_height=v[CSS_LINE_HEIGHT];s->align=v[CSS_ALIGN];s->positioned=v[CSS_POSITION]!=0;
    s->z=parent?parent->z:0;if(s->positioned&&v[CSS_Z_INDEX]!=CSS_AUTO)s->z+=v[CSS_Z_INDEX];paint_z=s->z;
    s->width=v[CSS_WIDTH]==CSS_AUTO?width:css_resolve(v[CSS_WIDTH],width);
    s->height=v[CSS_HEIGHT]==CSS_AUTO?0:css_resolve(v[CSS_HEIGHT],(int32_t)viewport_height);
    s->x=(parent?parent->x:0)+css_resolve(v[CSS_MARGIN_LEFT],width);s->y=cursor_y;
    if(s->positioned) {
        s->x=v[CSS_LEFT]!=CSS_AUTO?css_resolve(v[CSS_LEFT],width):v[CSS_RIGHT]!=CSS_AUTO?width-s->width-css_resolve(v[CSS_RIGHT],width):0;
        s->y=v[CSS_TOP]!=CSS_AUTO?css_resolve(v[CSS_TOP],(int32_t)viewport_height):v[CSS_BOTTOM]!=CSS_AUTO?(int32_t)viewport_height-s->height-css_resolve(v[CSS_BOTTOM],(int32_t)viewport_height):0;
    }
    s->x+=css_resolve(v[CSS_TRANSLATE_X],s->width);
    s->translate_y=v[CSS_TRANSLATE_Y]?v[CSS_TRANSLATE_Y]:parent?parent->translate_y:0;
    if(!s->hidden&&s->height>0&&s->width>0){
        int32_t top=s->y+css_resolve(s->translate_y,s->height);
        if((int32_t)s->background>=0)paint_rect(s->x,top,(uint32_t)s->width,(uint32_t)s->height,s->background);
        int32_t border=(int32_t)s->border;
        if(s->width>border*2&&s->height>border*2)paint_background(&s->computed,s->x+border,top+border,(uint32_t)(s->width-border*2),(uint32_t)(s->height-border*2));
    }
}
static const char *text_line(const char *p,int font,uint32_t size,int32_t available,int pre,int32_t *width) {
    const char *begin=p;*width=0;
    while(*p){const char *next=p;uint32_t cp=ttf_utf8(&next);if(cp=='\n'&&pre)break;
        int32_t advance=font_advance(font,cp,size);if(p!=begin&&*width+advance>available)break;
        *width+=advance;p=next;
    }
    return p;
}
static void draw(const char *text,style s,int pre) {
    if(s.hidden||!viewport_width) return;
    paint_z=s.z;
    char normalized[4096];uint32_t normalized_length=0;
    for(uint32_t i=0;text[i]&&normalized_length+1<sizeof(normalized);i++) {
        char c=text[i];if(c=='\r')continue;
        if((!pre||pre==2)&&space(c)&&!(pre==2&&c=='\n')){if(!normalized_length||normalized[normalized_length-1]==' '||normalized[normalized_length-1]=='\n')continue;c=' ';}
        if((unsigned char)c<32&&c!='\n')c=' ';
        normalized[normalized_length++]=c;
    }
    normalized[normalized_length]=0;text=normalized;
    uint32_t inset=s.padding+s.border;
    if(s.width<=(int32_t)(inset*2))return;
    int font=font_select(s.computed.font_family);int32_t available=s.width-(int32_t)inset*2;
    uint32_t lines=0;
    for(const char *p=text;*p;){int32_t width;const char *end=text_line(p,font,(uint32_t)s.font_size,available,pre,&width);lines++;p=end;if(*p=='\n'&&pre){p++;if(!*p)lines++;}}
    if(!lines)lines=1;
    uint32_t line_height=s.line_height>0?(uint32_t)s.line_height:(uint32_t)s.font_size+2;
    uint32_t height=lines*line_height+2*(s.padding+s.border);
    int32_t top=cursor_y+css_resolve(s.computed.value[CSS_MARGIN_TOP],s.width)+css_resolve(s.translate_y,(int32_t)height);
    if(!s.height){
        if((int32_t)s.background>=0||s.border)paint_rect(s.x+(int32_t)s.margin,top,(uint32_t)s.width-2*s.margin,height,s.border?(uint32_t)s.computed.value[CSS_BORDER_COLOR]:s.background);
        if((int32_t)s.background>=0)paint_rect(s.x+(int32_t)(s.margin+s.border),top+(int32_t)s.border,
            (uint32_t)s.width-2*(s.margin+s.border),height-2*s.border,s.background);
        if(s.width>(int32_t)(2*(s.margin+s.border))&&height>2*s.border)
            paint_background(&s.computed,s.x+(int32_t)(s.margin+s.border),top+(int32_t)s.border,(uint32_t)s.width-2*(s.margin+s.border),height-2*s.border);
    }
    int32_t y=top+(int32_t)(s.padding+s.border);
    for(const char *p=text;*p;){int32_t width;const char *end=text_line(p,font,(uint32_t)s.font_size,available,pre,&width);
        int32_t x=s.x+(int32_t)inset,spare=available-width;if(s.align)x+=s.align==1?spare/2:spare;
        if(y>=(int32_t)viewport_height)break;
        while(p<end){uint32_t cp=ttf_utf8(&p);paint_glyph(font,x,y,cp,(uint32_t)s.font_size,s.foreground);x+=font_advance(font,cp,(uint32_t)s.font_size);}
        if(*p=='\n'&&pre)p++;y+=(int32_t)line_height;
    }
    cursor_y=top+(int32_t)height+(int32_t)s.margin;
}
static void draw_image(const char *source,const char *alt,style s,uint32_t attribute_width,uint32_t attribute_height){
    if(s.hidden)return;
    paint_z=s.z;
    int32_t width=s.computed.value[CSS_WIDTH]==CSS_AUTO?(int32_t)attribute_width:s.width;
    int32_t height=s.computed.value[CSS_HEIGHT]==CSS_AUTO?(int32_t)attribute_height:s.height;
    if(width<0||height<0||width>8192||height>8192)return;
    image_surface image;
    if(!images_load(source,(uint32_t)width,(uint32_t)height,&image)){if(alt&&*alt)draw(alt,s,0);return;}
    if(!width&&!height){width=(int32_t)image.natural_width;height=(int32_t)image.natural_height;
        if(width>s.width&&s.width>0){height=height*s.width/width;width=s.width;}}
    else if(!width)width=(int32_t)image.natural_width*height/(int32_t)image.natural_height;
    else if(!height)height=(int32_t)image.natural_height*width/(int32_t)image.natural_width;
    if(width<=0||height<=0||width>8192||height>8192)return;
    int32_t inset=(int32_t)(s.padding+s.border),top=cursor_y+css_resolve(s.computed.value[CSS_MARGIN_TOP],s.width);
    top+=css_resolve(s.translate_y,height+inset*2);int32_t x=s.x+inset;
    if(s.align){int32_t spare=s.width-width-inset*2;x+=s.align==1?spare/2:spare;}
    if(s.border)paint_rect(x-(int32_t)s.border,top+inset-(int32_t)s.border,(uint32_t)width+2*s.border,(uint32_t)height+2*s.border,(uint32_t)s.computed.value[CSS_BORDER_COLOR]);
    if((int32_t)s.background>=0)paint_rect(x,top+inset,(uint32_t)width,(uint32_t)height,s.background);
    paint_image(source,x,top+inset,(uint32_t)width,(uint32_t)height);
    if(!s.positioned)cursor_y=top+height+inset*2+(int32_t)s.margin;
}
void html_view_begin(const char *title,const char *url) {
    (void)title; (void)url;
    const framebuffer *fb=framebuffer_get(); if(!fb) return;
    view_count=0;document_html=0;scroll_count=0;page_x=page_y=page_max_x=page_max_y=0;
    viewport_width=fb->width; viewport_height=fb->height; cursor_y=8;
    framebuffer_rect(0,0,fb->width,fb->height,0xffffff);
}
void html_view_status(const char *text) {
    (void)text; /* Runtime diagnostics stay on the debug console, outside the page. */
}
void html_view_element(const char *tag,const char *text,const char *inline_css) {
    if(!framebuffer_get()||!text||!tag) return;
    if(same(tag,"script")||same(tag,"style")) return;
    css_element element={tag,"","",0};style s;computed(&s,&body_style,&element,inline_css);draw(text,s,same(tag,"pre"));
}
static void attribute(const char *begin,const char *end,const char *wanted,char *value,uint32_t capacity) {
    value[0]=0;
    while(begin<end) {
        while(begin<end&&(space(*begin)||*begin=='/')) begin++;
        const char *key=begin; while(begin<end&&!space(*begin)&&*begin!='=') begin++;
        uint32_t n=(uint32_t)(begin-key), wanted_n=0; while(wanted[wanted_n]) wanted_n++;
        int match=n==wanted_n; for(uint32_t i=0;i<n&&match;i++) if(lower(key[i])!=wanted[i]) match=0;
        while(begin<end&&space(*begin)) begin++;
        if(begin==end||*begin!='=') continue;
        begin++; while(begin<end&&space(*begin)) begin++;
        char quote=0; if(begin<end&&(*begin=='\''||*begin=='"')) quote=*begin++;
        uint32_t used=0;
        while(begin<end&&(quote?*begin!=quote:!space(*begin))) {
            if(match&&used+1<capacity) value[used++]=*begin; begin++;
        }
        if(quote&&begin<end) begin++;
        if(match) {value[used]=0;return;}
    }
}
static uint32_t image_dimension(const char *text){uint32_t n=0;while(*text>='0'&&*text<='9'){n=n*10+(uint32_t)(*text++-'0');if(n>8192)return 0;}return n;}
/* Lookup skips raw script/style text and comments, and respects quoted attributes. */
int html_has_tag(const char *html,const char *tag){
    uint32_t size=0;while(tag[size]){char c=tag[size];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'))return 0;size++;}if(!size)return 0;
    while(*html){if(*html++!='<')continue;if(starts(html,"!--")){const char *end=find(html+3,"-->");if(!end)return 0;html=end+3;continue;}
        if(starts(html,tag)&&(html[size]=='>'||html[size]=='/'||space(html[size])))return 1;
        if(starts(html,"script")||starts(html,"style")){const char *end=find(html,starts(html,"script")?"</script>":"</style>");if(!end)return 0;html=find(end,">")+1;}
        else {char quote=0;while(*html){char c=*html++;if(quote){if(c==quote)quote=0;}else if(c=='\''||c=='"')quote=c;else if(c=='>')break;}}}
    return 0;
}
int html_find_element(const char *html,const char *id,char *tag,uint32_t capacity) {
    if(!id||!*id||!capacity) return 0;
    while(*html) {
        if(*html++!='<') continue;
        if(starts(html,"!--")) {
            const char *end=find(html+3,"-->");if(!end) return 0;html=end+3;continue;
        }
        const char *begin=html;char quote=0;
        while(*html) {
            if(quote) {if(*html==quote) quote=0;}
            else if(*html=='\''||*html=='"') quote=*html;
            else if(*html=='>') break;
            html++;
        }
        if(!*html) return 0;
        const char *end=html++;
        if(*begin=='/'||*begin=='!') continue;
        uint32_t used=0;
        while(begin<end&&!space(*begin)&&*begin!='/'&&used+1<capacity) tag[used++]=lower(*begin++);
        tag[used]=0;
        char value[256];attribute(begin,end,"id",value,sizeof(value));
        /* IDs are case sensitive even though HTML tag and attribute names aren't. */
        uint32_t i=0;while(value[i]&&id[i]&&value[i]==id[i]) i++;
        if(!value[i]&&!id[i]) return 1;
        if(same(tag,"script")||same(tag,"style")) {
            const char *close=find(html,same(tag,"script")?"</script>":"</style>");
            if(!close) return 0;html=find(close,">")+1;
        }
    }
    return 0;
}
/* Parse once per markup fragment, then measure boxes before placing/painting.
   This keeps empty containers, nested padding and auto-height flex rows real. */
typedef struct {
    css_element element;css_style css;
    char tag[16],id[128],classes[256],inline_css[512],source[256],handler[256];
    const char *text;int first,last,next,parent;uint32_t serial,object;
    int32_t x,y,w,h,padding[4],margin[4],scroll_x,scroll_y;uint32_t image_width,image_height;
} layout_node;
static layout_node layout_nodes[1024];static uint32_t layout_count;
static char layout_text[131072];static uint32_t layout_text_used;
static struct {int32_t x,y,w,h,z;uint32_t serial,parent,object;int scroll;char id[128];} boxes[2048];
static uint32_t box_count,serial_next,hover_path[32],hover_count;static uint8_t hover_buttons;
static int state_matches(const css_element *element,const char *state){
    if(state[0]=='a'&&!(hover_buttons&1))return 0;
    for(uint32_t i=0;i<layout_count;i++)if(element==&layout_nodes[i].element){for(uint32_t j=0;j<hover_count;j++)if(layout_nodes[i].serial==hover_path[j])return 1;break;}return 0;
}
static int pointer_box(int32_t x,int32_t y){int found=-1;
    for(uint32_t i=0;i<box_count;i++)if(x>=boxes[i].x&&y>=boxes[i].y&&x<boxes[i].x+boxes[i].w&&y<boxes[i].y+boxes[i].h&&(found<0||boxes[i].z>=boxes[found].z))found=(int)i;return found;}
const char *html_view_target(int32_t x,int32_t y){int i=pointer_box(x,y);for(uint32_t depth=0;i>=0&&depth<32;depth++){
    if(boxes[i].id[0])return boxes[i].id;uint32_t parent=boxes[i].parent;i=-1;for(uint32_t j=0;j<box_count;j++)if(boxes[j].serial==parent){i=(int)j;break;}}return "";}
uint32_t html_view_target_node(int32_t x,int32_t y){int i=pointer_box(x,y);for(uint32_t depth=0;i>=0&&depth<32;depth++){
    if(boxes[i].object)return boxes[i].object;uint32_t parent=boxes[i].parent;i=-1;for(uint32_t j=0;j<box_count;j++)if(boxes[j].serial==parent){i=(int)j;break;}}return 0;}
int html_view_bounds(uint32_t node,const char *id,int32_t *x,int32_t *y,int32_t *w,int32_t *h){
    for(uint32_t i=0;i<box_count;i++)if((node&&boxes[i].object==node)||(id&&*id&&same(boxes[i].id,id))){*x=boxes[i].x;*y=boxes[i].y;*w=boxes[i].w;*h=boxes[i].h;return 1;}return 0;}
int html_view_geometry(uint32_t node,const char *id,int32_t *x,int32_t *y,int32_t *w,int32_t *h){
    for(uint32_t i=0;i<layout_count;i++){layout_node *n=&layout_nodes[i];
        if(!((node&&n->object==node)||(id&&*id&&same(n->id,id))))continue;
        for(int j=(int)i;j>=0;j=layout_nodes[j].parent)if(layout_nodes[j].css.hidden)return 0;
        *x=n->x;*y=n->y;*w=n->w;*h=n->h;return 1;}return 0;}
int html_view_pointer(int32_t x,int32_t y,uint8_t buttons){
    uint32_t path[32],count=0;int i=pointer_box(x,y);while(i>=0&&count<32){path[count++]=boxes[i].serial;uint32_t parent=boxes[i].parent;i=-1;for(uint32_t j=0;j<box_count;j++)if(boxes[j].serial==parent){i=(int)j;break;}}
    int changed=hover_count!=count||((hover_buttons^buttons)&1);for(uint32_t j=0;j<count;j++){if(hover_path[j]!=path[j])changed=1;hover_path[j]=path[j];}hover_count=count;hover_buttons=buttons;
    if(changed){/* Moving between boxes only needs paint when the cascade changes. */
        for(uint32_t j=1;j<layout_count;j++){layout_node *n=&layout_nodes[j];if(same(n->tag,"#text"))continue;css_style computed;
            css_compute(&n->element,&layout_nodes[n->parent].css,n->inline_css,viewport_width,viewport_height,&computed);
            int different=computed.hidden!=n->css.hidden||!same(computed.font_family,n->css.font_family)||!same(computed.background_image,n->css.background_image);
            for(uint32_t k=0;k<CSS_PROPERTIES;k++)if(computed.value[k]!=n->css.value[k])different=1;
            if(different){html_view_repaint();return 1;}
        }
    }return 0;
}
static int32_t max_i(int32_t a,int32_t b){return a>b?a:b;}
static int32_t min_i(int32_t a,int32_t b){return a<b?a:b;}
static int scroll_position(int32_t *position,int32_t delta,int32_t maximum){
    int64_t next=(int64_t)*position+delta;if(next<0)next=0;if(next>maximum)next=maximum;
    if(*position==(int32_t)next)return 0;*position=(int32_t)next;return 1;
}
int html_view_scroll(int32_t x,int32_t y,int32_t dx,int32_t dy){
    int i=pointer_box(x,y),changed=0;
    for(uint32_t depth=0;i>=0&&depth<32;depth++){
        int state=boxes[i].scroll;
        if(state>=0){int moved_x=scroll_position(&scrolls[state].x,dx,scrolls[state].max_x);
            int moved_y=scroll_position(&scrolls[state].y,dy,scrolls[state].max_y);
            changed|=moved_x|moved_y;if(moved_x)dx=0;if(moved_y)dy=0;if(!dx&&!dy)break;}
        uint32_t parent=boxes[i].parent;i=-1;for(uint32_t j=0;j<box_count;j++)if(boxes[j].serial==parent){i=(int)j;break;}
    }
    if(page_allow_x)changed|=scroll_position(&page_x,dx,page_max_x);
    if(page_allow_y)changed|=scroll_position(&page_y,dy,page_max_y);
    if(changed)html_view_repaint();return changed;
}
static int add_node(int parent,const char *tag){
    if(layout_count==1024)return -1;int index=(int)layout_count++;layout_node *n=&layout_nodes[index];
    *n=(layout_node){.first=-1,.last=-1,.next=-1,.parent=parent,.serial=++serial_next};
    uint32_t i=0;while(tag[i]&&i<15){n->tag[i]=tag[i];i++;}n->tag[i]=0;
    n->element=(css_element){n->tag,n->id,n->classes,parent>=0?&layout_nodes[parent].element:0};
    if(parent>=0){layout_node *p=&layout_nodes[parent];if(p->last>=0)layout_nodes[p->last].next=index;else p->first=index;p->last=index;}
    return index;
}
static const char *store_text(const char *begin,const char *end){
    if(layout_text_used+1>=sizeof(layout_text))return "";char *out=layout_text+layout_text_used;uint32_t used=0;
    while(begin<end&&layout_text_used+used+1<sizeof(layout_text)){char c=*begin++;
        if(c=='&'){const char *entities[]={"amp;","lt;","gt;","quot;","apos;","nbsp;"};const char values[]="&<>\"' ";
            for(uint32_t i=0;i<6;i++){uint32_t size=0;while(entities[i][size])size++;
                if((uint32_t)(end-begin)>=size&&starts(begin,entities[i])){c=values[i];begin+=size;break;}}}
        out[used++]=c;}
    out[used]=0;layout_text_used+=used+1;return out;
}
static void inherit_text(layout_node *n,const css_style *parent){
    n->css=*parent;int32_t *v=n->css.value;
    v[CSS_POSITION]=0;v[CSS_Z_INDEX]=CSS_AUTO;v[CSS_TRANSLATE_X]=v[CSS_TRANSLATE_Y]=0;v[CSS_OPACITY]=255;
    v[CSS_GROW]=0;v[CSS_SHRINK]=1000;v[CSS_BASIS]=v[CSS_SELF]=CSS_AUTO;
    for(int i=CSS_MARGIN_TOP;i<=CSS_PADDING_LEFT;i++)v[i]=0;
}
static void parse_fragment(const char *html,int root,uint32_t recursion){
    if(recursion>16)return;int stack[32]={root};uint32_t depth=0;
    while(*html){if(starts(html,"</body"))break;
        if(*html!='<'){const char *begin=html;while(*html&&*html!='<')html++;const char *nonspace=begin;while(nonspace<html&&space(*nonspace))nonspace++;
            if(nonspace==html&&layout_nodes[stack[depth]].css.value[CSS_WHITE_SPACE]<2)continue;int text=add_node(stack[depth],"#text");if(text<0)break;
            layout_nodes[text].text=store_text(begin,html);inherit_text(&layout_nodes[text],&layout_nodes[stack[depth]].css);continue;}
        if(starts(html,"<!--")){const char *end=find(html+4,"-->");if(!end)break;html=end+3;continue;}
        const char *begin=++html;char quote=0;while(*html){if(quote){if(*html==quote)quote=0;}else if(*html=='\''||*html=='"')quote=*html;else if(*html=='>')break;html++;}
        if(!*html)break;const char *end=html++;int closing=*begin=='/';if(closing)begin++;
        char tag[16];uint32_t size=0;while(begin<end&&!space(*begin)&&*begin!='/'&&size<15)tag[size++]=lower(*begin++);tag[size]=0;
        if(closing){for(uint32_t d=depth;d;d--)if(same(layout_nodes[stack[d]].tag,tag)){depth=d-1;break;}continue;}
        if(same(tag,"script")||same(tag,"style")||same(tag,"head")){const char *close=find(html,same(tag,"script")?"</script>":same(tag,"style")?"</style>":"</head>");if(!close)break;html=find(close,">")+1;continue;}
        if(!tag[0]||tag[0]=='!'||same(tag,"meta")||same(tag,"link"))continue;
        int index=add_node(stack[depth],tag);if(index<0)break;layout_node *n=&layout_nodes[index];
        attribute(begin,end,"id",n->id,sizeof(n->id));attribute(begin,end,"class",n->classes,sizeof(n->classes));attribute(begin,end,"style",n->inline_css,sizeof(n->inline_css));
        attribute(begin,end,"onclick",n->handler,sizeof(n->handler));
        char identity[16];attribute(begin,end,"data-envy-node",identity,sizeof(identity));for(const char *p=identity;*p>='0'&&*p<='9';p++)n->object=n->object*10+(uint32_t)(*p-'0');
        append_live_style(n->object,n->id,n->inline_css,sizeof(n->inline_css));
        css_compute(&n->element,&layout_nodes[stack[depth]].css,n->inline_css,viewport_width,viewport_height,&n->css);
        if(same(tag,"pre")&&!n->css.value[CSS_WHITE_SPACE])n->css.value[CSS_WHITE_SPACE]=2;
        if(same(tag,"img")){char size_text[32];attribute(begin,end,"src",n->source,sizeof(n->source));
            attribute(begin,end,"width",size_text,sizeof(size_text));n->image_width=image_dimension(size_text);attribute(begin,end,"height",size_text,sizeof(size_text));n->image_height=image_dimension(size_text);}
        if(same(tag,"input")){char value[256];attribute(begin,end,"value",value,sizeof(value));const char *last=value;while(*last)last++;n->text=store_text(value,last);}
        const char *replacement=!n->object&&text_lookup&&n->id[0]?text_lookup(text_context,n->id):0;
        const char *replacement_markup=!n->object&&!replacement&&markup_lookup&&n->id[0]?markup_lookup(markup_context,n->id):0;
        if(replacement){int text=add_node(index,"#text");if(text>=0){layout_nodes[text].text=replacement;inherit_text(&layout_nodes[text],&n->css);}}
        else if(replacement_markup)parse_fragment(replacement_markup,index,recursion+1);
        if(same(tag,"img")||same(tag,"input")||same(tag,"br")||same(tag,"hr")||(end>begin&&end[-1]=='/'))continue;
        if(depth+1==32)break;stack[++depth]=index;
        if(replacement||replacement_markup){/* Consume the original subtree without creating duplicates. */
            int nesting=1;const char *scan=html;while(*scan&&nesting){if(*scan++!='<')continue;
                if(*scan=='/'){scan++;if(starts(scan,tag)&&(scan[size]=='>'||space(scan[size])))nesting--;}
                else if(starts(scan,tag)&&(scan[size]=='>'||space(scan[size])))nesting++;
                const char *close=find(scan,">");if(!close)break;scan=close+1;}
            html=scan;depth--;}
    }
}
static void parse_layout(const char *html,const style *parent,const css_element *parent_element){
    layout_count=layout_text_used=0;int root=add_node(-1,"body");layout_nodes[root].element=*parent_element;layout_nodes[root].css=parent->computed;
    const char *body=find(html,"<body");if(body){const char *end=find(body,">");if(!end)return;
        attribute(body+5,end,"style",layout_nodes[root].inline_css,sizeof(layout_nodes[root].inline_css));
        if(layout_nodes[root].inline_css[0])css_compute(parent_element,&parent->computed,layout_nodes[root].inline_css,viewport_width,viewport_height,&layout_nodes[root].css);html=end+1;}
    parse_fragment(html,root,0);int attached[128]={0};
    for(uint32_t i=1;i<layout_count;i++)if(layout_nodes[i].id[0])for(uint32_t j=0;j<view_count;j++){
        const view_node *view=&views[j];if(attached[j]||!view->parent_id||!same(view->parent_id,layout_nodes[i].id))continue;
        attached[j]=1;int child=add_node((int)i,view->tag);if(child<0)return;layout_node *n=&layout_nodes[child];
        uint32_t k=0;while(view->id[k]&&k+1<sizeof(n->id)){n->id[k]=view->id[k];k++;}n->id[k]=0;k=0;
        while(view->classes[k]&&k+1<sizeof(n->classes)){n->classes[k]=view->classes[k];k++;}n->classes[k]=0;k=0;
        if(view->inline_style){while(view->inline_style[k]&&k+1<sizeof(n->inline_css)){n->inline_css[k]=view->inline_style[k];k++;}n->inline_css[k]=0;}
        append_live_style(view->node,n->id,n->inline_css,sizeof(n->inline_css));css_compute(&n->element,&layout_nodes[i].css,n->inline_css,viewport_width,viewport_height,&n->css);
        n->text=view->text;n->image_width=view->image_width;n->image_height=view->image_height;
        if(view->image_source){k=0;while(view->image_source[k]&&k+1<sizeof(n->source)){n->source[k]=view->image_source[k];k++;}n->source[k]=0;}
        if(view->markup)parse_fragment(view->markup,child,0);
    }
}
static int32_t constrain(int32_t value,int32_t minimum,int32_t maximum,int32_t basis){
    if(maximum!=CSS_AUTO)value=min_i(value,css_resolve(maximum,basis));return max_i(value,max_i(0,css_resolve(minimum,basis)));
}
static uint32_t normalize_text(const char *text,char out[4096],int whitespace);
static int32_t text_width(const layout_node *n){
    char text[4096];normalize_text(n->text,text,n->css.value[CSS_WHITE_SPACE]);
    int font=font_select(n->css.font_family);int32_t width=0,line=0;const char *p=text;
    while(*p){uint32_t cp=ttf_utf8(&p);if(cp=='\n'){width=max_i(width,line);line=0;}else line+=font_advance(font,cp,(uint32_t)n->css.value[CSS_FONT_SIZE]);}return max_i(width,line);
}
static int32_t intrinsic_width(int index,int32_t basis,uint32_t depth){
    if(depth>32)return 0;layout_node *n=&layout_nodes[index];int32_t *v=n->css.value;if(n->css.hidden)return 0;
    if(same(n->tag,"#text"))return text_width(n);
    int32_t w=v[CSS_WIDTH]==CSS_AUTO?0:css_resolve(v[CSS_WIDTH],basis);
    if(same(n->tag,"img")){if(n->image_width)w=(int32_t)n->image_width;else if(!w){image_surface image;if(images_load(n->source,0,0,&image))w=(int32_t)image.natural_width;}}
    int32_t children=0;int row=v[CSS_DISPLAY]==CSS_FLEX&&!(v[CSS_FLEX_DIRECTION]&1);uint32_t count=0;
    for(int child=n->first;child>=0;child=layout_nodes[child].next){layout_node *c=&layout_nodes[child];if(c->css.hidden||c->css.value[CSS_POSITION]==1||c->css.value[CSS_POSITION]==2)continue;
        int32_t item=intrinsic_width(child,basis,depth+1);if(row)children+=item;else children=max_i(children,item);count++;}
    if(row&&count>1)children+=(int32_t)(count-1)*css_resolve(v[CSS_COLUMN_GAP],basis);
    if(v[CSS_WIDTH]==CSS_AUTO)w=max_i(w,children);
    int32_t extra=max_i(0,v[CSS_BORDER])*2+max_i(0,css_resolve(v[CSS_PADDING_LEFT],basis))+max_i(0,css_resolve(v[CSS_PADDING_RIGHT],basis));
    w=constrain(w,v[CSS_MIN_WIDTH],v[CSS_MAX_WIDTH],basis);if(!v[CSS_BOX_SIZING])w+=extra;return max_i(w,extra);
}
static uint32_t normalize_text(const char *text,char out[4096],int whitespace){
    uint32_t used=0;for(uint32_t i=0;text&&text[i]&&used<4095;i++){char c=text[i];if(c=='\r')continue;
        if(whitespace<2&&space(c)){if(!used||out[used-1]==' ')continue;c=' ';}out[used++]=c;}
    if(whitespace<2)while(used&&out[used-1]==' ')used--;out[used]=0;return used;
}
static int32_t text_height(const layout_node *n,int32_t width){
    char text[4096];if(!normalize_text(n->text,text,n->css.value[CSS_WHITE_SPACE]))return 0;
    int font=font_select(n->css.font_family),nowrap=n->css.value[CSS_WHITE_SPACE]==1||n->css.value[CSS_WHITE_SPACE]==2;
    int32_t lines=0;for(const char *p=text;*p;){int32_t used;const char *end=text_line(p,font,(uint32_t)n->css.value[CSS_FONT_SIZE],nowrap?2147483647:max_i(1,width),n->css.value[CSS_WHITE_SPACE]>=2,&used);lines++;p=end;if(*p=='\n')p++;}
    return lines*max_i(1,n->css.value[CSS_LINE_HEIGHT]);
}
static void measure_layout(int index,int32_t width,int32_t height,int shrink,int32_t force_w,int32_t force_h,uint32_t depth){
    if(depth>32)return;layout_node *n=&layout_nodes[index];int32_t *v=n->css.value;n->x=n->y=n->w=n->h=0;if(n->css.hidden)return;
    if(same(n->tag,"#text")){n->w=min_i(max_i(1,width),text_width(n));n->h=text_height(n,width);return;}
    int32_t border=max_i(0,v[CSS_BORDER]);for(int i=0;i<4;i++){n->padding[i]=max_i(0,css_resolve(v[CSS_PADDING_TOP+i],width))+border;n->margin[i]=v[CSS_MARGIN_TOP+i]==CSS_AUTO?0:css_resolve(v[CSS_MARGIN_TOP+i],width);}
    int32_t extra_w=n->padding[1]+n->padding[3],extra_h=n->padding[0]+n->padding[2];
    int32_t natural=intrinsic_width(index,width,0),w;
    if(force_w>=0)w=force_w;else if(v[CSS_WIDTH]!=CSS_AUTO)w=css_resolve(v[CSS_WIDTH],width)+(v[CSS_BOX_SIZING]?0:extra_w);
    else if(shrink||v[CSS_DISPLAY]==CSS_INLINE||v[CSS_DISPLAY]==CSS_INLINE_BLOCK)w=min_i(width,natural);
    else w=width-n->margin[1]-n->margin[3];
    int32_t min_width=css_resolve(v[CSS_MIN_WIDTH],width)+(v[CSS_BOX_SIZING]?0:extra_w),max_width=v[CSS_MAX_WIDTH]==CSS_AUTO?CSS_AUTO:css_resolve(v[CSS_MAX_WIDTH],width)+(v[CSS_BOX_SIZING]?0:extra_w);
    n->w=max_i(extra_w,constrain(w,min_width,max_width,width));
    int32_t content_w=max_i(0,n->w-extra_w),fixed_h=force_h>=0?force_h:v[CSS_HEIGHT]==CSS_AUTO?-1:css_resolve(v[CSS_HEIGHT],height)+(v[CSS_BOX_SIZING]?0:extra_h);
    if(same(n->tag,"img")){image_surface image;int32_t iw=(int32_t)n->image_width,ih=(int32_t)n->image_height;
        if(images_load(n->source,iw>0?(uint32_t)iw:0,ih>0?(uint32_t)ih:0,&image)){
            if(v[CSS_WIDTH]==CSS_AUTO&&force_w<0){iw=iw?iw:ih?((int32_t)image.natural_width*ih/(int32_t)image.natural_height):(int32_t)image.natural_width;n->w=constrain(iw,v[CSS_MIN_WIDTH],v[CSS_MAX_WIDTH],width)+extra_w;content_w=n->w-extra_w;}
            if(fixed_h<0)fixed_h=(ih?ih:(int32_t)image.natural_height*content_w/(int32_t)image.natural_width)+extra_h;}
    }
    int display=v[CSS_DISPLAY],column=v[CSS_FLEX_DIRECTION]&1,flex=display==CSS_FLEX,grid=display==CSS_GRID;
    int32_t gap_x=max_i(0,css_resolve(v[CSS_COLUMN_GAP],content_w)),gap_y=max_i(0,css_resolve(v[CSS_ROW_GAP],content_w));
    int32_t used=0,line_cross=0,cross=0;uint32_t count=0;int32_t grow=0,shrink_weight=0;
    int32_t content_h=fixed_h>=0?max_i(0,fixed_h-extra_h):height;
    for(int child=n->first;child>=0;child=layout_nodes[child].next){layout_node *c=&layout_nodes[child];int32_t *cv=c->css.value;
        if(cv[CSS_POSITION]==1||cv[CSS_POSITION]==2||c->css.hidden)continue;
        int32_t forced=-1;if(grid)forced=max_i(0,(content_w-(v[CSS_GRID_COLUMNS]-1)*gap_x)/v[CSS_GRID_COLUMNS]);
        else if(flex&&!column&&cv[CSS_BASIS]!=CSS_AUTO)forced=css_resolve(cv[CSS_BASIS],content_w)+(!cv[CSS_BOX_SIZING]?max_i(0,cv[CSS_BORDER])*2+max_i(0,css_resolve(cv[CSS_PADDING_LEFT],content_w))+max_i(0,css_resolve(cv[CSS_PADDING_RIGHT],content_w)):0);
        measure_layout(child,content_w,content_h,flex&&!column,forced,-1,depth+1);
        if(flex&&column&&cv[CSS_BASIS]!=CSS_AUTO)c->h=max_i(0,css_resolve(cv[CSS_BASIS],content_h));
        int32_t main=column?c->h+c->margin[0]+c->margin[2]:c->w+c->margin[1]+c->margin[3];
        used+=main;grow+=cv[CSS_GROW];shrink_weight+=cv[CSS_SHRINK]*max_i(0,main)/1000;count++;}
    if(flex&&count>1)used+=(int32_t)(count-1)*(column?gap_y:gap_x);
    int32_t main_space=column?content_h:content_w,free=main_space-used;
    if(flex&&!v[CSS_FLEX_WRAP]&&(!column||fixed_h>=0)&&((free>0&&grow)||(free<0&&shrink_weight))){
        for(int child=n->first;child>=0;child=layout_nodes[child].next){layout_node *c=&layout_nodes[child];int32_t *cv=c->css.value;
            if(cv[CSS_POSITION]==1||cv[CSS_POSITION]==2||c->css.hidden)continue;int32_t old=column?c->h:c->w;
            int32_t delta=free>0?(int32_t)((int64_t)free*cv[CSS_GROW]/grow):(int32_t)((int64_t)free*(cv[CSS_SHRINK]*old/1000)/shrink_weight);
            measure_layout(child,content_w,content_h,0,column?-1:max_i(0,old+delta),column?max_i(0,old+delta):-1,depth+1);}}
    int32_t offset=0;count=0;uint32_t grid_col=0;
    for(int child=n->first;child>=0;child=layout_nodes[child].next){layout_node *c=&layout_nodes[child];int32_t *cv=c->css.value;
        if(cv[CSS_POSITION]==1||cv[CSS_POSITION]==2||c->css.hidden)continue;
        if(grid){if(grid_col==(uint32_t)v[CSS_GRID_COLUMNS]){cross+=line_cross+gap_y;line_cross=0;grid_col=0;}c->x=(int32_t)grid_col*(c->w+gap_x)+c->margin[3];c->y=cross+c->margin[0];line_cross=max_i(line_cross,c->h+c->margin[0]+c->margin[2]);grid_col++;}
        else if(flex){int32_t main=column?c->h+c->margin[0]+c->margin[2]:c->w+c->margin[1]+c->margin[3],item_cross=column?c->w+c->margin[1]+c->margin[3]:c->h+c->margin[0]+c->margin[2];
            int32_t gap=column?gap_y:gap_x;if(v[CSS_FLEX_WRAP]&&count&&offset+gap+main>main_space){cross+=line_cross+(column?gap_x:gap_y);offset=0;line_cross=0;count=0;}
            if(count)offset+=gap;if(column){c->x=cross+c->margin[3];c->y=offset+c->margin[0];}else{c->x=offset+c->margin[3];c->y=cross+c->margin[0];}offset+=main;line_cross=max_i(line_cross,item_cross);count++;}
        else{c->x=c->margin[3];c->y=offset+c->margin[0];offset+=c->h+c->margin[0]+c->margin[2];}
    }
    int32_t auto_h=grid?cross+line_cross:flex?(column?offset:cross+line_cross):offset;
    if(n->text)auto_h=max_i(auto_h,text_height(n,content_w));if(same(n->tag,"br"))auto_h=max_i(1,v[CSS_LINE_HEIGHT]);if(same(n->tag,"hr"))auto_h=1;
    int32_t h=fixed_h>=0?fixed_h:auto_h+extra_h;
    n->h=max_i(extra_h,constrain(h,css_resolve(v[CSS_MIN_HEIGHT],height)+(v[CSS_BOX_SIZING]?0:extra_h),v[CSS_MAX_HEIGHT]==CSS_AUTO?CSS_AUTO:css_resolve(v[CSS_MAX_HEIGHT],height)+(v[CSS_BOX_SIZING]?0:extra_h),height));
    content_h=max_i(0,n->h-extra_h);
    if(flex&&!v[CSS_FLEX_WRAP]){int32_t occupied=column?auto_h:offset,spare=max_i(0,(column?content_h:content_w)-occupied),shift=0,between=0;
        if(v[CSS_JUSTIFY]==CSS_CENTER)shift=spare/2;else if(v[CSS_JUSTIFY]==CSS_END)shift=spare;
        else if(v[CSS_JUSTIFY]==CSS_BETWEEN&&count>1)between=spare/(int32_t)(count-1);
        else if(v[CSS_JUSTIFY]==CSS_AROUND&&count){between=spare/(int32_t)count;shift=between/2;}
        else if(v[CSS_JUSTIFY]==CSS_EVENLY&&count){between=spare/(int32_t)(count+1);shift=between;}
        uint32_t item=0;for(int child=n->first;child>=0;child=layout_nodes[child].next){layout_node *c=&layout_nodes[child];int32_t *cv=c->css.value;
            if(cv[CSS_POSITION]==1||cv[CSS_POSITION]==2||c->css.hidden)continue;int32_t align=cv[CSS_SELF]==CSS_AUTO?v[CSS_ITEMS]:cv[CSS_SELF];
            int32_t space=max_i(0,(column?content_w:content_h)-(column?c->w+c->margin[1]+c->margin[3]:c->h+c->margin[0]+c->margin[2]));
            if(align==CSS_STRETCH&&(column?cv[CSS_WIDTH]:cv[CSS_HEIGHT])==CSS_AUTO&&!same(c->tag,"#text")){
                int32_t saved_x=c->x,saved_y=c->y;
                measure_layout(child,content_w,content_h,0,column?c->w+space:c->w,column?c->h:c->h+space,depth+1);
                c->x=saved_x;c->y=saved_y;
            }
            int32_t main=shift+(int32_t)item*between;
            if(column){c->y+=main;if(align==CSS_CENTER)c->x+=space/2;else if(align==CSS_END)c->x+=space;}
            else{c->x+=main;if(align==CSS_CENTER)c->y+=space/2;else if(align==CSS_END)c->y+=space;}
            item++;}
    }
}
static void draw_layout_text(const layout_node *n,int32_t x,int32_t y,int32_t width){
    char text[4096];normalize_text(n->text,text,n->css.value[CSS_WHITE_SPACE]);int font=font_select(n->css.font_family),size=n->css.value[CSS_FONT_SIZE];
    int nowrap=n->css.value[CSS_WHITE_SPACE]==1||n->css.value[CSS_WHITE_SPACE]==2;
    for(const char *p=text;*p;){int32_t used;const char *end=text_line(p,font,(uint32_t)size,nowrap?2147483647:max_i(1,width),n->css.value[CSS_WHITE_SPACE]>=2,&used);
        int32_t tx=x;if(n->css.value[CSS_ALIGN]==1)tx+=(width-used)/2;else if(n->css.value[CSS_ALIGN]==2)tx+=width-used;
        while(p<end){uint32_t cp=ttf_utf8(&p);paint_glyph(font,tx,y,cp,(uint32_t)size,(uint32_t)n->css.value[CSS_COLOR]);if(n->css.value[CSS_FONT_WEIGHT]>=600)paint_glyph(font,tx+1,y,cp,(uint32_t)size,(uint32_t)n->css.value[CSS_COLOR]);tx+=font_advance(font,cp,(uint32_t)size);}if(*p=='\n')p++;y+=max_i(1,n->css.value[CSS_LINE_HEIGHT]);}
}
static int32_t layout_extent(int index,int axis,uint32_t depth){
    layout_node *n=&layout_nodes[index];int32_t extent=axis?n->h:n->w;if(depth>32)return extent;
    for(int child=n->first;child>=0;child=layout_nodes[child].next){layout_node *c=&layout_nodes[child];int32_t *v=c->css.value;
        if(c->css.hidden||v[CSS_POSITION]==2)continue;
        int32_t offset=axis?c->y:c->x;
        if(v[CSS_POSITION]==1){measure_layout(child,n->w,n->h,1,-1,-1,depth+1);
            int leading=axis?CSS_TOP:CSS_LEFT,trailing=axis?CSS_BOTTOM:CSS_RIGHT;int32_t basis=axis?n->h:n->w,size=axis?c->h:c->w;
            offset=v[leading]!=CSS_AUTO?css_resolve(v[leading],basis):v[trailing]!=CSS_AUTO?basis-size-css_resolve(v[trailing],basis):0;}
        int32_t size=v[axis?CSS_OVERFLOW_Y:CSS_OVERFLOW_X]?(axis?c->h:c->w):layout_extent(child,axis,depth+1);
        extent=max_i(extent,n->padding[axis?0:3]+offset+size+n->padding[axis?2:1]);
    }
    return extent;
}
static int layout_scroll(layout_node *n,int index){
    if(n->css.value[CSS_OVERFLOW_X]<2&&n->css.value[CSS_OVERFLOW_Y]<2)return -1;
    uint32_t state=0;for(;state<scroll_count;state++){
        if(n->id[0]?same(n->id,scrolls[state].id):!scrolls[state].id[0]&&n->serial==scrolls[state].serial)break;}
    if(state==scroll_count){if(scroll_count==2048)return -1;scroll_count++;
        for(uint32_t i=0;i<128;i++)scrolls[state].id[i]=n->id[i];scrolls[state].serial=n->serial;scrolls[state].x=scrolls[state].y=0;}
    scrolls[state].max_x=n->css.value[CSS_OVERFLOW_X]>=2?max_i(0,layout_extent(index,0,0)-n->w):0;
    scrolls[state].max_y=n->css.value[CSS_OVERFLOW_Y]>=2?max_i(0,layout_extent(index,1,0)-n->h):0;
    scroll_position(&scrolls[state].x,0,scrolls[state].max_x);scroll_position(&scrolls[state].y,0,scrolls[state].max_y);
    n->scroll_x=scrolls[state].x;n->scroll_y=scrolls[state].y;return (int)state;
}
static uint32_t query_nodes[1024];
static uint32_t query_materialize(int index,void *context,
    uint32_t (*materialize)(void *,uint32_t,uint32_t,const char *,const char *,const char *,const char *)){
    if(query_nodes[index])return query_nodes[index];layout_node *n=&layout_nodes[index];
    if(n->object)return query_nodes[index]=n->object;
    uint32_t parent=n->parent>=0?query_materialize(n->parent,context,materialize):0;
    query_nodes[index]=materialize(context,(uint32_t)index,parent,n->tag,n->id,n->classes,n->text?n->text:"");return query_nodes[index];
}
void html_query_selector(const char *html,const char *selector,const css_element *root,void *context,
    uint32_t (*materialize)(void *,uint32_t,uint32_t,const char *,const char *,const char *,const char *),
    void (*selected)(void *,uint32_t)){
    /* Queries must not replace the layout used for hover and hit testing. */
    extern void *envy_js_alloc(void *,void *,int);extern void *memcpy(void *,const void *,unsigned int);
    uint32_t saved_count=layout_count,saved_text=layout_text_used;
    uint32_t node_bytes=saved_count*sizeof(layout_node),backup_bytes=node_bytes+saved_text;
    char *backup=backup_bytes?envy_js_alloc(0,0,(int)backup_bytes):0;if(backup_bytes&&!backup)return;
    if(backup){memcpy(backup,layout_nodes,node_bytes);memcpy(backup+node_bytes,layout_text,saved_text);}
    uint32_t saved_serial=serial_next;serial_next=0;parse_layout(html,&body_style,root);
    for(uint32_t i=0;i<layout_count;i++)query_nodes[i]=0;
    for(uint32_t i=1;i<layout_count;i++)if(!same(layout_nodes[i].tag,"#text")&&css_selector_matches(&layout_nodes[i].element,selector))
        selected(context,query_materialize((int)i,context,materialize));
    if(backup){memcpy(layout_nodes,backup,node_bytes);memcpy(layout_text,backup+node_bytes,saved_text);envy_js_alloc(0,backup,0);}
    layout_count=saved_count;layout_text_used=saved_text;serial_next=saved_serial;
}
static void place_layout(int index,int32_t parent_x,int32_t parent_y,int containing,int32_t z,uint8_t opacity,uint32_t depth){
    if(depth>32)return;layout_node *n=&layout_nodes[index];int32_t *v=n->css.value;if(n->css.hidden)return;
    int32_t x=parent_x+n->x,y=parent_y+n->y;
    if(v[CSS_POSITION]==1||v[CSS_POSITION]==2){layout_node *cb=&layout_nodes[containing];int32_t bx=v[CSS_POSITION]==2?0:cb->x,by=v[CSS_POSITION]==2?0:cb->y;
        if(v[CSS_POSITION]==1){bx-=cb->scroll_x;by-=cb->scroll_y;}
        int32_t bw=v[CSS_POSITION]==2?(int32_t)viewport_width:cb->w,bh=v[CSS_POSITION]==2?(int32_t)viewport_height:cb->h;
        measure_layout(index,bw,bh,1,-1,-1,depth+1);
        if(v[CSS_WIDTH]==CSS_AUTO&&v[CSS_LEFT]!=CSS_AUTO&&v[CSS_RIGHT]!=CSS_AUTO)measure_layout(index,bw,bh,0,max_i(0,bw-css_resolve(v[CSS_LEFT],bw)-css_resolve(v[CSS_RIGHT],bw)),-1,depth+1);
        x=v[CSS_LEFT]!=CSS_AUTO?bx+css_resolve(v[CSS_LEFT],bw):v[CSS_RIGHT]!=CSS_AUTO?bx+bw-n->w-css_resolve(v[CSS_RIGHT],bw):parent_x;
        y=v[CSS_TOP]!=CSS_AUTO?by+css_resolve(v[CSS_TOP],bh):v[CSS_BOTTOM]!=CSS_AUTO?by+bh-n->h-css_resolve(v[CSS_BOTTOM],bh):parent_y;
        x+=n->margin[3];y+=n->margin[0];
    }else if(v[CSS_POSITION]==3){if(v[CSS_LEFT]!=CSS_AUTO)x+=css_resolve(v[CSS_LEFT],n->w);else if(v[CSS_RIGHT]!=CSS_AUTO)x-=css_resolve(v[CSS_RIGHT],n->w);
        if(v[CSS_TOP]!=CSS_AUTO)y+=css_resolve(v[CSS_TOP],n->h);else if(v[CSS_BOTTOM]!=CSS_AUTO)y-=css_resolve(v[CSS_BOTTOM],n->h);}
    x+=css_resolve(v[CSS_TRANSLATE_X],n->w);y+=css_resolve(v[CSS_TRANSLATE_Y],n->h);n->x=x;n->y=y;
    if(v[CSS_POSITION]&&v[CSS_Z_INDEX]!=CSS_AUTO)z+=v[CSS_Z_INDEX];paint_z=z;
    uint8_t inherited_opacity=opacity;opacity=(uint8_t)(((uint32_t)opacity*v[CSS_OPACITY]+127)/255);paint_opacity=opacity;
    int32_t old_left=paint_left,old_top=paint_top,old_right=paint_right,old_bottom=paint_bottom;
    int scroll=layout_scroll(n,index);
    if(!v[CSS_VISIBILITY]){
        if(same(n->tag,"#text")){paint_opacity=(uint8_t)(((uint32_t)inherited_opacity*v[CSS_COLOR_ALPHA]+127)/255);draw_layout_text(n,x,y,layout_nodes[n->parent].w-layout_nodes[n->parent].padding[1]-layout_nodes[n->parent].padding[3]);}
        else{paint_box(&n->css,x,y,(uint32_t)n->w,(uint32_t)n->h);
            if(box_count<2048&&n->w>0&&n->h>0){uint32_t box=box_count++;boxes[box].x=max_i(x,paint_left);boxes[box].y=max_i(y,paint_top);boxes[box].w=min_i(x+n->w,paint_right)-boxes[box].x;boxes[box].h=min_i(y+n->h,paint_bottom)-boxes[box].y;
                boxes[box].z=z;boxes[box].serial=n->serial;boxes[box].object=n->object;boxes[box].parent=n->parent>=0?layout_nodes[n->parent].serial:0;boxes[box].scroll=scroll;uint32_t i=0;do{boxes[box].id[i]=n->id[i];}while(n->id[i++]);}
            if(same(n->tag,"img"))paint_image(n->source,x+n->padding[3],y+n->padding[0],(uint32_t)max_i(0,n->w-n->padding[1]-n->padding[3]),(uint32_t)max_i(0,n->h-n->padding[0]-n->padding[2]));
            if(n->text)draw_layout_text(n,x+n->padding[3],y+n->padding[0],n->w-n->padding[1]-n->padding[3]);
            if(n->handler[0]&&opacity&&hit_count<128){uint32_t h=hit_count++;hits[h].x=max_i(x,paint_left);hits[h].y=max_i(y,paint_top);hits[h].width=min_i(x+n->w,paint_right)-hits[h].x;hits[h].height=min_i(y+n->h,paint_bottom)-hits[h].y;
                uint32_t i=0;do{hits[h].handler[i]=n->handler[i];}while(n->handler[i++]);}}
    }
    if(v[CSS_OVERFLOW_X]){paint_left=max_i(paint_left,x);paint_right=min_i(paint_right,x+n->w);}
    if(v[CSS_OVERFLOW_Y]){paint_top=max_i(paint_top,y);paint_bottom=min_i(paint_bottom,y+n->h);}
    if(paint_right<paint_left)paint_right=paint_left;if(paint_bottom<paint_top)paint_bottom=paint_top;
    for(int child=n->first;child>=0;child=layout_nodes[child].next)place_layout(child,x+n->padding[3]-n->scroll_x,y+n->padding[0]-n->scroll_y,v[CSS_POSITION]?index:containing,z,opacity,depth+1);
    paint_left=old_left;paint_top=old_top;paint_right=old_right;paint_bottom=old_bottom;paint_opacity=inherited_opacity;
}
static void render_markup(const char *html,const style *parent,const css_element *parent_element){
    parse_layout(html,parent,parent_element);measure_layout(0,parent->width,(int32_t)viewport_height,0,parent->width,-1,0);
    if(html==document_html){
        page_allow_x=page_allow_x&&layout_nodes[0].css.value[CSS_OVERFLOW_X]!=1;
        page_allow_y=page_allow_y&&layout_nodes[0].css.value[CSS_OVERFLOW_Y]!=1;
    }
    if(!page_allow_x)page_x=0;if(!page_allow_y)page_y=0;
    int32_t flow_height=layout_nodes[0].h;layout_nodes[0].h=max_i(layout_nodes[0].h,(int32_t)viewport_height);
    if(page_allow_x)page_max_x=max_i(page_max_x,layout_extent(0,0,0)-(int32_t)viewport_width);
    if(page_allow_y)page_max_y=max_i(page_max_y,cursor_y+layout_extent(0,1,0)-(int32_t)viewport_height);
    /* The fragment root is a virtual container; its owner paints its background. */
    layout_nodes[0].x=parent->x-page_x;layout_nodes[0].y=(parent->positioned?parent->y:0)-page_y;
    for(int child=layout_nodes[0].first;child>=0;child=layout_nodes[child].next)place_layout(child,parent->x+layout_nodes[0].padding[3]-page_x,cursor_y+layout_nodes[0].padding[0]-page_y,0,parent->z,255,0);
    cursor_y+=flow_height;
}
void html_view_repaint(void) {
    if(batch_depth){paint_pending=1;return;}paints++;
    if(!framebuffer_get())return;framebuffer_begin_scene();cursor_y=0;hit_count=0;
    box_count=serial_next=0;css_state_lookup(state_matches);
    paint_left=paint_top=0;paint_right=(int32_t)viewport_width;paint_bottom=(int32_t)viewport_height;paint_opacity=255;
    collecting=1;command_count=background_count=source_count=0;
    css_element root={"html","","",0},body={"body","","",&root};style root_style;
    computed(&root_style,0,&root,0);computed(&body_style,&root_style,&body,0);
    page_allow_x=root_style.computed.value[CSS_OVERFLOW_X]!=1&&body_style.computed.value[CSS_OVERFLOW_X]!=1;
    page_allow_y=root_style.computed.value[CSS_OVERFLOW_Y]!=1&&body_style.computed.value[CSS_OVERFLOW_Y]!=1;
    page_max_x=page_max_y=0;
    command_count=background_count=source_count=0;paint_z=(-2147483647-1);
    paint_rect(0,0,viewport_width,viewport_height,0xffffff);
    paint_box(&root_style.computed,0,0,viewport_width,viewport_height);
    paint_box(&body_style.computed,0,0,viewport_width,viewport_height);
    if(document_html)render_markup(document_html,&body_style,&body);
    for(uint32_t i=0;i<view_count;i++) {
        const view_node *node=&views[i];css_element element={node->tag,node->id,node->classes,&body};style s;
        if(node->parent_id&&*node->parent_id)continue;
        char inline_css[1024];uint32_t used=0;if(node->inline_style)while(node->inline_style[used]&&used<1023){inline_css[used]=node->inline_style[used];used++;}inline_css[used]=0;
        append_live_style(node->node,node->id,inline_css,sizeof(inline_css));
        int32_t saved=cursor_y;computed(&s,&body_style,&element,inline_css);if(s.positioned)cursor_y=s.y;
        if(same(node->tag,"img"))draw_image(node->image_source,node->text,s,node->image_width,node->image_height);
        else if(node->markup)render_markup(node->markup,&s,&element);else if(node->text)draw(node->text,s,same(node->tag,"pre"));
        if(s.positioned)cursor_y=saved;
    }
    collecting=0;paint_scene();framebuffer_end_frame();
    int clamped=scroll_position(&page_x,0,page_max_x)|scroll_position(&page_y,0,page_max_y);
    if(clamped)html_view_repaint();
}
void html_view_render(const char *html) {
    document_html=html;const char *p=html;
    while((p=find(p,"<style"))) {
        const char *start=find(p,">");if(!start)break;const char *end=find(start+1,"</style>");if(!end)break;
        css_add(start+1,(uint32_t)(end-start-1));p=end+8;
    }
    html_view_repaint();
}
void html_view_node(uint32_t node,const char *tag,const char *id,const char *classes,const char *text,const char *markup,const char *inline_style) {
    if(same(tag,"script")||same(tag,"style")||same(tag,"link"))return;
    uint32_t index=0;for(;index<view_count;index++)if(views[index].node==node)break;
    if(index==view_count){if(view_count==128)return;view_count++;}
    views[index]=(view_node){.node=node,.tag=tag,.id=id?id:"",.classes=classes?classes:"",.text=text,.markup=markup,.inline_style=inline_style};html_view_repaint();
}
void html_view_image(uint32_t node,const char *source,uint32_t width,uint32_t height){
    for(uint32_t i=0;i<view_count;i++)if(views[i].node==node){views[i].image_source=source;views[i].image_width=width;views[i].image_height=height;html_view_repaint();return;}
}
void html_view_parent(uint32_t node,const char *parent_id){for(uint32_t i=0;i<view_count;i++)if(views[i].node==node){views[i].parent_id=parent_id;html_view_repaint();return;}}
