/* Bounded native prestate replay. No native poststate is read here. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_dispatch.h"
#include "nba_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char raw[0x20000],present[0x20000];
static const unsigned ranges[][2]={{0,0x100},{0x500,0x500},{0x1600,0x300},{0x3400,0x1600}};
static uint16_t word(unsigned a) {
    if(a+1>=sizeof(raw)||!present[a]||!present[a+1]) {fprintf(stderr,"missing raw word %x\n",a);exit(3);}
    return (uint16_t)(raw[a]|((uint16_t)raw[a+1]<<8));
}
static void put(unsigned a,uint16_t v) {raw[a]=(unsigned char)v;raw[a+1]=(unsigned char)(v>>8);}
static void array(const char *name,unsigned a,unsigned n) {
    printf(",\"%s\":[",name);
    for(unsigned i=0;i<n;++i)printf("%s%u",i?",":"",word(a+i*2));
    printf("]");
}
static void load(const char *path) {
    FILE *f=fopen(path,"rb");if(!f)exit(2);
    memset(raw,0,sizeof(raw));memset(present,0,sizeof(present));
    for(unsigned i=0;i<sizeof(ranges)/sizeof(ranges[0]);++i){
        unsigned a=ranges[i][0],n=ranges[i][1];
        if(fread(raw+a,1,n,f)!=n)exit(2);memset(present+a,1,n);
    }
    if(fgetc(f)!=EOF)exit(2);fclose(f);
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    NbaRom rom={0};if(!nba_rom_load_file(&rom,argv[1]))return 2;
    char line[8192],mode[16],path[4096];
    while(fgets(line,sizeof(line),stdin)) {
        if(sscanf(line,"%15s %4095[^\r\n]",mode,path)!=2)return 2;
        load(path);
        unsigned actor=word(0x96);
        if(actor<0x34eb || (actor-0x34eb)%0x100 || actor>=0x3eeb)return 3;
        if(!strcmp(mode,"gate")) {
            int16_t controller=(int16_t)word(actor+0x16);
            uint16_t processed=0;
            if(controller>=0 && controller<5) {
                unsigned record=nba_rom_read16(&rom,0x87,(uint16_t)(0x9c71+controller*2));
                processed=word(record+4);
            }
            NbaHumanInputGate gate=nba_human_input_gate(controller,processed);
            if(gate==NBA_HUMAN_INPUT_INVALID)return 3;
            printf("{\"next_pc\":%u}\n",gate==NBA_HUMAN_INPUT_PUBLISH?0x87915du:0x87922eu);
        } else if(!strcmp(mode,"b")) {
            NbaHumanBAction next=nba_human_b_action(word(0xae),word(0xc2),word(0x93e),word(0x978),word(0x946),word(0x936),word(actor+0x4c));
            static const unsigned pc[]={0x84e2f2,0x84e3e6,0x84e2e4,0x84e2eb};
            printf("{\"next_pc\":%u}\n",pc[next]);
        } else if(!strcmp(mode,"motion")) {
            unsigned record=word(0x9a),context=word(0x9e);
            if(record<0x47eb || (record-0x47eb)%0x40 || record>=0x492b)return 3;
            uint16_t profile_address=(uint16_t)(word(0xe0)+0x42);
            uint8_t bank=(uint8_t)word(0xe2);
            uint8_t profile;
            if(bank==0x7e||bank==0x7f) {
                unsigned address=(bank==0x7f?0x10000:0)+profile_address;
                if(!present[address])return 3;profile=raw[address];
            } else profile=nba_rom_read8(&rom,bank,profile_address);
            NbaHumanMotion s={
                word(0xc2),word(0x978),word(actor+0x7a),word(actor+0x7e),word(0x946),
                word(0x936),word(0x952),word(context+0xc),word(0x996),word(0x954),word(0x93e),word(actor+0x5e),
                word(record+6),word(actor+0xc),word(actor+0x72),word(0xc6),profile,(int16_t)word(actor+0xe),(int16_t)word(actor+0x10),word(record+0x72)
            };
            NbaHumanMotionRoute route=nba_human_motion_step(&s);
            if(route==NBA_HUMAN_MOTION_INVALID)return 3;
            put(actor+0xe,(uint16_t)s.velocity_x);put(actor+0x10,(uint16_t)s.velocity_y);put(actor+0x72,s.boost);
            put(record+0x72,s.controller_word_72);
            printf("{\"accelerator_call\":%u",route==NBA_HUMAN_MOTION_ACCELERATE?0x85a82cu:0);
            array("actor_words",actor,128);array("controller_words",0x47eb,160);array("context_words",0x46eb,128);
            printf(",\"controller_word_72\":%u",word(record+0x72));
            printf("}\n");
        } else return 2;
    }
    nba_rom_free(&rom);return 0;
}
