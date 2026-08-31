/* Actual mode15/component prestate only. No expected output input. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_pass_release.h"
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char raw[0x20000],present[0x20000];
static const unsigned ranges[][2]={{0,0x2000},{0x3400,0x1600}};
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
static unsigned actor;
static NbaHumanPassPoseState input(void) {
 NbaHumanPassPoseState s={0};actor=word(0x96);if(actor_index(actor)==0xffffu)exit(3);
 NbaHumanPassPoseActor *a=&s.actor;
 a->x=word(actor+0x4);
 a->y=word(actor+0x8);
 a->z=word(actor+0xc);
 a->flags_28=word(actor+0x28);
 a->upper_2a=word(actor+0x2a);
 a->lower_2c=word(actor+0x2c);
 a->upper_30=word(actor+0x30);
 a->lower_32=word(actor+0x32);
 a->previous_upper_34=word(actor+0x34);
 a->previous_lower_36=word(actor+0x36);
 a->phase_3a=word(actor+0x3a);
 a->phase_3c=word(actor+0x3c);
 a->previous_phase_3e=word(actor+0x3e);
 a->previous_phase_40=word(actor+0x40);
 a->facing_4e=word(actor+0x4e);
 a->resolved_facing_52=word(actor+0x52);
 a->mode_5e=word(actor+0x5e);
 a->variant_6c=word(actor+0x6c);
 a->flags_7e=word(actor+0x7e);
 a->alternate_lower_a8=word(actor+0xa8);
 s.ball_x=word(0x3eef);
 s.ball_y=word(0x3ef3);
 s.ball_z=word(0x3ef7);
 s.previous_ball_x_0922=word(0x922);
 s.live_0936=word(0x936);
 s.scratch_00=word(0x0);
 s.scratch_02=word(0x2);
 s.scratch_04=word(0x4);
 s.scratch_06=word(0x6);
 s.pointer_47=word(0x47);
 s.bank_49=word(0x49);
 s.direction_index_ac=word(0xac);
 return s;
}
static void project(const NbaHumanPassPoseState *s) {
 const NbaHumanPassPoseActor *a=&s->actor;
 put(actor+0x4,a->x);
 put(actor+0x8,a->y);
 put(actor+0xc,a->z);
 put(actor+0x28,a->flags_28);
 put(actor+0x2a,a->upper_2a);
 put(actor+0x2c,a->lower_2c);
 put(actor+0x30,a->upper_30);
 put(actor+0x32,a->lower_32);
 put(actor+0x34,a->previous_upper_34);
 put(actor+0x36,a->previous_lower_36);
 put(actor+0x3a,a->phase_3a);
 put(actor+0x3c,a->phase_3c);
 put(actor+0x3e,a->previous_phase_3e);
 put(actor+0x40,a->previous_phase_40);
 put(actor+0x4e,a->facing_4e);
 put(actor+0x52,a->resolved_facing_52);
 put(actor+0x5e,a->mode_5e);
 put(actor+0x6c,a->variant_6c);
 put(actor+0x7e,a->flags_7e);
 put(actor+0xa8,a->alternate_lower_a8);
 put(0x3eef,s->ball_x);
 put(0x3ef3,s->ball_y);
 put(0x3ef7,s->ball_z);
 put(0x922,s->previous_ball_x_0922);
 put(0x936,s->live_0936);
 put(0x0,s->scratch_00);
 put(0x2,s->scratch_02);
 put(0x4,s->scratch_04);
 put(0x6,s->scratch_06);
 put(0x47,s->pointer_47);
 put(0x49,s->bank_49);
 put(0xac,s->direction_index_ac);
}
static NbaHumanPassReleaseState release_input(void) {
 NbaHumanPassReleaseState s={0};s.pose=input();
 s.controller_16=word(actor+0x16);
 s.timer_60=word(actor+0x60);
 s.behavior_64=word(actor+0x64);
 s.direction_66=word(actor+0x66);
 s.group_6e=word(actor+0x6e);
 s.family_c0=word(actor+0xc0);
 s.actor_index_c2=word(0xc2);
 s.delta_c6=word(0xc6);
 s.offense_093a=word(0x93a);
 s.owner_093e=word(0x93e);
 s.source_0942=word(0x942);
 s.source_0944=word(0x944);
 s.receiver_0946=word(0x946);
 s.flag_09c4=word(0x9c4);
 s.pointer_8e=word(0x8e);
 s.bank_90=word(0x90);
 s.scratch_aa=word(0xaa);
 s.scratch_ae=word(0xae);
 return s;
}
static void release_project(const NbaHumanPassReleaseState *s) {
 project(&s->pose);
 put(actor+0x16,s->controller_16);
 put(actor+0x60,s->timer_60);
 put(actor+0x64,s->behavior_64);
 put(actor+0x66,s->direction_66);
 put(actor+0x6e,s->group_6e);
 put(actor+0xc0,s->family_c0);
 put(0xc2,s->actor_index_c2);
 put(0xc6,s->delta_c6);
 put(0x93a,s->offense_093a);
 put(0x93e,s->owner_093e);
 put(0x942,s->source_0942);
 put(0x944,s->source_0944);
 put(0x946,s->receiver_0946);
 put(0x9c4,s->flag_09c4);
 put(0x8e,s->pointer_8e);
 put(0x90,s->bank_90);
 put(0xaa,s->scratch_aa);
 put(0xae,s->scratch_ae);
}
static unsigned char rom[0x180000];
static unsigned rd32(const unsigned char *p){return p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24);}
static void original_assets(const NbaAssetPack *assets) {
 const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_PLAYER_ANIMATIONS);
 if(!item||item->size<80u)exit(3);const unsigned char *data=item->data;
 static const unsigned asset_ranges[][4]={{20,0,0x848000,0x8000},{24,0,0xa9d86e,0x830},{24,0x830,0xa9d03e,0x830},
  {56,0,0xaca9cf,0x830},{60,0,0xacb267,0x830},{64,0,0xaca583,0x830},
  {68,0,0xaccc2f,0x830},{72,0,0xacbf4b,0x830},{76,0,0xacc397,0x830}};
 for(unsigned i=0;i<sizeof(asset_ranges)/sizeof(asset_ranges[0]);++i){
  unsigned offset=rd32(data+asset_ranges[i][0])+asset_ranges[i][1],pc=asset_ranges[i][2],size=asset_ranges[i][3];
  unsigned original=((pc>>16)&0x7f)*0x8000+(pc&0x7fff);
  if(offset>item->size||size>item->size-offset||original>sizeof(rom)||size>sizeof(rom)-original||memcmp(data+offset,rom+original,size))exit(3);
 }
}
int main(int argc,char **argv) {
 if(argc!=3)return 2;NbaAssetPack assets={0};
 int saved=_dup(_fileno(stdout));if(saved<0)return 2;
 if(_dup2(_fileno(stderr),_fileno(stdout))!=0)return 2;
 bool loaded=nba_assets_load(&assets,argv[1]);fflush(stdout);
 if(_dup2(saved,_fileno(stdout))!=0)return 2;_close(saved);if(!loaded)return 3;
 FILE *f=fopen(argv[2],"rb");if(!f)return 2;
 if(fread(rom,1,sizeof(rom),f)!=sizeof(rom)||fgetc(f)!=EOF)return 2;fclose(f);original_assets(&assets);
 NbaHumanPassReleaseTables tables={0};
 memcpy(tables.thresholds_a7a0,rom+0x327a0,8);
 for(unsigned i=0;i<10;++i) {
  unsigned p=0x39c7b+i*2;tables.actor_pointers_9c7b[i]=(uint16_t)(rom[p]|((uint16_t)rom[p+1]<<8));
 }
 char line[8192],mode[16],path[4096];
 while(fgets(line,sizeof(line),stdin)) {
  if(sscanf(line,"%15s %4095[^\r\n]",mode,path)!=2)return 2;
  load(path);NbaHumanPassReleaseState s=release_input();unsigned result=1;
  if(!strcmp(mode,"step"))result=nba_human_pass_release_step(&assets,&tables,&s);
  else if(!strcmp(mode,"dispatch"))result=nba_human_pass_release_dispatch(&s)?1u:0u;
  else if(!strcmp(mode,"turn"))nba_human_pass_release_turn(&s);
  else if(!strcmp(mode,"normalize"))nba_human_pass_release_normalize(&s);
  else if(!strcmp(mode,"after"))nba_human_pass_release_after_launch(&s);
  else if(!strcmp(mode,"attach"))result=nba_human_pass_pose_attach(&assets,&s.pose)?1u:0u;
  else if(!strcmp(mode,"offset"))result=nba_human_pass_pose_offset(&assets,&s.pose)?1u:0u;
  else return 2;
  if(!result)return 3;release_project(&s);
  printf("{\"result\":%u",result);
  array("dp_words",0,128);
  array("actor_words",0x34eb,11*128);array("controller_words",0x47eb,160);array("context_words",0x46eb,128);array("profile_words",0x3449,20);array("order_words",0x34d1,13);
  static const unsigned globals[]={0x7f6,0x904,0x90c,0x90e,0x910,0x922,0x936,0x93a,0x93e,0x942,0x944,0x946,0x978,0x9b8,0x9c4,0x9da};
  printf(",\"global_words\":[");for(unsigned i=0;i<sizeof(globals)/sizeof(globals[0]);++i)printf("%s%u",i?",":"",word(globals[i]));printf("]}\n");
 }
 nba_assets_free(&assets);return ferror(stdin)?1:0;
}
