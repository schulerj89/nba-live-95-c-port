/* AD3D catch and actual child entry states only; no native after-state input. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_pass_catch.h"
#include <io.h>
#include "nba_assets.h"
#include "nba_gameplay_ai.h"
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
static unsigned char rom[0x180000];
static unsigned char bus_byte(unsigned pc) {
 unsigned bank=(pc>>16)&255,address=pc&65535;
 if((bank&0x7f)<0x40&&address<0x2000){if(!present[address])exit(3);return raw[address];}
 if(bank==0x7e||bank==0x7f){unsigned a=(bank-0x7e)*0x10000+address;if(!present[a])exit(3);return raw[a];}
 if(address>=0x8000){unsigned offset=(bank&0x7f)*0x8000+(address&0x7fff);if(offset>=sizeof(rom))exit(3);return rom[offset];}
 fprintf(stderr,"unsupported source bus address %06x\n",pc);exit(3);
}
static uint16_t bus_word(unsigned pc){return (uint16_t)(bus_byte(pc)|((uint16_t)bus_byte((pc+1)&0xffffff)<<8));}
static unsigned pointer(unsigned dp){return word(dp)|((unsigned)(word(dp+2)&255)<<16);}
static uint16_t cursor(unsigned p){if(p<0x34d3||p>0x34e7||(p-0x34d1)%2)exit(3);return (uint16_t)((p-0x34d1)/2);}
static NbaHumanPassCatchState input(void) {
 NbaHumanPassCatchState s={0};s.source_slot=actor_index(word(0x96));s.receiver_slot=actor_index(word(0x8e));
 if(s.source_slot==0xffff||s.receiver_slot==0xffff)exit(3);
 s.basket_x=word(word(0x9e)+10);s.indirect_word_42=bus_word((pointer(0)+0x42)&0xffffff);s.profile_word_39=bus_word((pointer(0xe0)+0x39)&0xffffff);
 for(unsigned i=0;i<11;++i){unsigned p=0x34eb+i*256;NbaHumanPassCatchActor *a=&s.actors[i];
  a->x=word(p+4);a->y=word(p+8);a->team=word(p+0x6e);a->order_cursor=cursor(word(p+0x14));
  a->mode=word(p+0x5e);a->timer=word(p+0x60);a->flags=word(p+0x7e);a->pass_band=word(p+0x62);a->axis_88=word(p+0x88);
 }
 for(unsigned i=0;i<13;++i){unsigned p=word(0x34d1+i*2);s.order[i]=p==0?0xffff:p==0x3eeb?10:actor_index(p);if(p&&s.order[i]==0xffff)exit(3);}
 s.rng_07f6=word(0x7f6);
 s.attempt_0904=word(0x904);
 s.aa=word(0xaa);
 s.ae=word(0xae);
 s.ac=word(0xac);
 s.b2=word(0xb2);
 s.b6=word(0xb6);
 s.ba=word(0xba);
 s.be=word(0xbe);
 s.candidate_92=word(0x92);
 return s;
}
static void project(const NbaHumanPassCatchState *s) {
 put(0x7f6,s->rng_07f6);
 put(0x904,s->attempt_0904);
 put(0xaa,s->aa);
 put(0xae,s->ae);
 put(0xac,s->ac);
 put(0xb2,s->b2);
 put(0xb6,s->b6);
 put(0xba,s->ba);
 put(0xbe,s->be);
 put(0x92,s->candidate_92);
 put(0x96,(uint16_t)(0x34eb+s->source_slot*256));put(0x8e,(uint16_t)(0x34eb+s->receiver_slot*256));
 for(unsigned i=0;i<11;++i){unsigned p=0x34eb+i*256;const NbaHumanPassCatchActor *a=&s->actors[i];
  put(p+4,a->x);put(p+8,a->y);put(p+0x6e,a->team);put(p+0x14,(uint16_t)(0x34d1+a->order_cursor*2));
  put(p+0x5e,a->mode);put(p+0x60,a->timer);put(p+0x7e,a->flags);put(p+0x62,a->pass_band);put(p+0x88,a->axis_88);
 }
 for(unsigned i=0;i<13;++i)put(0x34d1+i*2,s->order[i]==0xffff?0:(uint16_t)(0x34eb+s->order[i]*256));
}
int main(int argc,char **argv) {
 if(argc!=3)return 2;NbaAssetPack assets={0};
 int saved=_dup(_fileno(stdout));if(saved<0)return 2;
 if(_dup2(_fileno(stderr),_fileno(stdout))!=0)return 2;
 bool loaded=nba_assets_load(&assets,argv[1]);fflush(stdout);
 if(_dup2(saved,_fileno(stdout))!=0)return 2;_close(saved);if(!loaded)return 3;
 FILE *f=fopen(argv[2],"rb");if(!f)return 2;
 if(fread(rom,1,sizeof(rom),f)!=sizeof(rom)||fgetc(f)!=EOF)return 2;fclose(f);
 char line[8192],mode[16],path[4096];
 while(fgets(line,sizeof(line),stdin)) {
  if(sscanf(line,"%15s %4095[^\r\n]",mode,path)!=2)return 2;
  load(path);NbaHumanPassCatchState s=input();bool ok=false,flow=false;NbaHumanPassCatchRoute route=NBA_HUMAN_PASS_CATCH_INVALID;
  if(!strcmp(mode,"geometry"))ok=nba_human_pass_catch_geometry(&s);
  else if(!strcmp(mode,"rng"))ok=nba_human_pass_catch_rng(&s);
  else if(!strcmp(mode,"direction"))ok=nba_human_pass_catch_direction(&s);
  else if(!strcmp(mode,"lane"))ok=nba_human_pass_catch_lane(&s);
  else {flow=true;
   if(!strcmp(mode,"attempt"))route=nba_human_pass_catch_attempt(&s);
   else if(!strcmp(mode,"receiver"))route=nba_human_pass_catch_receiver(&s);
   else if(!strcmp(mode,"prefix"))route=nba_human_pass_catch_prepare(&s);
   else return 2;
   ok=route!=NBA_HUMAN_PASS_CATCH_INVALID;
  }
  if(!ok)return 3;project(&s);
  printf("{\"%s\":%u,\"input_words\":[%u,%u,%u],\"dp_words\":[",flow?"route":"result",flow?(unsigned)route:1u,s.indirect_word_42,s.profile_word_39,s.basket_x);
  static const unsigned dp[]={0,2,0x51,0x8e,0x92,0x96,0x9a,0x9e,0xaa,0xac,0xae,0xb2,0xb6,0xba,0xbe,0xc2,0xa6};
  for(unsigned i=0;i<sizeof(dp)/sizeof(dp[0]);++i)printf("%s%u",i?",":"",word(dp[i]));printf("]");
  array("actor_words",0x34eb,11*128);array("controller_words",0x47eb,160);array("context_words",0x46eb,128);array("profile_words",0x3449,20);array("order_words",0x34d1,13);
  static const unsigned globals[]={0x7f6,0x904,0x90c,0x90e,0x910,0x922,0x936,0x93a,0x93e,0x942,0x944,0x946,0x978,0x9b8,0x9c4,0x9da};
  printf(",\"global_words\":[");for(unsigned i=0;i<sizeof(globals)/sizeof(globals[0]);++i)printf("%s%u",i?",":"",word(globals[i]));printf("]}\n");
 }
 nba_assets_free(&assets);return ferror(stdin)?1:0;
}
