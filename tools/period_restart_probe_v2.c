#define _CRT_SECURE_NO_WARNINGS
#include "nba_period_restart_v2.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const size_t offsets[]={
#define ACTOR(n,o)
#define BALL(n,o) offsetof(NbaPeriodRestartState,ball.n),
#define GLOBAL(n,o) offsetof(NbaPeriodRestartState,n),
#include "period_restart_probe_fields.inc"
#undef ACTOR
#undef BALL
#undef GLOBAL
};
static const size_t actor_offsets[]={
#define ACTOR(n,o) offsetof(NbaPeriodRestartActor,n),
#define BALL(n,o)
#define GLOBAL(n,o)
#include "period_restart_probe_fields.inc"
#undef ACTOR
#undef BALL
#undef GLOBAL
};
static int read_word(FILE *f,uint16_t *out){int a=fgetc(f),b=fgetc(f);if(a==EOF||b==EOF)return 0;*out=(uint16_t)(a|(b<<8));return 1;}
static int transfer(FILE *f,NbaPeriodRestartState *s,int output){
 unsigned i,j;uint16_t value;unsigned char *base=(unsigned char*)s;int comma=0;
#define WORD_AT(p) do {if(output){memcpy(&value,(p),2);printf("%s%u",comma?",":"",value);comma=1;}else {if(!read_word(f,&value))return 0;memcpy((p),&value,2);}}while(0)
 for(i=0;i<10;i++)for(j=0;j<sizeof(actor_offsets)/sizeof(actor_offsets[0]);j++)WORD_AT((unsigned char*)&s->actors[i]+actor_offsets[j]);
 for(i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++)WORD_AT(base+offsets[i]);
 for(i=0;i<12;i++)WORD_AT(&s->object_list[i]);
#undef WORD_AT
 return 1;
}
/* File protocol is typed component input, not raw WRAM: header magic/version,
 * period/tip/anchors, followed by fields above. Optional child return records
 * are DIAGNOSTIC ONLY, label-matched to source_pc/actor at excluded boundaries.
 * They validate parent segments; they are not normal-state or child emulation. */
int main(int argc,char **argv){
 FILE *f;uint16_t magic,version,period,tip,a0,a1,returns,pc,actor;
 NbaPeriodRestart w;NbaPeriodRestartState s;NbaPeriodRestartInput input;unsigned count=0,used=0;
 if(argc!=2)return 2;f=fopen(argv[1],"rb");if(!f)return 2;memset(&s,0,sizeof(s));
 if(!read_word(f,&magic)||!read_word(f,&version)||!read_word(f,&period)||!read_word(f,&tip)||!read_word(f,&a0)||!read_word(f,&a1)||!read_word(f,&returns)||magic!=0x5250||version!=1||returns>12||!transfer(f,&s,0)){fclose(f);return 3;}
 input.period=period;input.tip_winner=tip;memcpy(&input.anchor_x[0],&a0,2);memcpy(&input.anchor_x[1],&a1,2);
 if(!nba_period_restart_begin(&w,&input)){fclose(f);return 4;}
 for(;;){
  NbaPeriodRestartBoundary b=nba_period_restart_advance(&w,&s);NbaPeriodRestart old=w;NbaPeriodRestartState saved=s;
  NbaPeriodRestartBoundary repeat=nba_period_restart_advance(&w,&s);
  if(memcmp(&old,&w,sizeof(w))||memcmp(&saved,&s,sizeof(s))||b.kind!=repeat.kind){fclose(f);return 5;}
  printf("{\"kind\":%u,\"pc\":%u,\"child\":%u,\"actor\":%u,\"words\":[",(unsigned)b.kind,(unsigned)b.source_pc,(unsigned)b.child_pc,b.actor);transfer(f,&s,1);puts("]}");count++;
  if(b.kind==NBA_PERIOD_CONTROLLER||b.kind==NBA_PERIOD_OPENING){if(nba_period_restart_resume(&w)){fclose(f);return 6;}break;}
  if(b.kind==NBA_PERIOD_APPEARANCE||b.kind==NBA_PERIOD_APPEARANCE_GEOMETRY||b.kind==NBA_PERIOD_OBJECT_SORT){
   if(returns){
    if(used>=returns||!read_word(f,&pc)||!read_word(f,&actor)||pc!=(uint16_t)b.source_pc||actor!=b.actor||!transfer(f,&s,0)){fclose(f);return 7;}used++;
   }
  }
  if(!nba_period_restart_resume(&w)||count>30){fclose(f);return 8;}
 }
 if(used!=returns||fgetc(f)!=EOF){fclose(f);return 9;}fclose(f);return 0;
}
