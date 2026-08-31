#define _CRT_SECURE_NO_WARNINGS
#include "nba_bootstrap.h"
#include "nba_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {FILE *trace;NbaBootstrap *state;const char *directory;bool failed;} Output;
static bool save(const char *dir,const char *name,const void *data,size_t size){
 char path[2048];FILE *f;int n=snprintf(path,sizeof(path),"%s/%s",dir,name);
 if(n<0||(size_t)n>=sizeof(path))return false;f=fopen(path,"wb");if(!f)return false;
 if(fwrite(data,1,size,f)!=size){fclose(f);return false;}return fclose(f)==0;
}
static bool state_file(Output *o,const char *name){
 char path[2048];FILE *f;unsigned i;NbaBootstrap *s=o->state;
 if(snprintf(path,sizeof(path),"%s/%s.state",o->directory,name)<0)return false;
 f=fopen(path,"wb");if(!f)return false;
 fprintf(f,"spc.a=%u\nspc.x=%u\nspc.y=%u\nspc.sp=%u\nspc.ps=%u\nspc.pc=%u\nspc.cycle=%llu\nspc.romEnabled=%s\nspc.writeEnabled=%s\nspc.dspReg=%u\n",
  s->spc.a,s->spc.x,s->spc.y,s->spc.sp,s->spc.ps,s->spc.pc,(unsigned long long)s->clock.spc_ticks,s->control.rom_enabled?"true":"false",s->control.aram_write_enabled?"true":"false",s->spc_bus.dsp_address);
 for(i=0;i<4;i++)fprintf(f,"spc.cpuRegs[%u]=%u\nspc.outputReg[%u]=%u\n",i,s->spc_bus.cpu_to_spc[i],i,s->spc_bus.spc_to_cpu[i]);
 for(i=0;i<3;i++){
  NbaSetupSpcControlTimer *t=&s->control.timer[i];
  fprintf(f,"spc.timer%u.stage0=%u\nspc.timer%u.stage1=%u\nspc.timer%u.prevStage1=%u\nspc.timer%u.stage2=%u\nspc.timer%u.output=%u\nspc.timer%u.target=%u\nspc.timer%u.enabled=%s\nspc.timer%u.timersEnabled=%s\n",
   i,t->stage0,i,t->stage1,i,t->previous_stage1,i,t->stage2,i,t->output,i,t->target,i,t->enabled?"true":"false",i,t->globally_enabled?"true":"false");
 }return fclose(f)==0;
}
static void observe(void *context,const NbaBootstrapEvent *e){
 Output *o=context;NbaBootstrap *s=o->state;
 if(e->kind==NBA_BOOT_EVENT_CPU_ENTRY){
  NbaCodecWorkEntry *r=&s->cpu.work.registers;
  if(fprintf(o->trace,"{\"kind\":5,\"master\":%llu,\"pc\":%u,\"cycles\":%llu,\"a\":%u,\"x\":%u,\"y\":%u,\"sp\":%u,\"ps\":%u,\"db\":%u,\"dp\":0,\"emulation\":%s}\n",
   (unsigned long long)e->master,e->pc,(unsigned long long)s->cpu_cycles,r->value,r->symbol,r->stream_cursor,r->stack_pointer,r->status,r->data_bank,s->cpu.emulation?"true":"false")<0)o->failed=true;
 }else if(e->kind==NBA_BOOT_EVENT_SPC_ENTRY){
  if(fprintf(o->trace,"{\"kind\":6,\"master\":%llu,\"pc\":%u,\"cycles\":%llu,\"a\":%u,\"x\":%u,\"y\":%u,\"sp\":%u,\"ps\":%u}\n",
   (unsigned long long)e->master,e->pc,(unsigned long long)e->spc_ticks,s->spc.a,s->spc.x,s->spc.y,s->spc.sp,s->spc.ps)<0)o->failed=true;
 }else if(fprintf(o->trace,"{\"kind\":%u,\"master\":%llu,\"sample_master\":%llu,\"spc\":%llu,\"pc\":%u,\"address\":%u,\"value\":%u,\"bus\":%u,\"end\":%s}\n",
    (unsigned)e->kind,(unsigned long long)e->master,(unsigned long long)e->sample_master,(unsigned long long)e->spc_ticks,e->pc,e->address,e->value,e->bus_kind,e->instruction_end?"true":"false")<0)o->failed=true;
 if(e->kind==NBA_BOOT_EVENT_RESIDENT_ENTRY||e->kind==NBA_BOOT_EVENT_F1){
  const char *name=e->kind==NBA_BOOT_EVENT_F1?"f1.aram":"entry.aram";
  if(!save(o->directory,name,s->spc_bus.aram,65536))o->failed=true;
  if(!state_file(o,e->kind==NBA_BOOT_EVENT_F1?"f1":"entry"))o->failed=true;
  printf("BOUNDARY kind=%u master=%llu ticks=%llu SPC=%04X A=%02X X=%02X Y=%02X SP=%02X P=%02X upload=%llu\n",
    (unsigned)e->kind,(unsigned long long)e->master,(unsigned long long)e->spc_ticks,s->spc.pc,s->spc.a,s->spc.x,s->spc.y,s->spc.sp,s->spc.ps,(unsigned long long)s->upload_writes);
 }
}
int main(int argc,char **argv){
 NbaRom rom={0};NbaBootstrap *s,*prior;Output out;char path[2048];unsigned long steps;
 if(argc!=3)return 2;if(!nba_rom_load_file(&rom,argv[1]))return 3;
 s=calloc(1,sizeof(*s));prior=malloc(sizeof(*s));if(!s||!prior)return 4;
 if(!nba_bootstrap_power_on(s,rom.data,rom.size,NBA_BOOT_PROFILE_NTSC_ZERO_32040))return 5;
 if(snprintf(path,sizeof(path),"%s/events.jsonl",argv[2])<0)return 6;
 out.trace=fopen(path,"wx");out.state=s;out.directory=argv[2];out.failed=false;if(!out.trace)return 7;
 for(steps=0;steps<2000000&&s->status==NBA_BOOT_RUNNING;steps++)(void)nba_bootstrap_step(s,observe,&out);
 if(out.failed||fclose(out.trace))return 8;
 *prior=*s;if(nba_bootstrap_step(s,observe,&out)||memcmp(prior,s,sizeof(*s)))return 9;
 if(!save(argv[2],"final.aram",s->spc_bus.aram,65536)||!save(argv[2],"final.wram",s->wram,sizeof(s->wram)))return 10;
 printf("{\"status\":%u,\"boundary_pc\":%u,\"master\":%llu,\"spc_ticks\":%llu,\"cpu_cycles\":%llu,\"spc_pc\":%u,\"spc_phase\":%u,\"upload\":%llu,\"resident\":%s,\"f1\":%s,\"refresh\":%llu}\n",
   (unsigned)s->status,s->boundary_pc,(unsigned long long)s->clock.master,(unsigned long long)s->clock.spc_ticks,(unsigned long long)s->cpu_cycles,s->spc.pc,s->spc.phase,
   (unsigned long long)s->upload_writes,s->resident?"true":"false",s->f1_completed?"true":"false",(unsigned long long)s->clock.refreshes);
 free(prior);free(s);nba_rom_free(&rom);return steps==2000000?11:0;
}
