#include "ttf.h"
static uint16_t u16(const uint8_t *p){return (uint16_t)((uint16_t)p[0]<<8|p[1]);}
static int32_t s16(const uint8_t *p){return (int16_t)u16(p);}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int table(const uint8_t *d,uint32_t size,uint32_t tag,uint32_t *offset,uint32_t *count){
    for(uint32_t i=0;i<u16(d+4);i++){const uint8_t *p=d+12+i*16;
        if(u32(p)==tag){*offset=u32(p+8);*count=u32(p+12);return *offset<=size&&*count<=size-*offset;}}
    return 0;
}
int ttf_init(ttf_font *f,const void *bytes,uint32_t size){
    *f=(ttf_font){0};const uint8_t *d=bytes;uint32_t head,hn,hhea,an,maxp,mn,cn,ln,tn;
    if(!d||size<12||u32(d)!=0x00010000u||u16(d+4)>(size-12)/16)return 0;
    ttf_font n={.data=d,.size=size};
    if(!table(d,size,0x68656164,&head,&hn)||hn<54||!table(d,size,0x68686561,&hhea,&an)||an<36||
       !table(d,size,0x6d617870,&maxp,&mn)||mn<6||!table(d,size,0x636d6170,&cn,&tn)||tn<4||
       !table(d,size,0x676c7966,&n.glyf,&n.glyf_size)||!table(d,size,0x6c6f6361,&n.loca,&ln)||
       !table(d,size,0x686d7478,&n.hmtx,&mn))return 0;
    n.units=u16(d+head+18);n.glyphs=u16(d+maxp+4);n.metrics=u16(d+hhea+34);
    n.ascent=(int16_t)u16(d+hhea+4);n.descent=(int16_t)u16(d+hhea+6);n.long_loca=s16(d+head+50);
    if(n.units<16||n.units>16384||!n.glyphs||!n.metrics||n.metrics>n.glyphs||n.long_loca<0||n.long_loca>1||
       ln<((uint32_t)n.glyphs+1)*(n.long_loca?4u:2u)||mn<(uint32_t)n.metrics*4+(n.glyphs-n.metrics)*2u)return 0;
    uint32_t previous=0;
    for(uint32_t i=0;i<=(uint32_t)n.glyphs;i++){
        uint32_t off=n.long_loca?u32(d+n.loca+i*4):(uint32_t)u16(d+n.loca+i*2)*2;
        if(off<previous||off>n.glyf_size)return 0;previous=off;
    }
    if(u16(d+cn+2)>(tn-4)/8)return 0;
    int best=0;
    for(uint32_t i=0;i<u16(d+cn+2);i++){
        const uint8_t *record=d+cn+4+i*8;uint16_t platform=u16(record),encoding=u16(record+2);
        if(platform!=0&&!(platform==3&&(encoding==1||encoding==10)))continue;
        uint32_t off=u32(record+4);if(off>tn-2)continue;const uint8_t *p=d+cn+off;
        uint32_t format=u16(p),len=0;int score=0;
        if(format==4&&tn-off>=16){len=u16(p+2);score=1;uint32_t seg=u16(p+6)/2;
            if(!seg||(u16(p+6)&1)||len<16+seg*8)continue;}
        else if(format==12&&tn-off>=16){len=u32(p+4);score=2;if(len<16||u32(p+12)>(len-16)/12)continue;}
        if(!score||len>tn-off||score<=best)continue;
        n.cmap=cn+off;n.cmap_size=len;best=score;
    }
    if(!best)return 0;*f=n;return 1;
}
uint32_t ttf_glyph(const ttf_font *f,uint32_t cp){
    if(!f||!f->data)return 0;const uint8_t *p=f->data+f->cmap;uint32_t glyph=0;
    if(u16(p)==12){uint32_t lo=0,hi=u32(p+12);
        while(lo<hi){uint32_t mid=lo+(hi-lo)/2;const uint8_t *g=p+16+mid*12;
            if(cp<u32(g))hi=mid;else if(cp>u32(g+4))lo=mid+1;else{uint32_t delta=cp-u32(g);if(delta>0xffffffffu-u32(g+8))return 0;glyph=u32(g+8)+delta;break;}}}
    else if(cp<=65535){uint32_t seg=u16(p+6)/2;
        for(uint32_t i=0;i<seg;i++)if(cp<=u16(p+14+i*2)){
            if(cp<u16(p+16+seg*2+i*2))break;
            uint32_t delta=u16(p+16+seg*4+i*2),range=u16(p+16+seg*6+i*2);
            if(!range)glyph=(cp+delta)&65535;
            else{uint32_t off=16+seg*6+i*2+range+(cp-u16(p+16+seg*2+i*2))*2;
                if(off>f->cmap_size-2)return 0;glyph=u16(p+off);if(glyph)glyph=(glyph+delta)&65535;}break;
        }}
    return glyph<f->glyphs?glyph:0;
}
int32_t ttf_advance(const ttf_font *f,uint32_t glyph,uint32_t pixels){
    if(!f||!f->data||glyph>=f->glyphs||pixels>256)return 0;
    uint32_t index=glyph<f->metrics?glyph:f->metrics-1u;
    return (int32_t)(((uint32_t)u16(f->data+f->hmtx+index*4)*pixels+f->units/2)/f->units);
}
#define POINTS 4096
#define CONTOURS 512
#define EDGES 8192
typedef struct {int32_t x,y;uint8_t on;} point;
typedef struct {int32_t x0,y0,x1,y1;} edge;
static point points[POINTS];static uint16_t ends[CONTOURS];static uint8_t flags[POINTS];
static edge edges[EDGES];static uint32_t point_count,contour_count,edge_count;
static int32_t minimum_x,maximum_x,minimum_y,maximum_y;
static int outline(const ttf_font *f,uint32_t glyph,uint32_t depth){
    if(glyph>=f->glyphs||depth>8)return 0;const uint8_t *d=f->data;
    uint32_t off=f->long_loca?u32(d+f->loca+glyph*4):(uint32_t)u16(d+f->loca+glyph*2)*2;
    uint32_t last=f->long_loca?u32(d+f->loca+(glyph+1)*4):(uint32_t)u16(d+f->loca+(glyph+1)*2)*2;
    if(off==last)return 1;if(last-off<10)return 0;
    const uint8_t *p=d+f->glyf+off,*end=d+f->glyf+last;int32_t contours=s16(p);p+=10;
    uint32_t base=point_count;
    if(contours>=0){
        if(!contours)return 1;
        if((uint32_t)contours>CONTOURS-contour_count||(uint32_t)(end-p)<(uint32_t)contours*2+2)return 0;
        uint32_t total=(uint32_t)u16(p+(contours-1)*2)+1;
        if(total>POINTS-base)return 0;
        uint32_t previous=0;
        for(int32_t i=0;i<contours;i++){uint32_t n=u16(p+i*2);if(n>=total||(i&&n<=previous))return 0;
            ends[contour_count++]=(uint16_t)(base+n);previous=n;}
        p+=contours*2;uint32_t instructions=u16(p);p+=2;if(instructions>(uint32_t)(end-p))return 0;p+=instructions;
        for(uint32_t i=0;i<total;){if(p==end)return 0;uint8_t flag=*p++;uint32_t repeat=0;
            if(flag&8){if(p==end)return 0;repeat=*p++;}if(repeat>=total-i)return 0;
            do{flags[i++]=flag;}while(repeat--);}
        for(uint32_t axis=0;axis<2;axis++){int32_t coordinate=0;
            for(uint32_t i=0;i<total;i++){uint8_t flag=flags[i];int32_t delta=0;uint8_t short_bit=axis?4:2,same_bit=axis?32:16;
                if(flag&short_bit){if(p==end)return 0;delta=*p++;if(!(flag&same_bit))delta=-delta;}
                else if(!(flag&same_bit)){if(end-p<2)return 0;delta=s16(p);p+=2;}
                coordinate+=delta;if(coordinate<-32768||coordinate>32767)return 0;
                if(axis)points[base+i].y=coordinate;else points[base+i].x=coordinate;points[base+i].on=flag&1;
            }}
        point_count+=total;return 1;
    }
    uint32_t components=0;uint16_t bits;
    do{
        if(end-p<4||++components>512)return 0;bits=u16(p);uint32_t child=u16(p+2);p+=4;
        int32_t a,b,xx=16384,xy=0,yx=0,yy=16384;
        if(bits&1){if(end-p<4)return 0;a=bits&2?s16(p):u16(p);b=bits&2?s16(p+2):u16(p+2);p+=4;}
        else{if(end-p<2)return 0;a=bits&2?(int8_t)p[0]:p[0];b=bits&2?(int8_t)p[1]:p[1];p+=2;}
        if(bits&8){if(end-p<2)return 0;xx=yy=s16(p);p+=2;}
        else if(bits&64){if(end-p<4)return 0;xx=s16(p);yy=s16(p+2);p+=4;}
        else if(bits&128){if(end-p<8)return 0;xx=s16(p);yx=s16(p+2);xy=s16(p+4);yy=s16(p+6);p+=8;}
        uint32_t start=point_count;if(!outline(f,child,depth+1))return 0;
        for(uint32_t i=start;i<point_count;i++){int32_t x=points[i].x,y=points[i].y;
            points[i].x=(int32_t)(((int64_t)x*xx+(int64_t)y*xy)>>14);
            points[i].y=(int32_t)(((int64_t)x*yx+(int64_t)y*yy)>>14);}
        int32_t dx=a,dy=b;
        if(!(bits&2)){if(a<0||b<0||(uint32_t)a>=start-base||(uint32_t)b>=point_count-start)return 0;
            dx=points[base+(uint32_t)a].x-points[start+(uint32_t)b].x;dy=points[base+(uint32_t)a].y-points[start+(uint32_t)b].y;}
        else if(bits&0x800){dx=(int32_t)(((int64_t)a*xx+(int64_t)b*xy)>>14);dy=(int32_t)(((int64_t)a*yx+(int64_t)b*yy)>>14);}
        for(uint32_t i=start;i<point_count;i++){points[i].x+=dx;points[i].y+=dy;
            if(points[i].x<-32768||points[i].x>32767||points[i].y<-32768||points[i].y>32767)return 0;}
    }while(bits&32);
    if(bits&256){if(end-p<2)return 0;uint32_t n=u16(p);p+=2;if(n>(uint32_t)(end-p))return 0;}
    return 1;
}
static int line(point a,point b){
    if(a.y==b.y)return 1;if(edge_count==EDGES)return 0;
    edges[edge_count++]=(edge){a.x,a.y,b.x,b.y};return 1;
}
static point midpoint(point a,point b){return (point){(a.x+b.x)/2,(a.y+b.y)/2,1};}
static int curve(point a,point b,point c,uint32_t depth){
    int32_t dx=a.x-2*b.x+c.x,dy=a.y-2*b.y+c.y;
    if(depth==8||(dx>=-1&&dx<=1&&dy>=-1&&dy<=1))return line(a,c);
    point ab=midpoint(a,b),bc=midpoint(b,c),middle=midpoint(ab,bc);
    return curve(a,ab,middle,depth+1)&&curve(middle,bc,c,depth+1);
}
typedef struct {int32_t x,direction;} crossing;
static crossing crossings[EDGES];static uint8_t coverage[4096];
int ttf_render(const ttf_font *f,uint32_t glyph,uint32_t pixels,ttf_pixel pixel,void *context){
    if(!f||!f->data||!pixel||!pixels||pixels>256)return 0;
    point_count=contour_count=edge_count=0;if(!outline(f,glyph,0))return 0;if(!point_count)return 1;
    minimum_x=minimum_y=16384;maximum_x=maximum_y=-16384;
    for(uint32_t i=0;i<point_count;i++){
        points[i].x=points[i].x*(int32_t)pixels*4/f->units;points[i].y=-points[i].y*(int32_t)pixels*4/f->units;
        if(points[i].x<-8192||points[i].x>8192||points[i].y<-8192||points[i].y>8192)return 0;
        if(points[i].x<minimum_x)minimum_x=points[i].x;if(points[i].x>maximum_x)maximum_x=points[i].x;
        if(points[i].y<minimum_y)minimum_y=points[i].y;if(points[i].y>maximum_y)maximum_y=points[i].y;
    }
    uint32_t first=0;
    for(uint32_t contour=0;contour<contour_count;contour++){
        uint32_t last=ends[contour];point start;uint32_t count=last-first+1,index;
        if(points[first].on){start=points[first];index=first+1;count--;}
        else if(points[last].on){start=points[last];index=first;count--;}
        else{start=midpoint(points[last],points[first]);index=first;}
        point pen=start;
        while(count){if(index>last)index=first;point next=points[index++];count--;
            if(next.on){if(!line(pen,next))return 0;pen=next;}
            else{point after=count?points[index>last?first:index]:start;
                if(after.on){if(!curve(pen,next,after,0))return 0;pen=after;if(count){index++;count--;}}
                else{point middle=midpoint(next,after);if(!curve(pen,next,middle,0))return 0;pen=middle;}}
        }
        if(!line(pen,start))return 0;first=last+1;
    }
    /* Four by four coverage samples, nonzero winding fill (preserves holes). */
    int32_t left=minimum_x>=0?minimum_x/4:(minimum_x-3)/4;
    int32_t right=(maximum_x+3)/4,top=minimum_y>=0?minimum_y/4:(minimum_y-3)/4;
    int32_t bottom=(maximum_y+3)/4,width=right-left;if(width<=0)return 1;if(width>4096)return 0;
    for(int32_t y=top;y<bottom;y++){
        for(int32_t x=0;x<width;x++)coverage[x]=0;
        for(int32_t sub=0;sub<4;sub++){
            int32_t sample=(y*4+sub)*2+1;uint32_t count=0;
            for(uint32_t i=0;i<edge_count;i++){
                edge e=edges[i];int32_t y0=e.y0*2,y1=e.y1*2;
                if(!((y0<=sample&&sample<y1)||(y1<=sample&&sample<y0)))continue;
                int32_t x=e.x0*2+(e.x1-e.x0)*2*(sample-y0)/(y1-y0);
                crossing value={x,y1>y0?1:-1};uint32_t j=count++;
                while(j&&crossings[j-1].x>x){crossings[j]=crossings[j-1];j--;}crossings[j]=value;
            }
            int32_t winding=0,begin=0;
            for(uint32_t i=0;i<count;){int32_t x=crossings[i].x,old=winding;
                do{winding+=crossings[i++].direction;}while(i<count&&crossings[i].x==x);
                if(!old&&winding)begin=x;
                else if(old&&!winding){for(int32_t sx=begin/2-1;sx<=x/2+1;sx++){
                    if(sx*2+1<begin||sx*2+1>=x)continue;int32_t px=sx>=0?sx/4:(sx-3)/4;
                    if(px>=left&&px<right)coverage[px-left]++;}}
            }
        }
        for(int32_t x=0;x<width;x++)if(coverage[x])pixel(context,left+x,y,(uint8_t)((coverage[x]*255+8)/16));
    }
    return 1;
}
uint32_t ttf_utf8(const char **text){
    const uint8_t *p=(const uint8_t *)*text;if(!*p)return 0;uint32_t cp=*p++,count=0,minimum=0;
    if(cp<128){*text=(const char *)p;return cp;}
    if(cp>=0xc2&&cp<=0xdf){cp&=31;count=1;minimum=128;}
    else if(cp>=0xe0&&cp<=0xef){cp&=15;count=2;minimum=2048;}
    else if(cp>=0xf0&&cp<=0xf4){cp&=7;count=3;minimum=65536;}
    else{*text=(const char *)p;return 0xfffd;}
    for(uint32_t i=0;i<count;i++){if((*p&0xc0)!=0x80){*text=(const char *)p;return 0xfffd;}cp=(cp<<6)|(*p++&63);}
    *text=(const char *)p;return cp<minimum||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff)?0xfffd:cp;
}
