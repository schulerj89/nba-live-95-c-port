#include "nba_setup_spc_init.h"
#include <string.h>
typedef struct {uint16_t pc;uint8_t length,bytes[3];} Source;
/* Canonical uploaded source; only these concrete PCs are implemented. */
static const Source source[]={
 {0x380,1,{0x20}},{0x381,2,{0xcd,0xff}},{0x383,1,{0xbd}},{0x384,3,{0x8f,0x30,0xf1}},
 {0x387,2,{0xe8,0x00}},{0x389,2,{0xcd,0x7b}},{0x38b,2,{0xd4,0x00}},{0x38d,1,{0x1d}},
 {0x38e,2,{0x10,0xfb}},{0x390,1,{0xfd}},{0x391,3,{0xd6,0x00,0x01}},{0x394,2,{0xfe,0xfb}},
 {0x396,3,{0xd6,0x00,0x02}},{0x399,2,{0xfe,0xfb}},{0x39b,2,{0x8d,0x7f}},{0x39d,3,{0xd6,0x00,0x03}},
 {0x3a0,1,{0xdc}},{0x3a1,2,{0x10,0xfa}},{0x3a3,3,{0x8f,0x70,0x00}},{0x3a6,3,{0x8f,0x08,0x01}},
 {0x3a9,2,{0xe8,0xff}},{0x3ab,1,{0x80}},{0x3ac,2,{0xa4,0x00}},{0x3ae,2,{0xc4,0x02}},
 {0x3b0,2,{0xe8,0xff}},{0x3b2,1,{0x80}},{0x3b3,2,{0xa4,0x01}},{0x3b5,2,{0xc4,0x03}},
 {0x3b7,2,{0xe8,0x00}},{0x3b9,1,{0xfd}},{0x3ba,2,{0xf8,0x02}},{0x3bc,2,{0xd7,0x00}},
 {0x3be,1,{0xfc}},{0x3bf,1,{0x1d}},{0x3c0,2,{0xd0,0xfa}},{0x3c2,2,{0xab,0x01}},
 {0x3c4,2,{0xc4,0x00}},{0x3c6,2,{0x8d,0x00}},{0x3c8,2,{0xf8,0x03}},{0x3ca,2,{0xd7,0x00}},
 {0x3cc,1,{0xfc}},{0x3cd,2,{0xd0,0xfb}},{0x3cf,2,{0xab,0x01}},{0x3d1,1,{0x1d}},
 {0x3d2,2,{0xd0,0xf6}},{0x3d4,2,{0xe8,0x20}},{0x3d6,2,{0xc4,0x04}},{0x3d8,3,{0x8f,0x6c,0xf2}},
 {0x3db,2,{0xc4,0xf3}}
};
static const Source *find(uint16_t pc){unsigned i;for(i=0;i<sizeof(source)/sizeof(source[0]);i++)if(source[i].pc==pc)return &source[i];return 0;}
static void nz(NbaSetupSpcInit *s,uint8_t v){s->ps=(uint8_t)((s->ps&~0x82u)|(v&0x80u)|(v?0:2));}
bool nba_setup_spc_init_begin(NbaSetupSpcInit *s,uint16_t pc,uint8_t a,uint8_t x,uint8_t y,uint8_t sp,uint8_t ps){
 if(!s)return false;memset(s,0,sizeof(*s));if(pc!=0x380&&(pc!=0x387||(ps&0x20)))return false;
 s->pc=pc;s->a=a;s->x=x;s->y=y;s->sp=sp;s->ps=ps;s->valid=true;return true;
}
NbaSetupSpcInitWork nba_setup_spc_init_peek(const NbaSetupSpcInit *s){
 NbaSetupSpcInitWork q={{NBA_SPC_INVALID,0,0,0,false},false};NbaSetupSpcResidentWork *w=&q.bus;const Source *p;unsigned t;
 if(!s||!s->valid)return q;p=find(s->pc);w->pc=s->pc;if(!p){w->kind=NBA_SPC_UNSUPPORTED;return q;}t=s->phase;w->kind=NBA_SPC_IDLE;
 if((t<p->length&&!(s->pc==0x394||s->pc==0x399))||t==0){
  if(t<p->length){w->kind=NBA_SPC_FETCH;w->address=(uint16_t)(s->pc+t);w->value=p->bytes[t];}
 }
 switch(s->pc){
 case 0x380:case 0x383:case 0x38d:case 0x390:case 0x3a0:case 0x3ab:case 0x3b2:case 0x3b9:case 0x3be:case 0x3bf:case 0x3cc:case 0x3d1:
  if(t==1){w->kind=NBA_SPC_READ;w->address=(uint16_t)(s->pc+1);}w->instruction_end=t==1;break;
 case 0x381:case 0x387:case 0x389:case 0x39b:case 0x3a9:case 0x3b0:case 0x3b7:case 0x3c6:case 0x3d4:w->instruction_end=t==1;break;
 case 0x384:case 0x3a3:case 0x3a6:case 0x3d8:
  if(t>=3){w->kind=t==3?NBA_SPC_READ:NBA_SPC_WRITE;w->address=p->bytes[2];w->value=p->bytes[1];}w->instruction_end=t==4;
  if(s->pc==0x384&&t==4)q.control_publication=true;break;
 case 0x38b:
  if(t>=3){w->kind=t==3?NBA_SPC_READ:NBA_SPC_WRITE;w->address=s->x;w->value=s->a;}w->instruction_end=t==4;break;
 case 0x391:case 0x396:case 0x39d:
  if(t>=4){w->kind=t==4?NBA_SPC_READ:NBA_SPC_WRITE;w->address=(uint16_t)(((uint16_t)p->bytes[2]<<8)+s->y);w->value=s->a;}w->instruction_end=t==5;break;
 case 0x394:case 0x399:
  if(t==1){w->kind=NBA_SPC_READ;w->address=(uint16_t)(s->pc+1);}
  if(t==3){w->kind=NBA_SPC_FETCH;w->address=(uint16_t)(s->pc+1);w->value=p->bytes[1];}
  w->instruction_end=t==((t<=3?(uint8_t)(s->y-1):s->y)?5u:3u);break;
 case 0x38e:case 0x3a1:w->instruction_end=t==((s->ps&0x80)?1u:3u);break;
 case 0x3c0:case 0x3cd:case 0x3d2:w->instruction_end=t==((s->ps&2)?1u:3u);break;
 case 0x3ac:case 0x3b3:case 0x3ba:case 0x3c8:
  if(t==2){w->kind=NBA_SPC_READ;w->address=p->bytes[1];}w->instruction_end=t==2;break;
 case 0x3ae:case 0x3b5:case 0x3c4:case 0x3d6:case 0x3db:
  if(t>=2){w->kind=t==2?NBA_SPC_READ:NBA_SPC_WRITE;w->address=p->bytes[1];w->value=s->a;}w->instruction_end=t==3;break;
 case 0x3c2:case 0x3cf:
  if(t>=2){w->kind=t==2?NBA_SPC_READ:NBA_SPC_WRITE;w->address=1;w->value=(uint8_t)(s->temporary+1);}w->instruction_end=t==3;break;
 case 0x3bc:case 0x3ca:
  if(t==2||t==3){w->kind=NBA_SPC_READ;w->address=(uint16_t)(t-2);}
  if(t>=5){w->kind=t==5?NBA_SPC_READ:NBA_SPC_WRITE;w->address=(uint16_t)(s->temporary+s->y);w->value=s->a;}w->instruction_end=t==6;break;
 default:break;
 }
 if(w->kind==NBA_SPC_READ&&w->address==0xf3)w->kind=NBA_SPC_DSP;
 return q;
}
bool nba_setup_spc_init_accept(NbaSetupSpcInit *s,NbaSetupSpcResidentBus *b){
 NbaSetupSpcInitWork q;NbaSetupSpcResidentWork w;const Source *p;uint8_t v=0;unsigned t;uint16_t next;
 if(!s||!b)return false;q=nba_setup_spc_init_peek(s);w=q.bus;if(q.control_publication||w.kind>=NBA_SPC_TIMER)return false;
 p=find(s->pc);if(!p)return false;t=s->phase;next=(uint16_t)(s->pc+p->length);
 if(w.kind==NBA_SPC_FETCH){if(b->aram[w.address]!=w.value)return false;v=w.value;}
 if(w.kind==NBA_SPC_READ){v=w.address==0xf1?0:w.address==0xf2?b->dsp_address:b->aram[w.address];}
 if(w.kind==NBA_SPC_WRITE){b->aram[w.address]=w.value;if(w.address==0xf2)b->dsp_address=w.value;}
 if((s->pc==0x394||s->pc==0x399)&&t==3)s->y--; /* DBNZ Y does not alter PS. */
 if((s->pc==0x3c2||s->pc==0x3cf)&&t==2)s->temporary=v;
 if(s->pc==0x3bc||s->pc==0x3ca){if(t==2)s->temporary=v;if(t==3)s->temporary|=(uint16_t)v<<8;}
 if(w.instruction_end){
  switch(s->pc){
  case 0x380:s->ps=(uint8_t)(s->ps&~0x20u);break;
  case 0x381:case 0x389:s->x=p->bytes[1];nz(s,s->x);break;
  case 0x383:s->sp=s->x;break;
  case 0x387:case 0x3a9:case 0x3b0:case 0x3b7:case 0x3d4:s->a=p->bytes[1];nz(s,s->a);break;
  case 0x39b:case 0x3c6:s->y=p->bytes[1];nz(s,s->y);break;
  case 0x38d:case 0x3bf:case 0x3d1:s->x--;nz(s,s->x);break;
  case 0x390:case 0x3b9:s->y=s->a;nz(s,s->y);break;
  case 0x3a0:s->y--;nz(s,s->y);break;
  case 0x3be:case 0x3cc:s->y++;nz(s,s->y);break;
  case 0x394:case 0x399:if(s->y)next=(uint16_t)(next+(int8_t)p->bytes[1]);break;
  case 0x38e:case 0x3a1:if(!(s->ps&0x80))next=(uint16_t)(next+(int8_t)p->bytes[1]);break;
  case 0x3c0:case 0x3cd:case 0x3d2:if(!(s->ps&2))next=(uint16_t)(next+(int8_t)p->bytes[1]);break;
  case 0x3ab:case 0x3b2:s->ps|=1;break;
  case 0x3ac:case 0x3b3:{
   /* Source FF-70 produces8F, not90:0870..08FE are cleared;08FF is
    * intentionally left unchanged. Preserve this original loop omission. */
   uint8_t a=s->a;unsigned bcomp=(uint8_t)~v;unsigned r=a+bcomp+(s->ps&1);
   s->ps=(uint8_t)((s->ps&~0xcbu)|(r>255?1:0)|(((~(a^bcomp)&(a^r))&128)?64:0)|(((a&15)+(bcomp&15)+(s->ps&1)>15)?8:0));s->a=(uint8_t)r;nz(s,s->a);break;}
  case 0x3ba:case 0x3c8:s->x=v;nz(s,s->x);break;
  case 0x3c2:case 0x3cf:nz(s,w.value);break;
  default:break;
  }
  s->pc=next;s->phase=0;s->instructions++;
 }else s->phase++;
 s->cycles++;return true;
}
