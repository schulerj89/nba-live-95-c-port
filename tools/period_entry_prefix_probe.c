#define _CRT_SECURE_NO_WARNINGS
#include "nba_period_entry_prefix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char memory[131072];
static uint16_t word(unsigned a){return (uint16_t)(memory[a]|((uint16_t)memory[a+1]<<8));}
static void put(unsigned a,uint16_t w){memory[a]=(unsigned char)w;memory[a+1]=(unsigned char)(w>>8);}
static NbaPeriodEntryPrefixState input(void){NbaPeriodEntryPrefixState s={0};
#define ACTOR(name,off) for(unsigned i=0;i<10;++i)s.actors[i].name=word(0x34eb+i*256+(off));
#define GLOBAL(name,off) s.name=word(off);
#include "period_entry_prefix_fields.inc"
#undef ACTOR
#undef GLOBAL
return s;}
static void project(const NbaPeriodEntryPrefixState *s){
#define ACTOR(name,off) for(unsigned i=0;i<10;++i)put(0x34eb+i*256+(off),s->actors[i].name);
#define GLOBAL(name,off) put(off,s->name);
#include "period_entry_prefix_fields.inc"
#undef ACTOR
#undef GLOBAL
}
static void emit(unsigned pc,unsigned ok,const NbaPeriodEntryPrefixState *s,bool compact){
 project(s);printf("{\"pc\":%u,\"result\":%u,\"words\":[",pc,ok);
 if(compact){bool first=true;
#define ACTOR(name,off) for(unsigned i=0;i<10;++i){printf("%s%u",first?"":",",word(0x34eb+i*256+(off)));first=false;}
#define GLOBAL(name,off) printf("%s%u",first?"":",",word(off));first=false;
#include "period_entry_prefix_fields.inc"
#undef ACTOR
#undef GLOBAL
 }else for(unsigned i=0;i<65536;++i)printf("%s%u",i?",":"",word(i*2));
 puts("]}");
}
int main(int argc,char **argv){
 if(argc!=3&&argc!=4)return 2;bool compact=argc==4;
 if(compact&&strcmp(argv[3],"--typed"))return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 if(fseek(f,0x36392,SEEK_SET)){fclose(f);return 2;}
 unsigned char raw[8];if(fread(raw,1,8,f)!=8){fclose(f);return 2;}fclose(f);
 NbaPeriodEntryPrefixTables t;for(unsigned i=0;i<4;++i)t.overtime_clock[i]=(uint16_t)(raw[i*2]|((uint16_t)raw[i*2+1]<<8));
 f=fopen(argv[2],"rb");if(!f)return 2;
 if(fread(memory,1,sizeof(memory),f)!=sizeof(memory)||fgetc(f)!=EOF){fclose(f);return 2;}fclose(f);
 NbaPeriodEntryPrefixState s=input();
 if(compact){bool ok=nba_period_entry_prefix(&t,&s);emit(0x86dd97,ok?1:0,&s,true);return 0;}
 nba_period_entry_prefix_reset(&s);emit(0x86dd2d,1,&s,false);
 if(!nba_period_entry_prefix_clock(&t,&s)){emit(0x86dd47,0,&s,false);return 0;}
 emit(0x86dd47,1,&s,false);nba_period_entry_prefix_table(&s);emit(0x86dd97,1,&s,false);return 0;
}
