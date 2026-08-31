#include "nba_setup_spc_control.h"
bool nba_setup_spc_control_commit(NbaSetupSpcControl *h,NbaSetupSpcResidentBus *b,uint8_t value){
 unsigned i;
 if(!h||!b)return false;
 /* Spc::Write increments the hardware cycle BEFORE this commit boundary.
  * ARAM write gating does not suppress the control register's effects. */
 if(h->aram_write_enabled)b->aram[0xf1]=value;
 if(value&0x10){
  b->cpu_to_spc[0]=b->cpu_to_spc[1]=0;
  h->staged_cpu_input[0]=h->staged_cpu_input[1]=0;
 }
 if(value&0x20){
  b->cpu_to_spc[2]=b->cpu_to_spc[3]=0;
  h->staged_cpu_input[2]=h->staged_cpu_input[3]=0;
 }
 /* Source hardware clears both currently visible and staged CPU inputs.
  * It leaves output latches and the pending-update flag unchanged. Retain
  * that flag even when all staged bytes became zero; do not normalize it. */
 for(i=0;i<3;i++){
  bool enabled=(value&(1u<<i))!=0;
  if(!h->timer[i].enabled&&enabled){
   h->timer[i].stage2=0;h->timer[i].output=0;
  }
  /* Enable edges preserve prescaler stage0/stage1/previous_stage1, target,
   * and global gating. Repeated enable or disable never clears output. */
  h->timer[i].enabled=enabled;
 }
 h->rom_enabled=(value&0x80)!=0;
 return true;
}
bool nba_setup_spc_control_ipl_visible(const NbaSetupSpcControl *h,uint16_t address){return h&&h->rom_enabled&&address>=0xffc0;}
