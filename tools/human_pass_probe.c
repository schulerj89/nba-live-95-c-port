/* Bounded pass receiver selection and F1C1 distance from native entry only. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_pass.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char raw[0x20000],present[0x20000];
static const unsigned ranges[][2]={{0,0x100},{0x500,0x500},{0x1600,0x300},{0x3400,0x1600}};
static uint16_t word(unsigned a) {
    if(a+1>=sizeof(raw)||!present[a]||!present[a+1]){
        fprintf(stderr,"missing raw word %x\n",a);exit(3);
    }
    return (uint16_t)(raw[a]|((uint16_t)raw[a+1]<<8));
}
static void put(unsigned a,uint16_t v){raw[a]=(unsigned char)v;raw[a+1]=(unsigned char)(v>>8);}
static void array(const char *name,unsigned a,unsigned n){
    printf(",\"%s\":[",name);
    for(unsigned i=0;i<n;++i)printf("%s%u",i?",":"",word(a+i*2));
    printf("]");
}
static void load(const char *path){
    FILE *f=fopen(path,"rb");if(!f)exit(2);
    memset(raw,0,sizeof(raw));memset(present,0,sizeof(present));
    for(unsigned i=0;i<sizeof(ranges)/sizeof(ranges[0]);++i){
        unsigned a=ranges[i][0],n=ranges[i][1];
        if(fread(raw+a,1,n,f)!=n)exit(2);
        memset(present+a,1,n);
    }
    if(fgetc(f)!=EOF)exit(2);fclose(f);
}
static uint16_t actor_index(unsigned p){
    return p>=0x34eb&&p<0x3eeb&&(p-0x34eb)%0x100==0?(uint16_t)((p-0x34eb)/0x100):0xffffu;
}
int main(void){
    char line[8192],mode[16],path[4096];
    while(fgets(line,sizeof(line),stdin)){
        if(sscanf(line,"%15s %4095[^\r\n]",mode,path)!=2)return 2;
        load(path);
        if(!strcmp(mode,"metric")){
            printf("{\"distance\":%u}\n",nba_human_pass_distance((int16_t)word(0xaa),(int16_t)word(0xae)));
            continue;
        }
        if(strcmp(mode,"pass"))return 2;
        unsigned context=word(0x9e),record=word(0x9a);
        if(context!=0x46eb&&context!=0x476b)return 3;
        if(record<0x47eb||record>=0x492b||(record-0x47eb)%0x40)return 3;
        NbaHumanPassInput s={0};
        s.actor=actor_index(word(0x96));s.group_first=actor_index(word(context+4));
        s.context_group=word(context+0xc);s.direction=word(record+6);s.controller_id_090e=word(0x90e);
        for(unsigned i=0;i<10;++i){
            unsigned p=0x34eb+i*0x100;
            s.actors[i].x=(int16_t)word(p+4);s.actors[i].y=(int16_t)word(p+8);
            s.actors[i].mode=word(p+0x5e);s.actors[i].anchor_distance_8c=word(p+0x8c);
        }
        NbaHumanPassSelection out=nba_human_pass_select(&s);
        if(out.route==NBA_HUMAN_PASS_INVALID){fprintf(stderr,"invalid native pass input %s\n",path);return 3;}
        put(0x944,out.controller_tag_0944);
        printf("{\"route\":%u,\"score\":%u,\"handoff_words\":[",(unsigned)out.route,out.score);
        if(out.route==NBA_HUMAN_PASS_CONTINUE_INITIALIZER)printf("%u,%u",out.receiver_identity,0x34eb+out.receiver_slot*0x100);
        printf("]");
        array("actor_words",0x34eb,11*128);array("controller_words",0x47eb,160);array("context_words",0x46eb,128);
        static const unsigned globals[]={0x90c,0x90e,0x910,0x936,0x93a,0x93e,0x944,0x946,0x978};
        printf(",\"global_words\":[");
        for(unsigned i=0;i<sizeof(globals)/sizeof(globals[0]);++i)printf("%s%u",i?",":"",word(globals[i]));
        printf("]}\n");
    }
    return 0;
}
