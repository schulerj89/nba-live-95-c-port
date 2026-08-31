#define _CRT_SECURE_NO_WARNINGS
#include "nba_bootstrap_internal.h"
#include "nba_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"contract failed line%u\n",(unsigned)__LINE__);return 1;}}while(0)
int main(int argc,char **argv){
 NbaRom rom={0};NbaBootstrap *s,*old;NbaSetupSpcControlTimer t;unsigned i,rate;
 if(argc!=2||!nba_rom_load_file(&rom,argv[1]))return 2;
 s=malloc(sizeof(*s));old=malloc(sizeof(*s));if(!s||!old)return 3;
 memset(s,0xa5,sizeof(*s));*old=*s;
 CHECK(!nba_bootstrap_power_on(s,rom.data,rom.size,(NbaBootstrapProfile)0));CHECK(!memcmp(s,old,sizeof(*s)));
 CHECK(!nba_bootstrap_power_on(s,rom.data,rom.size-1,NBA_BOOT_PROFILE_NTSC_ZERO_32040));CHECK(!memcmp(s,old,sizeof(*s)));
 CHECK(!nba_bootstrap_power_on(0,rom.data,rom.size,NBA_BOOT_PROFILE_NTSC_ZERO_32040));
 CHECK(!nba_bootstrap_power_on(s,0,rom.size,NBA_BOOT_PROFILE_NTSC_ZERO_32040));
 CHECK(nba_bootstrap_power_on(s,rom.data,rom.size,NBA_BOOT_PROFILE_NTSC_ZERO_32040));
 CHECK(s->clock.master==186&&s->clock.spc_ticks==4&&s->spc.pc==0xffc0&&!s->resident&&!s->f1_completed);
 for(i=0;i<65536;i++)CHECK(s->spc_bus.aram[i]==0); /* No hidden uploaded image. */
 CHECK(s->cpu.work.registers.stack_pointer==0x1ff&&s->cpu.work.registers.status==0x34&&s->cpu.emulation);
 for(rate=16;rate<=128;rate+=112){
  memset(&t,0,sizeof(t));t.globally_enabled=true;t.output=15;
  for(i=0;i<10000;i++)nba_bootstrap_timer_step(&t,(uint8_t)rate);
  CHECK(t.output==15&&t.stage2==0&&t.stage0==(20000u%rate)&&t.stage1==((20000u/rate)&1));
  memset(&t,0,sizeof(t));t.globally_enabled=true;t.enabled=true;t.target=3;
  for(i=0;i<rate*3;i++)nba_bootstrap_timer_step(&t,(uint8_t)rate);
  CHECK(t.output==1&&t.stage2==0&&t.stage0==0&&t.stage1==0); /* 3 complete falling edges */
  memset(&t,0,sizeof(t));t.globally_enabled=true;t.enabled=true;t.target=0;
  for(i=0;i<rate*256;i++)nba_bootstrap_timer_step(&t,(uint8_t)rate);
  CHECK(t.output==1&&t.stage2==0); /* target0 counts256 falling edges */
 }
 for(i=0;i<1000000&&s->status==NBA_BOOT_RUNNING;i++)(void)nba_bootstrap_step(s,0,0);
 CHECK(s->status==NBA_BOOT_CPU_SOURCE&&s->boundary_pc==0x8080bc&&s->f1_completed&&s->upload_writes==1264);
 *old=*s;CHECK(!nba_bootstrap_step(s,0,0));CHECK(!memcmp(s,old,sizeof(*s)));
 printf("PASS %u bootstrap contracts\n",checks);free(s);free(old);nba_rom_free(&rom);return 0;
}
