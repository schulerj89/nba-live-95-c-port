#define _CRT_SECURE_NO_WARNINGS
#include "nba_period_roles_v2.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
static const size_t fields[]={
#define FIELD(n,o,width) offsetof(NbaPeriodRoleStateV2,n),
#include "period_roles_probe_fields_v2.inc"
#undef FIELD
};
static int read_word(FILE *f,uint16_t *out){int a=fgetc(f),b=fgetc(f);if(a==EOF||b==EOF)return 0;*out=(uint16_t)(a|(b<<8));return 1;}
static int transfer(FILE *f,NbaPeriodRoleStateV2 *s,int output){
 unsigned i;uint16_t value;int comma=0;
#define WORD_AT(p) do {if(output){memcpy(&value,(p),2);printf("%s%u",comma?",":"",value);comma=1;}else {if(!read_word(f,&value))return 0;memcpy((p),&value,2);}}while(0)
 for(i=0;i<sizeof(fields)/sizeof(fields[0]);i++)WORD_AT((unsigned char*)s+fields[i]);
#undef WORD_AT
 return 1;
}
/* One typed before-state only. There is no child-return/expected-after input. */
int main(int argc,char **argv){
 FILE *f;uint16_t magic,version;NbaPeriodRolesV2 w;NbaPeriodRoleStateV2 s;unsigned count=0;
 if(argc!=2)return 2;f=fopen(argv[1],"rb");if(!f)return 2;memset(&s,0,sizeof(s));
 if(!read_word(f,&magic)||!read_word(f,&version)||magic!=0x5252||version!=2||!transfer(f,&s,0)||fgetc(f)!=EOF){fclose(f);return 3;}fclose(f);
 if(!nba_period_roles_v2_begin(&w,&s))return 4;
 for(;;){
  NbaPeriodRoleBoundaryV2 b=nba_period_roles_v2_advance(&w,&s);NbaPeriodRolesV2 old=w;NbaPeriodRoleStateV2 saved=s;
  NbaPeriodRoleBoundaryV2 repeat=nba_period_roles_v2_advance(&w,&s);
  if(memcmp(&old,&w,sizeof(w))||memcmp(&saved,&s,sizeof(s))||b.kind!=repeat.kind)return 5;
  printf("{\"kind\":%u,\"pc\":%u,\"completed_calls\":%u,\"record_pointer\":%u,\"words\":[",(unsigned)b.kind,(unsigned)b.source_pc,b.completed_calls,b.record_pointer);transfer(NULL,&s,1);puts("]}");count++;
  if(b.kind!=NBA_PERIOD_ROLES_V2_FIRST_RETURN){if(nba_period_roles_v2_resume(&w))return 6;break;}
  if(!nba_period_roles_v2_resume(&w)||count>1)return 7;
 }
 return 0;
}

