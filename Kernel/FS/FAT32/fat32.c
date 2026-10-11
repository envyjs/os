#include "fat32.h"
static uint16_t u16(const uint8_t *p){return p[0]|((uint16_t)p[1]<<8);}
static uint32_t u32(const uint8_t *p){return u16(p)|((uint32_t)u16(p+2)<<16);}
static void put32(uint8_t *p,uint32_t n){for(int i=0;i<4;i++)p[i]=(uint8_t)(n>>(i*8));}
static int valid_cluster(const fat32 *fs,uint32_t n){return n>=2&&n-2<fs->clusters;}
static int read_sector(fat32 *fs,uint32_t sector,uint8_t *p){if(sector>=fs->sectors)return FAT32_INVALID;return fs->read(fs->context,fs->start+sector,1,p)?FAT32_IO:0;}
static int write_sector(fat32 *fs,uint32_t sector,const uint8_t *p){if(!fs->write)return FAT32_READ_ONLY;if(sector>=fs->sectors)return FAT32_INVALID;return fs->write(fs->context,fs->start+sector,1,p)?FAT32_IO:0;}
static uint32_t cluster_sector(fat32 *fs,uint32_t cluster){return fs->data_start+(cluster-2)*fs->sectors_per_cluster;}
static int fat_value(fat32 *fs,uint32_t cluster,uint32_t *out){
    if(!valid_cluster(fs,cluster))return FAT32_INVALID;uint32_t sector=fs->fat_start+(uint32_t)fs->active_fat*fs->fat_sectors+cluster/128;
    if(fs->cached_fat!=sector){int r=read_sector(fs,sector,fs->fat_cache);if(r)return r;fs->cached_fat=sector;}*out=u32(fs->fat_cache+(cluster%128)*4)&0x0fffffffu;return 0;
}
static int next_cluster(fat32 *fs,uint32_t cluster,uint32_t *next){uint32_t n;int r=fat_value(fs,cluster,&n);if(r)return r;if(n>=0x0ffffff8u){*next=0;return 0;}if(!valid_cluster(fs,n)||n==cluster)return FAT32_INVALID;*next=n;return 0;}
static int set_fat(fat32 *fs,uint32_t cluster,uint32_t value){
    if(!valid_cluster(fs,cluster))return FAT32_INVALID;uint8_t sector[512];uint32_t first=fs->mirrored?0:fs->active_fat,last=fs->mirrored?fs->fats:first+1;
    for(uint32_t i=first;i<last;i++){uint32_t at=fs->fat_start+i*fs->fat_sectors+cluster/128;int r=read_sector(fs,at,sector);if(r)return r;uint8_t *entry=sector+(cluster%128)*4;put32(entry,(u32(entry)&0xf0000000u)|(value&0x0fffffffu));r=write_sector(fs,at,sector);fs->cached_fat=0xffffffffu;if(r)return r;}return 0;
}
int fat32_mount(fat32 *fs,fat32_read_sectors read,fat32_write_sectors write,fat32_flush_device flush,void *context,uint64_t sectors){
    if(!fs||!read||!sectors)return FAT32_INVALID;*fs=(fat32){.read=read,.write=write,.flush=flush,.context=context,.device_sectors=sectors,.cached_fat=0xffffffffu,.next_free=2};
    if(read(context,0,1,fs->scratch))return FAT32_IO;uint8_t *b=fs->scratch;if(u16(b+510)!=0xaa55)return FAT32_INVALID;uint32_t partition_size=0;
    if(u16(b+11)!=512||u16(b+17)||u16(b+22)){
        int found=0;for(int i=0;i<4;i++){uint8_t *p=b+446+i*16;if(p[4]!=0x0b&&p[4]!=0x0c&&p[4]!=0x1b&&p[4]!=0x1c)continue;uint32_t start=u32(p+8),size=u32(p+12);if(!start||!size||(uint64_t)start+size>sectors)return FAT32_INVALID;fs->start=start;partition_size=size;found=1;break;}if(!found)return FAT32_UNSUPPORTED;
        if(read(context,fs->start,1,b))return FAT32_IO;
    }
    uint32_t total=u16(b+19)?u16(b+19):u32(b+32),fat_size=u32(b+36),reserved=u16(b+14);uint8_t spc=b[13],fats=b[16];uint16_t flags=u16(b+40);
    if(u16(b+510)!=0xaa55||u16(b+11)!=512||u16(b+17)||u16(b+22)||u16(b+42)||!spc||(spc&(spc-1))||spc>128||!reserved||!fats||fats>2||!fat_size||!total||(uint64_t)reserved+(uint64_t)fats*fat_size>=total||fs->start+total>sectors||(partition_size&&total>partition_size))return FAT32_INVALID;
    uint32_t data=reserved+fats*fat_size,clusters=(total-data)/spc;if(clusters<65525||clusters>0x0fffffedu||(uint64_t)fat_size*128<clusters+2)return FAT32_UNSUPPORTED;
    fs->sectors=total;fs->fat_start=reserved;fs->fat_sectors=fat_size;fs->data_start=data;fs->clusters=clusters;fs->root=u32(b+44)&0x0fffffffu;fs->sectors_per_cluster=spc;fs->fats=fats;fs->mirrored=!(flags&128);fs->active_fat=fs->mirrored?0:(uint8_t)(flags&15);
    uint32_t info=u16(b+48),backup=u16(b+50);if(info&&info<reserved){fs->info_sector=info;if(backup&&backup+info<reserved)fs->backup_info=backup+info;}
    if(!valid_cluster(fs,fs->root)||fs->active_fat>=fats)return FAT32_INVALID;uint32_t root_value;int r=fat_value(fs,fs->root,&root_value);if(r)return r;return root_value>=0x0ffffff8u||valid_cluster(fs,root_value)?0:FAT32_INVALID;
}
static uint8_t checksum(const uint8_t *name){uint8_t n=0;for(int i=0;i<11;i++)n=(uint8_t)(((n&1)?128:0)+(n>>1)+name[i]);return n;}
static char lower(char c){return c>='A'&&c<='Z'?c+32:c;}
static int same(const char *a,const char *b){while(*a&&lower(*a)==lower(*b)){a++;b++;}return !*a&&!*b;}
static void short_name(const uint8_t *entry,char *name){uint32_t n=0;for(int i=0;i<8&&entry[i]!=' ';i++){uint8_t c=entry[i];if(!i&&c==5)c=0xe5;name[n++]=(entry[12]&8)?lower((char)c):(char)c;}if(entry[8]!=' '){name[n++]='.';for(int i=8;i<11&&entry[i]!=' ';i++)name[n++]=(entry[12]&16)?lower((char)entry[i]):(char)entry[i];}name[n]=0;}
static int utf8_name(const uint16_t *units,char *name){uint32_t out=0;for(uint32_t i=0;i<255;i++){uint32_t cp=units[i];if(!cp||cp==0xffff){name[out]=0;return out!=0;}if(cp>=0xd800&&cp<=0xdbff){if(i+1>=255||units[i+1]<0xdc00||units[i+1]>0xdfff)return 0;cp=0x10000+((cp-0xd800)<<10)+(units[++i]-0xdc00);}else if(cp>=0xdc00&&cp<=0xdfff)return 0;
        if(cp<128)name[out++]=(char)cp;else if(cp<2048){name[out++]=(char)(0xc0|(cp>>6));name[out++]=(char)(0x80|(cp&63));}else if(cp<65536){name[out++]=(char)(0xe0|(cp>>12));name[out++]=(char)(0x80|((cp>>6)&63));name[out++]=(char)(0x80|(cp&63));}else{ name[out++]=(char)(0xf0|(cp>>18));name[out++]=(char)(0x80|((cp>>12)&63));name[out++]=(char)(0x80|((cp>>6)&63));name[out++]=(char)(0x80|(cp&63));}}
    name[out]=0;return 1;
}
int fat32_list(fat32 *fs,const fat32_node *dir,fat32_visit visit,void *context){
    if(!fs||!dir||!visit||!(dir->attributes&16)||!valid_cluster(fs,dir->cluster))return FAT32_INVALID;
    uint16_t long_name[260];long_name[0]=0xffff;uint32_t cluster=dir->cluster;uint8_t sequence=0,sum=0;
    for(uint32_t hops=0;hops<fs->clusters;hops++){uint32_t first=cluster_sector(fs,cluster);
        for(uint32_t s=0;s<fs->sectors_per_cluster;s++){uint8_t sector[512];int r=read_sector(fs,first+s,sector);if(r)return r;
            for(uint32_t offset=0;offset<512;offset+=32){const uint8_t *e=sector+offset;if(!e[0])return 0;if(e[0]==0xe5){sequence=0;sum=0;long_name[0]=0xffff;continue;}
                if(e[11]==15){uint8_t ordinal=e[0]&31;if(e[0]&64){sequence=ordinal;sum=e[13];for(int i=0;i<260;i++)long_name[i]=0xffff;}if(!ordinal||ordinal>20||ordinal!=sequence||e[13]!=sum||e[12]||u16(e+26)||(e[0]&0xa0)){sequence=0;sum=0;long_name[0]=0xffff;continue;}const uint8_t positions[]={1,3,5,7,9,14,16,18,20,22,24,28,30};for(int i=0;i<13;i++)long_name[(ordinal-1)*13+i]=u16(e+positions[i]);sequence--;continue;}
                if(e[11]&8){sequence=0;sum=0;long_name[0]=0xffff;continue;}char name[1024];short_name(e,name);
                fat32_node node={.cluster=((uint32_t)u16(e+20)<<16|u16(e+26))&0x0fffffffu,.size=u32(e+28),.entry_sector=first+s,.entry_offset=(uint16_t)offset,.attributes=e[11]};
                short_name(e,node.short_name);if(!sequence&&sum==checksum(e)&&long_name[0]!=0xffff){char decoded[1024];if(utf8_name(long_name,decoded)){uint32_t i=0;do{name[i]=decoded[i];}while(decoded[i++]);}}sequence=0;sum=0;long_name[0]=0xffff;
                if(node.cluster&&!valid_cluster(fs,node.cluster))return FAT32_INVALID;if(!node.cluster&&(node.size||((node.attributes&16)&&e[0]!='.')))return FAT32_INVALID;
                if(visit(context,name,&node))return 0;
            }
        }uint32_t next;int r=next_cluster(fs,cluster,&next);if(r)return r;if(!next)return 0;cluster=next;
    }return FAT32_INVALID;
}
typedef struct {const char *name;fat32_node *node;int found;} search;
static int find_entry(void *context,const char *name,const fat32_node *node){search *s=context;if(same(s->name,name)||same(s->name,node->short_name)){*s->node=*node;s->found=1;return 1;}return 0;}
int fat32_lookup(fat32 *fs,const char *path,fat32_node *node){
    if(!fs||!path||!node||!fs->sectors)return FAT32_INVALID;*node=(fat32_node){.cluster=fs->root,.attributes=16};
    while(*path){while(*path=='/')path++;if(!*path)break;char part[1024];uint32_t n=0;while(*path&&*path!='/'){if(n+1==sizeof(part))return FAT32_INVALID;part[n++]=*path++;}part[n]=0;if(same(part,"."))continue;if(!(node->attributes&16))return FAT32_NOT_FILE;
        if(same(part,"..")&&node->cluster==fs->root)continue;fat32_node found;search s={part,&found,0};int r=fat32_list(fs,node,find_entry,&s);if(r)return r;if(!s.found)return FAT32_NOT_FOUND;*node=found;if(!node->cluster&&(node->attributes&16))node->cluster=fs->root;
    }return 0;
}
int fat32_read(fat32 *fs,const fat32_node *node,uint32_t offset,void *buffer,uint32_t count,uint32_t *done){
    if(done)*done=0;if(!fs||!node||(!buffer&&count)||!done)return FAT32_INVALID;if(node->attributes&16)return FAT32_NOT_FILE;if(offset>=node->size||!count)return 0;if(count>node->size-offset)count=node->size-offset;
    uint32_t cluster=node->cluster,cluster_bytes=(uint32_t)fs->sectors_per_cluster*512,skip=offset/cluster_bytes;if(skip>=fs->clusters)return FAT32_INVALID;
    for(uint32_t i=0;i<skip;i++){int r=next_cluster(fs,cluster,&cluster);if(r)return r;if(!cluster)return FAT32_INVALID;}offset%=cluster_bytes;uint8_t *out=buffer;uint32_t hops=0;
    while(count){if(!valid_cluster(fs,cluster)||hops++>=fs->clusters)return FAT32_INVALID;for(uint32_t sector=offset/512;sector<fs->sectors_per_cluster&&count;sector++){int r=read_sector(fs,cluster_sector(fs,cluster)+sector,fs->scratch);if(r)return r;uint32_t within=offset%512,n=512-within;if(n>count)n=count;for(uint32_t i=0;i<n;i++)out[i]=fs->scratch[within+i];out+=n;count-=n;*done+=n;offset=0;}if(count){int r=next_cluster(fs,cluster,&cluster);if(r)return r;}}
    return 0;
}
static int allocation_hint(fat32 *fs){
    uint32_t info[]={fs->info_sector,fs->backup_info};uint8_t sector[512];for(int i=0;i<2;i++){if(!info[i])continue;int r=read_sector(fs,info[i],sector);if(r)return r;if(u32(sector)!=0x41615252u||u32(sector+484)!=0x61417272u||u32(sector+508)!=0xaa550000u)continue;put32(sector+488,0xffffffffu);put32(sector+492,fs->next_free);r=write_sector(fs,info[i],sector);if(r)return r;}return 0;
}
static int allocate(fat32 *fs,uint32_t *cluster){
    for(uint32_t scanned=0;scanned<fs->clusters;scanned++){uint32_t n=fs->next_free++;if(fs->next_free-2>=fs->clusters)fs->next_free=2;uint32_t value;int r=fat_value(fs,n,&value);if(r)return r;if(value)continue;
        uint8_t zero[512]={0};for(uint32_t i=0;i<fs->sectors_per_cluster;i++){r=write_sector(fs,cluster_sector(fs,n)+i,zero);if(r)return r;}r=set_fat(fs,n,0x0fffffffu);if(r)return r;r=allocation_hint(fs);if(r)return r;*cluster=n;return 0;}
    return FAT32_NO_SPACE;
}
static int update_entry(fat32 *fs,const fat32_node *node){if(node->entry_offset>480||(node->entry_offset&31))return FAT32_INVALID;int r=read_sector(fs,node->entry_sector,fs->scratch);if(r)return r;uint8_t *p=fs->scratch+node->entry_offset;p[20]=(uint8_t)(node->cluster>>16);p[21]=(uint8_t)(node->cluster>>24);p[26]=(uint8_t)node->cluster;p[27]=(uint8_t)(node->cluster>>8);put32(p+28,node->size);p[11]|=32;return write_sector(fs,node->entry_sector,fs->scratch);}
int fat32_write(fat32 *fs,fat32_node *node,uint32_t offset,const void *buffer,uint32_t count,uint32_t *done){
    if(done)*done=0;if(!fs||!node||(!buffer&&count)||!done||offset>node->size||count>0xffffffffu-offset)return FAT32_INVALID;if(!fs->write||node->attributes&1)return FAT32_READ_ONLY;if(node->attributes&16)return FAT32_NOT_FILE;if(!count)return 0;
    uint32_t bytes=(uint32_t)fs->sectors_per_cluster*512,cluster=node->cluster,skip=offset/bytes;if(skip>=fs->clusters)return FAT32_NO_SPACE;
    if(!cluster){int r=allocate(fs,&cluster);if(r)return r;node->cluster=cluster;}
    for(uint32_t i=0;i<skip;i++){uint32_t next;int r=next_cluster(fs,cluster,&next);if(r)return r;if(!next){r=allocate(fs,&next);if(r)return r;r=set_fat(fs,cluster,next);if(r)return r;}cluster=next;}
    uint32_t within=offset%bytes,hops=0;const uint8_t *in=buffer;
    while(count){if(!valid_cluster(fs,cluster)||hops++>=fs->clusters)return FAT32_INVALID;uint32_t sector=cluster_sector(fs,cluster)+within/512,start=within%512,n=512-start;if(n>count)n=count;
        int r=0;if(start||n!=512){r=read_sector(fs,sector,fs->scratch);if(r)return r;}for(uint32_t i=0;i<n;i++)fs->scratch[start+i]=in[i];r=write_sector(fs,sector,fs->scratch);if(r)return r;in+=n;count-=n;*done+=n;within+=n;
        if(within==bytes&&count){uint32_t next;r=next_cluster(fs,cluster,&next);if(r)return r;if(!next){r=allocate(fs,&next);if(r)return r;r=set_fat(fs,cluster,next);if(r)return r;}cluster=next;within=0;}
    }
    if(offset+*done>node->size)node->size=offset+*done;int r=update_entry(fs,node);if(r)return r;return fs->flush&&fs->flush(fs->context)?FAT32_IO:0;
}
static int encode_name(const char *name,uint8_t out[11]){for(int i=0;i<11;i++)out[i]=' ';uint32_t base=0,ext=0;int dot=0;for(;*name;name++){uint8_t c=(uint8_t)*name;if(c=='.'){if(dot||!base)return 0;dot=1;continue;}if(c<=32||c>=127||c=='"'||c=='*'||c=='+'||c==','||c=='/'||c==':'||c==';'||c=='<'||c=='='||c=='>'||c=='?'||c=='['||c=='\\'||c==']'||c=='|')return 0;if(c>='a'&&c<='z')c-=32;if(dot){if(ext==3)return 0;out[8+ext++]=c;}else{if(base==8)return 0;out[base++]=c;}}return base&&(!dot||ext);}
static int end_marker(fat32 *fs,uint32_t cluster,uint32_t sector,uint32_t offset){
    if(offset<480){fs->scratch[offset+32]=0;return 0;}uint32_t next_sector;
    if(sector+1<fs->sectors_per_cluster)next_sector=cluster_sector(fs,cluster)+sector+1;
    else{uint32_t next;int r=next_cluster(fs,cluster,&next);if(r)return r;if(!next)return 0;next_sector=cluster_sector(fs,next);}
    uint8_t next[512];int r=read_sector(fs,next_sector,next);if(r)return r;next[0]=0;return write_sector(fs,next_sector,next);
}
int fat32_create(fat32 *fs,const char *path,fat32_node *node){
    if(!fs||!path||!node)return FAT32_INVALID;if(!fs->write)return FAT32_READ_ONLY;fat32_node existing;int r=fat32_lookup(fs,path,&existing);if(!r)return FAT32_EXISTS;if(r!=FAT32_NOT_FOUND)return r;
    char parent[1024];uint32_t length=0,last=0;while(path[length]){if(length+1==sizeof(parent))return FAT32_INVALID;if(path[length]=='/')last=length+1;parent[length]=path[length];length++;}parent[last]=0;uint8_t name[11];if(!encode_name(path+last,name))return FAT32_UNSUPPORTED;
    fat32_node dir;r=fat32_lookup(fs,parent,&dir);if(r)return r;if(!(dir.attributes&16))return FAT32_NOT_FILE;if(dir.attributes&1)return FAT32_READ_ONLY;uint32_t cluster=dir.cluster;
    for(uint32_t hops=0;hops<fs->clusters;hops++){for(uint32_t s=0;s<fs->sectors_per_cluster;s++){uint32_t sector=cluster_sector(fs,cluster)+s;r=read_sector(fs,sector,fs->scratch);if(r)return r;for(uint32_t offset=0;offset<512;offset+=32){uint8_t *p=fs->scratch+offset;if(p[0]&&p[0]!=0xe5)continue;if(!p[0]){r=end_marker(fs,cluster,s,offset);if(r)return r;}for(int i=0;i<32;i++)p[i]=0;for(int i=0;i<11;i++)p[i]=name[i];p[11]=32;r=write_sector(fs,sector,fs->scratch);if(r)return r;*node=(fat32_node){.entry_sector=sector,.entry_offset=(uint16_t)offset,.attributes=32};short_name(p,node->short_name);return fs->flush&&fs->flush(fs->context)?FAT32_IO:0;}}
        uint32_t next;r=next_cluster(fs,cluster,&next);if(r)return r;if(!next){r=allocate(fs,&next);if(r)return r;r=set_fat(fs,cluster,next);if(r)return r;}cluster=next;
    }return FAT32_NO_SPACE;
}
