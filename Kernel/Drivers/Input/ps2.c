#include "ps2.h"
#include "../../Arch/x86/io.h"
static input_event queue[64];static uint32_t read_index,write_index;
static uint8_t extended,pause_bytes,keys[256],modifiers,mouse[4],mouse_used,buttons,mouse_id;
static int32_t pointer_x,pointer_y,max_x,max_y;
static void push(input_event event){
    if(event.type==INPUT_POINTER&&!event.wheel&&read_index!=write_index){input_event *last=&queue[(write_index+63)%64];
        if(last->type==INPUT_POINTER&&!last->wheel&&last->buttons==event.buttons&&last->modifiers==event.modifiers){
            last->x=event.x;last->y=event.y;last->dx+=event.dx;last->dy+=event.dy;return;}}
    uint32_t next=(write_index+1)%64;if(next==read_index)read_index=(read_index+1)%64;queue[write_index]=event;write_index=next;
}
void input_reset(uint32_t width,uint32_t height) {
    read_index=write_index=0;extended=pause_bytes=modifiers=mouse_used=buttons=mouse_id=0;
    for(uint32_t i=0;i<256;i++)keys[i]=0;
    max_x=width?(int32_t)width-1:0;max_y=height?(int32_t)height-1:0;pointer_x=max_x/2;pointer_y=max_y/2;
}
void input_mouse_id(uint8_t id){mouse_id=id==3||id==4?id:0;mouse_used=0;}
void input_feed(uint8_t status,uint8_t byte) {
    if(status&0xc0){mouse_used=extended=pause_bytes=0;return;}
    if(status&0x20) {
        if(!mouse_used&&(!(byte&8)||byte==0xfa||byte==0xfe))return;
        mouse[mouse_used++]=byte;if(mouse_used<(mouse_id?4:3))return;mouse_used=0;
        int32_t dx=(int32_t)mouse[1]-((mouse[0]&16)?256:0),dy=(int32_t)mouse[2]-((mouse[0]&32)?256:0);
        if(mouse[0]&0xc0)dx=dy=0;pointer_x+=dx;pointer_y-=dy;
        if(pointer_x<0)pointer_x=0;if(pointer_x>max_x)pointer_x=max_x;
        if(pointer_y<0)pointer_y=0;if(pointer_y>max_y)pointer_y=max_y;
        int32_t wheel=mouse_id?(int32_t)(int8_t)mouse[3]:0;
        if(mouse_id==4){wheel=mouse[3]&15;if(wheel&8)wheel-=16;}
        buttons=(mouse[0]&7)|(mouse_id==4?(mouse[3]&0x30)>>1:0);
        push((input_event){.type=INPUT_POINTER,.buttons=buttons,.modifiers=modifiers,.x=pointer_x,.y=pointer_y,.dx=dx,.dy=-dy,.wheel=wheel});return;
    }
    if(pause_bytes){pause_bytes--;return;}
    if(byte==0xe1){pause_bytes=5;extended=0;return;}
    if(byte==0xe0){extended=1;return;}
    if(byte==0xfa||byte==0xfe)return;
    uint8_t scan=byte&127,is_extended=extended;extended=0;
    if(is_extended&&(scan==0x2a||scan==0x36))return; /* Print Screen's synthetic Shift bytes. */
    uint16_t key=(uint16_t)(scan+(is_extended?128:0));int pressed=!(byte&128),repeat=pressed&&keys[key];keys[key]=(uint8_t)pressed;
    if(scan==0x3a&&pressed&&!repeat)modifiers^=INPUT_CAPS;
    modifiers&=INPUT_CAPS;
    if(keys[0x2a]||keys[0x36])modifiers|=INPUT_SHIFT;
    if(keys[0x1d]||keys[0x9d])modifiers|=INPUT_CONTROL;
    if(keys[0x38]||keys[0xb8])modifiers|=INPUT_ALT;
    static const char normal[128]={
        [2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',[12]='-',[13]='=',[14]='\b',[15]='\t',
        [16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',[22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[28]='\n',
        [30]='a',[31]='s',[32]='d',[33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]='\'',[41]='`',[43]='\\',
        [44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',[53]='/',[55]='*',[57]=' '};
    static const char shifted[128]={[2]='!',[3]='@',[4]='#',[5]='$',[6]='%',[7]='^',[8]='&',[9]='*',[10]='(',[11]=')',[12]='_',[13]='+',[26]='{',[27]='}',[39]=':',[40]='"',[41]='~',[43]='|',[51]='<',[52]='>',[53]='?'};
    uint8_t character=is_extended?(scan==28?'\n':0):(uint8_t)normal[scan];
    if(character>='a'&&character<='z'){if(!!(modifiers&INPUT_SHIFT)!=!!(modifiers&INPUT_CAPS))character-=32;}
    else if(!is_extended&&(modifiers&INPUT_SHIFT)&&shifted[scan])character=(uint8_t)shifted[scan];
    push((input_event){.type=INPUT_KEY,.key=(uint16_t)(scan+(is_extended?0x100:0)),.scancode=byte,.character=pressed?character:0,.modifiers=modifiers,.pressed=(uint8_t)pressed,.repeat=(uint8_t)repeat});
}
int input_next(input_event *event){if(read_index==write_index)return 0;*event=queue[read_index];read_index=(read_index+1)%64;return 1;}
static int writable(void){for(uint32_t n=0;n<100000;n++)if(!(in8(0x64)&2))return 1;return 0;}
static int command(uint8_t value){if(!writable())return 0;out8(0x64,value);return 1;}
static int data(uint8_t value){if(!writable())return 0;out8(0x60,value);return 1;}
static int device_command(int auxiliary,uint8_t value) {
    for(uint32_t attempt=0;attempt<3;attempt++) {
        if(auxiliary&&!command(0xd4))return 0;if(!data(value))return 0;
        for(uint32_t n=0;n<100000;n++) {
            uint8_t status=in8(0x64);if(!(status&1))continue;uint8_t response=in8(0x60);
            if(!!(status&0x20)==!!auxiliary&&!(status&0xc0)){if(response==0xfa)return 1;if(response==0xfe)break;}
            input_feed(status,response);
        }
    }
    return 0;
}
int input_init(uint32_t width,uint32_t height) {
    input_reset(width,height);if(!command(0xad)||!command(0xa7))return 0;
    for(uint32_t n=0;n<64&&(in8(0x64)&1);n++)(void)in8(0x60);
    if(!command(0x20))return 0;uint8_t configuration=0;int received=0;
    for(uint32_t n=0;n<100000;n++)if(in8(0x64)&1){configuration=in8(0x60);received=1;break;}
    if(!received||!command(0x60)||!data((uint8_t)((configuration|0x40)&~0x33u)))return 0;
    if(!command(0xae)||!command(0xa8))return 0;
    int keyboard=device_command(0,0xf0)&&device_command(0,2)&&device_command(0,0xf4);
    int mouse_ok=device_command(1,0xf6);
    if(mouse_ok){
        /* IntelliMouse's sample-rate sequence enables four-byte wheel packets. */
        int wheel=device_command(1,0xf3)&&device_command(1,200)&&device_command(1,0xf3)&&device_command(1,100)&&device_command(1,0xf3)&&device_command(1,80)&&device_command(1,0xf2);
        if(wheel)for(uint32_t n=0;n<100000;n++){uint8_t status=in8(0x64);if(!(status&1))continue;uint8_t id=in8(0x60);
            if((status&0x20)&&!(status&0xc0)){input_mouse_id(id);break;}input_feed(status,id);}
        mouse_ok=device_command(1,0xf4);
    }
    return (keyboard?1:0)|(mouse_ok?2:0);
}
void input_poll(void){for(uint32_t n=0;n<64;n++){uint8_t status=in8(0x64);if(!(status&1))break;input_feed(status,in8(0x60));}}
