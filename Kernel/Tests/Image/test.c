#include "../../Runtime/Image/image.h"
#include "../../Browser/images.h"
#include "../../Browser/html.h"
#include "../../Browser/css.h"
#include "../../Browser/browser.h"
#include "../../Drivers/Video/framebuffer.h"
#include "fixtures.h"
static uint32_t length(const char *text){uint32_t n=0;while(text[n])n++;return n;}
static int same(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static const char *clock_text="12:34";
static const char *lookup(void *context,const char *id){(void)context;return id[0]=='c'&&id[1]=='l'&&id[2]=='o'&&id[3]=='c'&&id[4]=='k'&&!id[5]?clock_text:0;}
static uint32_t color_at(int32_t x,int32_t y){
    const framebuffer *fb=framebuffer_get();if(!fb)return 0;uint32_t raw=0;
    const volatile uint8_t *p=fb->pixels+(uint32_t)y*fb->pitch+(uint32_t)x*fb->bytes_per_pixel;
    for(uint32_t i=0;i<fb->bytes_per_pixel;i++)raw|=(uint32_t)p[i]<<(i*8);
    return (((raw>>fb->red_shift)&((1u<<fb->red_bits)-1))*255/((1u<<fb->red_bits)-1))<<16|
           (((raw>>fb->green_shift)&((1u<<fb->green_bits)-1))*255/((1u<<fb->green_bits)-1))<<8|
           (((raw>>fb->blue_shift)&((1u<<fb->blue_bits)-1))*255/((1u<<fb->blue_bits)-1));
}
int image_self_test(cdfs *fs){
#define CHECK(condition,code) do{if(!(condition))return code;}while(0)
    image_surface image;
    CHECK(image_decode(png_fixture,sizeof(png_fixture),0,0,&image)&&image.width==2&&image.height==2,401);
    CHECK(image.pixels[0]==255&&image.pixels[1]==0&&image.pixels[3]==255,402);
    CHECK(image.pixels[4]==0&&image.pixels[5]==255&&image.pixels[7]==128&&image.pixels[11]==0,403);
    framebuffer_rect(0,0,10,10,0);images_draw(&image,0,0,4,4);
    CHECK(color_at(0,0)==0xff0000&&color_at(3,0)==0x008000&&color_at(0,3)==0&&color_at(3,3)==0xffffff,404);
    CHECK(image_decode(jpeg_fixture,sizeof(jpeg_fixture),0,0,&image)&&image.width==32&&image.height==16,405);
    CHECK(image.pixels[0]>=29&&image.pixels[0]<=35&&image.pixels[1]>=61&&image.pixels[1]<=67&&image.pixels[2]>=125&&image.pixels[2]<=131&&image.pixels[3]==255,406);
    const char *svg="<svg xmlns='http://www.w3.org/2000/svg' width='4' height='2' viewBox='0 0 4 2'><rect width='2' height='2' fill='#ff0000'/><path d='M2 0h2v2H2Z' fill='#00ff00' opacity='.5'/></svg>";
    CHECK(image_decode(svg,length(svg),40,20,&image)&&image.width==40&&image.height==20&&image.natural_width==4&&image.natural_height==2,407);
    CHECK(image.pixels[(10*40+10)*4]==255&&image.pixels[(10*40+10)*4+3]==255&&image.pixels[(10*40+30)*4+1]==255&&image.pixels[(10*40+30)*4+3]>=126&&image.pixels[(10*40+30)*4+3]<=129,408);
    CHECK(!image_decode(png_fixture,12,0,0,&image)&&!image.pixels,409);
    CHECK(!image_decode("broken",6,0,0,&image)&&!image.pixels,410);
    CHECK(!image_decode(svg,length(svg),100000,0,&image)&&!image.pixels,411);
    images_init(fs);
    const char *files[]={"./Assets/user/cat.jpg","./Assets/legacy/wp2.png","./Assets/envy.svg","./Assets/wallpaper/stable/wp.jpg"};
    for(uint32_t i=0;i<4;i++){CHECK(images_load(files[i],72,44,&image)&&image.width&&image.height&&image.pixels,412+(int)i);}
    CHECK(!images_load("https://example.com/a.png",0,0,&image)&&!image.pixels,416);
    CHECK(!images_load("./Assets/no-such-image.png",0,0,&image),417);
    css_init();css_element element={"div","bg","",0};css_style style;
    css_compute(&element,0,"background: url('./Assets/envy.svg') center / contain no-repeat;",800,600,&style);
    framebuffer_rect(0,0,200,120,0);images_background(&style,0,0,100,100);
    CHECK(color_at(50,10)==0&&color_at(50,50)==0xbf49d7&&color_at(100,50)==0,418);
    css_compute(&element,0,"background: url('./Assets/envy.svg') center / cover no-repeat;",800,600,&style);
    framebuffer_rect(0,0,200,120,0);images_background(&style,0,0,100,100);
    CHECK(color_at(50,50)==0xbf49d7&&color_at(100,50)==0,419);
    css_compute(&element,0,"background: url('./Assets/envy.svg') 0 0 / 72px 44px repeat;",800,600,&style);
    framebuffer_rect(0,0,200,120,0);images_background(&style,0,0,180,100);
    CHECK(color_at(36,22)==0xbf49d7&&color_at(108,22)==0xbf49d7&&color_at(36,66)==0xbf49d7&&color_at(180,22)==0,420);
    style.value[CSS_BACKGROUND_REPEAT]=1;framebuffer_rect(0,0,200,120,0);images_background(&style,0,0,180,100);
    CHECK(color_at(108,22)==0xbf49d7&&color_at(36,66)==0,421);
    style.value[CSS_BACKGROUND_REPEAT]=2;framebuffer_rect(0,0,200,120,0);images_background(&style,0,0,180,100);
    CHECK(color_at(108,22)==0&&color_at(36,66)==0xbf49d7,422);
    css_compute(&element,0,"background: url('./Assets/envy.svg') right bottom / 72px 44px no-repeat;",800,600,&style);
    framebuffer_rect(0,0,200,120,0);images_background(&style,0,0,100,100);
    CHECK(color_at(64,78)==0xbf49d7&&color_at(36,22)==0,423);
    css_init();const char *sheet="#bg{position:absolute;left:10px;top:10px;width:100px;height:100px;background:#000 url('../Assets/envy.svg') center/contain no-repeat;}";
    CHECK(css_add_file(sheet,length(sheet),"/Envy/Styles/test.css"),424);
    html_view_begin("Images","/Envy/index.html");html_view_render("<body><div id='bg'></div><img src='./Assets/envy.svg' width='72' height='44' style='position:absolute;left:130px;top:10px;'></body>");
    CHECK(color_at(60,20)==0&&color_at(60,60)==0xbf49d7&&color_at(166,32)==0xbf49d7,425);
    /* Drawing is clipped to the viewport, including partially negative boxes. */
    framebuffer_rect(0,0,200,120,0);images_background(&style,-50,-50,100,100);
    CHECK(color_at(14,28)==0xbf49d7&&color_at(51,28)==0,426);
    css_init();sheet=".watermark{position:absolute;right:10px;top:0;width:80px;height:30px;background:red;} #lock{position:absolute;left:0;top:0;width:100%;height:100%;background:url('../Assets/wallpaper/stable/wp.jpg') center/cover no-repeat;z-index:-5;} #clock{position:absolute;left:40px;top:40px;color:white;}";
    CHECK(css_add_file(sheet,length(sheet),"/Envy/Styles/base.css"),427);
    html_view_begin("Lock screen","/Envy/index.html");html_view_render("<body></body>");html_view_text_lookup(lookup,0);
    /* The overlay is appended first, just like PrereleaseWatermark.js. */
    html_view_node(500,"div","","",0,"<div class='watermark'></div>",0);
    html_view_node(501,"div","","",0,"<div id='lock'><div id='clock'></div><img src='./Assets/envy.svg' width='72' height='44' style='position:absolute;left:150px;top:50px'></div>",0);
    CHECK(color_at((int32_t)framebuffer_get()->width-20,10)==0xff0000,428);
    uint32_t decodes=images_decode_count();html_view_repaint();
    CHECK(images_decode_count()==decodes&&color_at((int32_t)framebuffer_get()->width-20,10)==0xff0000,429);
    clock_text="12:35";html_view_repaint();
    CHECK(images_decode_count()==decodes&&color_at((int32_t)framebuffer_get()->width-20,10)==0xff0000,430);
    html_view_text_lookup(0,0);
    /* Composing a frame never clears or draws into the displayed framebuffer. */
    const framebuffer *fb=framebuffer_get();volatile uint8_t *front=fb->pixels;uint8_t saved[4];
    for(uint32_t i=0;i<fb->bytes_per_pixel;i++)saved[i]=front[i];
    framebuffer_begin_frame();framebuffer_rect(0,0,1,1,0x102030);
    for(uint32_t i=0;i<fb->bytes_per_pixel;i++)CHECK(front[i]==saved[i],431);
    framebuffer_end_frame();CHECK(color_at(0,0)==0x102030,432);
    css_init();html_view_begin("CSS layout","/Envy/index.html");
    html_view_render("<body><div style='position:absolute;left:10px;top:10px;width:200px;height:80px;box-sizing:border-box;padding:10px;display:flex;align-items:center;justify-content:center;gap:10px'><div style='width:40px;height:20px;background:red'></div><div style='width:40px;height:20px;background:green'></div></div></body>");
    CHECK(color_at(70,45)==0xff0000&&color_at(120,45)==0x008000&&color_at(30,45)==0xffffff,433);
    html_view_render("<body><div style='position:absolute;left:10px;top:10px;width:200px;height:30px;display:flex;gap:10px'><div style='flex:1;background:red'></div><div style='flex:1;background:blue'></div></div></body>");
    CHECK(color_at(90,20)==0xff0000&&color_at(110,20)==0xffffff&&color_at(120,20)==0x0000ff,434);
    html_view_render("<body><div style='position:absolute;left:10px;top:10px;width:100px;padding:5px 10px;background:black'><div style='height:20px;background:red'></div><div style='height:30px;background:blue'></div></div></body>");
    CHECK(color_at(15,12)==0&&color_at(25,20)==0xff0000&&color_at(25,40)==0x0000ff&&color_at(25,67)==0&&color_at(25,72)==0xffffff,435);
    html_view_render("<body><div style='position:absolute;left:10px;top:10px;width:80px;height:80px;background:rgba(0,0,0,.5);border:2px solid gray;border-radius:15px'></div></body>");
    CHECK(color_at(10,10)==0xffffff&&color_at(50,50)==0x7f7f7f&&color_at(11,50)==0x808080,436);
    html_view_render("<body><div style='position:absolute;left:10px;top:10px;width:100px;height:50px;overflow:hidden'><div style='width:200px;height:100px;background:red'></div></div><div style='position:absolute;left:130px;top:10px;width:50px;height:50px;background:blue;opacity:0'></div></body>");
    CHECK(color_at(20,20)==0xff0000&&color_at(115,20)==0xffffff&&color_at(20,65)==0xffffff&&color_at(140,20)==0xffffff,437);
    html_view_render("<body><div style='position:absolute;left:10px;top:10px;width:100px;height:20px;background:linear-gradient(to right,black,white)'></div></body>");
    CHECK(color_at(10,20)==0&&color_at(109,20)>0xf0f0f0,438);
    html_view_render("<body><div style='position:absolute;left:100px;top:100px;width:100px;height:80px;background:black'><div style='position:absolute;right:5px;bottom:5px;width:20px;height:20px;background:red'></div></div></body>");
    CHECK(color_at(180,160)==0xff0000&&color_at(110,110)==0&&color_at(80,60)==0xffffff,439);
    html_view_render("<body><div style='position:absolute;left:10px;top:10px;width:90px;display:flex;flex-wrap:wrap;gap:10px'><div style='width:40px;height:20px;background:red'></div><div style='width:40px;height:20px;background:blue'></div><div style='width:40px;height:20px;background:green'></div></div></body>");
    CHECK(color_at(20,20)==0xff0000&&color_at(70,20)==0x0000ff&&color_at(20,50)==0x008000,440);
    html_view_render("<body><div style='position:absolute;left:10px;top:10px;width:140px;display:grid;grid-template-columns:repeat(3, 1fr);gap:10px'><div style='height:20px;background:red'></div><div style='height:20px;background:blue'></div><div style='height:20px;background:green'></div><div style='height:20px;background:black'></div></div></body>");
    CHECK(color_at(20,20)==0xff0000&&color_at(70,20)==0x0000ff&&color_at(120,20)==0x008000&&color_at(20,50)==0,441);
    css_init();const char *hover_css="#hover{position:absolute;left:10px;top:10px;width:40px;height:40px;background:red;}#hover:hover{background:blue;}#hover:active{background:green;}";
    CHECK(css_add(hover_css,length(hover_css)),442);html_view_render("<body><div id='hover'></div></body>");
    html_view_pointer(20,20,0);CHECK(color_at(20,20)==0x0000ff,443);html_view_pointer(20,20,1);CHECK(color_at(20,20)==0x008000,444);
    html_view_pointer(200,200,0);CHECK(color_at(20,20)==0xff0000,445);
    css_init();html_view_begin("Scroll tests","/Envy/index.html");
    html_view_render("<body><div id='scroll' style='position:absolute;left:10px;top:10px;width:60px;height:40px;overflow:auto'><div id='red' style='height:40px;background:red'></div><div id='blue' style='height:40px;background:blue'></div></div><div style='position:fixed;left:120px;top:10px;width:20px;height:20px;background:green'></div></body>");
    CHECK(color_at(20,20)==0xff0000&&html_view_scroll(20,20,0,40)&&color_at(20,20)==0x0000ff&&color_at(20,55)==0xffffff,480);
    CHECK(!html_view_scroll(20,20,0,4000)&&html_view_scroll(20,20,0,-4000)&&color_at(20,20)==0xff0000,481);
    CHECK(html_view_scroll(20,20,0,40)&&same(html_view_target(20,20),"blue"),482);
    html_view_repaint();CHECK(color_at(20,20)==0x0000ff&&color_at(125,15)==0x008000,483);
    html_view_begin("Horizontal scroll","/Envy/index.html");
    html_view_render("<body><div style='position:absolute;left:10px;top:10px;width:50px;height:40px;overflow-x:auto;overflow-y:hidden'><div style='display:flex;width:100px;height:40px'><div style='width:50px;height:40px;background:red'></div><div style='width:50px;height:40px;background:blue'></div></div></div></body>");
    CHECK(color_at(20,20)==0xff0000&&html_view_scroll(20,20,50,0)&&color_at(20,20)==0x0000ff&&color_at(65,20)==0xffffff,484);
    html_view_begin("Nested scroll","/Envy/index.html");
    html_view_render("<body><div style='position:absolute;left:10px;top:10px;width:100px;height:60px;overflow:auto'><div style='height:40px;overflow:auto'><div style='height:40px;background:red'></div><div style='height:40px;background:blue'></div></div><div style='height:60px;background:green'></div></div></body>");
    CHECK(html_view_scroll(20,20,0,40)&&color_at(20,20)==0x0000ff,485);
    CHECK(html_view_scroll(20,20,0,40)&&color_at(20,20)==0x008000,486);
    html_view_begin("Page scroll","/Envy/index.html");
    html_view_render("<body><div style='height:800px;background:red'></div><div style='height:800px;background:blue'></div><div style='position:fixed;left:120px;top:10px;width:20px;height:20px;background:green'></div></body>");
    CHECK(html_view_scroll(20,20,0,800)&&color_at(20,20)==0x0000ff&&color_at(125,15)==0x008000,487);
    CHECK(html_view_scroll(20,20,0,-2147483647)&&color_at(20,20)==0xff0000,488);
    html_view_begin("Hidden overflow","/Envy/index.html");
    html_view_render("<body style='overflow:hidden'><div style='height:2000px;background:red'></div></body>");
    CHECK(!html_view_scroll(20,20,0,48),489);
    css_init();html_view_begin("Backdrop filters","/Envy/index.html");
    html_view_render("<body><div style='position:absolute;left:80px;top:0;width:80px;height:80px;background:black'></div><div style='position:absolute;left:20px;top:20px;width:120px;height:40px;border-radius:8px;background:rgba(86,85,88,0);backdrop-filter:brightness(.5)blur(4px)'></div></body>");
    CHECK(color_at(40,40)==0x808080&&color_at(120,40)==0&&color_at(20,20)==0xffffff&&color_at(40,70)==0xffffff,510);
    CHECK(color_at(79,40)>0&&color_at(79,40)<0x808080&&color_at(80,40)>0&&color_at(80,40)<color_at(79,40),511);
    html_view_repaint();CHECK(color_at(40,40)==0x808080,512);
    extern int browser_self_test(cdfs *filesystem);
    int failure=browser_self_test(fs);if(failure)return failure;
    const char *scripts[]={"/Envy/Libs/wm.correr.js","/Envy/Apps/Terminal.js"};
    const char *html="<body><div id='userland'></div><div id='taskbarApps'></div></body>";
    const char *verification="var __testPassed=terminal&&document.activeElement.tagName==='input';";
    failure=browser_test_scripts(fs,html,scripts,2,verification);
    if(failure)return 499+failure;
    html="<body><table id='sheet'><tbody><tr><td><input id='first' class='cell' value='A'/></td><td><input id='second' class='cell'/></td></tr></tbody></table><input id='outside'/></body>";
    verification="var cells=document.querySelectorAll('table input');var selected=0;cells.forEach(input=>{input.addEventListener('focus',()=>{selected=selected+1;});});cells[0].focus();var sheet=document.getElementById('sheet');var scoped=sheet.querySelectorAll('input');var __testPassed=selected===1&&cells.length===2&&scoped.length===2&&cells[0]===document.querySelector('#first')&&document.querySelectorAll('.missing').length===0&&document.querySelectorAll('#first, #second').length===2;";
    failure=browser_test_scripts(fs,html,0,0,verification);if(failure)return 513;
    const char *wm_assets[]={"/Envy/Styles/server.css","/Envy/Libs/wm.correr.js"};
    html="<body><div id='userland' style='position:absolute;left:0;top:0;width:800px;height:500px'></div><div id='taskbarApps' style='position:absolute;left:0;top:540px;width:800px;height:40px'></div></body>";
    failure=browser_test_scripts(fs,html,wm_assets,2,"createWindow('Test','<p>Window body</p>',400,300);var __testPassed=true;");CHECK(!failure,530);
    CHECK(browser_test_verify("var win=document.querySelector('.window');var windowId=win.id;var __testPassed=win.offsetWidth===400&&win.offsetHeight===300&&document.querySelectorAll('.resize-handle').length===8&&document.querySelectorAll('.window-header-buttons button').length===3;"),531);
    int32_t wx,wy,ww,wh;CHECK(browser_test_bounds(".window-header",&wx,&wy,&ww,&wh),532);
    input_event mouse={.type=INPUT_POINTER,.x=wx+20,.y=wy+10,.buttons=1};browser_input(&mouse);
    mouse.x+=40;mouse.y+=35;mouse.dx=40;mouse.dy=35;browser_input(&mouse);mouse.buttons=0;mouse.dx=mouse.dy=0;browser_input(&mouse);
    CHECK(browser_test_verify("var __testPassed=win.offsetLeft===60&&win.offsetTop===55;"),533);
    CHECK(browser_test_bounds(".resize-handle.right",&wx,&wy,&ww,&wh),534);
    mouse=(input_event){.type=INPUT_POINTER,.x=wx+1,.y=wy+20,.buttons=1};browser_input(&mouse);mouse.x+=30;mouse.dx=30;browser_input(&mouse);mouse.buttons=0;mouse.dx=0;browser_input(&mouse);
    CHECK(browser_test_verify("var __testPassed=win.offsetWidth===430&&win.offsetHeight===300;"),535);
    CHECK(browser_test_verify("minimizeWindow(windowId);var __testPassed=win.classList.contains('minimized');"),536);
    CHECK(browser_test_bounds(".taskbar-button",&wx,&wy,&ww,&wh),537);
    mouse=(input_event){.type=INPUT_POINTER,.x=wx+2,.y=wy+2,.buttons=1};browser_input(&mouse);mouse.buttons=0;browser_input(&mouse);
    CHECK(browser_test_verify("var __testPassed=!win.classList.contains('minimized');"),538);
    CHECK(browser_test_verify("maximizeWindow(windowId);var __testPassed=win.classList.contains('maximized');"),539);
    CHECK(browser_test_verify("var __testPassed=win.offsetWidth===800;maximizeWindow(windowId);"),540);
    CHECK(browser_test_verify("closeWindow(windowId);var __testPassed=win.classList.contains('close')&&document.querySelectorAll('.window').length===1;"),541);
    browser_test_advance(399);CHECK(browser_test_verify("var __testPassed=document.querySelectorAll('.window').length===1;"),543);
    browser_test_advance(1);CHECK(browser_test_verify("var __testPassed=document.querySelectorAll('.window').length===0&&document.querySelectorAll('.taskbar-button').length===0;"),544);
    CHECK(browser_test_verify("createWindow('A','A');createWindow('B','B');createWindow('C','C');createWindow('D','D');createWindow('E','E');createWindow('F','F');var wins=document.querySelectorAll('.window');var __testPassed=wins.length===6&&wins[0].id!==wins[1].id&&document.querySelectorAll('.taskbar-button').length===6;"),545);
    css_init();html_view_begin("Hover performance","/Envy/index.html");html_view_render("<body><div style='position:absolute;left:0;top:0;width:800px;height:600px;background:black'></div><div style='position:absolute;left:20px;top:20px;width:200px;height:200px;backdrop-filter:blur(10px)'></div></body>");
    uint32_t hover_paints=html_view_paint_count();for(int32_t i=0;i<100;i++)html_view_pointer(i*5,50,0);CHECK(html_view_paint_count()==hover_paints,542);
    const char *module_assets[]={"/Envy/Libs/html.js"};
    html="<body><div id='userland'></div></body>";
    failure=browser_test_scripts(fs,html,module_assets,1,"var __testPassed=false;import('/Envy/Libs/html.js').then(module=>{var Html=module.default;var parent=new Html('div').attr({id:'wrapped'}).appendTo('#userland');var child=new Html('span').text('Hello').classOn('one','two').style({'background-color':'red'}).attr({title:'test'});parent.appendMany(child,new Html('div'));var found=parent.query('.one');child.classOff('two');var optional=new Html('div').appendTo('#missing');var native=new Html('span').appendTo(parent.elm);var __ok=found===child.elm&&child.elm.classList.contains('one')&&!child.elm.classList.contains('two')&&child.elm.style['background-color']==='red'&&child.elm.title==='test'&&parent.elm instanceof HTMLElement&&native.elm.parentNode===parent.elm;parent.clear();__testPassed=__ok;});");CHECK(!failure,568);
    CHECK(browser_test_verify("var fetched=0;var caught=false;async function loadApps(){var response=await fetch('./UI/desktopapps.json');var data=await response.json();fetched=data.length;return fetched;}loadApps().then(count=>{__testPassed=count===10;});var __testPassed=false;"),569);
    CHECK(browser_test_verify("var resumed=0;async function later(){await new Promise(resolve=>setTimeout(resolve,25));resumed=1;}later();var __testPassed=resumed===0;"),570);
    browser_test_advance(24);CHECK(browser_test_verify("var __testPassed=resumed===0;"),571);
    browser_test_advance(1);CHECK(browser_test_verify("var __testPassed=resumed===1;"),572);
    const char *desktop_wm[]={"/Envy/Styles/base.css","/Envy/Libs/wm.correr.js"};
    html="<body><div id='userland'></div><div id='taskbarApps' style='position:fixed;left:0;top:550px;width:800px;height:50px'></div></body>";
    failure=browser_test_scripts(fs,html,desktop_wm,2,"var win=createWindow('Desktop test','Content',400,300);var windowId=win.id;var __testPassed=true;");CHECK(!failure,575);
    CHECK(browser_test_verify("var __testPassed=win.offsetWidth===400&&win.offsetHeight===300;"),576);
    CHECK(color_at(250,250)==0x1e1e1e,577);
    CHECK(browser_test_bounds(".resize-handle.left",&wx,&wy,&ww,&wh),578);
    mouse=(input_event){.type=INPUT_POINTER,.x=wx+1,.y=wy+80,.buttons=1};browser_input(&mouse);mouse.x+=300;mouse.dx=300;browser_input(&mouse);mouse.x+=20;mouse.dx=20;browser_input(&mouse);mouse.buttons=0;mouse.dx=0;browser_input(&mouse);
    CHECK(browser_test_verify("var __testPassed=win.offsetWidth===200&&win.offsetLeft===220;"),579);
    CHECK(browser_test_bounds(".window-minimize-btn",&wx,&wy,&ww,&wh),580);
    mouse=(input_event){.type=INPUT_POINTER,.x=wx+2,.y=wy+2,.buttons=1};browser_input(&mouse);mouse.buttons=0;browser_input(&mouse);
    CHECK(browser_test_verify("var __testPassed=win.classList.contains('minimized');"),581);
    CHECK(browser_test_bounds(".taskbar-button",&wx,&wy,&ww,&wh),582);
    mouse=(input_event){.type=INPUT_POINTER,.x=wx+2,.y=wy+2,.buttons=1};browser_input(&mouse);mouse.buttons=0;browser_input(&mouse);
    CHECK(browser_test_verify("var __testPassed=!win.classList.contains('minimized');"),583);
    CHECK(browser_test_bounds(".window-maximize-btn",&wx,&wy,&ww,&wh),584);
    mouse=(input_event){.type=INPUT_POINTER,.x=wx+2,.y=wy+2,.buttons=1};browser_input(&mouse);mouse.buttons=0;browser_input(&mouse);
    CHECK(browser_test_verify("var __testPassed=win.classList.contains('maximized')&&win.offsetWidth===800&&win.offsetHeight===527;"),585);
    CHECK(browser_test_bounds(".window-close-btn",&wx,&wy,&ww,&wh),586);
    mouse=(input_event){.type=INPUT_POINTER,.x=wx+2,.y=wy+2,.buttons=1};browser_input(&mouse);mouse.buttons=0;browser_input(&mouse);browser_test_advance(400);
    CHECK(browser_test_verify("var __testPassed=document.querySelectorAll('.window').length===0&&document.querySelectorAll('.taskbar-button').length===0;"),587);
    return 0;
}
