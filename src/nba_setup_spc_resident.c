#include "nba_setup_spc_resident.h"
#include <string.h>
#define Z 2u
#define C 1u
typedef struct { uint16_t pc; uint8_t length,bytes[3]; } Source;
/* ROM00:C687+PC-0380. Only these source instructions are compiled. */
static const Source source[]={
 {0x441,2,{0xc4,0xf4}}, {0x443,2,{0xe4,0xf4}}, {0x445,2,{0xd0,0xfc}},
 {0x447,3,{0x8f,0x00,0xf4}}, {0x44a,3,{0x3f,0x8b,0x04}},
 {0x44d,2,{0xe4,0xf4}}, {0x44f,2,{0xf0,0xf9}},
 {0x451,1,{0}}, {0x452,1,{0}}, {0x453,3,{0x2e,0xf4,0xf4}},
 {0x456,1,{0x1c}}, {0x457,1,{0x5d}}, {0x458,3,{0x1f,0x61,0x04}},
 {0x48b,2,{0xe4,0xfd}},
 {0x613,3,{0x8f,0x05,0xf4}}, {0x616,2,{0xe4,0xf5}},
 {0x618,1,{0x5d}}, {0x619,1,{0x9f}}, {0x61a,1,{0x60}},
 {0x61b,2,{0x88,0x07}}, {0x61d,1,{0xfd}}, {0x61e,2,{0xe8,0xbf}},
 {0x620,2,{0xcb,0xf2}}, {0x622,2,{0xc4,0xf3}}
};
static const Source *find(uint16_t pc) {
 unsigned i;for(i=0;i<sizeof(source)/sizeof(source[0]);i++)if(source[i].pc==pc)return &source[i];return 0;
}
static void nz(NbaSetupSpcResident *s,uint8_t v) {
 s->ps=(uint8_t)((s->ps&~0x82u)|(v&0x80u)|(v==0?Z:0));
}
bool nba_setup_spc_resident_begin(NbaSetupSpcResident *s,uint16_t pc,uint8_t a,uint8_t x,uint8_t y,uint8_t sp,uint8_t ps) {
 if(!s)return false;memset(s,0,sizeof(*s));
 if((ps&0x20u)||(pc!=0x441&&pc!=0x443&&pc!=0x447&&pc!=0x44d&&pc!=0x613))return false;
 s->pc=pc;s->a=a;s->x=x;s->y=y;s->sp=sp;s->ps=ps;s->valid=true;return true;
}
bool nba_setup_spc_resident_visible_input(NbaSetupSpcResidentBus *b,unsigned port,uint8_t v) {
 if(!b||port>=4)return false;b->cpu_to_spc[port]=v;return true;
}
NbaSetupSpcResidentWork nba_setup_spc_resident_peek(const NbaSetupSpcResident *s) {
 NbaSetupSpcResidentWork w={NBA_SPC_INVALID,0,0,0,false};const Source *p;unsigned t;
 if(!s||!s->valid)return w;w.pc=s->pc;p=find(s->pc);
 if(!p){w.kind=NBA_SPC_UNSUPPORTED;return w;}t=s->phase;
 w.kind=NBA_SPC_IDLE;
 /* CBNE's relative operand is fetched AFTER the live port read and idle. */
 if(t<p->length && !(s->pc==0x453&&t==2)) {
  w.kind=NBA_SPC_FETCH;w.address=(uint16_t)(s->pc+t);w.value=p->bytes[t];
 }
 switch(s->pc) {
 case 0x441:case 0x620:case 0x622:
  if(t>=2){w.address=(uint16_t)(s->pc==0x441?0xf4:s->pc==0x620?0xf2:0xf3);w.kind=t==2?NBA_SPC_READ:NBA_SPC_WRITE;w.value=s->pc==0x620?s->y:s->a;}
  w.instruction_end=t==3;break;
 case 0x447:case 0x613:
  /* MOV dp,#imm performs an input read even when writing an output latch. */
  if(t>=3){w.address=0xf4;w.kind=t==3?NBA_SPC_READ:NBA_SPC_WRITE;w.value=s->pc==0x613?5:0;}w.instruction_end=t==4;break;
 case 0x443:case 0x44d:case 0x616:case 0x48b:
  if(t==2){w.kind=NBA_SPC_READ;w.address=(uint16_t)(s->pc==0x616?0xf5:s->pc==0x48b?0xfd:0xf4);}w.instruction_end=t==2;break;
 case 0x445:case 0x44f:{bool take=s->pc==0x445?!(s->ps&Z):(s->ps&Z)!=0;w.instruction_end=t==(take?3u:1u);break;}
 case 0x44a:
  if(t==4||t==5){w.kind=NBA_SPC_WRITE;w.address=(uint16_t)(0x100u+s->sp);w.value=t==4?4:0x4d;}w.instruction_end=t==7;break;
 case 0x453:
  if(t==2){w.kind=NBA_SPC_READ;w.address=0xf4;}
  if(t==4){w.kind=NBA_SPC_FETCH;w.address=0x455;w.value=0xf4;}
  w.instruction_end=t==(s->a==(uint8_t)s->temporary?4u:6u);break;
 case 0x458:
  if(t==4||t==5){w.kind=NBA_SPC_READ;w.address=(uint16_t)(0x461+s->x+(t==5));}w.instruction_end=t==5;break;
 case 0x619:
  if(t==1){w.kind=NBA_SPC_READ;w.address=0x61a;}w.instruction_end=t==4;break;
 case 0x451:case 0x452:case 0x456:case 0x457:case 0x618:case 0x61a:case 0x61d:
  if(t==1){w.kind=NBA_SPC_READ;w.address=(uint16_t)(s->pc+1);}w.instruction_end=t==1;break;
 case 0x61b:case 0x61e:w.instruction_end=t==1;break;
 default:break;
 }
 /* $0622 STA F3 reads F3 BEFORE writing. Do not invent even that DSP read. */
 if(w.kind==NBA_SPC_READ&&w.address==0xfd)w.kind=NBA_SPC_TIMER;
 if((w.kind==NBA_SPC_READ||w.kind==NBA_SPC_WRITE)&&w.address==0xf3)w.kind=NBA_SPC_DSP;
 return w;
}
bool nba_setup_spc_resident_accept(NbaSetupSpcResident *s,NbaSetupSpcResidentBus *b) {
 NbaSetupSpcResidentWork w;const Source *p;uint8_t v=0;uint16_t next;unsigned t;
 if(!s||!b)return false;w=nba_setup_spc_resident_peek(s);if(w.kind>=NBA_SPC_TIMER)return false;
 p=find(s->pc);if(!p)return false;t=s->phase;next=(uint16_t)(s->pc+p->length);
 if(w.kind==NBA_SPC_FETCH){if(b->aram[w.address]!=w.value)return false;v=w.value;}
 if(w.kind==NBA_SPC_READ){v=w.address>=0xf4&&w.address<=0xf7?b->cpu_to_spc[w.address-0xf4]:w.address==0xf2?b->dsp_address:b->aram[w.address];}
 if(w.kind==NBA_SPC_WRITE){
  b->aram[w.address]=w.value; /* Real SPC writes also change underlying ARAM. */
  if(w.address>=0xf4&&w.address<=0xf7)b->spc_to_cpu[w.address-0xf4]=w.value;
  if(w.address==0xf2)b->dsp_address=w.value;
 }
 if(s->pc==0x44a&&(t==4||t==5))s->sp--;
 if(s->pc==0x453&&t==2)s->temporary=v;
 if(s->pc==0x458&&t==4)s->temporary=v;
 if(w.instruction_end){
  switch(s->pc){
  case 0x443:case 0x44d:case 0x616:s->a=v;nz(s,v);break;
  case 0x445:if(!(s->ps&Z))next=0x443;break;
  case 0x44f:if(s->ps&Z)next=0x44a;break;
  case 0x44a:next=0x48b;break;
  case 0x453:if(s->a!=(uint8_t)s->temporary)next=0x44a;break; /* CBNE leaves PS unchanged. */
  /* Original $0456 aliases commands through eight-bit ASL; do not insert
   * a command-range check (e.g. $85 also indexes command $05). */
  case 0x456:s->ps=(uint8_t)((s->ps&~C)|(s->a>>7));s->a=(uint8_t)(s->a<<1);nz(s,s->a);break;
  case 0x457:case 0x618:s->x=s->a;nz(s,s->x);break;
  case 0x458:next=(uint16_t)(s->temporary|((uint16_t)v<<8));break;
  /* $0619 uses the entire input byte; source does not mask a voice index. */
  case 0x619:s->a=(uint8_t)((s->a>>4)|(s->a<<4));nz(s,s->a);break;
  case 0x61a:s->ps=(uint8_t)(s->ps&~C);break;
  case 0x61b:{unsigned r=s->a+7u+(s->ps&C);uint8_t a=s->a;
   s->ps=(uint8_t)((s->ps&~0xcbu)|(r>255?1:0)|(((~(a^7u)&(a^r))&128)?64:0)|(((a&15)+7+(s->ps&C)>15)?8:0));s->a=(uint8_t)r;nz(s,s->a);break;}
  case 0x61d:s->y=s->a;nz(s,s->y);break;
  case 0x61e:s->a=0xbf;nz(s,s->a);break;
  default:break;
  }
  s->pc=next;s->phase=0;s->instructions++;
 }else s->phase++;
 s->cycles++;return true;
}
