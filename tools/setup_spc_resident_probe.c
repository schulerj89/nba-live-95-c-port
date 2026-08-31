#define _CRT_SECURE_NO_WARNINGS
#include "nba_setup_spc_resident.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static NbaSetupSpcResidentBus bus;
static unsigned char input[65552];
static int load(const char *path){FILE *f=fopen(path,"rb");size_t n;if(!f)return 0;n=fread(input,1,sizeof(input),f);if(n!=sizeof(input)||fgetc(f)!=EOF){fclose(f);return 0;}fclose(f);return 1;}
static void state(const NbaSetupSpcResident *s,const char *kind){
 printf("{\"kind\":\"%s\",\"pc\":%u,\"cycles\":%llu,\"a\":%u,\"x\":%u,\"y\":%u,\"sp\":%u,\"ps\":%u,\"phase\":%u}\n",kind,s->pc,(unsigned long long)s->cycles,s->a,s->x,s->y,s->sp,s->ps,s->phase);
}
static int selftest(void){
 NbaSetupSpcResident s,before;NbaSetupSpcResidentBus *saved=malloc(sizeof(bus));unsigned i;NbaSetupSpcResidentWork w;
 if(!saved)return 1;memset(&bus,0,sizeof(bus));
 if(nba_setup_spc_resident_begin(&s,0x441,0,0,0,255,0x20)||nba_setup_spc_resident_begin(&s,0x380,0,0,0,255,0))return 2;
 for(i=0;i<4;i++){bus.spc_to_cpu[i]=(uint8_t)(50+i);if(!nba_setup_spc_resident_visible_input(&bus,i,(uint8_t)(20+i))||bus.spc_to_cpu[i]!=50+i||bus.aram[0xf4+i])return 3;}
 if(nba_setup_spc_resident_visible_input(&bus,4,0))return 4;
 bus.aram[0x613]=0x8f;bus.aram[0x614]=5;bus.aram[0x615]=0xf4;bus.aram[0x616]=0xe4;bus.aram[0x617]=0xf5;
 if(!nba_setup_spc_resident_begin(&s,0x613,222,1,2,254,0x49))return 5;
 for(i=0;i<5;i++)if(!nba_setup_spc_resident_accept(&s,&bus))return 6;
 if(bus.spc_to_cpu[0]!=5||bus.cpu_to_spc[0]!=20||bus.aram[0xf4]!=5||s.a!=222||s.ps!=0x49||s.pc!=0x616)return 7;
 if(!nba_setup_spc_resident_accept(&s,&bus)||!nba_setup_spc_resident_accept(&s,&bus))return 8;
 nba_setup_spc_resident_visible_input(&bus,1,99);
 if(!nba_setup_spc_resident_accept(&s,&bus)||s.a!=99)return 9;
 nba_setup_spc_resident_begin(&s,0x447,1,2,3,255,0);s.pc=0x48b;s.phase=2;before=s;*saved=bus;w=nba_setup_spc_resident_peek(&s);
 if(w.kind!=NBA_SPC_TIMER||nba_setup_spc_resident_accept(&s,&bus)||memcmp(&before,&s,sizeof(s))||memcmp(saved,&bus,sizeof(bus)))return 10;
 s.pc=0x622;before=s;w=nba_setup_spc_resident_peek(&s);
 if(w.kind!=NBA_SPC_DSP||nba_setup_spc_resident_accept(&s,&bus)||memcmp(&before,&s,sizeof(s))||memcmp(saved,&bus,sizeof(bus)))return 11;
 /* A change between the two command reads must take CBNE without changing
  * status. The source polls again through timer service, not an echo helper. */
 bus.aram[0x44d]=0xe4;bus.aram[0x44e]=0xf4;bus.aram[0x44f]=0xf0;bus.aram[0x450]=0xf9;
 bus.aram[0x451]=0;bus.aram[0x452]=0;bus.aram[0x453]=0x2e;bus.aram[0x454]=0xf4;bus.aram[0x455]=0xf4;
 nba_setup_spc_resident_begin(&s,0x44d,0,0,0,255,0x49);bus.cpu_to_spc[0]=5;
 for(i=0;i<30&&!(s.pc==0x453&&s.phase==2);i++)if(!nba_setup_spc_resident_accept(&s,&bus))return 12;
 if(i==30)return 13;before=s;bus.cpu_to_spc[0]=6;
 for(i=0;i<5;i++)if(!nba_setup_spc_resident_accept(&s,&bus))return 14;
 if(s.pc!=0x44a||s.ps!=before.ps||s.a!=5||bus.spc_to_cpu[0]!=5)return 15;
 free(saved);puts("PASS 15 source/latch/boundary contracts");return 0;
}
int main(int argc,char **argv){
 NbaSetupSpcResident s;NbaSetupSpcResidentWork w;FILE *f;unsigned i;uint8_t value;
 if(argc==2&&!strcmp(argv[1],"--selftest"))return selftest();
 if(argc!=3||!load(argv[1]))return 2;
 memcpy(bus.aram,input+16,65536);memcpy(bus.cpu_to_spc,input+7,4);memcpy(bus.spc_to_cpu,input+11,4);bus.dsp_address=input[15];
 if(!nba_setup_spc_resident_begin(&s,(uint16_t)(input[0]|((uint16_t)input[1]<<8)),input[2],input[3],input[4],input[5],input[6]))return 3;
 for(i=0;i<10000;i++){
  if(!s.phase)state(&s,"instruction");w=nba_setup_spc_resident_peek(&s);
  if(w.kind>=NBA_SPC_TIMER){state(&s,"stop");break;}
  value=w.value;
  if(w.kind==NBA_SPC_READ)value=w.address>=0xf4&&w.address<=0xf7?bus.cpu_to_spc[w.address-0xf4]:w.address==0xf2?bus.dsp_address:bus.aram[w.address];
  if(!nba_setup_spc_resident_accept(&s,&bus))return 4;
  printf("{\"kind\":\"cycle\",\"pc\":%u,\"cycles\":%llu,\"bus\":%u,\"address\":%u,\"value\":%u,\"end\":%s}\n",w.pc,(unsigned long long)s.cycles,w.kind,w.address,value,w.instruction_end?"true":"false");
 }
 if(i==10000)return 5;
 input[0]=(uint8_t)s.pc;input[1]=(uint8_t)(s.pc>>8);input[2]=s.a;input[3]=s.x;input[4]=s.y;input[5]=s.sp;input[6]=s.ps;
 memcpy(input+7,bus.cpu_to_spc,4);memcpy(input+11,bus.spc_to_cpu,4);input[15]=bus.dsp_address;memcpy(input+16,bus.aram,65536);
 f=fopen(argv[2],"wb");if(!f)return 6;if(fwrite(input,1,sizeof(input),f)!=sizeof(input)){fclose(f);return 7;}return fclose(f)!=0;
}
