#include "../../Browser/css.h"
static uint32_t length(const char *s){uint32_t n=0;while(s[n])n++;return n;}
static int add(const char *s){return css_add(s,length(s));}
static int same(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
int css_self_test(void) {
#define CHECK(condition,code) do{if(!(condition))return code;}while(0)
    css_init();
    css_element root={"html","","",0},body={"body","","",&root},panel={"div","panel","panel accented",&body},text={"p","label","caption",&panel};
    css_style root_style,body_style,panel_style,text_style;
    CHECK(add("/* sheet */ :root {--accent:#5a3;} body {color:white;background-color:#123456;} .panel { padding:4px 8px; } #panel {color:var(--accent);width:50%;} .panel > p.caption {font-size:24px;} div p {color:red;} #label {color:blue;} p {color:yellow !important;}"),70);
    css_compute(&root,0,0,800,600,&root_style);css_compute(&body,&root_style,0,800,600,&body_style);
    css_compute(&panel,&body_style,0,800,600,&panel_style);css_compute(&text,&panel_style,"color:magenta",800,600,&text_style);
    CHECK(body_style.value[CSS_COLOR]==0xffffff&&body_style.value[CSS_BACKGROUND]==0x123456,71);
    CHECK(panel_style.value[CSS_COLOR]==0x55aa33&&css_resolve(panel_style.value[CSS_WIDTH],800)==400,72);
    CHECK(panel_style.value[CSS_PADDING_TOP]==4&&panel_style.value[CSS_PADDING_LEFT]==8,73);
    CHECK(text_style.value[CSS_COLOR]==0xffff00&&text_style.value[CSS_FONT_SIZE]==24,74);
    css_compute(&text,&panel_style,"color:magenta!important",800,600,&text_style);
    CHECK(text_style.value[CSS_COLOR]==0xff00ff,75);
    CHECK(add("#label {color:#010203 !important;} .hidden {display:none;} .caption {font-size:25px;}"),76);
    css_compute(&text,&panel_style,0,800,600,&text_style);
    CHECK(text_style.value[CSS_COLOR]==0x010203&&text_style.value[CSS_FONT_SIZE]==24,77);
    css_element hidden={"div","","hidden",&body},child={"span","","",&hidden};css_style hidden_style,child_style;
    css_compute(&hidden,&body_style,0,800,600,&hidden_style);css_compute(&child,&hidden_style,"display:block",800,600,&child_style);
    CHECK(hidden_style.hidden&&child_style.hidden,78);
    CHECK(add("@font-face {font-family:ignored;src:url('a;b.ttf');} @media print {body {color:black;}} p, span {margin:1px 2px 3px 4px;} p {padding: /* hi */ 6px;unknown-property:12px;color:#000000;}"),79);
    css_compute(&text,&panel_style,0,800,600,&text_style);
    CHECK(text_style.value[CSS_MARGIN_TOP]==1&&text_style.value[CSS_MARGIN_RIGHT]==2&&text_style.value[CSS_MARGIN_BOTTOM]==3&&text_style.value[CSS_MARGIN_LEFT]==4,80);
    CHECK(text_style.value[CSS_PADDING_TOP]==6&&text_style.value[CSS_COLOR]==0x010203,81);
    css_element different={"p","Label","Caption",&panel};css_compute(&different,&panel_style,0,800,600,&text_style);
    CHECK(text_style.value[CSS_COLOR]==0xffff00&&text_style.value[CSS_FONT_SIZE]==16,82);
    css_init();CHECK(add("div {color:rgb(12, 34, 56);position:absolute;left:25%;top:10vh;background:#abc;}"),83);
    css_compute(&panel,0,0,800,600,&panel_style);
    CHECK(panel_style.value[CSS_COLOR]==0x0c2238&&panel_style.value[CSS_BACKGROUND]==0xaabbcc,84);
    CHECK(panel_style.value[CSS_POSITION]==1&&css_resolve(panel_style.value[CSS_LEFT],800)==200&&panel_style.value[CSS_TOP]==60,85);
    CHECK(!add("p { color: red;"),86);
    css_init();
    const char *background="div{background:#123456 url('../Assets/envy.svg') center / cover no-repeat;} .panel{background-size:contain;} #panel{background-image:url('../Assets/user/cat.jpg') !important;background-position:right bottom;}";
    char path[64]="/Envy/Styles/base.css";
    CHECK(css_add_file(background,length(background),path),87);path[0]='x'; /* Rules own their stylesheet path. */
    css_compute(&panel,0,"background-image:none;background-size:50% auto",800,600,&panel_style);
    CHECK(same(panel_style.background_image,"/Envy/Assets/user/cat.jpg"),88);
    CHECK(panel_style.value[CSS_BACKGROUND]==0x123456&&panel_style.value[CSS_BACKGROUND_REPEAT]==0,89);
    CHECK(panel_style.value[CSS_BACKGROUND_X]==CSS_PERCENT+100&&panel_style.value[CSS_BACKGROUND_Y]==CSS_PERCENT+100,90);
    CHECK(panel_style.value[CSS_BACKGROUND_SIZE]==CSS_BG_EXPLICIT&&panel_style.background_width==CSS_PERCENT+50&&panel_style.background_height==CSS_AUTO,91);
    css_compute(&text,&panel_style,0,800,600,&text_style);
    CHECK(!text_style.background_image[0]&&text_style.value[CSS_BACKGROUND_REPEAT]==3&&text_style.value[CSS_BACKGROUND_X]==CSS_PERCENT,92);
    css_compute(&panel,0,"background:none !important",800,600,&panel_style);
    CHECK(!panel_style.background_image[0]&&panel_style.value[CSS_BACKGROUND]==-1&&panel_style.value[CSS_BACKGROUND_SIZE]==CSS_BG_AUTO&&panel_style.value[CSS_BACKGROUND_REPEAT]==3,93);
    css_compute(&panel,0,"background: url('./Assets/envy.svg') 25% 10px / 40px 20px repeat-x !important",800,600,&panel_style);
    CHECK(same(panel_style.background_image,"/Envy/Assets/envy.svg")&&panel_style.value[CSS_BACKGROUND_X]==CSS_PERCENT+25&&panel_style.value[CSS_BACKGROUND_Y]==10,94);
    CHECK(panel_style.background_width==40&&panel_style.background_height==20&&panel_style.value[CSS_BACKGROUND_REPEAT]==1,95);
    css_compute(&panel,0,"background-size:cover;background-size:invalid;background-position:top left;background-repeat:repeat-y",800,600,&panel_style);
    CHECK(panel_style.value[CSS_BACKGROUND_SIZE]==CSS_BG_COVER&&panel_style.value[CSS_BACKGROUND_X]==CSS_PERCENT&&panel_style.value[CSS_BACKGROUND_Y]==CSS_PERCENT&&panel_style.value[CSS_BACKGROUND_REPEAT]==2,96);
    css_compute(&panel,0,"background-image:none!important;background:linear-gradient(red,blue)",800,600,&panel_style);
    CHECK(!panel_style.background_image[0],97);
    CHECK(panel_style.value[CSS_BACKGROUND]==-1,110);
    CHECK(panel_style.gradient_count==0,111); /* Important background-image:none suppresses the later gradient. */
    css_compute(&panel,0,"position:absolute;z-index:-5",800,600,&panel_style);
    CHECK(panel_style.value[CSS_Z_INDEX]==-5,98);
    css_compute(&text,&panel_style,0,800,600,&text_style);CHECK(text_style.value[CSS_Z_INDEX]==CSS_AUTO,99);
    css_init();
    CHECK(add("div{width:calc(100% - 20px);height:calc(100% - 42px - 12px);padding:.5rem 1.25em;font-size:20px;line-height:1.5;min-width:140px;max-height:300px;display:flex;gap:5px 10px;align-items:center;justify-content:space-between;} @media screen and (max-width:650px){div{width:100%;display:none;}} @media print{div{color:red;}}"),100);
    css_compute(&panel,0,0,800,600,&panel_style);
    CHECK(css_resolve(panel_style.value[CSS_WIDTH],400)==380&&css_resolve(panel_style.value[CSS_HEIGHT],600)==546,101);
    CHECK(panel_style.value[CSS_PADDING_TOP]==8&&panel_style.value[CSS_PADDING_RIGHT]==25&&panel_style.value[CSS_LINE_HEIGHT]==30,102);
    CHECK(panel_style.value[CSS_DISPLAY]==CSS_FLEX&&panel_style.value[CSS_ROW_GAP]==5&&panel_style.value[CSS_COLUMN_GAP]==10&&panel_style.value[CSS_ITEMS]==CSS_CENTER&&panel_style.value[CSS_JUSTIFY]==CSS_BETWEEN,103);
    css_compute(&panel,0,0,650,600,&panel_style);CHECK(panel_style.hidden&&css_resolve(panel_style.value[CSS_WIDTH],400)==400,104);
    css_compute(&panel,0,"width:calc((100% - 40px) / 2);opacity:.5;background:rgba(86,85,88,0);border:2px solid gray",800,600,&panel_style);
    CHECK(css_resolve(panel_style.value[CSS_WIDTH],400)==180&&panel_style.value[CSS_OPACITY]==128&&panel_style.value[CSS_BACKGROUND_ALPHA]==0&&panel_style.value[CSS_BORDER_COLOR]==0x808080,105);
    CHECK(add(":root{--channels:12,34,56;--size:20px;} .panel:not(.focus){color:rgba(var(--channels),.5);min-height:calc(var(--size) + 4px);} #panel{background:linear-gradient(to right,#1e1e1e 20%,#3f3f3f 100%);}"),106);
    css_compute(&panel,0,"font-size:var(--missing, 18px);width:25px;width:broken;margin:1px garbage",800,600,&panel_style);
    CHECK(panel_style.value[CSS_COLOR]==0x0c2238&&panel_style.value[CSS_COLOR_ALPHA]==128&&panel_style.value[CSS_FONT_SIZE]==18&&panel_style.value[CSS_WIDTH]==25&&panel_style.value[CSS_MARGIN_TOP]==0,107);
    CHECK(panel_style.gradient_count==2&&panel_style.gradient_angle==90&&panel_style.gradient_color[0]==0x1e1e1e&&panel_style.value[CSS_MIN_HEIGHT]==24,108);
    css_compute(&panel,0,"transform:translateX(100%);box-sizing:border-box;flex:1;grid-template-columns:repeat(3, 1fr)",800,600,&panel_style);
    CHECK(css_resolve(panel_style.value[CSS_TRANSLATE_X],80)==80&&panel_style.value[CSS_BOX_SIZING]==1&&panel_style.value[CSS_GROW]==1000&&panel_style.value[CSS_GRID_COLUMNS]==3,109);
    css_compute(&panel,0,"background:rgba(86,85,88,0);backdrop-filter:brightness(.7)blur(10px)",800,600,&panel_style);
    CHECK(panel_style.value[CSS_BACKGROUND_ALPHA]==0&&panel_style.value[CSS_BACKDROP_BLUR]==10&&panel_style.value[CSS_BACKDROP_BRIGHTNESS]==700,110);
    css_compute(&panel,&panel_style,0,800,600,&text_style);
    CHECK(text_style.value[CSS_BACKDROP_BLUR]==0&&text_style.value[CSS_BACKDROP_BRIGHTNESS]==1000,111);
    css_compute(&panel,0,"-webkit-backdrop-filter:blur(25px) brightness(50%);backdrop-filter:garbage",800,600,&panel_style);
    CHECK(panel_style.value[CSS_BACKDROP_BLUR]==25&&panel_style.value[CSS_BACKDROP_BRIGHTNESS]==500,112);
    css_compute(&panel,0,"backdrop-filter:blur(10px);backdrop-filter:none",800,600,&panel_style);
    CHECK(panel_style.value[CSS_BACKDROP_BLUR]==0&&panel_style.value[CSS_BACKDROP_BRIGHTNESS]==1000,113);
    css_init();return 0;
}
