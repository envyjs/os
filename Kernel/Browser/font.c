#include "font.h"
#include "../Runtime/Font/ttf.h"
#include "../Drivers/Video/framebuffer.h"
typedef struct {char family[64],path[256];ttf_font font;int tried;} face;
static face faces[24];static uint32_t face_count,used;
static uint8_t memory[1536u*1024u];static cdfs *filesystem;
typedef struct {const ttf_font *font;uint32_t glyph,size,offset;int32_t left,top,width,height;} glyph_bitmap;
static glyph_bitmap glyphs[256];static uint32_t glyph_count,glyph_used,rasterizations;
static uint8_t glyph_pixels[320u*1024u],glyph_scratch[512u*512u];
uint32_t font_rasterization_count(void){return rasterizations;}
static int space(char c){return c==' '||c=='\t'||c=='\n'||c=='\r';}
static char lower(char c){return c>='A'&&c<='Z'?(char)(c+32):c;}
static int same(const char *a,const char *b){while(*a&&lower(*a)==lower(*b)){a++;b++;}return *a==*b;}
static void copy(char *to,const char *from,uint32_t size){uint32_t n=0;while(from[n]&&n+1<size){to[n]=from[n];n++;}to[n]=0;}
static int path(const char *url,const char *base,char *result){
    char joined[512];uint32_t n=0;
    if(*url!='/'){uint32_t last=0;for(uint32_t i=0;base[i];i++)if(base[i]=='/')last=i+1;
        if(last>=sizeof(joined))return 0;for(uint32_t i=0;i<last;i++)joined[n++]=base[i];}
    while(*url){if(*url==':'||*url=='\\'||*url=='?'||*url=='#'||n+1>=sizeof(joined))return 0;joined[n++]=*url++;}joined[n]=0;
    uint32_t out=0;const char *p=joined;result[out++]='/';
    while(*p){while(*p=='/')p++;if(!*p)break;const char *start=p;while(*p&&*p!='/')p++;uint32_t len=(uint32_t)(p-start);
        if(len==1&&start[0]=='.')continue;
        if(len==2&&start[0]=='.'&&start[1]=='.'){if(out<=1)return 0;out--;while(out>1&&result[out-1]!='/')out--;continue;}
        if(out+len+1>=256)return 0;for(uint32_t i=0;i<len;i++)result[out++]=start[i];result[out++]='/';}
    if(out>1)out--;result[out]=0;return out>6&&lower(result[1])=='e'&&lower(result[2])=='n'&&lower(result[3])=='v'&&lower(result[4])=='y'&&result[5]=='/';
}
static int load(uint32_t index){
    face *f=&faces[index];if(f->tried)return f->font.data!=0;f->tried=1;
    cdfs_node node;uint32_t got;if(!filesystem||cdfs_lookup(filesystem,f->path,&node)||(node.flags&2)||node.size>sizeof(memory)-used)return 0;
    if(cdfs_read(filesystem,&node,0,memory+used,node.size,&got)||got!=node.size||!ttf_init(&f->font,memory+used,got))return 0;
    used+=got;return 1;
}
void font_face(const char *family,const char *sources,const char *stylesheet){
    char name[64];uint32_t n=0;while(space(*family))family++;char quote=(*family=='\''||*family=='"')?*family++:0;
    while(*family&&(quote?*family!=quote:1)&&n+1<sizeof(name))name[n++]=*family++;
    while(n&&space(name[n-1]))n--;name[n]=0;if(!n)return;
    const char *p=sources;
    while(*p){if(lower(p[0])!='u'||!p[1]||lower(p[1])!='r'||!p[2]||lower(p[2])!='l'){p++;continue;}
        p+=3;while(space(*p))p++;if(*p!='(')continue;p++;while(space(*p))p++;
        char q=(*p=='\''||*p=='"')?*p++:0;char url[256];n=0;
        while(*p&&(q?*p!=q:*p!=')')&&n+1<sizeof(url))url[n++]=*p++;
        if((q&&*p!=q)||(!q&&*p!=')'))continue;if(q)p++;while(space(*p))p++;if(*p!=')')continue;p++;
        while(n&&space(url[n-1]))n--;url[n]=0;char resolved[256];if(!path(url,stylesheet,resolved))continue;
        for(uint32_t i=0;i<face_count;i++)if(same(faces[i].family,name)&&same(faces[i].path,resolved))return;
        if(face_count==24)return;face *f=&faces[face_count++];*f=(face){0};copy(f->family,name,sizeof(f->family));copy(f->path,resolved,sizeof(f->path));
    }
}
void font_init(cdfs *fs){filesystem=fs;face_count=used=0;
    glyph_count=glyph_used=rasterizations=0;
    font_face("Adwaita Sans","url('/Envy/Assets/fonts/AdwaitaSans-Regular.ttf')","/Envy/index.html");
    font_face("ReadexPro","url('/Envy/Assets/fonts/ReadexPro-Regular.ttf')","/Envy/index.html");
    font_face("Ubuntu Mono","url('/Envy/Assets/fonts/UbuntuMono-R.ttf')","/Envy/index.html");
}
int font_select(const char *families){
    const char *p=families;
    while(*p){while(space(*p)||*p==',')p++;char quote=(*p=='\''||*p=='"')?*p++:0;char name[64];uint32_t n=0;
        while(*p&&(quote?*p!=quote:*p!=',')&&n+1<sizeof(name))name[n++]=*p++;
        if(quote&&*p==quote)p++;while(n&&space(name[n-1]))n--;name[n]=0;
        const char *wanted=same(name,"monospace")?"Ubuntu Mono":same(name,"sans-serif")||same(name,"system-ui")||same(name,"serif")?"Adwaita Sans":name;
        for(uint32_t i=0;i<face_count;i++)if(same(faces[i].family,wanted)&&load(i))return (int)i;
        while(*p&&*p!=',')p++;
    }
    for(uint32_t i=0;i<face_count;i++)if(same(faces[i].family,"Adwaita Sans")&&load(i))return (int)i;
    return -1;
}
static const ttf_font *selected(int font,uint32_t cp,uint32_t *glyph){
    if(font<0||(uint32_t)font>=face_count||!faces[font].font.data)return 0;
    const ttf_font *f=&faces[font].font;*glyph=ttf_glyph(f,cp);if(*glyph)return f;
    for(uint32_t i=0;i<face_count;i++)if(i!=(uint32_t)font&&load(i)){uint32_t g=ttf_glyph(&faces[i].font,cp);if(g){*glyph=g;return &faces[i].font;}}
    return f;
}
int32_t font_advance(int font,uint32_t cp,uint32_t size){uint32_t glyph;const ttf_font *f=selected(font,cp,&glyph);
    return f?ttf_advance(f,glyph,size):(int32_t)(size+1)/2;}
