#include "../../Runtime/Font/ttf.h"
#include "../../Browser/font.h"
#include "../../Browser/css.h"
static uint8_t bytes[900000];
static uint32_t ink,partial;
static void pixel(void *context,int32_t x,int32_t y,uint8_t alpha){(void)context;(void)x;(void)y;ink+=alpha;if(alpha<255)partial++;}
static uint32_t length(const char *p){uint32_t n=0;while(p[n])n++;return n;}
static int same(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
int font_self_test(cdfs *fs){
#define CHECK(condition,code) do{if(!(condition))return code;}while(0)
    const char *files[]={"/Envy/Assets/fonts/AdwaitaSans-Regular.ttf","/Envy/Assets/fonts/ReadexPro-Regular.ttf","/Envy/Assets/fonts/UbuntuMono-R.ttf"};
    for(uint32_t i=0;i<3;i++){
        cdfs_node node;uint32_t got;ttf_font f;
        CHECK(!cdfs_lookup(fs,files[i],&node)&&node.size<=sizeof(bytes),300);
        CHECK(!cdfs_read(fs,&node,0,bytes,node.size,&got)&&got==node.size,301);
        CHECK(ttf_init(&f,bytes,got),302);
        uint32_t narrow=ttf_glyph(&f,'i'),wide=ttf_glyph(&f,'W');CHECK(narrow&&wide,303);
        CHECK(i==2?ttf_advance(&f,narrow,32)==ttf_advance(&f,wide,32):ttf_advance(&f,narrow,32)<ttf_advance(&f,wide,32),304);
        ink=partial=0;CHECK(ttf_render(&f,wide,32,pixel,0)&&ink&&partial,305);
        CHECK(ttf_glyph(&f,0xe9)!=0,306);ink=0;CHECK(ttf_render(&f,ttf_glyph(&f,0xe9),32,pixel,0)&&ink,307);
        ink=0;CHECK(ttf_render(&f,ttf_glyph(&f,' '),32,pixel,0)&&!ink,308);
        for(uint32_t cp=33;cp<127;cp++)CHECK(ttf_render(&f,ttf_glyph(&f,cp),16,pixel,0),309);
        CHECK(!ttf_render(&f,f.glyphs,16,pixel,0),310);
        CHECK(!ttf_init(&f,bytes,11)&&!f.data,311);
        uint8_t saved=bytes[0];bytes[0]=255;CHECK(!ttf_init(&f,bytes,got),312);bytes[0]=saved;
    }
    const char *p="\xc3\xa9\xf0\x9f\x98\x80";CHECK(ttf_utf8(&p)==0xe9&&ttf_utf8(&p)==0x1f600&&!*p,313);
    p="\xe2";CHECK(ttf_utf8(&p)==0xfffd&&!*p,314);
    font_init(fs);int sans=font_select("'Adwaita Sans'"),mono=font_select("Missing, 'Ubuntu Mono', sans-serif"),readex=font_select("ReadexPro");
    CHECK(sans>=0&&mono>=0&&readex>=0&&sans!=mono&&sans!=readex,315);
    CHECK(font_advance(sans,'i',32)<font_advance(sans,'W',32)&&font_advance(mono,'i',32)==font_advance(mono,'W',32),316);
    css_init();const char *sheet="@font-face{font-family:'Alias';src:local(''),url('../Assets/fonts/UbuntuMono-R.ttf') format('truetype');} body{font-family:'Adwaita Sans','ReadexPro';} .code{font-family:Missing,'Alias',monospace !important;}";
    CHECK(css_add_file(sheet,length(sheet),"/Envy/Styles/base.css"),317);
    CHECK(font_select("Alias")>=0&&font_advance(font_select("Alias"),'i',32)==font_advance(mono,'i',32),318);
    css_element body={"body","","",0},child={"p","","code",&body};css_style parent,style;
    css_compute(&body,0,0,800,600,&parent);css_compute(&child,&parent,"font-family:ReadexPro",800,600,&style);
    CHECK(same(parent.font_family,"'Adwaita Sans','ReadexPro'")&&same(style.font_family,"Missing,'Alias',monospace"),319);
    css_compute(&child,&parent,"font-family:inherit !important",800,600,&style);CHECK(same(style.font_family,parent.font_family),320);
    css_compute(&child,&parent,"font-family:initial !important",800,600,&style);CHECK(same(style.font_family,"sans-serif"),321);
    for(uint32_t i=0;i<3;i++){const char *text="Wi: TrueType caf\xc3\xa9 0123456789";int32_t x=20;
        int face=i==0?sans:i==1?readex:mono;
        while(*text){uint32_t cp=ttf_utf8(&text);font_draw(face,x,40+(int32_t)i*48,cp,32,0xffffff);x+=font_advance(face,cp,32);}}
    return 0;
}
