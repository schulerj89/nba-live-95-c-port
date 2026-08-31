#define _CRT_SECURE_NO_WARNINGS
#include "nba_period_roles.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
static const size_t actors[]={
#define ACTOR(n,o) offsetof(NbaPeriodRoleActor,n),
#define CONTEXT(n,o)
#define GLOBAL(n,o)
#include "period_roles_probe_fields.inc"
#undef ACTOR
#undef CONTEXT
#undef GLOBAL
};
static const size_t contexts[]={
#define ACTOR(n,o)
#define CONTEXT(n,o) offsetof(NbaPeriodRoleContext,n),
#define GLOBAL(n,o)
#include "period_roles_probe_fields.inc"
#undef ACTOR
#undef CONTEXT
#undef GLOBAL
};
static const size_t globals[]={
#define ACTOR(n,o)
#define CONTEXT(n,o)
#define GLOBAL(n,o) offsetof(NbaPeriodRoleState,n),
#include "period_roles_probe_fields.inc"
#undef ACTOR
#undef CONTEXT
#undef GLOBAL
};
static int read_word(FILE *f,uint16_t *out){int a=fgetc(f),b=fgetc(f);if(a==EOF||b==EOF)return 0;*out=(uint16_t)(a|(b<<8));return 1;}
static int transfer(FILE *f,NbaPeriodRoleState *s,int output){
 unsigned i,j;uint16_t value;int comma=0;
#define WORD_AT(p) do {if(output){memcpy(&value,(p),2);printf("%s%u",comma?",":"",value);comma=1;}else {if(!read_word(f,&value))return 0;memcpy((p),&value,2);}}while(0)
 for(i=0;i<10;i++)for(j=0;j<sizeof(actors)/sizeof(actors[0]);j++)WORD_AT((unsigned char*)&s->actors[i]+actors[j]);
 for(i=0;i<2;i++)for(j=0;j<sizeof(contexts)/sizeof(contexts[0]);j++)WORD_AT((unsigned char*)&s->contexts[i]+contexts[j]);
 for(i=0;i<sizeof(globals)/sizeof(globals[0]);i++)WORD_AT((unsigned char*)s+globals[i]);
#undef WORD_AT
 return 1;
}
/* One typed before-state only. There is no child-return/expected-after input. */
int main(int argc,char **argv){
 FILE *f;uint16_t magic,version;NbaPeriodRoles w;NbaPeriodRoleState s;unsigned count=0;
 if(argc!=2)return 2;f=fopen(argv[1],"rb");if(!f)return 2;memset(&s,0,sizeof(s));
 if(!read_word(f,&magic)||!read_word(f,&version)||magic!=0x5252||version!=1||!transfer(f,&s,0)||fgetc(f)!=EOF){fclose(f);return 3;}fclose(f);
 if(!nba_period_roles_begin(&w,&s))return 4;
 for(;;){
  NbaPeriodRoleBoundary b=nba_period_roles_advance(&w,&s);NbaPeriodRoles old=w;NbaPeriodRoleState saved=s;
  NbaPeriodRoleBoundary repeat=nba_period_roles_advance(&w,&s);
  if(memcmp(&old,&w,sizeof(w))||memcmp(&saved,&s,sizeof(s))||b.kind!=repeat.kind)return 5;
  printf("{\"kind\":%u,\"pc\":%u,\"completed_calls\":%u,\"words\":[",(unsigned)b.kind,(unsigned)b.source_pc,b.completed_calls);transfer(NULL,&s,1);puts("]}");count++;
  if(b.kind!=NBA_PERIOD_ROLES_FIRST_RETURN){if(nba_period_roles_resume(&w))return 6;break;}
  if(!nba_period_roles_resume(&w)||count>1)return 7;
 }
 return 0;
}
