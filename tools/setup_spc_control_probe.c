#define _CRT_SECURE_NO_WARNINGS
#include "nba_setup_spc_control.h"
#include <stdio.h>
#include <string.h>
static uint8_t data[65577];static NbaSetupSpcResidentBus bus;
int main(int argc,char**argv){
 NbaSetupSpcControl h;FILE*f;unsigned i;size_t n;
 if(argc!=3)return 2;f=fopen(argv[1],"rb");if(!f)return 2;n=fread(data,1,sizeof(data),f);
 if(n!=sizeof(data)||fgetc(f)!=EOF){fclose(f);return 2;}fclose(f);
 if(data[1]>1||data[2]>1||data[3]>1)return 3;
 memset(&h,0,sizeof(h));h.aram_write_enabled=data[1]!=0;h.rom_enabled=data[2]!=0;h.pending_cpu_input_update=data[3]!=0;memcpy(h.staged_cpu_input,data+4,4);
 for(i=0;i<3;i++){
  uint8_t *p=data+8+8*i;NbaSetupSpcControlTimer*t=&h.timer[i];
  if(p[0]>=(i==2?16:128)||p[1]>1||p[2]>1||p[6]>1||p[7]>1)return 3;
  t->stage0=p[0];t->stage1=p[1];t->previous_stage1=p[2];t->stage2=p[3];t->output=p[4];t->target=p[5];t->enabled=p[6]!=0;t->globally_enabled=p[7]!=0;
 }
 memcpy(bus.cpu_to_spc,data+32,4);memcpy(bus.spc_to_cpu,data+36,4);bus.dsp_address=data[40];memcpy(bus.aram,data+41,65536);
 if(!nba_setup_spc_control_commit(&h,&bus,data[0]))return 4;
 if(nba_setup_spc_control_commit(0,&bus,0)||nba_setup_spc_control_commit(&h,0,0))return 5;
 if(nba_setup_spc_control_ipl_visible(&h,0xffbf)||nba_setup_spc_control_ipl_visible(&h,0xffc0)!=h.rom_enabled||nba_setup_spc_control_ipl_visible(&h,0xffff)!=h.rom_enabled||nba_setup_spc_control_ipl_visible(0,0xffff))return 6;
 data[1]=(uint8_t)h.aram_write_enabled;data[2]=(uint8_t)h.rom_enabled;data[3]=(uint8_t)h.pending_cpu_input_update;memcpy(data+4,h.staged_cpu_input,4);
 for(i=0;i<3;i++){
  uint8_t*p=data+8+8*i;const NbaSetupSpcControlTimer*t=&h.timer[i];p[0]=t->stage0;p[1]=t->stage1;p[2]=t->previous_stage1;p[3]=t->stage2;p[4]=t->output;p[5]=t->target;p[6]=(uint8_t)t->enabled;p[7]=(uint8_t)t->globally_enabled;
 }
 memcpy(data+32,bus.cpu_to_spc,4);memcpy(data+36,bus.spc_to_cpu,4);data[40]=bus.dsp_address;memcpy(data+41,bus.aram,65536);
 f=fopen(argv[2],"wb");if(!f)return 7;if(fwrite(data,1,sizeof(data),f)!=sizeof(data)||fclose(f))return 8;return 0;
}