typedef struct {int32_t x,y;uint32_t color;} pen;
static void pixel(void *context,int32_t x,int32_t y,uint8_t coverage){pen *p=context;framebuffer_blend(p->x+x,p->y+y,p->color,coverage);}
typedef struct {pen pen;int32_t left,top,right,bottom;int fits;} bitmap_pen;
static void bitmap_pixel(void *context,int32_t x,int32_t y,uint8_t coverage){
    bitmap_pen *p=context;pixel(&p->pen,x,y,coverage);
    if(x<-256||x>=256||y<-384||y>=128){p->fits=0;return;}
    glyph_scratch[(y+384)*512+x+256]=coverage;
    if(x<p->left)p->left=x;if(x>=p->right)p->right=x+1;if(y<p->top)p->top=y;if(y>=p->bottom)p->bottom=y+1;
}
void font_draw(int font,int32_t x,int32_t top,uint32_t cp,uint32_t size,uint32_t color){
    uint32_t glyph;const ttf_font *f=selected(font,cp,&glyph);
    if(f){int32_t baseline=top+(int32_t)f->ascent*(int32_t)size/f->units;
        for(uint32_t i=0;i<glyph_count;i++){const glyph_bitmap *g=&glyphs[i];if(g->font!=f||g->glyph!=glyph||g->size!=size)continue;
            for(int32_t row=0;row<g->height;row++)for(int32_t col=0;col<g->width;col++){uint8_t alpha=glyph_pixels[g->offset+row*g->width+col];if(alpha)framebuffer_blend(x+g->left+col,baseline+g->top+row,color,alpha);}return;}
        uint8_t *scratch=glyph_scratch;uint32_t words=sizeof(glyph_scratch)/4;__asm__ volatile("cld; rep stosl":"+D"(scratch),"+c"(words):"a"(0):"memory");
        bitmap_pen p={{x,baseline,color},256,128,-256,-384,1};rasterizations++;
        if(ttf_render(f,glyph,size,bitmap_pixel,&p)){
            if(p.fits){int32_t width=p.right>p.left?p.right-p.left:0,height=p.bottom>p.top?p.bottom-p.top:0;uint32_t bytes=(uint32_t)(width*height);
                if(glyph_count==256||bytes>sizeof(glyph_pixels)-glyph_used)glyph_count=glyph_used=0;
                glyphs[glyph_count++]=(glyph_bitmap){f,glyph,size,glyph_used,p.left,p.top,width,height};
                for(int32_t row=0;row<height;row++)for(int32_t col=0;col<width;col++)glyph_pixels[glyph_used+row*width+col]=glyph_scratch[(p.top+384+row)*512+p.left+256+col];glyph_used+=bytes;
            }return;}
    }
    char text[2]={cp>=32&&cp<127?(char)cp:'?',0};framebuffer_text_size(x,top,text,color,size);
}
