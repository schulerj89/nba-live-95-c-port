#include "nba_bootstrap_fill.h"
#include <string.h>

/* Pinned Mesen reference, b9fa69...: SpcTimer::Run/ClockTimer, normal speed2.
 * Timer output is stored as a byte; nibble masking belongs to the read owner. */
static void fill_timer_step(NbaSetupSpcControlTimer *t,uint8_t rate){
 uint8_t previous,current;
 t->stage0=(uint8_t)(t->stage0+2u);
 if(t->stage0<rate)return;
 t->stage0=(uint8_t)(t->stage0-rate);t->stage1^=1;
 previous=t->previous_stage1;current=t->globally_enabled?t->stage1:0;
 t->previous_stage1=current;
 if(!t->enabled||!previous||current)return;
 t->stage2++;
 if(t->stage2==t->target){t->stage2=0;t->output++;}
}
static void event(NbaBootstrap *s,NbaBootstrapObserver f,void *ctx,
                  NbaBootstrapEventKind kind,uint32_t pc,uint32_t address,
                  uint8_t value,uint8_t bus_kind,bool end,uint64_t sample){
 NbaBootstrapEvent e;
 if(!f)return;e.kind=kind;e.master=s->clock.master;e.spc_ticks=s->clock.spc_ticks;
 e.sample_master=sample;e.pc=pc;e.address=address;e.value=value;e.bus_kind=bus_kind;e.instruction_end=end;f(ctx,&e);
}
static bool stop(NbaBootstrap *s,NbaBootstrapStatus status,uint32_t pc){s->status=status;s->boundary_pc=pc;return false;}
static uint8_t spc_read(const NbaBootstrap *s,uint16_t address){
 if(nba_setup_spc_control_ipl_visible(&s->control,address))return nba_bootstrap_ipl_byte(address);
 if(address==0xf1)return 0;
 if(address==0xf2)return s->spc_bus.dsp_address;
 if(address>=0xf4&&address<=0xf7)return s->spc_bus.cpu_to_spc[address-0xf4];
 return s->spc_bus.aram[address];
}
static bool spc_run(NbaBootstrap *s,NbaBootstrapObserver f,void *ctx){
 /* Spc::Run target=floor(master*(32000+default40)*64/21477270)-1.
  * Exact integer rational for this explicit profile; no measured ratio input. */
 uint64_t target=(s->clock.master*2050560u)/21477270u;
 while(target>s->clock.spc_ticks+1u){
  NbaSetupSpcInitWork q;
  NbaSetupSpcResidentWork w;uint8_t v=0;unsigned i;bool ok;
  if(s->resident){q=nba_setup_spc_init_peek(&s->spc);w=q.bus;}
  else {memset(&q,0,sizeof(q));w=nba_bootstrap_ipl_peek(&s->spc);}
  if(w.kind==NBA_SPC_DSP)return stop(s,NBA_BOOT_DSP_READ,w.pc);
  if(w.kind>=NBA_SPC_TIMER)return stop(s,NBA_BOOT_SPC_SOURCE,w.pc);
  if(w.kind==NBA_SPC_FETCH&&spc_read(s,w.address)!=w.value)return stop(s,NBA_BOOT_BAD_SOURCE,w.pc);
  if(!s->spc.phase)event(s,f,ctx,NBA_BOOT_EVENT_SPC_ENTRY,w.pc,0,0,0,false,s->clock.master);
  /* Source IncCycleCount executes DSP then timers BEFORE memory effect.
   * Before first F3 access, reset FLG E0 prevents both echo RAM writes
   * (Dsp::EchoStep29/30); no CPU-visible DSP value is supplied here. Keep
   * elapsed unresolved DSP history explicit, never pretend a full DSP state. */
  s->clock.spc_ticks+=2;s->clock.spc_steps++;s->clock.dsp_unresolved_steps++;
  for(i=0;i<3;i++)fill_timer_step(&s->control.timer[i],i==2?16:128);
  if(w.kind==NBA_SPC_FETCH||w.kind==NBA_SPC_READ)v=spc_read(s,w.address);
  else if(w.kind==NBA_SPC_WRITE)v=w.value;
  event(s,f,ctx,NBA_BOOT_EVENT_SPC,w.pc,w.address,v,(uint8_t)w.kind,w.instruction_end,s->clock.master);
  if(s->resident&&q.control_publication){
   if(w.pc!=0x384||s->spc.phase!=4||w.address!=0xf1||w.value!=0x30)return stop(s,NBA_BOOT_BAD_SOURCE,w.pc);
   ok=nba_setup_spc_control_commit(&s->control,&s->spc_bus,w.value);
   /* Commit the actual source MOV's last cycle; do not call begin0387,
    * reset counters or clear its refusal flag to forge acceptance. */
   if(ok){s->spc.cycles++;s->spc.instructions++;s->spc.pc=0x387;s->spc.phase=0;s->f1_completed=true;
    event(s,f,ctx,NBA_BOOT_EVENT_F1,0x384,0xf1,0x30,NBA_SPC_WRITE,true,s->clock.master);}
  }else if(s->resident){ok=nba_setup_spc_init_accept(&s->spc,&s->spc_bus);}
  else {
   if(w.kind==NBA_SPC_WRITE){
    s->spc_bus.aram[w.address]=w.value;
    if(w.address>=0xf4&&w.address<=0xf7)s->spc_bus.spc_to_cpu[w.address-0xf4]=w.value;
    if(w.pc==0xffe2)s->upload_writes++;
   }
   ok=nba_bootstrap_ipl_accept(&s->spc,v);
   if(ok&&s->spc.pc==0x380&&s->spc.phase==0){s->resident=true;
    event(s,f,ctx,NBA_BOOT_EVENT_RESIDENT_ENTRY,0x380,0,0,NBA_SPC_IDLE,false,s->clock.master);}
  }
  if(!ok)return stop(s,NBA_BOOT_BAD_SOURCE,w.pc);
  /* Spc::ProcessCycle publishes ALL staged inputs only AFTER that cycle's
   * read/write and semantics. F1 can zero staged bytes without clearing flag. */
  if(s->control.pending_cpu_input_update){
   memcpy(s->spc_bus.cpu_to_spc,s->control.staged_cpu_input,4);
   s->control.pending_cpu_input_update=false;
  }
 }
 return true;
}
static bool tick2(NbaBootstrap *s,NbaBootstrapObserver f,void *ctx){
 unsigned i;s->clock.master+=2;s->clock.hclock+=2;
 if(s->clock.hclock==s->clock.refresh_position){
  s->clock.refreshes++;
  /* SnesMemoryManager refresh blocks CPU for20 two-master ticks. */
  for(i=0;i<20;i++)if(!tick2(s,f,ctx))return false;
 }
 if(s->clock.hclock>=1364 || (s->clock.hclock==1360&&s->clock.scanline==240&&s->clock.odd_frame)){
  s->clock.hclock=0;s->clock.scanline++;
  if(s->clock.scanline==262){s->clock.scanline=0;s->clock.odd_frame=!s->clock.odd_frame;}
  s->clock.refresh_position=(uint16_t)(538u-(s->clock.master&7u));
 }
 return spc_run(s,f,ctx);
}
static bool clocks(NbaBootstrap *s,unsigned count,NbaBootstrapObserver f,void *ctx){
 unsigned i;for(i=0;i<count;i+=2)if(!tick2(s,f,ctx))return false;return true;
}
static unsigned speed(const NbaBootstrap *s,uint32_t address){
 unsigned bank=address>>16,lo=address&65535u;
 if(bank&0x40)return bank>=0xc0&&s->fast_rom?6:8;
 if(lo>=0x8000)return bank>=0x80&&s->fast_rom?6:8;
 if(lo<0x2000||lo>=0x6000)return 8;
 if(lo>=0x4000&&lo<0x4200)return 12;return 6;
}
static bool cpu_read(NbaBootstrap *s,uint32_t address,uint8_t *v){
 unsigned bank=address>>16,lo=address&65535u;
 if(bank==0x7e||bank==0x7f){*v=s->wram[address-0x7e0000u];return true;}
 if(!(bank&0x40)&&lo<0x2000){*v=s->wram[lo];return true;}
 if(!(bank&0x40)&&lo>=0x2140&&lo<=0x217f){*v=s->spc_bus.spc_to_cpu[lo&3];return true;}
 if(lo>=0x8000){size_t offset=((size_t)(bank&0x7f)<<15)|(lo&0x7fff);if(offset<s->rom_size){*v=s->rom[offset];return true;}}
 return stop(s,NBA_BOOT_HARDWARE,address);
}
static bool plain_cpu_write(NbaBootstrap *s,uint32_t address,uint8_t v){
 unsigned bank=address>>16,lo=address&65535u;
 if(bank==0x7e||bank==0x7f){s->wram[address-0x7e0000u]=v;return true;}
 if(!(bank&0x40)&&lo<0x2000){s->wram[lo]=v;return true;}
 if(!(bank&0x40)&&lo>=0x2140&&lo<=0x217f){
  unsigned port=lo&3;
  if(s->control.staged_cpu_input[port]!=v){
   uint64_t scaled=s->clock.master*2050560u;
   s->control.staged_cpu_input[port]=v;
   /* Real CPU write adapter: catch-up happened at write access, now choose
    * first-half immediate visibility or post-next-cycle staged publication. */
   if(scaled<=((s->clock.spc_ticks+1u)*21477270u))s->spc_bus.cpu_to_spc[port]=v;
   else s->control.pending_cpu_input_update=true;
  }return true;
 }
 if(!(bank&0x40)&&lo>=0x2000&&lo<=0x43ff){
  /* This S1 source path writes reset PPU/control registers only. Active
   * DMA/NMI and later PPU consumers require dedicated owners, never no-ops. */
  if((lo==0x420b||lo==0x420c||lo==0x4200)&&v)return stop(s,NBA_BOOT_HARDWARE,address);
  s->io[lo-0x2000]=v;if(lo==0x420d)s->fast_rom=(v&1)!=0;return true;
 }
 return stop(s,NBA_BOOT_HARDWARE,address);
}

