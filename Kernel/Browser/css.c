#include "css.h"
#include "font.h"
#include "images.h"
typedef struct { const char *selector,*body;uint32_t selector_length,body_length;char path[256];int32_t min_width,max_width,min_height,max_height; } rule;
typedef struct { char name[64],value[256];uint32_t priority; } variable;
static rule rules[1024];static uint32_t rule_count;
static variable variables[128];static uint32_t variable_count;
/* Intern mixed lengths rather than resolving percentages against the viewport.
   Their actual basis is only known when the containing box has been laid out. */
#define EXPRESSION_BASE 0x20000000
typedef struct {int32_t px,percent,em;int scalar;} expression;
static expression expressions[4096];static uint32_t expression_count;
static int32_t font_basis=16;
static int32_t root_font_basis=16;
static int (*state_lookup)(const css_element *,const char *);
static int add_rules(const char *source,uint32_t size,const char *path,const int32_t bounds[4],uint32_t depth);
void css_state_lookup(int (*lookup)(const css_element *,const char *)){state_lookup=lookup;}
static int space(char c) {return c==' '||c=='\n'||c=='\r'||c=='\t';}
static char lower(char c) {return c>='A'&&c<='Z'?(char)(c+32):c;}
static uint32_t length(const char *s) {uint32_t n=0;while(s[n])n++;return n;}
static int equal(const char *s,uint32_t n,const char *text,int folded) {
    if(n!=length(text))return 0;
    for(uint32_t i=0;i<n;i++)if((folded?lower(s[i]):s[i])!=(folded?lower(text[i]):text[i]))return 0;
    return 1;
}
static const char *skip(const char *p,const char *end) {
    for(;;) {
        while(p<end&&space(*p))p++;
        if(end-p<2||p[0]!='/'||p[1]!='*')return p;
        p+=2;while(end-p>=2&&!(p[0]=='*'&&p[1]=='/'))p++;
        if(end-p<2)return end;p+=2;
    }
}
static int name_char(char c) {return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_';}
static int member(const char *classes,const char *s,uint32_t n) {
    while(*classes) {
        while(space(*classes))classes++;const char *begin=classes;
        while(*classes&&!space(*classes))classes++;
        if((uint32_t)(classes-begin)==n) {uint32_t i=0;while(i<n&&begin[i]==s[i])i++;if(i==n)return 1;}
    }
    return 0;
}
static int simple_depth(const char *s,uint32_t n,const css_element *e,uint32_t depth) {
    if(depth>8)return -1;
    int score=0;uint32_t i=0;
    if(i<n&&s[i]=='*')i++;
    else if(i<n&&name_char(s[i])) {
        uint32_t begin=i;while(i<n&&name_char(s[i]))i++;
        if(!equal(s+begin,i-begin,e->tag,1))return -1;score++;
    }
    while(i<n) {
        char kind=s[i++];uint32_t begin=i;while(i<n&&name_char(s[i]))i++;
        if(i==begin)return -1;
        if(kind=='#') {if(!equal(s+begin,i-begin,e->id,0))return -1;score+=100;}
        else if(kind=='.') {if(!member(e->classes,s+begin,i-begin))return -1;score+=10;}
        else if(kind==':'&&equal(s+begin,i-begin,"root",1)&&!e->parent)score+=10;
        else if(kind==':'&&(equal(s+begin,i-begin,"hover",1)||equal(s+begin,i-begin,"active",1))){
            const char *state=equal(s+begin,i-begin,"hover",1)?"hover":"active";if(!state_lookup||!state_lookup(e,state))return -1;score+=10;}
        else if(kind==':'&&equal(s+begin,i-begin,"not",1)&&i<n&&s[i]=='('){uint32_t start=++i;int nesting=1;
            while(i<n&&nesting){if(s[i]=='(')nesting++;if(s[i]==')')nesting--;if(nesting)i++;}
            if(nesting)return -1;int sub=simple_depth(s+start,i-start,e,depth+1);if(sub>=0)return -1;i++;score+=10;}
        else return -1;
    }
    return score;
}
static int matches(const char *s,uint32_t n,const css_element *e,uint32_t depth) {
    if(!e||depth>32)return -1;
    while(n&&space(s[n-1]))n--;
    uint32_t begin=n;int nesting=0;while(begin){char c=s[begin-1];if(c==')')nesting++;else if(c=='(')nesting--;
        if(!nesting&&(space(c)||c=='>'))break;begin--;}
    int score=simple_depth(s+begin,n-begin,e,0);if(score<0)return -1;
    while(begin&&space(s[begin-1]))begin--;
    if(!begin)return score;
    if(s[begin-1]=='>') {int parent=matches(s,begin-1,e->parent,depth+1);return parent<0?-1:parent+score;}
    for(e=e->parent;e;e=e->parent) {int parent=matches(s,begin,e,depth+1);if(parent>=0)return parent+score;}
    return -1;
}
int css_selector_matches(const css_element *e,const char *selector){
    const char *part=selector;int nesting=0;
    for(const char *p=selector;;p++){
        if(*p=='(')nesting++;else if(*p==')')nesting--;
        if(!*p||(*p==','&&!nesting)){const char *start=part;while(start<p&&space(*start))start++;
            if(start<p&&matches(start,(uint32_t)(p-start),e,0)>=0)return 1;
            if(!*p)break;part=p+1;}
    }
    return 0;
}
static int number(const char **source,int32_t *value) {
    const char *p=*source;while(space(*p))p++;int sign=1;if(*p=='-'){sign=-1;p++;}else if(*p=='+')p++;
    if(*p<'0'||*p>'9')return 0;int32_t n=0;
    while(*p>='0'&&*p<='9'){if(n>10000)return 0;n=n*10+(*p++-'0');}
    if(*p=='.'){p++;while(*p>='0'&&*p<='9')p++;}
    *value=n*sign;*source=p;return 1;
}
static int decimal(const char **source,int32_t *value){
    const char *p=*source;while(space(*p))p++;int sign=1;
    if(*p=='-'||*p=='+'){if(*p=='-')sign=-1;p++;}
    int digits=0;int32_t whole=0,fraction=0,scale=100;
    while(*p>='0'&&*p<='9'){if(whole>100000)return 0;whole=whole*10+*p++-'0';digits=1;}
    if(*p=='.'){p++;while(*p>='0'&&*p<='9'){if(scale){fraction+=(*p-'0')*scale;scale/=10;}p++;digits=1;}}
    if(!digits)return 0;*value=sign*(whole*1000+fraction);*source=p;return 1;
}
static int expression_value(expression v,int32_t *out){
    if(!v.em&&!v.percent&&v.px%1000==0){*out=v.px/1000;return 1;}
    if(!v.em&&!v.px&&v.percent%1000==0&&v.percent>-10000000&&v.percent<10000000){*out=CSS_PERCENT+v.percent/1000;return 1;}
    v.scalar=0;
    for(uint32_t i=0;i<expression_count;i++)if(expressions[i].px==v.px&&expressions[i].percent==v.percent&&expressions[i].em==v.em){*out=EXPRESSION_BASE+(int32_t)i;return 1;}
    if(expression_count==4096)return 0;expressions[expression_count]=v;*out=EXPRESSION_BASE+(int32_t)expression_count++;return 1;
}
static int sum(const char **source,expression *out,uint32_t width,uint32_t height,uint32_t depth);
static int atom(const char **source,expression *v,uint32_t width,uint32_t height,uint32_t depth){
    const char *p=*source;while(space(*p))p++;if(depth>8)return 0;*v=(expression){0};
    if(*p=='('||(p[0]=='c'&&p[1]=='a'&&p[2]=='l'&&p[3]=='c'&&p[4]=='(')){
        p+=*p=='('?1:5;if(!sum(&p,v,width,height,depth+1))return 0;while(space(*p))p++;
        if(*p++!=')')return 0;*source=p;return 1;
    }
    int32_t n;if(!decimal(&p,&n))return 0;
    if(*p=='%'){v->percent=n;p++;}
    else if(p[0]=='p'&&p[1]=='x'){v->px=n;p+=2;}
    else if(p[0]=='r'&&p[1]=='e'&&p[2]=='m'){v->px=n*root_font_basis;p+=3;}
    else if(p[0]=='e'&&p[1]=='m'){v->em=n;p+=2;}
    else if(p[0]=='v'&&(p[1]=='w'||p[1]=='h')){v->px=(int32_t)((int64_t)n*(p[1]=='w'?width:height)/100);p+=2;}
    else if(name_char(*p))return 0;
    else {v->px=n;v->scalar=1;}
    *source=p;return 1;
}
static int product(const char **source,expression *v,uint32_t width,uint32_t height,uint32_t depth){
    const char *p=*source;if(!atom(&p,v,width,height,depth))return 0;
    for(;;){const char *op=p;while(space(*op))op++;if(*op!='*'&&*op!='/')break;
        char kind=*op++;expression right;if(!atom(&op,&right,width,height,depth))return 0;
        if(kind=='*'&&!v->scalar&&!right.scalar)return 0;
        if(kind=='/'&&(!right.scalar||!right.px))return 0;
        if(kind=='*'&&v->scalar&&!right.scalar){expression tmp=*v;*v=right;right=tmp;}
        int32_t scale=right.px;
        if(kind=='*'){v->px=(int32_t)((int64_t)v->px*scale/1000);v->percent=(int32_t)((int64_t)v->percent*scale/1000);v->em=(int32_t)((int64_t)v->em*scale/1000);}
        else{v->px=(int32_t)((int64_t)v->px*1000/scale);v->percent=(int32_t)((int64_t)v->percent*1000/scale);v->em=(int32_t)((int64_t)v->em*1000/scale);}
        v->scalar=v->scalar&&right.scalar;p=op;
    }*source=p;return 1;
}
static int sum(const char **source,expression *v,uint32_t width,uint32_t height,uint32_t depth){
    const char *p=*source;if(!product(&p,v,width,height,depth))return 0;
    for(;;){const char *op=p;while(space(*op))op++;if(*op!='+'&&*op!='-')break;
        int sign=*op++=='+'?1:-1;expression right;if(!product(&op,&right,width,height,depth)||v->scalar!=right.scalar)return 0;
        v->px+=sign*right.px;v->percent+=sign*right.percent;v->em+=sign*right.em;p=op;
    }*source=p;return 1;
}
static int dimension(const char **source,int32_t *value,uint32_t width,uint32_t height) {
    const char *p=*source;while(space(*p))p++;expression v;
    if(!atom(&p,&v,width,height,0)||!expression_value(v,value))return 0;*source=p;return 1;
}
int32_t css_resolve(int32_t value,int32_t basis) {
    if(value>=EXPRESSION_BASE&&value<EXPRESSION_BASE+(int32_t)expression_count){expression v=expressions[value-EXPRESSION_BASE];return (int32_t)(((int64_t)v.px+(int64_t)v.em*font_basis+(int64_t)v.percent*basis/100)/1000);}
    if(value>CSS_PERCENT-10000&&value<CSS_PERCENT+10000)return (int32_t)((int64_t)(value-CSS_PERCENT)*basis/100);
    return value;
}
static int hex(char c) {c=lower(c);return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;}
static int32_t parsed_alpha;
static int color(const char *p,int32_t *result) {
    parsed_alpha=255;
    while(space(*p))p++;
    const char *names[]={"black","white","red","green","blue","gray","grey","yellow","magenta","cyan","transparent"};
    const int32_t colors[]={0,0xffffff,0xff0000,0x008000,0x0000ff,0x808080,0x808080,0xffff00,0xff00ff,0xffff,-1};
    uint32_t n=0;while(name_char(p[n]))n++;
    for(uint32_t i=0;i<11;i++)if(equal(p,n,names[i],1)){*result=colors[i];if(i==10)parsed_alpha=0;return 1;}
    if(*p=='#') {
        p++;uint32_t used=0,value=0;while(hex(p[used])>=0&&used<8){value=value*16+(uint32_t)hex(p[used]);used++;}
        if(used==3||used==4){if(used==4){parsed_alpha=(value&15)*17;value>>=4;}*result=(int32_t)(((value>>8)&15)*0x110000+((value>>4)&15)*0x1100+(value&15)*0x11);return 1;}
        if(used==6||used==8){*result=(int32_t)(used==8?value>>8:value);if(used==8){parsed_alpha=value&255;if(!parsed_alpha)*result=-1;}return 1;}
    }
    if(p[0]=='r'&&p[1]=='g'&&p[2]=='b'&&(p[3]=='('||p[3]=='a')) {
        int alpha=p[3]=='a';p+=alpha?5:4;int32_t v[3];
        for(uint32_t i=0;i<3;i++){if(!number(&p,&v[i])||v[i]<0||v[i]>255)return 0;while(space(*p))p++;if(i<2){if(*p!=',')return 0;p++;}}
        while(space(*p))p++;
        if(alpha){if(*p++!=',')return 0;int32_t a;if(!decimal(&p,&a))return 0;if(*p=='%'){a/=100;p++;}if(a<0)a=0;if(a>1000)a=1000;parsed_alpha=(a*255+500)/1000;while(space(*p))p++;}
        if(*p!=')')return 0;*result=parsed_alpha?v[0]*65536+v[1]*256+v[2]:-1;return 1;
    }
    return 0;
}
static void set(css_style *s,uint32_t *ranks,int property,int32_t value,uint32_t rank) {
    if(rank>=ranks[property]){s->value[property]=value;ranks[property]=rank;}
}
static void set_color(css_style *s,uint32_t *ranks,int property,int32_t rgb,int32_t alpha,uint32_t rank){
    if(rank<ranks[property])return;set(s,ranks,property,rgb,rank);
    int opacity=property==CSS_COLOR?CSS_COLOR_ALPHA:property==CSS_BACKGROUND?CSS_BACKGROUND_ALPHA:CSS_BORDER_ALPHA;
    set(s,ranks,opacity,alpha,rank);
}
static void copy_text(char *out,const char *text){uint32_t n=0;do{out[n]=text[n];}while(text[n++]);}
static int background_url(const char *text,const char *path,char output[256]){
    if(equal(text,length(text),"none",1)){output[0]=0;return 1;}
    if(length(text)<5||!equal(text,3,"url",1))return 0;
    const char *p=text+3;while(space(*p))p++;if(*p!='(')return 0;p++;while(space(*p))p++;
    char quote=(*p=='\''||*p=='"')?*p++:0;char url[256];uint32_t n=0;
    while(*p&&(quote?*p!=quote:*p!=')')){if(n+1>=sizeof(url)||*p=='\\')return 0;url[n++]=*p++;}
    if(quote){if(*p!=quote)return 0;p++;while(space(*p))p++;}else while(n&&space(url[n-1]))n--;
    if(*p++!=')')return 0;while(space(*p))p++;if(*p)return 0;url[n]=0;
    if(!images_resolve(url,path,output))copy_text(output,url);return 1;
}
static int background_repeat(const char *text,int32_t *repeat){
    const char *names[]={"no-repeat","repeat-x","repeat-y","repeat"};
    for(uint32_t i=0;i<4;i++)if(equal(text,length(text),names[i],1)){*repeat=(int32_t)i;return 1;}
    return 0;
}
static int background_size(const char *text,uint32_t width,uint32_t height,int32_t *mode,int32_t *x,int32_t *y){
    *x=*y=CSS_AUTO;
    if(equal(text,length(text),"cover",1)){*mode=CSS_BG_COVER;return 1;}
    if(equal(text,length(text),"contain",1)){*mode=CSS_BG_CONTAIN;return 1;}
    const char *p=text;
    for(uint32_t i=0;i<2;i++){while(space(*p))p++;int32_t value;
        if(p[0]=='a'&&p[1]=='u'&&p[2]=='t'&&p[3]=='o'&&(!p[4]||space(p[4]))){value=CSS_AUTO;p+=4;}
        else if(!dimension(&p,&value,width,height)||value<0)return 0;
        if(i)*y=value;else *x=value;while(space(*p))p++;if(!*p)break;}
    if(*p)return 0;*mode=*x==CSS_AUTO&&*y==CSS_AUTO?CSS_BG_AUTO:CSS_BG_EXPLICIT;return 1;
}
static int position_token(const char *text,int32_t *value,int *axis,uint32_t width,uint32_t height){
    *axis=0;
    if(equal(text,length(text),"left",1)){*value=CSS_PERCENT;*axis=1;return 1;}
    if(equal(text,length(text),"right",1)){*value=CSS_PERCENT+100;*axis=1;return 1;}
    if(equal(text,length(text),"top",1)){*value=CSS_PERCENT;*axis=2;return 1;}
    if(equal(text,length(text),"bottom",1)){*value=CSS_PERCENT+100;*axis=2;return 1;}
    if(equal(text,length(text),"center",1)){*value=CSS_PERCENT+50;return 1;}
    const char *p=text;if(!dimension(&p,value,width,height))return 0;return !*p;
}
static int background_position(const char *text,uint32_t width,uint32_t height,int32_t *x,int32_t *y){
    char tokens[2][64];uint32_t count=0;const char *p=text;
    while(*p){while(space(*p))p++;if(!*p)break;if(count==2)return 0;uint32_t n=0;
        while(*p&&!space(*p)){if(n+1>=sizeof(tokens[0]))return 0;tokens[count][n++]=*p++;}tokens[count++][n]=0;}
    if(!count)return 0;int axes[2];int32_t values[2];
    for(uint32_t i=0;i<count;i++)if(!position_token(tokens[i],&values[i],&axes[i],width,height))return 0;
    *x=*y=CSS_PERCENT+50;
    if(count==1){if(axes[0]==2)*y=values[0];else *x=values[0];return 1;}
    if(axes[0]&&axes[0]==axes[1])return 0;
    if(axes[0]==2||axes[1]==1){*x=values[1];*y=values[0];}else{*x=values[0];*y=values[1];}return 1;
}
static void set_background_image(css_style *s,uint32_t *ranks,const char *image,uint32_t rank){
    if(rank>=ranks[CSS_BACKGROUND_IMAGE]){copy_text(s->background_image,image);ranks[CSS_BACKGROUND_IMAGE]=rank;}
}
static void set_background_size(css_style *s,uint32_t *ranks,int32_t mode,int32_t x,int32_t y,uint32_t rank){
    if(rank>=ranks[CSS_BACKGROUND_SIZE]){set(s,ranks,CSS_BACKGROUND_SIZE,mode,rank);s->background_width=x;s->background_height=y;}
}
static int gradient(const char *text,css_style *s,uint32_t width,uint32_t height){
    if(length(text)<17||!equal(text,16,"linear-gradient(",1))return 0;
    const char *p=text+16;s->gradient_count=0;s->gradient_angle=180;
    while(*p){char token[256];uint32_t n=0;int nesting=0;
        while(*p&&(nesting||(*p!=','&&*p!=')'))){if(n+1==sizeof(token))return 0;char c=*p++;token[n++]=c;if(c=='(')nesting++;else if(c==')')nesting--;}
        while(n&&space(token[n-1]))n--;token[n]=0;const char *t=token;while(space(*t))t++;
        if(!s->gradient_count&&equal(t,length(t),"to right",1))s->gradient_angle=90;
        else if(!s->gradient_count&&equal(t,length(t),"to left",1))s->gradient_angle=270;
        else if(!s->gradient_count&&equal(t,length(t),"to top",1))s->gradient_angle=0;
        else if(!s->gradient_count&&equal(t,length(t),"to bottom",1))s->gradient_angle=180;
        else{int32_t rgb;if(!color(t,&rgb)||s->gradient_count==8)return 0;int i=s->gradient_count++;
            s->gradient_color[i]=rgb;s->gradient_alpha[i]=parsed_alpha;s->gradient_position[i]=CSS_AUTO;
            if(*t=='#'){t++;while(hex(*t)>=0)t++;}else if(*t=='r'&&t[1]=='g'){while(*t&&*t!=')')t++;if(*t)t++;}else while(name_char(*t))t++;
            while(space(*t))t++;if(*t){if(!dimension(&t,&s->gradient_position[i],width,height))return 0;while(space(*t))t++;if(*t)return 0;}}
        if(*p==','){p++;continue;}if(*p++!=')')return 0;while(space(*p))p++;if(*p)return 0;break;
    }
    if(s->gradient_count<2)return 0;
    if(s->gradient_position[0]==CSS_AUTO)s->gradient_position[0]=CSS_PERCENT;
    if(s->gradient_position[s->gradient_count-1]==CSS_AUTO)s->gradient_position[s->gradient_count-1]=CSS_PERCENT+100;
    for(int i=0;i<s->gradient_count;i++)if(s->gradient_position[i]==CSS_AUTO){int start=i-1,end=i;while(s->gradient_position[end]==CSS_AUTO)end++;
        int32_t a=css_resolve(s->gradient_position[start],1000),b=css_resolve(s->gradient_position[end],1000);
        for(;i<end;i++)s->gradient_position[i]=CSS_PERCENT+(a+(b-a)*(i-start)/(end-start))/10;i--;}
    return 1;
}
static void set_gradient(css_style *s,uint32_t *ranks,const css_style *g,uint32_t rank){
    if(rank<ranks[CSS_GRADIENT])return;set(s,ranks,CSS_GRADIENT,g->gradient_count!=0,rank);
    s->gradient_count=g->gradient_count;s->gradient_angle=g->gradient_angle;
    for(int i=0;i<g->gradient_count;i++){s->gradient_color[i]=g->gradient_color[i];s->gradient_alpha[i]=g->gradient_alpha[i];s->gradient_position[i]=g->gradient_position[i];}
}
static int background_declaration(css_style *s,uint32_t *ranks,const char *key,uint32_t key_length,const char *text,
                                  uint32_t rank,uint32_t width,uint32_t height,const char *path){
    int32_t mode,x,y,repeat;char image[256];
    if(equal(key,key_length,"background-image",1)){css_style g={0};if(gradient(text,&g,width,height)){set_gradient(s,ranks,&g,rank);set_background_image(s,ranks,"",rank);}
        else if(background_url(text,path,image)){set_background_image(s,ranks,image,rank);set_gradient(s,ranks,&g,rank);}return 1;}
    if(equal(key,key_length,"background-size",1)){if(background_size(text,width,height,&mode,&x,&y))set_background_size(s,ranks,mode,x,y,rank);return 1;}
    if(equal(key,key_length,"background-repeat",1)){if(background_repeat(text,&repeat))set(s,ranks,CSS_BACKGROUND_REPEAT,repeat,rank);return 1;}
    if(equal(key,key_length,"background-position",1)){if(background_position(text,width,height,&x,&y)){set(s,ranks,CSS_BACKGROUND_X,x,rank);set(s,ranks,CSS_BACKGROUND_Y,y,rank);}return 1;}
    if(!equal(key,key_length,"background",1))return 0;
    css_style g={0};if(gradient(text,&g,width,height)){set_gradient(s,ranks,&g,rank);set_background_image(s,ranks,"",rank);set_color(s,ranks,CSS_BACKGROUND,-1,0,rank);return 1;}
    /* One image layer. Parse first, so unsupported/malformed shorthand is atomic. */
    css_style parsed={0};parsed.value[CSS_BACKGROUND]=-1;parsed.value[CSS_BACKGROUND_ALPHA]=0;parsed.value[CSS_BACKGROUND_REPEAT]=3;
    parsed.value[CSS_BACKGROUND_X]=parsed.value[CSS_BACKGROUND_Y]=CSS_PERCENT;parsed.background_width=parsed.background_height=CSS_AUTO;
    char position[128]={0},size[128]={0};uint32_t position_used=0,size_used=0;int after_slash=0,has_image=0,has_color=0,has_repeat=0;
    const char *p=text;
    while(*p){while(space(*p))p++;if(!*p)break;if(*p=='/'){if(after_slash||!position_used)return 1;after_slash=1;p++;continue;}
        char token[256];uint32_t n=0;int nesting=0;char quote=0;
        while(*p&&(quote||nesting||(!space(*p)&&*p!='/'))){char c=*p++;
            if(n+1>=sizeof(token))return 1;token[n++]=c;
            if(quote){if(c==quote)quote=0;}else if(c=='\''||c=='"')quote=c;else if(c=='(')nesting++;else if(c==')')nesting--;}
        token[n]=0;if(!n||quote||nesting)return 1;
        if(background_url(token,path,image)){if(has_image)return 1;has_image=1;copy_text(parsed.background_image,image);}
        else if(background_repeat(token,&repeat)){if(has_repeat)return 1;has_repeat=1;parsed.value[CSS_BACKGROUND_REPEAT]=repeat;}
        else if(color(token,&x)){if(has_color)return 1;has_color=1;parsed.value[CSS_BACKGROUND]=x;parsed.value[CSS_BACKGROUND_ALPHA]=parsed_alpha;}
        else{char *target=after_slash?size:position;uint32_t *used=after_slash?&size_used:&position_used;
            if(*used+n+2>128)return 1;if(*used)target[(*used)++]=' ';for(uint32_t i=0;i<n;i++)target[(*used)++]=token[i];target[*used]=0;}
    }
    if(position_used&&!background_position(position,width,height,&parsed.value[CSS_BACKGROUND_X],&parsed.value[CSS_BACKGROUND_Y]))return 1;
    if(after_slash&&(!size_used||!background_size(size,width,height,&parsed.value[CSS_BACKGROUND_SIZE],&parsed.background_width,&parsed.background_height)))return 1;
    if(!has_image&&!has_color&&!has_repeat&&!position_used)return 1;
    set_color(s,ranks,CSS_BACKGROUND,parsed.value[CSS_BACKGROUND],parsed.value[CSS_BACKGROUND_ALPHA],rank);set_background_image(s,ranks,parsed.background_image,rank);set_gradient(s,ranks,&parsed,rank);
    set_background_size(s,ranks,parsed.value[CSS_BACKGROUND_SIZE],parsed.background_width,parsed.background_height,rank);
    set(s,ranks,CSS_BACKGROUND_X,parsed.value[CSS_BACKGROUND_X],rank);set(s,ranks,CSS_BACKGROUND_Y,parsed.value[CSS_BACKGROUND_Y],rank);
    set(s,ranks,CSS_BACKGROUND_REPEAT,parsed.value[CSS_BACKGROUND_REPEAT],rank);return 1;
}
static int expand_variables(char text[256],uint32_t depth){
    if(depth>8)return 0;char out[256];uint32_t used=0;const char *p=text;
    while(*p){if(p[0]=='v'&&p[1]=='a'&&p[2]=='r'&&p[3]=='('){p+=4;while(space(*p))p++;const char *name=p;while(name_char(*p))p++;
            uint32_t n=(uint32_t)(p-name);while(space(*p))p++;const char *fallback=0,*end=p;int nesting=0;
            if(*p==','){fallback=++p;end=p;while(*end&&(nesting||*end!=')')){if(*end=='(')nesting++;else if(*end==')')nesting--;end++;}}
            if(*end!=')')return 0;char replacement[256];int found=0;
            for(uint32_t i=0;i<variable_count;i++)if(equal(name,n,variables[i].name,0)){copy_text(replacement,variables[i].value);found=1;break;}
            if(!found){if(!fallback)return 0;uint32_t bytes=(uint32_t)(end-fallback);if(bytes>=256)return 0;for(uint32_t i=0;i<bytes;i++)replacement[i]=fallback[i];replacement[bytes]=0;}
            if(!expand_variables(replacement,depth+1))return 0;for(uint32_t i=0;replacement[i];i++){if(used==255)return 0;out[used++]=replacement[i];}p=end+1;
        }else{if(used==255)return 0;out[used++]=*p++;}}
    out[used]=0;copy_text(text,out);return 1;
}
static int keyword(const char *text,const char *const *options,uint32_t count,int32_t *value){
    for(uint32_t i=0;i<count;i++)if(equal(text,length(text),options[i],1)){*value=(int32_t)i;return 1;}return 0;
}
static int single_dimension(const char *text,int32_t *value,uint32_t width,uint32_t height){
    const char *p=text;if(equal(text,length(text),"auto",1)||equal(text,length(text),"none",1)){*value=CSS_AUTO;return 1;}
    if(!dimension(&p,value,width,height))return 0;while(space(*p))p++;return !*p;
}
static void declarations(css_style *s,uint32_t *ranks,const char *p,uint32_t size,uint32_t rank,uint32_t width,uint32_t height,int collect,const char *path) {
    const char *end=p+size;
    while((p=skip(p,end))<end) {
        if(*p==';'){p++;continue;}
        const char *key=p;while(p<end&&*p!=':'&&*p!=';')p++;
        if(p==end)break;if(*p!=':'){p++;continue;}
        const char *key_end=p++;while(key_end>key&&space(key_end[-1]))key_end--;
        const char *value=skip(p,end);p=value;int nesting=0;char quote=0;
        while(p<end) {
            if(quote){if(*p==quote)quote=0;else if(*p=='\\'&&p+1<end)p++;}
            else if(*p=='\''||*p=='"')quote=*p;
            else if(*p=='(')nesting++;else if(*p==')'&&nesting)nesting--;
            else if(*p==';'&&!nesting)break;p++;
        }
        char text[256];uint32_t used=0;const char *v=value;
        while(v<p&&used+1<sizeof(text)) {
            if(p-v>=2&&v[0]=='/'&&v[1]=='*'){v=skip(v,p);continue;}
            text[used++]=*v++;
        }
        if(v<p){if(p<end)p++;continue;}
        while(used&&space(text[used-1]))used--;text[used]=0;uint32_t priority=rank;
        if(used>=10&&equal(text+used-10,10,"!important",1)){priority|=0x80000000u;used-=10;while(used&&space(text[used-1]))used--;text[used]=0;}
        uint32_t key_length=(uint32_t)(key_end-key);
        if(collect&&key_length>2&&key[0]=='-'&&key[1]=='-'&&key_length<64) {
            uint32_t index=0;for(;index<variable_count;index++)if(equal(key,key_length,variables[index].name,0))break;
            if(index<128){if(index==variable_count){variables[index].priority=0;variable_count++;}
                if(priority>=variables[index].priority){for(uint32_t i=0;i<key_length;i++)variables[index].name[i]=key[i];variables[index].name[key_length]=0;
                    for(uint32_t i=0;i<=used;i++)variables[index].value[i]=text[i];variables[index].priority=priority;}}
        }
        if(!s){if(p<end)p++;continue;}
        if(!expand_variables(text,0)){if(p<end)p++;continue;}used=length(text);
        if(equal(key,key_length,"backdrop-filter",1)||equal(key,key_length,"-webkit-backdrop-filter",1)){
            const char *filter=text;int32_t blur=0,brightness=1000;int valid=1;
            if(!equal(text,used,"none",1))while(*filter){while(space(*filter))filter++;if(!*filter)break;
                if(length(filter)>=5&&equal(filter,5,"blur(",1)){filter+=5;while(space(*filter))filter++;
                    if(*filter!=')'&&(!dimension(&filter,&blur,width,height)||blur<0||blur>=CSS_PERCENT)){valid=0;break;}
                    if(blur>32)blur=32;
                }else if(length(filter)>=11&&equal(filter,11,"brightness(",1)){filter+=11;while(space(*filter))filter++;
                    if(*filter!=')'&&!decimal(&filter,&brightness)){valid=0;break;}if(*filter=='%'){brightness/=100;filter++;}
                    if(brightness<0){valid=0;break;}if(brightness>8000)brightness=8000;
                }else{valid=0;break;}
                while(space(*filter))filter++;if(*filter++!=')'){valid=0;break;}
            }
            if(valid){set(s,ranks,CSS_BACKDROP_BLUR,blur,priority);set(s,ranks,CSS_BACKDROP_BRIGHTNESS,brightness,priority);}
            if(p<end)p++;continue;
        }
        if(background_declaration(s,ranks,key,key_length,text,priority,width,height,path)){if(p<end)p++;continue;}
        int property=-1;int32_t parsed=0;const char *cursor=text;
        const char *names[]={"color","background-color","font-size","line-height","text-align","display","position","left","top","right","bottom","width","height","margin-top","margin-right","margin-bottom","margin-left","padding-top","padding-right","padding-bottom","padding-left","border-width","border-color","translate-x","translate-y","font-family"};
        for(uint32_t i=0;i<sizeof(names)/sizeof(names[0]);i++)if(equal(key,key_length,names[i],1)){property=(int)i;break;}
        if(equal(key,key_length,"z-index",1))property=CSS_Z_INDEX;
        const char *more[]={"min-width","max-width","min-height","max-height","box-sizing","flex-direction","flex-wrap","justify-content","align-items","align-self","flex-grow","flex-shrink","flex-basis","row-gap","column-gap","border-radius","opacity","visibility","overflow-x","overflow-y","white-space","font-weight"};
        for(uint32_t i=0;i<sizeof(more)/sizeof(more[0]);i++)if(equal(key,key_length,more[i],1))property=CSS_MIN_WIDTH+(int)i;
        if(equal(key,key_length,"background",1))property=CSS_BACKGROUND;
        if(property==CSS_FONT_FAMILY) {
            if(priority>=ranks[property]&&text[0]){uint32_t i=0;do{s->font_family[i]=text[i];}while(text[i++]);ranks[property]=priority;}
        } else if(property==CSS_COLOR||property==CSS_BACKGROUND||property==CSS_BORDER_COLOR) {
            if(equal(text,length(text),"currentColor",1)){parsed=s->value[CSS_COLOR];parsed_alpha=s->value[CSS_COLOR_ALPHA];set_color(s,ranks,property,parsed,parsed_alpha,priority);}
            else if(color(text,&parsed))set_color(s,ranks,property,parsed,parsed_alpha,priority);
        } else if(property==CSS_DISPLAY||property==CSS_POSITION||property==CSS_ALIGN) {
            const char *const displays[]={"none","block","flex","inline","inline-block","grid"};
            const char *const positions[]={"static","absolute","fixed","relative"};const char *const aligns[]={"left","center","right"};
            if(equal(text,length(text),"initial",1))set(s,ranks,property,property==CSS_DISPLAY?CSS_INLINE:0,priority);
            else if(property==CSS_DISPLAY&&equal(text,length(text),"inline-flex",1))set(s,ranks,property,CSS_FLEX,priority);
            else if(keyword(text,property==CSS_DISPLAY?displays:property==CSS_POSITION?positions:aligns,property==CSS_DISPLAY?6:property==CSS_POSITION?4:3,&parsed))set(s,ranks,property,parsed,priority);
        } else if(property==CSS_BOX_SIZING||property==CSS_FLEX_DIRECTION||property==CSS_FLEX_WRAP||property==CSS_VISIBILITY||property==CSS_WHITE_SPACE||property==CSS_OVERFLOW_X||property==CSS_OVERFLOW_Y) {
            const char *const sizing[]={"content-box","border-box"},*const direction[]={"row","column","row-reverse","column-reverse"},*const wrap[]={"nowrap","wrap"},*const visibility[]={"visible","hidden","collapse"},*const whitespace[]={"normal","nowrap","pre","pre-wrap"},*const overflow[]={"visible","hidden","auto","scroll"};
            const char *const *choices=property==CSS_BOX_SIZING?sizing:property==CSS_FLEX_DIRECTION?direction:property==CSS_FLEX_WRAP?wrap:property==CSS_VISIBILITY?visibility:property==CSS_WHITE_SPACE?whitespace:overflow;
            uint32_t count=property==CSS_BOX_SIZING||property==CSS_FLEX_WRAP?2:property==CSS_VISIBILITY?3:4;
            if(keyword(text,choices,count,&parsed))set(s,ranks,property,parsed,priority);
        } else if(property==CSS_JUSTIFY||property==CSS_ITEMS||property==CSS_SELF){
            const char *const alignment[]={"flex-start","center","flex-end","space-between","space-around","space-evenly","stretch"};
            if(equal(text,length(text),"auto",1)&&property==CSS_SELF)set(s,ranks,property,CSS_AUTO,priority);
            else if(equal(text,length(text),"start",1))set(s,ranks,property,CSS_START,priority);
            else if(equal(text,length(text),"end",1))set(s,ranks,property,CSS_END,priority);
            else if(keyword(text,alignment,7,&parsed))set(s,ranks,property,parsed,priority);
        } else if(property==CSS_OPACITY||property==CSS_GROW||property==CSS_SHRINK){
            if(decimal(&cursor,&parsed)&&!*cursor){if(parsed<0)parsed=0;if(property==CSS_OPACITY){if(parsed>1000)parsed=1000;parsed=(parsed*255+500)/1000;}set(s,ranks,property,parsed,priority);}
        } else if(property==CSS_FONT_WEIGHT){
            if(equal(text,length(text),"normal",1)||equal(text,length(text),"initial",1))set(s,ranks,property,400,priority);
            else if(equal(text,length(text),"bold",1))set(s,ranks,property,700,priority);
            else if(number(&cursor,&parsed)&&!*cursor&&parsed>=1&&parsed<=1000)set(s,ranks,property,parsed,priority);
        } else if(property==CSS_RADIUS){
            /* Uniform radius; use the first corner for multi-corner declarations. */
            if(dimension(&cursor,&parsed,width,height)&&parsed>=0)set(s,ranks,property,parsed,priority);
        } else if(property==CSS_FONT_SIZE){
            const char *const sizes[]={"xx-small","x-small","small","medium","large","x-large","xx-large","xxx-large"};const int32_t pixels[]={9,10,13,16,18,24,32,48};
            if(keyword(text,sizes,8,&parsed))set(s,ranks,property,pixels[parsed],priority);
            else if(equal(text,length(text),"inherit",1)||equal(text,length(text),"unset",1))set(s,ranks,property,CSS_PERCENT+100,priority);
            else if(equal(text,length(text),"initial",1))set(s,ranks,property,16,priority);
            else if(equal(text,length(text),"larger",1))set(s,ranks,property,CSS_PERCENT+120,priority);
            else if(equal(text,length(text),"smaller",1))set(s,ranks,property,CSS_PERCENT+83,priority);
            else if(single_dimension(text,&parsed,width,height)&&parsed>=0)set(s,ranks,property,parsed,priority);
        } else if(property==CSS_LINE_HEIGHT){
            if(equal(text,length(text),"normal",1))set(s,ranks,property,CSS_AUTO,priority);
            else if(decimal(&cursor,&parsed)&&!*cursor&&parsed>=0)set(s,ranks,property,-100000-parsed,priority);
            else if(single_dimension(text,&parsed,width,height))set(s,ranks,property,parsed,priority);
        } else if((property==CSS_WIDTH||property==CSS_HEIGHT)&&equal(text,length(text),"auto",1)) {
            set(s,ranks,property,CSS_AUTO,priority);
        } else if(property>=0) {
            if(single_dimension(text,&parsed,width,height))set(s,ranks,property,parsed,priority);
        } else if(equal(key,key_length,"margin",1)||equal(key,key_length,"padding",1)) {
            int base=equal(key,key_length,"margin",1)?CSS_MARGIN_TOP:CSS_PADDING_TOP;
            int32_t values[4];uint32_t n=0;while(n<4){while(space(*cursor))cursor++;if(!*cursor)break;
                if(base==CSS_MARGIN_TOP&&cursor[0]=='a'&&cursor[1]=='u'&&cursor[2]=='t'&&cursor[3]=='o'){values[n]=CSS_AUTO;cursor+=4;}
                else if(!dimension(&cursor,&values[n],width,height))break;n++;}
            while(space(*cursor))cursor++;
            if(n&&!*cursor){set(s,ranks,base,values[0],priority);set(s,ranks,base+1,values[n>1?1:0],priority);set(s,ranks,base+2,values[n>2?2:0],priority);set(s,ranks,base+3,values[n>3?3:n>1?1:0],priority);}
        } else if(equal(key,key_length,"inset",1)){
            int32_t values[4];uint32_t n=0;while(n<4){while(space(*cursor))cursor++;if(!*cursor)break;if(!dimension(&cursor,&values[n],width,height))break;n++;}while(space(*cursor))cursor++;
            if(n&&!*cursor){set(s,ranks,CSS_TOP,values[0],priority);set(s,ranks,CSS_RIGHT,values[n>1?1:0],priority);set(s,ranks,CSS_BOTTOM,values[n>2?2:0],priority);set(s,ranks,CSS_LEFT,values[n>3?3:n>1?1:0],priority);}
        } else if(equal(key,key_length,"gap",1)){
            int32_t x,y;if(dimension(&cursor,&x,width,height)){while(space(*cursor))cursor++;y=x;if(*cursor&&!dimension(&cursor,&y,width,height))goto next_declaration;while(space(*cursor))cursor++;
                if(!*cursor&&x>=0&&y>=0){set(s,ranks,CSS_ROW_GAP,x,priority);set(s,ranks,CSS_COLUMN_GAP,y,priority);}}
        } else if(equal(key,key_length,"overflow",1)){
            const char *const choices[]={"visible","hidden","auto","scroll"};if(keyword(text,choices,4,&parsed)){set(s,ranks,CSS_OVERFLOW_X,parsed,priority);set(s,ranks,CSS_OVERFLOW_Y,parsed,priority);}
        } else if(equal(key,key_length,"flex",1)){
            if(equal(text,length(text),"none",1)){set(s,ranks,CSS_GROW,0,priority);set(s,ranks,CSS_SHRINK,0,priority);set(s,ranks,CSS_BASIS,CSS_AUTO,priority);}
            else if(decimal(&cursor,&parsed)&&!*cursor&&parsed>=0){set(s,ranks,CSS_GROW,parsed,priority);set(s,ranks,CSS_SHRINK,1000,priority);set(s,ranks,CSS_BASIS,0,priority);}
        } else if(equal(key,key_length,"grid-template-columns",1)){
            int32_t widths[32]={0},fractions[32]={0};uint32_t tracks=0;int valid=1;
            if(used>13&&equal(text,7,"repeat(",1)){cursor=text+7;if(number(&cursor,&parsed)&&parsed>0&&parsed<=32){while(space(*cursor))cursor++;if(*cursor++==','){while(space(*cursor))cursor++;if(equal(cursor,length(cursor),"1fr)",1)){tracks=(uint32_t)parsed;for(uint32_t i=0;i<tracks;i++)fractions[i]=1000;}}}}
            else while(*cursor&&tracks<32){while(space(*cursor))cursor++;if(!*cursor)break;int minmax=equal(cursor,7,"minmax(",1);if(minmax){cursor+=7;int32_t minimum;if(!dimension(&cursor,&minimum,width,height)){valid=0;break;}while(space(*cursor))cursor++;if(*cursor++!=','){valid=0;break;}while(space(*cursor))cursor++;}
                const char *start=cursor;int32_t fraction;if(decimal(&cursor,&fraction)&&cursor[0]=='f'&&cursor[1]=='r'){fractions[tracks]=fraction;cursor+=2;}else{cursor=start;if(!dimension(&cursor,&widths[tracks],width,height)){valid=0;break;}}
                if(minmax){while(space(*cursor))cursor++;if(*cursor++!=')'){valid=0;break;}}tracks++;while(space(*cursor))cursor++;}
            if(valid&&tracks&&priority>=ranks[CSS_GRID_TRACKS]){set(s,ranks,CSS_GRID_COLUMNS,(int32_t)tracks,priority);set(s,ranks,CSS_GRID_TRACKS,1,priority);for(uint32_t i=0;i<tracks;i++){s->grid_width[i]=widths[i];s->grid_fraction[i]=fractions[i];}}
        } else if(equal(key,key_length,"border-left",1)){
            if(equal(text,length(text),"none",1))set(s,ranks,CSS_BORDER_LEFT,0,priority);
            else if(dimension(&cursor,&parsed,width,height)){set(s,ranks,CSS_BORDER_LEFT,parsed,priority);while(space(*cursor))cursor++;if(equal(cursor,5,"solid",1)){cursor+=5;while(space(*cursor))cursor++;}if(color(cursor,&parsed))set_color(s,ranks,CSS_BORDER_COLOR,parsed,parsed_alpha,priority);}
        } else if((equal(key,key_length,"transform",1)||equal(key,key_length,"-webkit-transform",1))&&used>=10&&equal(text,10,"translate(",1)) {
            cursor=text+10;int32_t x,y;
            if(dimension(&cursor,&x,width,height)){while(space(*cursor))cursor++;if(*cursor==',')cursor++;
                if(dimension(&cursor,&y,width,height)){set(s,ranks,CSS_TRANSLATE_X,x,priority);set(s,ranks,CSS_TRANSLATE_Y,y,priority);}}
        } else if(equal(key,key_length,"transform",1)&&used>=11&&(equal(text,11,"translateX(",1)||equal(text,11,"translateY(",1))){
            cursor=text+11;if(dimension(&cursor,&parsed,width,height)&&*cursor==')')set(s,ranks,text[9]=='X'||text[9]=='x'?CSS_TRANSLATE_X:CSS_TRANSLATE_Y,parsed,priority);
        } else if(equal(key,key_length,"border",1)) {
            if(equal(text,length(text),"none",1))set(s,ranks,CSS_BORDER,0,priority);
            else if(dimension(&cursor,&parsed,width,height)) {
                set(s,ranks,CSS_BORDER,parsed,priority);
                while(space(*cursor))cursor++;if(equal(cursor,5,"solid",1)){cursor+=5;while(space(*cursor))cursor++;}
                if(color(cursor,&parsed))set_color(s,ranks,CSS_BORDER_COLOR,parsed,parsed_alpha,priority);
            }
        }
next_declaration:
        if(p<end)p++;
    }
}
void css_init(void){
    static const char controls[]=
        "button { display:inline-block; padding:6px 12px; border:1px solid #667085; "
        "border-radius:4px; background-color:#343b46; color:#f2f4f8; text-align:center; }"
        "button:hover { background-color:#465365; border-color:#9aa9bc; }"
        "button:active { background-color:#242a33; border-color:#667085; }";
    const int32_t bounds[]={0,2147483647,0,2147483647};
    rule_count=variable_count=expression_count=0;font_basis=root_font_basis=16;
    (void)add_rules(controls,sizeof(controls)-1,"(browser defaults)",bounds,0);
}
static void face_rule(const char *p,uint32_t size,const char *path) {
    const char *end=p+size;char family[64]={0},sources[256]={0};
    while((p=skip(p,end))<end){const char *key=p;while(p<end&&*p!=':'&&*p!=';')p++;if(p==end)break;if(*p==';'){p++;continue;}
        const char *last=p++;while(last>key&&space(last[-1]))last--;const char *begin=skip(p,end);p=begin;char quote=0;int nesting=0;
        while(p<end){if(quote){if(*p==quote)quote=0;else if(*p=='\\'&&p+1<end)p++;}
            else if(*p=='\''||*p=='"')quote=*p;else if(*p=='(')nesting++;else if(*p==')'&&nesting)nesting--;else if(*p==';'&&!nesting)break;p++;}
        char *target=0;uint32_t capacity=0;if(equal(key,(uint32_t)(last-key),"font-family",1)){target=family;capacity=sizeof(family);}
        if(equal(key,(uint32_t)(last-key),"src",1)){target=sources;capacity=sizeof(sources);}
        if(target){uint32_t n=0;while(begin<p&&n+1<capacity){if(p-begin>=2&&begin[0]=='/'&&begin[1]=='*'){begin=skip(begin,p);continue;}target[n++]=*begin++;}target[n]=0;}
        if(p<end)p++;
    }
    if(family[0]&&sources[0])font_face(family,sources,path);
}
int css_add(const char *source,uint32_t size){return css_add_file(source,size,"/Envy/index.html");}
static int media_bounds(const char *p,const char *end,int32_t bounds[4]){
    while(p<end){while(p<end&&space(*p))p++;if(p==end)break;
        if(*p!='('){const char *word=p;while(p<end&&name_char(*p))p++;uint32_t n=(uint32_t)(p-word);
            if(!n||(!equal(word,n,"screen",1)&&!equal(word,n,"all",1)&&!equal(word,n,"and",1)))return 0;continue;}
        const char *key=++p;while(p<end&&*p!=':')p++;if(p==end)return 0;const char *last=p++;while(last>key&&space(last[-1]))last--;
        const char *names[]={"min-width","max-width","min-height","max-height"};int index=-1;for(int i=0;i<4;i++)if(equal(key,(uint32_t)(last-key),names[i],1))index=i;
        int32_t value;if(index<0||!dimension(&p,&value,0,0)||value<0||value>=CSS_PERCENT-10000)return 0;
        while(p<end&&space(*p))p++;if(p==end||*p++!=')')return 0;
        if(index%2){if(value<bounds[index])bounds[index]=value;}else if(value>bounds[index])bounds[index]=value;
    }return 1;
}
static int add_rules(const char *source,uint32_t size,const char *path,const int32_t bounds[4],uint32_t depth) {
    if(depth>8)return 0;
    if(!path||length(path)>=256)return 0;
    const char *p=source,*end=source+size;
    while((p=skip(p,end))<end) {
        const char *selector=p;while(p<end&&*p!='{'&&*p!=';')p++;
        if(p==end)break;if(*p==';'){p++;continue;}
        const char *selector_end=p++;while(selector_end>selector&&space(selector_end[-1]))selector_end--;
        const char *body=p;uint32_t nesting=1;char quote=0;
        while(p<end&&nesting) {
            if(quote){if(*p==quote)quote=0;else if(*p=='\\'&&p+1<end)p++;}
            else if(end-p>=2&&p[0]=='/'&&p[1]=='*'){p=skip(p,end);continue;}
            else if(*p=='\''||*p=='"')quote=*p;else if(*p=='{')nesting++;else if(*p=='}')nesting--;
            if(nesting)p++;
        }
        if(nesting)return 0;uint32_t body_length=(uint32_t)(p-body);p++;
        if(*selector=='@'){
            if(equal(selector,(uint32_t)(selector_end-selector),"@font-face",1))face_rule(body,body_length,path);
            else if(selector_end-selector>=6&&equal(selector,6,"@media",1)){int32_t nested[4];for(int i=0;i<4;i++)nested[i]=bounds[i];
                if(media_bounds(selector+6,selector_end,nested)&&!add_rules(body,body_length,path,nested,depth+1))return 0;}
            continue; /* Other at-rules never leak into ordinary rules. */
        }
        if(equal(selector,(uint32_t)(selector_end-selector),":root",1))declarations(0,0,body,body_length,rule_count+1,0,0,1,path);
        const char *part=selector;
        for(const char *q=selector;q<=selector_end;q++)if(q==selector_end||*q==',') {
            const char *last=q;while(part<last&&space(*part))part++;while(last>part&&space(last[-1]))last--;
            if(part<last){if(rule_count==1024)return 0;rule *r=&rules[rule_count++];
                *r=(rule){.selector=part,.body=body,.selector_length=(uint32_t)(last-part),.body_length=body_length,
                    .min_width=bounds[0],.max_width=bounds[1],.min_height=bounds[2],.max_height=bounds[3]};
                copy_text(r->path,path);}
            part=q+1;
        }
    }
    return 1;
}
int css_add_file(const char *source,uint32_t size,const char *path){const int32_t bounds[]={0,2147483647,0,2147483647};return add_rules(source,size,path,bounds,0);}
void css_compute(const css_element *e,const css_style *parent,const char *inline_style,uint32_t width,uint32_t height,css_style *s) {
    if(e->tag&&equal(e->tag,length(e->tag),"html",1))root_font_basis=16;
    uint32_t ranks[CSS_PROPERTIES]={0};for(uint32_t i=0;i<CSS_PROPERTIES;i++)s->value[i]=0;
    const char *family=parent?parent->font_family:"sans-serif";uint32_t family_index=0;
    do{s->font_family[family_index]=family[family_index];}while(family[family_index++]);
    s->value[CSS_COLOR]=parent?parent->value[CSS_COLOR]:0x202020;s->value[CSS_BACKGROUND]=-1;
    s->value[CSS_COLOR_ALPHA]=parent?parent->value[CSS_COLOR_ALPHA]:255;s->value[CSS_BACKGROUND_ALPHA]=s->value[CSS_BORDER_ALPHA]=255;
    s->value[CSS_OPACITY]=255;s->value[CSS_VISIBILITY]=parent?parent->value[CSS_VISIBILITY]:0;
    s->value[CSS_BACKDROP_BRIGHTNESS]=1000;
    s->value[CSS_WHITE_SPACE]=parent?parent->value[CSS_WHITE_SPACE]:0;s->value[CSS_FONT_WEIGHT]=parent?parent->value[CSS_FONT_WEIGHT]:400;
    s->gradient_count=0;s->gradient_angle=180;
    s->background_image[0]=0;s->background_width=s->background_height=CSS_AUTO;
    s->value[CSS_BACKGROUND_X]=s->value[CSS_BACKGROUND_Y]=CSS_PERCENT;s->value[CSS_BACKGROUND_REPEAT]=3;
    s->value[CSS_Z_INDEX]=CSS_AUTO;
    s->value[CSS_FONT_SIZE]=parent?parent->value[CSS_FONT_SIZE]:16;s->value[CSS_LINE_HEIGHT]=parent?parent->value[CSS_LINE_HEIGHT]:18;
    s->value[CSS_ALIGN]=parent?parent->value[CSS_ALIGN]:0;s->value[CSS_DISPLAY]=1;
    for(uint32_t i=CSS_LEFT;i<=CSS_HEIGHT;i++)s->value[i]=CSS_AUTO;
    s->value[CSS_BORDER_COLOR]=s->value[CSS_COLOR];s->hidden=parent?parent->hidden:0;
    s->value[CSS_MAX_WIDTH]=s->value[CSS_MAX_HEIGHT]=s->value[CSS_BASIS]=s->value[CSS_SELF]=CSS_AUTO;
    s->value[CSS_ITEMS]=CSS_STRETCH;s->value[CSS_SHRINK]=1000;s->value[CSS_GRID_COLUMNS]=1;
    font_basis=parent?parent->value[CSS_FONT_SIZE]:16;
    if(e->tag&&e->tag[0]=='h'&&e->tag[1]>='1'&&e->tag[1]<='6'&&!e->tag[2]){
        uint32_t level=(uint32_t)(e->tag[1]-'1');const int32_t sizes[]={200,150,117,100,83,67};const char *const margins[]={".67em",".83em","1em","1.33em","1.67em","2.33em"};
        s->value[CSS_FONT_SIZE]=CSS_PERCENT+sizes[level];s->value[CSS_FONT_WEIGHT]=700;const char *margin=margins[level];int32_t value;if(dimension(&margin,&value,width,height))s->value[CSS_MARGIN_TOP]=s->value[CSS_MARGIN_BOTTOM]=value;
    }
    for(uint32_t i=0;i<rule_count;i++) {
        if((int32_t)width<rules[i].min_width||(int32_t)width>rules[i].max_width||(int32_t)height<rules[i].min_height||(int32_t)height>rules[i].max_height)continue;
        int specificity=matches(rules[i].selector,rules[i].selector_length,e,0);
        if(specificity>=0)declarations(s,ranks,rules[i].body,rules[i].body_length,((uint32_t)specificity<<12)+i+1,width,height,0,rules[i].path);
    }
    if(inline_style)declarations(s,ranks,inline_style,length(inline_style),0x40000000u,width,height,0,"/Envy/index.html");
    if(equal(s->font_family,length(s->font_family),"inherit",1)||equal(s->font_family,length(s->font_family),"unset",1)){
        family=parent?parent->font_family:"sans-serif";family_index=0;do{s->font_family[family_index]=family[family_index];}while(family[family_index++]);
    }else if(equal(s->font_family,length(s->font_family),"initial",1)){family="sans-serif";family_index=0;do{s->font_family[family_index]=family[family_index];}while(family[family_index++]);}
    s->hidden=s->hidden||!s->value[CSS_DISPLAY];
    s->value[CSS_FONT_SIZE]=css_resolve(s->value[CSS_FONT_SIZE],font_basis);
    if(s->value[CSS_FONT_SIZE]<1)s->value[CSS_FONT_SIZE]=1;if(s->value[CSS_FONT_SIZE]>256)s->value[CSS_FONT_SIZE]=256;
    if(e->tag&&equal(e->tag,length(e->tag),"html",1))root_font_basis=s->value[CSS_FONT_SIZE];
    font_basis=s->value[CSS_FONT_SIZE];
    for(uint32_t i=0;i<CSS_PROPERTIES;i++){int32_t v=s->value[i];if(v>=EXPRESSION_BASE&&v<EXPRESSION_BASE+(int32_t)expression_count&&expressions[v-EXPRESSION_BASE].em){expression resolved=expressions[v-EXPRESSION_BASE];resolved.px+=resolved.em*font_basis;resolved.em=0;expression_value(resolved,&s->value[i]);}}
    s->line_height_spec=ranks[CSS_LINE_HEIGHT]?s->value[CSS_LINE_HEIGHT]:parent?parent->line_height_spec:CSS_AUTO;
    s->value[CSS_LINE_HEIGHT]=s->line_height_spec;
    if(s->value[CSS_LINE_HEIGHT]==CSS_AUTO)s->value[CSS_LINE_HEIGHT]=s->value[CSS_FONT_SIZE]+2;
    else if(s->value[CSS_LINE_HEIGHT]<=-100000)s->value[CSS_LINE_HEIGHT]=(int32_t)((int64_t)(-s->value[CSS_LINE_HEIGHT]-100000)*s->value[CSS_FONT_SIZE]/1000);
    else s->value[CSS_LINE_HEIGHT]=css_resolve(s->value[CSS_LINE_HEIGHT],s->value[CSS_FONT_SIZE]);
}
