#define _CRT_SECURE_NO_WARNINGS
#include "nba_bootstrap_fill.h"
#include "nba_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned assertions,bytes_checked;
#define CHECK(x) do{assertions++;if(!(x)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
 NbaRom rom={0};NbaBootstrapFill *s,*prior;unsigned steps,j,k;
 static const uint8_t patterns[]={0,0x5a,0x80,0xff};
 if(argc!=2||!nba_rom_load_file(&rom,argv[1]))return 2;
 s=malloc(sizeof(*s));prior=malloc(sizeof(*prior));if(!s||!prior)return 3;
 memset(s,0xa5,sizeof(*s));*prior=*s;
 CHECK(!nba_bootstrap_fill_power_on(s,rom.data,rom.size-1,NBA_BOOT_PROFILE_NTSC_ZERO_32040));
 CHECK(!memcmp(s,prior,sizeof(*s)));
 CHECK(!nba_bootstrap_fill_power_on(s,rom.data,rom.size,(NbaBootstrapProfile)0));
 CHECK(!memcmp(s,prior,sizeof(*s)));
 for(k=0;k<sizeof(patterns);k++){
  CHECK(nba_bootstrap_fill_power_on(s,rom.data,rom.size,NBA_BOOT_PROFILE_NTSC_ZERO_32040));
  for(steps=0;steps<2000000&&s->core.status==NBA_BOOT_RUNNING&&s->dma.phase!=NBA_BOOT_FILL_READ;steps++)
   (void)nba_bootstrap_fill_step(s,NULL,NULL);
  CHECK(s->dma.phase==NBA_BOOT_FILL_READ&&s->dma.transferred==0&&s->dma.pending);
  CHECK(s->core.wram[0x16]==0&&s->core.wram[0x17]==0);
  /* ISOLATED hardware tests after real normal source reaches DMA. Pattern
   * injection is confined to these tests, never bootstrap initialization or
   * a native differential. Differing16/17 proves fixed-source, not word fill. */
  if(k){s->core.wram[0x16]=patterns[k];s->core.wram[0x17]=(uint8_t)~patterns[k];memset(s->vram,0xa5,sizeof(s->vram));}
  for(steps=0;steps<2000000&&s->core.status==NBA_BOOT_RUNNING;steps++)
   (void)nba_bootstrap_fill_step(s,NULL,NULL);
  CHECK(s->core.status==NBA_BOOT_CPU_SOURCE&&s->core.boundary_pc==0x8080c0);
  CHECK(s->dma.transferred==65536&&s->dma.phase==NBA_BOOT_FILL_NONE&&!s->dma.pending);
  CHECK(s->vram_address==0&&s->dma.source_index==0);
  CHECK(s->core.io[0x2312]==0x16&&s->core.io[0x2313]==0);
  CHECK(s->core.io[0x2315]==0&&s->core.io[0x2316]==0);
  CHECK(s->dma.clock_counter==24); /* Pinned software's byte-counter wrap. */
  for(j=0;j<65536;j++){bytes_checked++;if(s->vram[j]!=patterns[k]){fprintf(stderr,"VRAM mismatch%u\n",j);return 1;}}
  *prior=*s;CHECK(!nba_bootstrap_fill_step(s,NULL,NULL)&&!memcmp(s,prior,sizeof(*s)));
 }
 printf("PASS %u contracts and%u byte checks;3nonzero cases are isolated hardware tests, not native initialization\n",assertions,bytes_checked);
 free(prior);free(s);nba_rom_free(&rom);return 0;
}
