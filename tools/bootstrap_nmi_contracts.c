/* Isolated hardware source contracts, not normal initialization fixtures.
 * Include the new private owner only to reach its unexported hardware actions;
 * the production ROM-only public API gains no state-injection entry point. */
#include "../src/nba_bootstrap_nmi.c"
#include <stdio.h>
#include <stdlib.h>

static unsigned checks;
#define CHECK(x) do { checks++; if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} }while(0)

int main(void){
 NbaBootstrapNmi *o=calloc(1,sizeof(*o));NbaBootstrap *s;
 unsigned flag,bus,h,delay,locked,i;uint8_t value;
 if(!o)return 2;s=&o->machine.core;
 /* Readable4210 status remains set in the reference's h2..5 protected window.
  * Revision2 and only bits4..6 of the external open bus enter this register.
  * Every byte is exercised, including bits the read must NOT preserve. */
 for(flag=0;flag<2;flag++)for(bus=0;bus<256;bus++)for(h=2;h<=6;h+=2){
  memset(o,0,sizeof(*o));o->nmi.flag=flag!=0;o->nmi.open_bus=(uint8_t)bus;
  s->clock.scanline=225;s->clock.hclock=(uint16_t)h;
  CHECK(cpu_read(s,0x804210,&value));
  CHECK(value==((flag?128:0)|2|(bus&0x70)));
  CHECK(o->nmi.flag==(flag!=0&&h<6));CHECK(o->nmi.open_bus==bus);
 }
 for(flag=0;flag<2;flag++)for(bus=0;bus<256;bus++){
  memset(o,0,sizeof(*o));o->nmi.irq_flag=flag!=0;o->nmi.open_bus=(uint8_t)bus;
  CHECK(cpu_read(s,0x4211,&value));CHECK(value==((flag?128:0)|(bus&127)));
  CHECK(!o->nmi.irq_flag&&o->nmi.open_bus==bus);
 }
 /* Delay is consumed once per CPU cycle, including DMA's interrupt lock. */
 for(delay=0;delay<=2;delay++)for(locked=0;locked<2;locked++){
  memset(o,0,sizeof(*o));o->nmi.delay=(uint8_t)delay;
  nmi_cpu_cycle(s,locked!=0);
  CHECK(o->nmi.need==(delay==1&&!locked));
  CHECK(o->nmi.delay==(delay==1?(locked?1:0):delay?delay-1:0));
 }
 /* Enabling during a readable NMI sets two-cycle delay once. Disable does
  * not retroactively erase a pending delay. These are source quirks retained. */
 memset(o,0,sizeof(*o));o->nmi.flag=true;o->nmi.auto_disabled=true;s->clock.master=1000;
 CHECK(control_write(s,128));CHECK(o->nmi.enabled&&o->nmi.delay==2);
 nmi_cpu_cycle(s,false);CHECK(o->nmi.delay==1&&!o->nmi.need);
 CHECK(control_write(s,128));CHECK(o->nmi.delay==1);
 CHECK(control_write(s,0));CHECK(!o->nmi.enabled&&o->nmi.delay==1);
 nmi_cpu_cycle(s,false);CHECK(o->nmi.need&&o->nmi.delay==0);
 /* Catch-up MUST see the old disabled state. At this point setting the new
  * enable flag first would try to latch a controller instead of skipping it. */
 memset(o,0,sizeof(*o));s->clock.master=1400;o->nmi.auto_start=1000;o->nmi.auto_next=1000;
 CHECK(control_write(s,1));CHECK(o->nmi.auto_enabled&&o->nmi.auto_disabled);
 CHECK(!o->nmi.controller_pending&&!o->nmi.auto_strobe&&o->nmi.auto_next==1512);
 /* Actual changed strobe in the first256-clock window must refuse BEFORE
  * publishing a fake controller latch or completing the4200 state write. */
 memset(o,0,sizeof(*o));s->clock.master=1100;o->nmi.auto_start=1000;o->nmi.auto_next=1000;
 CHECK(!control_write(s,1));CHECK(s->status==NBA_BOOT_HARDWARE&&s->boundary_pc==0x4016);
 CHECK(o->nmi.controller_pending&&o->nmi.requested_strobe&&!o->nmi.auto_enabled&&!o->nmi.auto_strobe);
 for(i=0;i<4;i++)CHECK(s->spc_bus.cpu_to_spc[i]==0&&s->spc_bus.spc_to_cpu[i]==0);
 /* H/V modes are explicit unowned effects, not silently ignored bits. */
 for(i=0x10;i<=0x30;i+=0x10){
  memset(o,0,sizeof(*o));CHECK(!control_write(s,(uint8_t)i));
  CHECK(s->status==NBA_BOOT_HARDWARE&&s->boundary_pc==0x4200&&!o->nmi.enabled);
 }
 /* Line transition flags and the h6 enable check are separate source events. */
 memset(o,0,sizeof(*o));s->clock.scanline=225;s->clock.hclock=2;nmi_tick(s);
 CHECK(o->nmi.flag&&!o->nmi.delay);s->clock.hclock=6;nmi_tick(s);CHECK(!o->nmi.delay);
 o->nmi.enabled=true;nmi_tick(s);CHECK(o->nmi.delay==1);
 s->clock.scanline=0;s->clock.hclock=2;nmi_tick(s);CHECK(!o->nmi.flag&&o->nmi.delay==1);
 /* The poll's next automatic serial action is refused without fabricating
  * a no-buttons read. Native no-input capture alone would not prove this. */
 memset(o,0,sizeof(*o));o->nmi.auto_enabled=true;o->nmi.auto_start=1000;o->nmi.auto_next=1384;s->clock.master=1400;
 CHECK(!auto_catch_up(s));CHECK(s->status==NBA_BOOT_HARDWARE&&s->boundary_pc==0x4016);
 free(o);printf("PASS %u isolated source hardware assertions; no normal-state or controller-response seeding\n",checks);return 0;
}
