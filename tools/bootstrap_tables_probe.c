#define _CRT_SECURE_NO_WARNINGS
#include "nba_bootstrap_tables.h"
#include "nba_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {FILE *trace;NbaBootstrap *state;NbaBootstrapTables *owner;const char *directory;bool failed;} Output;
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
static bool final_state(const char *directory,const NbaBootstrapTables *owner){
 char path[2048];FILE *f;const NbaBootstrap *s=&owner->core;const NbaCodecWorkEntry *r=&s->cpu.work.registers;
 if(snprintf(path,sizeof(path),"%s/final.state",directory)<0)return false;
 f=fopen(path,"wb");if(!f)return false;
 fprintf(f,"masterClock=%llu\ncpu.cycleCount=%llu\ncpu.a=%u\ncpu.x=%u\ncpu.y=%u\ncpu.sp=%u\ncpu.ps=%u\ncpu.k=%u\ncpu.pc=%u\ncpu.dbr=%u\ncpu.d=0\ncpu.emulationMode=%s\n",
  (unsigned long long)s->clock.master,(unsigned long long)s->cpu_cycles,r->value,r->symbol,r->stream_cursor,r->stack_pointer,r->status,s->cpu.program_bank,s->boundary_pc&65535u,r->data_bank,s->cpu.emulation?"true":"false");
 fprintf(f,"ppu.vramAddress=%u\nppu.vramReadBuffer=%u\ndmaController.channel[1].srcAddress=%u\ndmaController.channel[1].srcBank=%u\ndmaController.channel[1].transferSize=%u\ndmaController.channel[1].transferMode=%u\ndmaController.channel[1].fixedTransfer=%s\ndmaController.channel[1].invertDirection=%s\ndmaController.channel[1].decrement=%s\ndmaController.channel[1].dmaActive=%s\n",
  owner->vram_address,owner->vram_read_buffer,s->io[0x2312]|((unsigned)s->io[0x2313]<<8),s->io[0x2314],s->io[0x2315]|((unsigned)s->io[0x2316]<<8),s->io[0x2310]&7,
  s->io[0x2310]&8?"true":"false",s->io[0x2310]&128?"true":"false",s->io[0x2310]&16?"true":"false",owner->dma.pending?"true":"false");
 return fclose(f)==0;
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
 if(e->kind==NBA_BOOT_EVENT_CPU_ENTRY){
  const char *name=NULL;
  switch(e->pc){case 0x8080c0:name="cpu_fill_return.wram";break;case 0x8080ca:name="cpu_clear_after.wram";break;
   case 0x8080fd:name="cpu_table_after.wram";break;case 0x808101:name="cpu_da72_return.wram";break;
   case 0x808141:name="cpu_ab7e_return.wram";break;default:break;}
  if(name&&!save(o->directory,name,s->wram,sizeof(s->wram)))o->failed=true;
 }
 if(e->kind==NBA_BOOT_EVENT_CPU_ENTRY&&e->pc==0x8080bc){if(!save(o->directory,"cpu_80bc.wram",s->wram,sizeof(s->wram)))o->failed=true;}
 if(e->kind==NBA_BOOT_EVENT_RESIDENT_ENTRY||e->kind==NBA_BOOT_EVENT_F1){
  const char *name=e->kind==NBA_BOOT_EVENT_F1?"f1.aram":"entry.aram";
  if(!save(o->directory,name,s->spc_bus.aram,65536))o->failed=true;
  if(!state_file(o,e->kind==NBA_BOOT_EVENT_F1?"f1":"entry"))o->failed=true;
  printf("BOUNDARY kind=%u master=%llu ticks=%llu SPC=%04X A=%02X X=%02X Y=%02X SP=%02X P=%02X upload=%llu\n",
    (unsigned)e->kind,(unsigned long long)e->master,(unsigned long long)e->spc_ticks,s->spc.pc,s->spc.a,s->spc.x,s->spc.y,s->spc.sp,s->spc.ps,(unsigned long long)s->upload_writes);
 }
}
int main(int argc,char **argv){
 NbaRom rom={0};NbaBootstrap *s;NbaBootstrapTables *owner,*prior;Output out;char path[2048];unsigned long steps;
 if(argc!=3)return 2;if(!nba_rom_load_file(&rom,argv[1]))return 3;
 owner=calloc(1,sizeof(*owner));prior=malloc(sizeof(*owner));if(!owner||!prior)return 4;s=&owner->core;
 if(!nba_bootstrap_tables_power_on(owner,rom.data,rom.size,NBA_BOOT_PROFILE_NTSC_ZERO_32040))return 5;
 if(snprintf(path,sizeof(path),"%s/events.jsonl",argv[2])<0)return 6;
 out.trace=fopen(path,"wx");out.state=s;out.owner=owner;out.directory=argv[2];out.failed=false;if(!out.trace)return 7;
 for(steps=0;steps<2000000&&s->status==NBA_BOOT_RUNNING;steps++)(void)nba_bootstrap_tables_step(owner,observe,&out);
 if(out.failed||fclose(out.trace))return 8;
 *prior=*owner;if(nba_bootstrap_tables_step(owner,observe,&out)||memcmp(prior,owner,sizeof(*owner)))return 9;
 if(!save(argv[2],"final.aram",s->spc_bus.aram,65536)||!save(argv[2],"final.wram",s->wram,sizeof(s->wram)))return 10;
 if(!save(argv[2],"final.vram",owner->vram,sizeof(owner->vram))||!final_state(argv[2],owner))return 12;
 printf("{\"status\":%u,\"boundary_pc\":%u,\"master\":%llu,\"spc_ticks\":%llu,\"cpu_cycles\":%llu,\"spc_pc\":%u,\"spc_phase\":%u,\"upload\":%llu,\"resident\":%s,\"f1\":%s,\"refresh\":%llu,",
   (unsigned)s->status,s->boundary_pc,(unsigned long long)s->clock.master,(unsigned long long)s->clock.spc_ticks,(unsigned long long)s->cpu_cycles,s->spc.pc,s->spc.phase,
   (unsigned long long)s->upload_writes,s->resident?"true":"false",s->f1_completed?"true":"false",(unsigned long long)s->clock.refreshes);
 printf("\"dma_bytes\":%u,\"vram_address\":%u,\"sync_counter\":%u,\"source_index\":%u}\n",owner->dma.transferred,owner->vram_address,owner->dma.clock_counter,owner->dma.source_index);
 free(prior);free(owner);nba_rom_free(&rom);return steps==2000000?11:0;
}