static uint16_t io_word(const NbaBootstrap *s,unsigned address){
 return (uint16_t)(s->io[address-0x2000]|((uint16_t)s->io[address-0x1fff]<<8));
}
static void io_word_store(NbaBootstrap *s,unsigned address,uint16_t value){
 s->io[address-0x2000]=(uint8_t)value;s->io[address-0x1fff]=(uint8_t)(value>>8);
}
static bool fill_cpu_write(NbaBootstrapFill *owner,uint32_t address,uint8_t value){
 NbaBootstrap *s=&owner->core;unsigned bank=address>>16,lo=address&65535u;
 if(!(bank&0x40)&&lo==0x420b&&value){
  /* Only the source-produced first reset fill is owned here. Other channels,
   * transfer modes, queued helper branch8A93 or HDMA require a new owner. */
  if(value!=2||s->cpu.work.pending[0].cycle.source_pc!=0x808a8d||
     s->io[0x2310]!=9||s->io[0x2311]!=0x18||s->io[0x2314]!=0||
     io_word(s,0x4312)!=0x16||io_word(s,0x4315)!=0||
     owner->vram_control!=0x80||!(s->io[0x100]&0x80)||owner->dma.pending)
   return stop(s,NBA_BOOT_HARDWARE,address);
  owner->dma.pending=true;owner->dma.start_delay=true;s->io[0x220b]=value;return true;
 }
 if(!plain_cpu_write(s,address,value))return false;
 if(!(bank&0x40)){
  if(lo==0x2115)owner->vram_control=value;
  if(lo==0x2116||lo==0x2117){
   if(lo==0x2116)owner->vram_address=(uint16_t)((owner->vram_address&0x7f00)|value);
   else owner->vram_address=(uint16_t)((owner->vram_address&255)|((uint16_t)(value&127)<<8));
   /* The normal reset writes these while forced blank; address remapping is
    * zero. Preserve the PPU read buffer ownership, despite an all-zero fill. */
   if(owner->vram_control&12)return stop(s,NBA_BOOT_HARDWARE,address);
   owner->vram_read_buffer=(s->io[0x100]&0x80)?
    (uint16_t)(owner->vram[owner->vram_address*2u]|((uint16_t)owner->vram[owner->vram_address*2u+1u]<<8)):0;
  }
 }
 return true;
}
static bool dma_step(NbaBootstrapFill *owner,const NbaCodecBusCycle *suspended,
                    NbaBootstrapObserver observer,void *context){
 NbaBootstrap *s=&owner->core;unsigned clocks_due;uint16_t remaining,address;
 switch(owner->dma.phase){
 case NBA_BOOT_FILL_SYNC:
  owner->dma.clock_counter=8u-(unsigned)(s->clock.master&7u);
  if(!clocks(s,owner->dma.clock_counter,observer,context))return false;
  owner->dma.phase=NBA_BOOT_FILL_GLOBAL;return true;
 case NBA_BOOT_FILL_GLOBAL:
  if(!clocks(s,8,observer,context))return false;
  owner->dma.clock_counter+=8;owner->dma.phase=NBA_BOOT_FILL_CHANNEL;return true;
 case NBA_BOOT_FILL_CHANNEL:
  if(!clocks(s,8,observer,context))return false;
  owner->dma.clock_counter+=8;owner->dma.phase=NBA_BOOT_FILL_READ;return true;
 case NBA_BOOT_FILL_READ:
  /* Mode09 is FIXED source. Both low/high VRAM writes read0016; the source
   * does not alternate0016/0017 even though8A69 stored a word there. */
  address=io_word(s,0x4312);
  if(!clocks(s,4,observer,context)||!cpu_read(s,address,&owner->dma.value))return false;
  event(s,observer,context,NBA_BOOT_FILL_DMA_EVENT,suspended->source_pc,address,
        owner->dma.value,NBA_CODEC_READ,false,s->clock.master);
  owner->dma.phase=NBA_BOOT_FILL_WRITE;return true;
 case NBA_BOOT_FILL_WRITE:
  address=(uint16_t)(0x2118u+(owner->dma.source_index&1u));
  if(!clocks(s,4,observer,context))return false;
  event(s,observer,context,NBA_BOOT_FILL_DMA_EVENT,suspended->source_pc,address,
        owner->dma.value,NBA_CODEC_WRITE,false,s->clock.master);
  owner->vram[owner->vram_address*2u+(address&1u)]=owner->dma.value;
  if(address==0x2119)owner->vram_address=(uint16_t)((owner->vram_address+1u)&0x7fff);
  remaining=(uint16_t)(io_word(s,0x4315)-1u);io_word_store(s,0x4315,remaining);
  owner->dma.source_index++;owner->dma.transferred++;
  owner->dma.phase=remaining?NBA_BOOT_FILL_READ:NBA_BOOT_FILL_FINISH;return true;
 case NBA_BOOT_FILL_FINISH:
  /* Pinned reference RunDma uses uint8_t i, including its wrap at256, when
   * adding8*i to the synchronization counter. Preserve that SOFTWARE profile
   * here; this is not documented as an original NBA95 game bug or HW law. */
  owner->dma.clock_counter+=8u*owner->dma.source_index;
  clocks_due=owner->dma.cpu_speed-(owner->dma.clock_counter%owner->dma.cpu_speed);
  if(!clocks(s,clocks_due,observer,context))return false;
  event(s,observer,context,NBA_BOOT_FILL_DMA_END_EVENT,suspended->source_pc,0,0,NBA_CODEC_IDLE,false,s->clock.master);
  owner->dma.pending=false;owner->dma.phase=NBA_BOOT_FILL_NONE;return true;
 default:return stop(s,NBA_BOOT_HARDWARE,suspended->source_pc);
 }
}
bool nba_bootstrap_fill_power_on(NbaBootstrapFill *owner,const uint8_t *rom,
                               size_t size,NbaBootstrapProfile profile){
 unsigned channel,index;
 if(!owner||!rom||size!=1572864||profile!=NBA_BOOT_PROFILE_NTSC_ZERO_32040||!nba_bootstrap_rom_valid(rom,size))return false;
 memset(owner,0,sizeof(*owner));
 if(!nba_bootstrap_power_on(&owner->core,rom,size,profile)||!nba_bootstrap_fill_cpu_power_on(&owner->core.cpu))return false;
 /* SnesDmaController constructor writes FF to43x0..43xB, not zero. These
  * registers are later overwritten by actual reset source, not fixture seeds. */
 for(channel=0;channel<8;channel++)for(index=0;index<=11;index++)owner->core.io[0x2300+channel*16+index]=255;
 return true;
}
bool nba_bootstrap_fill_step(NbaBootstrapFill *owner,NbaBootstrapObserver observer,void *context){
 NbaBootstrap *s;NbaCodecBusCycle w;uint8_t value=0;unsigned duration;uint64_t sample;
 if(!owner)return false;s=&owner->core;if(s->status!=NBA_BOOT_RUNNING)return false;
 if(!nba_bootstrap_fill_cpu_peek(&s->cpu,&w))return stop(s,NBA_BOOT_CPU_SOURCE,s->cpu.boundary_pc);
 duration=w.kind==NBA_CODEC_IDLE?6:speed(s,w.address);
 if(owner->dma.phase!=NBA_BOOT_FILL_NONE)return dma_step(owner,&w,observer,context);
 if(!s->cpu.work.pending_index)event(s,observer,context,NBA_BOOT_EVENT_CPU_ENTRY,w.source_pc,0,0,0,false,s->clock.master);
 /* ProcessCpuCycle delays DMA start for one real CPU cycle after420B. The
  * following memory access is suspended until DMA synchronizes and finishes. */
 if(owner->dma.pending){
  if(owner->dma.start_delay)owner->dma.start_delay=false;
  else{owner->dma.phase=NBA_BOOT_FILL_SYNC;owner->dma.cpu_speed=(uint8_t)duration;return dma_step(owner,&w,observer,context);}
 }
 if(w.kind==NBA_CODEC_READ){
  if(!clocks(s,duration-4,observer,context)||!cpu_read(s,w.address,&value))return false;
  sample=s->clock.master;if(!clocks(s,4,observer,context))return false;
  event(s,observer,context,NBA_BOOT_EVENT_CPU,w.source_pc,w.address,value,(uint8_t)w.kind,w.instruction_end,sample);
 }else{
  if(!clocks(s,duration,observer,context))return false;
  value=w.value;if(w.kind==NBA_CODEC_WRITE&&!fill_cpu_write(owner,w.address,value))return false;
  event(s,observer,context,NBA_BOOT_EVENT_CPU,w.source_pc,w.address,value,(uint8_t)w.kind,w.instruction_end,s->clock.master);
 }
 s->cpu_cycles++;
 if(!nba_bootstrap_fill_cpu_accept(&s->cpu,value))return stop(s,NBA_BOOT_BAD_SOURCE,w.source_pc);
 return true;
}
