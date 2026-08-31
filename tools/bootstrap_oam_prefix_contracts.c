/* Controlled component contracts. These states are never startup fixtures. */
#include "../src/nba_bootstrap_oam_prefix.c"
#include <stdio.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)

static void boundary(NbaBootstrapOamPrefix *o){
 NbaBootstrap *s=&o->prefix.machine.core;memset(o,0,sizeof(*o));
 s->status=NBA_BOOT_CPU_SOURCE;s->boundary_pc=s->cpu.boundary_pc=0x808184;
 s->cpu.program_bank=0x80;s->cpu.emulation=false;s->cpu.work.registers.status=0x20;
}
int main(void){
 NbaBootstrapOamPrefix o;NbaBootstrap *s=&o.prefix.machine.core;unsigned value,low,i;
 volatile unsigned cycle_count=(unsigned)(sizeof(oam_cycles)/sizeof(oam_cycles[0]));uint8_t keep;
 boundary(&o);CHECK(resume_prefix(&o));CHECK(o.continuation_active&&s->status==NBA_BOOT_RUNNING);
 for(i=0;i<7;i++){
  boundary(&o);
  if(i==0)s->boundary_pc++;
  if(i==1)s->cpu.boundary_pc++;
  if(i==2)s->cpu.program_bank=0;
  if(i==3)s->cpu.emulation=true;
  if(i==4)s->cpu.work.registers.status=0;
  if(i==5){s->cpu.work.pending_count=1;s->cpu.work.pending_index=0;}
  if(i==6)s->status=NBA_BOOT_HARDWARE;
  CHECK(!resume_prefix(&o));CHECK(!o.continuation_active);
 }
 for(low=0;low<256;low+=17)for(value=0;value<256;value++){
  memset(&o,0,sizeof(o));s=&o.prefix.machine.core;s->status=NBA_BOOT_RUNNING;s->io[0x102]=(uint8_t)low;
  CHECK(oam_write(&o,(uint8_t)value));
  CHECK(s->io[0x103]==value);
  CHECK(o.oam_ram_address==(uint16_t)(low|((value&1u)<<8)));
  CHECK(o.oam_internal_address==(uint16_t)(o.oam_ram_address<<1));
  CHECK(o.oam_priority_rotation==((value&0x80u)!=0));
 }
 for(value=0;value<256;value++){
  memset(&o,0,sizeof(o));s=&o.prefix.machine.core;s->cpu.work.registers.value=(uint16_t)(0x5a00|value);
  keep=(uint8_t)(0x7d&~0x82u);s->cpu.work.registers.status=0x7d;o.loaded_value=(uint8_t)value;o.source_stage=1;
  complete_instruction(&o,0x808188);
  CHECK(s->cpu.work.registers.value==(uint16_t)(0x5a00|value));
  CHECK((s->cpu.work.registers.status&~0x82u)==keep);
  CHECK(((s->cpu.work.registers.status&2u)!=0)==(value==0));
  CHECK(((s->cpu.work.registers.status&0x80u)!=0)==((value&0x80u)!=0));
 }
 CHECK(cycle_count==12);
 for(i=0;i<12;i++)CHECK(oam_cycles[i].address!=0x213e);
 CHECK(oam_cycles[4].address==0x2103&&oam_cycles[4].kind==NBA_CODEC_WRITE&&oam_cycles[4].last);
 CHECK(oam_cycles[9].address==0x8fe&&oam_cycles[9].source_read&&oam_cycles[9].last);
 printf("PASS %u isolated OAM-prefix assertions; no normal-state, PPU-status, controller or DSP seeding\n",checks);return 0;
}
