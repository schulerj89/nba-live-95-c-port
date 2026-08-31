#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "nba_period_formation.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
typedef struct {size_t offset;unsigned width;} Field;
static const Field fields[]={
#define FIELD(n,a,w) {offsetof(NbaPeriodFormationState,n),w},
#include "period_formation_fields.inc"
#undef FIELD
};
static int transfer(FILE *f,NbaPeriodFormationState *s,int output){
 unsigned i,j;for(i=0;i<sizeof(fields)/sizeof(fields[0]);i++){
  uint32_t value=0;unsigned char *p=(unsigned char*)s+fields[i].offset;
  if(output){memcpy(&value,p,fields[i].width);printf("%s%u",i?",":"",(unsigned)value);}
  else{for(j=0;j<fields[i].width;j++){int b=fgetc(f);if(b==EOF)return 0;value|=(uint32_t)b<<(8*j);}memcpy(p,&value,fields[i].width);}
 }return 1;
}
int main(int argc,char **argv){
 FILE *f;unsigned char header[4];NbaPeriodFormationState state;NbaPeriodFormation work;NbaAssetPack assets={0};unsigned guard=0;
 if(argc!=3)return 2;f=fopen(argv[2],"rb");if(!f)return 2;memset(&state,0,sizeof(state));
 if(fread(header,1,4,f)!=4||memcmp(header,"PFC1",4)||!transfer(f,&state,0)||fgetc(f)!=EOF){fclose(f);return 3;}fclose(f);
 if(!nba_period_formation_begin(&work,&state))return 4;
 fflush(stdout);int saved=_dup(_fileno(stdout));if(saved<0||_dup2(_fileno(stderr),_fileno(stdout)))return 2;
 bool loaded=nba_assets_load(&assets,argv[1]);fflush(stdout);if(_dup2(saved,_fileno(stdout)))return 2;_close(saved);if(!loaded)return 2;
 for(;;){
  NbaPeriodFormationBoundary b=nba_period_formation_advance(&work,&state,&assets);NbaPeriodFormation old=work;NbaPeriodFormationState before=state;
  NbaPeriodFormationBoundary same=nba_period_formation_advance(&work,&state,&assets);
  if(memcmp(&old,&work,sizeof(work))||memcmp(&before,&state,sizeof(state))||b.kind!=same.kind)return 5;
  printf("{\"kind\":%u,\"pc\":%u,\"actor\":%u,\"refusal\":%u,\"role_kind\":%u,\"role_pc\":%u,\"role_pointer\":%u,\"role_calls\":%u,\"values\":[",(unsigned)b.kind,(unsigned)b.source_pc,b.actor,(unsigned)b.refusal,(unsigned)b.role.kind,(unsigned)b.role.source_pc,b.role.record_pointer,b.role.completed_calls);transfer(NULL,&state,1);puts("]}");
  if(b.kind!=NBA_PERIOD_FORMATION_CHECKPOINT){if(nba_period_formation_resume(&work))return 6;break;}
  if(++guard>40||!nba_period_formation_resume(&work))return 7;
 }
 nba_assets_free(&assets);return 0;
}
