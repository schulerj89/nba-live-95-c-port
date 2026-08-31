/* Source-specific saved frames captured before the initializer/selector.
 * No assets, native after-state inputs, opcode interpreter or owner writes. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_pass_return.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char memory[0x4a00],present[0x4a00];
static const unsigned ranges[][2]={{0,0x2000},{0x3400,0x1600}};
static const unsigned saved_addresses[8]={0xb8,0xb6,0xbc,0xba,0xc0,0xbe,0x9c,0x9a};
static uint16_t word(unsigned a) {
    if(a+1>=sizeof(memory)||!present[a]||!present[a+1])exit(3);
    return (uint16_t)(memory[a]|((uint16_t)memory[a+1]<<8));
}
static void put(unsigned a,uint16_t value){memory[a]=(unsigned char)value;memory[a+1]=(unsigned char)(value>>8);}
static void load(const char *path) {
    FILE *file=fopen(path,"rb");if(!file)exit(2);
    memset(memory,0,sizeof(memory));memset(present,0,sizeof(present));
    for(unsigned i=0;i<2;++i){unsigned address=ranges[i][0],size=ranges[i][1];
        if(fread(memory+address,1,size,file)!=size)exit(2);memset(present+address,1,size);
    }
    if(fgetc(file)!=EOF)exit(2);fclose(file);
}
static NbaHumanPassReturnWords input(void) {
    NbaHumanPassReturnWords s={word(0xb8),word(0xb6),word(0xbc),word(0xba),word(0xc0),word(0xbe),word(0x9c),word(0x9a)};
    return s;
}
static void values(const NbaHumanPassReturnWords *s,uint16_t result[8]) {
    result[0]=s->b8;result[1]=s->b6;result[2]=s->bc;result[3]=s->ba;
    result[4]=s->c0;result[5]=s->be;result[6]=s->word_9c;result[7]=s->word_9a;
}
static void array(const char *name,unsigned a,unsigned count) {
    printf(",\"%s\":[",name);
    for(unsigned i=0;i<count;++i)printf("%s%u",i?",":"",word(a+2*i));printf("]");
}
int main(int argc,char **argv) {
    (void)argv;if(argc!=1)return 2;
    char line[20000];
    while(fgets(line,sizeof(line),stdin)) {
        char *context=NULL,*part[5];unsigned n=0;
        for(char *p=strtok_s(line,"|\r\n",&context);p;p=strtok_s(NULL,"|\r\n",&context)){
            if(n==5)return 2;part[n++]=p;
        }
        if(n!=5)return 2;
        load(part[2]);NbaHumanPassReturnWords from=input(),initializer;
        if(!nba_human_pass_return_save(&from,&initializer))return 3;
        load(part[3]);from=input();NbaHumanPassReturnWords selector;
        if(!nba_human_pass_return_save(&from,&selector))return 3;
        load(part[4]);uint16_t human_b6=word(0xb6);
        load(part[1]);NbaHumanPassReturnWords scratch=input();bool ok=false;
        if(!strcmp(part[0],"initializer"))ok=nba_human_pass_return_restore(&scratch,&initializer);
        else if(!strcmp(part[0],"selector"))ok=nba_human_pass_return_restore(&scratch,&selector);
        else if(!strcmp(part[0],"human"))ok=nba_human_pass_return_human_tail(&scratch,human_b6);
        else if(!strcmp(part[0],"chain"))ok=nba_human_pass_return_finish(&scratch,&initializer,&selector,human_b6);
        else return 2;
        if(!ok)return 3;
        uint16_t state_values[8],init_values[8],pass_values[8];
        values(&scratch,state_values);values(&initializer,init_values);values(&selector,pass_values);
        for(unsigned i=0;i<8;++i)put(saved_addresses[i],state_values[i]);
        printf("{\"result\":1,\"saved_words\":[");
        for(unsigned i=0;i<8;++i)printf("%s%u",i?",":"",init_values[i]);
        for(unsigned i=0;i<8;++i)printf(",%u",pass_values[i]);printf(",%u]",human_b6);
        array("dp_words",0,128);array("actor_words",0x34eb,1408);array("controller_words",0x47eb,160);
        array("context_words",0x46eb,128);array("profile_words",0x3449,20);array("order_words",0x34d1,13);
        static const unsigned globals[]={0x7f6,0x904,0x90c,0x90e,0x910,0x922,0x936,0x93a,0x93e,0x942,0x944,0x946,0x978,0x9b8,0x9c4,0x9da};
        printf(",\"global_words\":[");for(unsigned i=0;i<16;++i)printf("%s%u",i?",":"",word(globals[i]));printf("]}\n");
    }
    return ferror(stdin)?1:0;
}
