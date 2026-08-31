#include "nba_bootstrap_internal.h"
/* Fixed SNES IPL firmware, pinned Spc.h _spcBios. These concrete source PCs
 * are compiled C. No arbitrary SPC opcodes or uploaded instructions decode. */
static const uint8_t ipl[64]={
 0xcd,0xef,0xbd,0xe8,0x00,0xc6,0x1d,0xd0,0xfc,0x8f,0xaa,0xf4,0x8f,0xbb,0xf5,0x78,
 0xcc,0xf4,0xd0,0xfb,0x2f,0x19,0xeb,0xf4,0xd0,0xfc,0x7e,0xf4,0xd0,0x0b,0xe4,0xf5,
 0xcb,0xf4,0xd7,0x00,0xfc,0xd0,0xf3,0xab,0x01,0x10,0xef,0x7e,0xf4,0x10,0xeb,0xba,
 0xf6,0xda,0x00,0xba,0xf4,0xc4,0xf4,0xdd,0x5d,0xd0,0xdb,0x1f,0x00,0x00,0xc0,0xff};
uint8_t nba_bootstrap_ipl_byte(uint16_t address){return ipl[address&63u];}
static uint8_t length(uint16_t pc){
 switch(pc){
 case 0xffc2:case 0xffc5:case 0xffc6:case 0xffe4:case 0xfff7:case 0xfff8:return 1;
 case 0xffc9:case 0xffcc:case 0xffcf:case 0xfffb:return 3;
 case 0xffc0:case 0xffc3:case 0xffc7:case 0xffd2:case 0xffd4:case 0xffd6:
 case 0xffd8:case 0xffda:case 0xffdc:case 0xffde:case 0xffe0:case 0xffe2:
 case 0xffe5:case 0xffe7:case 0xffe9:case 0xffeb:case 0xffed:case 0xffef:
 case 0xfff1:case 0xfff3:case 0xfff5:case 0xfff9:return 2;
 default:return 0;
 }
}
static void nz(NbaSetupSpcInit *s,uint8_t v){s->ps=(uint8_t)((s->ps&~0x82u)|(v&128u)|(v?0:2));}
static void cmp(NbaSetupSpcInit *s,uint8_t a,uint8_t b){s->ps=(uint8_t)((s->ps&~1u)|(a>=b));nz(s,(uint8_t)(a-b));}
NbaSetupSpcResidentWork nba_bootstrap_ipl_peek(const NbaSetupSpcInit *s){
 NbaSetupSpcResidentWork w={NBA_SPC_INVALID,0,0,0,false};uint8_t n,t;
 if(!s||!s->valid)return w;w.pc=s->pc;n=length(s->pc);t=s->phase;
 if(!n){w.kind=NBA_SPC_UNSUPPORTED;return w;}w.kind=NBA_SPC_IDLE;
 if(t<n){w.kind=NBA_SPC_FETCH;w.address=(uint16_t)(s->pc+t);w.value=nba_bootstrap_ipl_byte(w.address);}
 switch(s->pc){
 case 0xffc0:case 0xffc3:w.instruction_end=t==1;break;
 case 0xffc2:case 0xffc6:case 0xffe4:case 0xfff7:case 0xfff8:
  if(t==1){w.kind=NBA_SPC_READ;w.address=(uint16_t)(s->pc+1);}w.instruction_end=t==1;break;
 case 0xffc5:
  if(t==1){w.kind=NBA_SPC_READ;w.address=0xffc6;}
  if(t>=2){w.kind=t==2?NBA_SPC_READ:NBA_SPC_WRITE;w.address=s->x;w.value=s->a;}w.instruction_end=t==3;break;
 case 0xffc9:case 0xffcc:
  if(t>=3){w.kind=t==3?NBA_SPC_READ:NBA_SPC_WRITE;w.address=(uint16_t)(s->pc==0xffc9?0xf4:0xf5);w.value=s->pc==0xffc9?0xaa:0xbb;}w.instruction_end=t==4;break;
 case 0xffcf:
  if(t==3){w.kind=NBA_SPC_READ;w.address=0xf4;}w.instruction_end=t==4;break;
 case 0xffc7:case 0xffd2:case 0xffd4:case 0xffd8:case 0xffdc:case 0xffe5:case 0xffe9:case 0xffed:case 0xfff9:{
  bool take=s->pc==0xffd4?true:(s->pc==0xffe9||s->pc==0xffed)?!(s->ps&128):!(s->ps&2);
  w.instruction_end=t==(take?3u:1u);break;}
 case 0xffd6:case 0xffda:case 0xffde:case 0xffeb:
  if(t==2){w.kind=NBA_SPC_READ;w.address=(uint16_t)(s->pc==0xffde?0xf5:0xf4);}w.instruction_end=t==2;break;
 case 0xffe0:case 0xfff5:
  if(t>=2){w.kind=t==2?NBA_SPC_READ:NBA_SPC_WRITE;w.address=0xf4;w.value=s->pc==0xffe0?s->y:s->a;}w.instruction_end=t==3;break;
 case 0xffe2:
  if(t==2||t==3){w.kind=NBA_SPC_READ;w.address=(uint16_t)(t-2);}
  if(t>=5){w.kind=t==5?NBA_SPC_READ:NBA_SPC_WRITE;w.address=(uint16_t)(s->temporary+s->y);w.value=s->a;}w.instruction_end=t==6;break;
 case 0xffe7:
  if(t>=2){w.kind=t==2?NBA_SPC_READ:NBA_SPC_WRITE;w.address=1;w.value=(uint8_t)(s->temporary+1);}w.instruction_end=t==3;break;
 case 0xffef:case 0xfff3:
  if(t==2||t==4){w.kind=NBA_SPC_READ;w.address=(uint16_t)((s->pc==0xffef?0xf6:0xf4)+(t==4));}w.instruction_end=t==4;break;
 case 0xfff1:
  if(t==2){w.kind=NBA_SPC_READ;w.address=0;}
  if(t>=3){w.kind=NBA_SPC_WRITE;w.address=(uint16_t)(t-3);w.value=t==3?s->a:s->y;}w.instruction_end=t==4;break;
 case 0xfffb:
  if(t>=4){w.kind=NBA_SPC_READ;w.address=(uint16_t)(s->x+t-4);}w.instruction_end=t==5;break;
 default:break;
 }return w;
}
bool nba_bootstrap_ipl_accept(NbaSetupSpcInit *s,uint8_t v){
 NbaSetupSpcResidentWork w;uint16_t next;unsigned t;
 if(!s)return false;w=nba_bootstrap_ipl_peek(s);if(w.kind>=NBA_SPC_TIMER)return false;
 if(w.kind==NBA_SPC_FETCH&&w.value!=v)return false;t=s->phase;next=(uint16_t)(s->pc+length(s->pc));
 if(s->pc==0xffcf&&t==3)cmp(s,v,0xcc);
 if(s->pc==0xffe2){if(t==2)s->temporary=v;if(t==3)s->temporary|=(uint16_t)v<<8;}
 if(s->pc==0xffe7&&t==2)s->temporary=v;
 if((s->pc==0xffef||s->pc==0xfff3)&&t==2)s->temporary=v;
 if(s->pc==0xfffb&&t==4)s->temporary=v;
 if(w.instruction_end){
  switch(s->pc){
  case 0xffc0:s->x=0xef;nz(s,s->x);break;
  case 0xffc2:s->sp=s->x;break;
  case 0xffc3:s->a=0;nz(s,s->a);break;
  case 0xffc6:s->x--;nz(s,s->x);break;
  case 0xffe4:s->y++;nz(s,s->y);break;
  case 0xfff7:s->a=s->y;nz(s,s->a);break;
  case 0xfff8:s->x=s->a;nz(s,s->x);break;
  case 0xffd6:s->y=v;nz(s,s->y);break;
  case 0xffde:s->a=v;nz(s,s->a);break;
  case 0xffda:case 0xffeb:cmp(s,s->y,v);break;
  case 0xffe7:nz(s,w.value);break;
  case 0xffef:case 0xfff3:
   s->a=(uint8_t)s->temporary;s->y=v;s->ps=(uint8_t)((s->ps&~0x82u)|(v&128u)|((s->a||s->y)?0:2));break;
  case 0xfffb:next=(uint16_t)(s->temporary|((uint16_t)v<<8));break;
  case 0xffc7:case 0xffd2:case 0xffd4:case 0xffd8:case 0xffdc:case 0xffe5:case 0xffe9:case 0xffed:case 0xfff9:
   if(t==3)next=(uint16_t)(next+(int8_t)ipl[(s->pc+1)&63]);break;
  default:break;
  }s->pc=next;s->phase=0;s->instructions++;
 }else s->phase++;
 s->cycles++;return true;
}
