#define _CRT_SECURE_NO_WARNINGS
#include "nba_bootstrap_tables.h"
#include "nba_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned assertions,bytes_checked;
#define CHECK(x) do{assertions++;if(!(x)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);return 1;}}while(0)
typedef struct { NbaBootstrapTables *owner; unsigned clear_writes,queue_writes,checkpoints; bool bad; } Watch;
static unsigned word(const uint8_t *m,unsigned i){return m[i]|((unsigned)m[i+1]<<8);}
static void observe(void *context,const NbaBootstrapEvent *e){
 Watch *w=context;unsigned i;
 if(e->kind==NBA_BOOT_EVENT_CPU&&e->bus_kind==NBA_CODEC_WRITE&&e->pc==0x8080c5){
  /*80:C5 indexes the shared low WRAM, including0100..02FF; not a private queue.*/
  if(w->clear_writes>=8192||e->address!=8191u-w->clear_writes||e->value)w->bad=true;
  if(e->address>=0x100&&e->address<0x300)w->queue_writes++;
  w->clear_writes++;
 }
 if(e->kind==NBA_BOOT_EVENT_CPU_ENTRY&&e->pc==0x8080ca){
  w->checkpoints++;
  for(i=0;i<8192;i++){bytes_checked++;if(w->owner->core.wram[i])w->bad=true;}
 }
 if(e->kind==NBA_BOOT_EVENT_CPU&&e->bus_kind==NBA_CODEC_WRITE&&e->address==0x4200&&e->value)w->bad=true;
}
int main(int argc,char **argv){
 NbaRom rom={0};NbaBootstrapTables *s,*prior;Watch watch;unsigned i,steps;uint8_t *m;
 static const unsigned fields[][2]={
  {0x62c,1},{0x7f2,2},{0x7fe,0},{0x8fe,0}, /*DA72..DA89*/
  {0x31,0},{0x33,0},{0x35,0},{0x37,0},{0x39,0}, /*8130..8136 and80C5*/
  {0x5cb,0},{0x5eb,0},{0x5ed,0},{0x5ef,0x1a0},{0x5f1,0},{0x5f9,2},
  {0x5e5,0x80},{0x5f3,0x2000},{0x5f5,0x2420},{0x5df,0x2000},
  {0x5e3,0x2000},{0x5e1,0x2220} /*AC89 changes the published buffer pair.*/
 };
 if(argc!=2||!nba_rom_load_file(&rom,argv[1]))return 2;
 s=malloc(sizeof(*s));prior=malloc(sizeof(*prior));if(!s||!prior)return 3;
 memset(s,0xa5,sizeof(*s));*prior=*s;
 CHECK(!nba_bootstrap_tables_power_on(s,rom.data,rom.size-1,NBA_BOOT_PROFILE_NTSC_ZERO_32040));
 CHECK(!memcmp(s,prior,sizeof(*s)));
 CHECK(!nba_bootstrap_tables_power_on(s,rom.data,rom.size,(NbaBootstrapProfile)0));
 CHECK(!memcmp(s,prior,sizeof(*s)));
 CHECK(nba_bootstrap_tables_power_on(s,rom.data,rom.size,NBA_BOOT_PROFILE_NTSC_ZERO_32040));
 watch.owner=s;watch.clear_writes=watch.queue_writes=watch.checkpoints=0;watch.bad=false;
 for(steps=0;steps<2000000&&s->core.status==NBA_BOOT_RUNNING;steps++)
  (void)nba_bootstrap_tables_step(s,observe,&watch);
 CHECK(!watch.bad&&watch.clear_writes==8192&&watch.queue_writes==512&&watch.checkpoints==1);
 CHECK(s->core.status==NBA_BOOT_CPU_SOURCE&&s->core.boundary_pc==0x808145);
 CHECK(s->core.io[0x2200]==0); /*8145 STA4200 has NOT executed.*/
 CHECK(s->core.resident&&s->core.f1_completed&&s->core.upload_writes==1264);
 CHECK(s->dma.transferred==65536&&!s->dma.pending);
 m=s->core.wram;
 for(i=0;i<sizeof(fields)/sizeof(fields[0]);i++)CHECK(word(m,fields[i][0])==fields[i][1]);
 for(i=0x100;i<0x300;i++){bytes_checked++;CHECK(m[i]==0);}
 CHECK(m[0x566]==1); /*ACBC's publication request; no NMI consumption yet.*/
 for(i=0;i<6;i++){
  CHECK(m[6+i]==rom.data[6+i]);CHECK(m[0x59e+i]==rom.data[6+i]);CHECK(m[0x8fee+i]==rom.data[6+i]);
 }
 for(i=0;i<16;i++)CHECK(m[0x3425+i]==rom.data[0x171f+i]); /*80F0*/
 for(i=0;i<3;i++)CHECK(m[0x5c2+3*i]==0x0c&&m[0x5c3+3*i]==0x80&&m[0x5c4+3*i]==0x80);
 /*AC13 clears 0x831+1 bytes, including BOTH bytes at index0830.*/
 for(i=0x2640;i<0x2e72;i++){bytes_checked++;CHECK(m[i]==255);}
 /*AC97's E100 words every four bytes; untouched upper halves stay power-on0.*/
 for(i=0x2000;i<0x2200;i+=4){CHECK(word(m,i)==0xe100);CHECK(word(m,i+2)==0);}
 CHECK(m[0x3363]==0x60&&m[0x33c3]==0&&m[0x3424]==0&&m[0x33c4]==255);
 /*ABA7 includes index0 before SBC producesF8; ABBA then replaces link0 byFF.
  * Keep this actual source order; no inferred allocator normalization.*/
 for(i=0;i<=0x68;i+=8){CHECK(m[0x3271+i]==(i==0x68?0:8));CHECK(m[0x32ea+i]==(i?i-8:255));}
 *prior=*s;CHECK(!nba_bootstrap_tables_step(s,observe,&watch)&&!memcmp(s,prior,sizeof(*s)));
 printf("PASS %u source/refusal assertions; %u explicitly checked bytes; ROM-only power-on, no state injection\n",assertions,bytes_checked);
 free(prior);free(s);nba_rom_free(&rom);return 0;
}

