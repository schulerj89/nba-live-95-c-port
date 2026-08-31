/* Original99C4/helper entry only. No assets or native after-state input. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_pass_launch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char memory[0x4a00],present[0x4a00];
static const unsigned ranges[][2]={{0,0x2000},{0x3400,0x1600}};
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
static NbaHumanPassLaunchMath math_input(void) {
 NbaHumanPassLaunchMath m={0};
 m.cc=word(0xcc);
 m.ce=word(0xce);
 m.d0=word(0xd0);
 m.remainder_0806=word(0x806);
 m.remainder_high_0808=word(0x808);
 m.divisor_080a=word(0x80a);
 m.divisor_high_080c=word(0x80c);
 m.quotient_080e=word(0x80e);
 m.quotient_high_0810=word(0x810);
 m.product_low_0820=word(0x820);
 m.cross_high_0822=word(0x822);
 m.sign_0824=word(0x824);
 m.count_085a=word(0x85a);
 return m;
}
static void math_project(const NbaHumanPassLaunchMath *m) {
 put(0xcc,m->cc);
 put(0xce,m->ce);
 put(0xd0,m->d0);
 put(0x806,m->remainder_0806);
 put(0x808,m->remainder_high_0808);
 put(0x80a,m->divisor_080a);
 put(0x80c,m->divisor_high_080c);
 put(0x80e,m->quotient_080e);
 put(0x810,m->quotient_high_0810);
 put(0x820,m->product_low_0820);
 put(0x822,m->cross_high_0822);
 put(0x824,m->sign_0824);
 put(0x85a,m->count_085a);
}
static uint16_t index(unsigned pointer) {
 if(pointer<0x34eb||pointer>=0x3eeb||(pointer-0x34eb)%256)return 0xffff;
 return (uint16_t)((pointer-0x34eb)/256);
}
static NbaHumanPassLaunchState input(void) {
 NbaHumanPassLaunchState s={0};s.math=math_input();
 s.source_index=index(word(0x96));s.receiver_index=index(word(0x8e));
 for(unsigned i=0;i<10;++i){unsigned p=0x34eb+i*256;NbaHumanPassLaunchActor *a=&s.actors[i];
 a->x=word(p+0x4);
 a->y=word(p+0x8);
 a->z=word(p+0xc);
 a->velocity_x=word(p+0xe);
 a->velocity_y=word(p+0x10);
 a->flags_28=word(p+0x28);
 a->upper_30=word(p+0x30);
 a->delay_5a=word(p+0x5a);
 a->mode_5e=word(p+0x5e);
 a->timer_60=word(p+0x60);
 a->band_62=word(p+0x62);
 a->behavior_64=word(p+0x64);
 a->group_6e=word(p+0x6e);
 a->flags_7e=word(p+0x7e);
 a->family_c0=word(p+0xc0);
 }
 s.ball_x=word(0x3eef);
 s.ball_y=word(0x3ef3);
 s.ball_z=word(0x3ef7);
 s.ball_velocity_x=word(0x3ef9);
 s.ball_velocity_y=word(0x3efb);
 s.ball_velocity_z=word(0x3efd);
 s.pointer_92=word(0x92);
 s.duration_b2=word(0xb2);
 s.profile_e0=word(0xe0);
 s.profile_e2=word(0xe2);
 s.ball_pointer_0910=word(0x910);
 s.profile_0914=word(0x914);
 s.profile_0916=word(0x916);
 s.live_0936=word(0x936);
 s.offense_093a=word(0x93a);
 s.owner_093e=word(0x93e);
 s.released_094a=word(0x94a);
 return s;
}
static void project(const NbaHumanPassLaunchState *s) {
 math_project(&s->math);
 for(unsigned i=0;i<10;++i){unsigned p=0x34eb+i*256;const NbaHumanPassLaunchActor *a=&s->actors[i];
 put(p+0x4,a->x);
 put(p+0x8,a->y);
 put(p+0xc,a->z);
 put(p+0xe,a->velocity_x);
 put(p+0x10,a->velocity_y);
 put(p+0x28,a->flags_28);
 put(p+0x30,a->upper_30);
 put(p+0x5a,a->delay_5a);
 put(p+0x5e,a->mode_5e);
 put(p+0x60,a->timer_60);
 put(p+0x62,a->band_62);
 put(p+0x64,a->behavior_64);
 put(p+0x6e,a->group_6e);
 put(p+0x7e,a->flags_7e);
 put(p+0xc0,a->family_c0);
 }
 put(0x3eef,s->ball_x);
 put(0x3ef3,s->ball_y);
 put(0x3ef7,s->ball_z);
 put(0x3ef9,s->ball_velocity_x);
 put(0x3efb,s->ball_velocity_y);
 put(0x3efd,s->ball_velocity_z);
 put(0x92,s->pointer_92);
 put(0xb2,s->duration_b2);
 put(0xe0,s->profile_e0);
 put(0xe2,s->profile_e2);
 put(0x910,s->ball_pointer_0910);
 put(0x914,s->profile_0914);
 put(0x916,s->profile_0916);
 put(0x936,s->live_0936);
 put(0x93a,s->offense_093a);
 put(0x93e,s->owner_093e);
 put(0x94a,s->released_094a);
}
static void array(const char *name,unsigned a,unsigned count) {
    printf(",\"%s\":[",name);
    for(unsigned i=0;i<count;++i)printf("%s%u",i?",":"",word(a+2*i));printf("]");
}
int main(int argc,char **argv) {
 if(argc!=2)return 2;static unsigned char rom[0x180000];FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 if(fread(rom,1,sizeof(rom),f)!=sizeof(rom)||fgetc(f)!=EOF)return 2;fclose(f);
 NbaHumanPassLaunchTables tables={0};
 for(unsigned family=0;family<3;++family)for(unsigned n=0;n<18;++n){
  unsigned p=0x31c6f+family*36+n*2;tables.family[family][n]=(uint16_t)(rom[p]|((uint16_t)rom[p+1]<<8));
 }
 char line[8192],mode[16],path[4096];
 while(fgets(line,sizeof(line),stdin)) {
  unsigned a,x,y;char extra;
  if(sscanf(line,"%15[^|]|%4095[^|]|%u|%u|%u %c",mode,path,&a,&x,&y,&extra)!=5||a>65535||x>65535||y>65535)return 2;
  load(path);NbaHumanPassLaunchRegisters result={(uint16_t)a,(uint16_t)x,(uint16_t)y};bool helper=true;
  if(!strcmp(mode,"launch")) { NbaHumanPassLaunchState s=input();if(!nba_human_pass_launch(&tables,&s))return 3;project(&s);helper=false; }
  else { NbaHumanPassLaunchMath m=math_input();
   if(!strcmp(mode,"mul"))result=nba_human_pass_launch_multiply(&m,result);
   else if(!strcmp(mode,"divide"))result=nba_human_pass_launch_divide(&m,result);
   else return 2;math_project(&m);
  }
  printf("{\"result\":1,\"return_words\":[");if(helper)printf("%u,%u,%u",result.a,result.x,result.y);printf("]");
  array("dp_words",0,128);array("actor_words",0x34eb,1408);array("controller_words",0x47eb,160);
  array("context_words",0x46eb,128);array("profile_words",0x3449,20);array("order_words",0x34d1,13);
  static const unsigned globals[]={0x7f6,0x904,0x90c,0x90e,0x910,0x914,0x916,0x922,0x936,0x93a,0x93e,0x942,0x944,0x946,0x94a,0x978,0x9b8,0x9c4,0x9da};
  static const unsigned math_words[]={0x806,0x808,0x80a,0x80c,0x80e,0x810,0x820,0x822,0x824,0x85a};
  printf(",\"global_words\":[");for(unsigned i=0;i<sizeof(globals)/sizeof(globals[0]);++i)printf("%s%u",i?",":"",word(globals[i]));printf("]");
  printf(",\"math_words\":[");for(unsigned i=0;i<sizeof(math_words)/sizeof(math_words[0]);++i)printf("%s%u",i?",":"",word(math_words[i]));printf("]}\n");
 }
 return ferror(stdin)?1:0;
}
