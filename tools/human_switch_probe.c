/* Full switch-child persistent effects replay from native entry only. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_switch.h"
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
static uint16_t controller_index(unsigned p){
    return p>=0x47eb&&p<0x492b&&(p-0x47eb)%0x40==0?(uint16_t)((p-0x47eb)/0x40):0xffffu;
}
int main(void){
    char line[8192];
    while(fgets(line,sizeof(line),stdin)){
        line[strcspn(line,"\r\n")]=0;load(line);
        unsigned context=word(0x9e);
        if(context!=0x46eb&&context!=0x476b)return 3;
        NbaHumanSwitchState s={0};
        s.context_controller_count=word(context+0x3b);
        if(s.context_controller_count<5u){
            unsigned end=word(context+6),record=word(0x9a),anchor=word(0x910);
            if(controller_index(record)==0xffffu)return 3;
            s.actor=actor_index(word(0x96));s.group_first=actor_index(word(context+4));
            if(end<0x34eb||end>0x3eeb||(end-0x34eb)%0x100)return 3;
            s.group_end=(uint16_t)((end-0x34eb)/0x100);
            s.controller_090c=controller_index(word(0x90c));s.direction=word(record+6);
            s.fallback_actor=actor_index(word(0xa6));s.incoming_index_c2=word(0xc2);
            s.neutral_anchor_x=(int16_t)word(anchor+4);s.neutral_anchor_y=(int16_t)word(anchor+8);
            for(unsigned i=0;i<10;++i){
                unsigned p=0x34eb+i*0x100;
                s.actors[i].x=(int16_t)word(p+4);s.actors[i].y=(int16_t)word(p+8);
                s.actors[i].group=word(p+0x6e);
                s.controllers.actor_assignment[i]=(int16_t)word(p+0x16);
            }
            for(unsigned i=0;i<5;++i)s.controllers.record[i].actor=word(0x47ed+i*0x40);
        }
        NbaHumanSwitchRoute route=nba_human_switch_control(&s);
        if(route==NBA_HUMAN_SWITCH_INVALID){fprintf(stderr,"invalid native switch input %s\n",line);return 3;}
        if(route!=NBA_HUMAN_SWITCH_ALL_CONTROLLED){
            for(unsigned i=0;i<10;++i)put(0x3501+i*0x100,(uint16_t)s.controllers.actor_assignment[i]);
            for(unsigned i=0;i<5;++i)put(0x47ed+i*0x40,s.controllers.record[i].actor);
        }
        printf("{\"route\":%u",(unsigned)route);
        array("actor_words",0x34eb,11*128);array("controller_words",0x47eb,160);array("context_words",0x46eb,128);
        static const unsigned globals[]={0x90c,0x90e,0x910,0x936,0x93a,0x93e,0x944,0x946,0x978,0xa6,0x9a,0xc2,0xbe,0xba,0xb6};
        printf(",\"preserved_words\":[");
        for(unsigned i=0;i<sizeof(globals)/sizeof(globals[0]);++i)printf("%s%u",i?",":"",word(globals[i]));
        printf("]}\n");
    }
    return 0;
}
