#define _CRT_SECURE_NO_WARNINGS
#include "nba_setup_spc_resident.h"
#include <stdio.h>
#include <string.h>
static NbaSetupSpcResidentBus b,saved_bus;
static uint8_t payload[1264];
static void seed(unsigned command,unsigned voice){
 memset(&b,0xa5,sizeof(b));memcpy(b.aram+0x380,payload,sizeof(payload));
 b.cpu_to_spc[0]=(uint8_t)command;b.cpu_to_spc[1]=(uint8_t)voice;
 b.spc_to_cpu[0]=0x77;b.dsp_address=0x12;
}
int main(int argc,char **argv){
 FILE *f;NbaSetupSpcResident s,saved;NbaSetupSpcResidentWork w;
 const uint8_t status[]={0,0x49,0xdf,0x1f};unsigned command,voice,pi,cases=0,refusals=0,i;
 if(argc!=2||(f=fopen(argv[1],"rb"))==NULL)return 2;
 if(fseek(f,0x4687,SEEK_SET)||fread(payload,1,sizeof(payload),f)!=sizeof(payload)){fclose(f);return 2;}fclose(f);
 for(command=5;command<=0x85;command+=128)for(voice=0;voice<256;voice++)for(pi=0;pi<4;pi++){
  unsigned swapped=(voice%16)*16+voice/16,total=swapped+7,flags=(status[pi]&0x14)|0x80;
  unsigned writes=0,table_reads=0,ack_read=0;
  if(total>255)flags|=1;if(swapped%16+7>15)flags|=8;if(swapped<128&&total>=128&&total<256)flags|=64;
  seed(command,voice);if(!nba_setup_spc_resident_begin(&s,0x44d,0xaa,0xbb,0xcc,3,status[pi]))return 3;
  while((w=nba_setup_spc_resident_peek(&s)).kind<NBA_SPC_TIMER){
   if(s.pc==0x613&&!s.phase&&(s.a!=10||s.x!=10||(s.ps&1u)!=(command>>7)))return 4;
   if(w.pc==0x458&&w.kind==NBA_SPC_READ){if(w.address!=0x46b+table_reads)return 5;table_reads++;}
   if(w.pc==0x613&&w.kind==NBA_SPC_READ){if(w.address!=0xf4||b.cpu_to_spc[0]!=command||b.spc_to_cpu[0]!=0x77)return 6;ack_read++;}
   if(w.kind==NBA_SPC_WRITE){
    if(writes==0&&(w.pc!=0x613||w.address!=0xf4||w.value!=5||ack_read!=1||s.cycles!=28))return 7;
    if(writes==1&&(w.pc!=0x620||w.address!=0xf2||w.value!=(uint8_t)total))return 8;
    if(writes>1)return 9;writes++;
   }
   if(!nba_setup_spc_resident_accept(&s,&b))return 10;
  }
  if(w.kind!=NBA_SPC_DSP||s.pc!=0x622||s.phase!=2||s.cycles!=53||s.a!=0xbf||s.x!=voice||s.y!=(uint8_t)total||s.ps!=flags||s.sp!=3||writes!=2||table_reads!=2)return 11;
  if(b.cpu_to_spc[0]!=command||b.spc_to_cpu[0]!=5||b.aram[0xf4]!=5||b.dsp_address!=(uint8_t)total||b.aram[0xf2]!=(uint8_t)total||b.aram[0xf3]!=0xa5)return 12;
  saved=s;saved_bus=b;if(nba_setup_spc_resident_accept(&s,&b)||memcmp(&s,&saved,sizeof(s))||memcmp(&b,&saved_bus,sizeof(b)))return 13;refusals++;cases++;
 }
 for(voice=0;voice<256;voice++){
  seed(5,7);if(!nba_setup_spc_resident_begin(&s,0x44d,0,0,0,0,0x49))return 14;
  for(i=0;i<60;i++){
   w=nba_setup_spc_resident_peek(&s);if(w.kind>=NBA_SPC_TIMER)break;
   if(w.pc==0x453&&w.kind==NBA_SPC_READ){if(!nba_setup_spc_resident_visible_input(&b,0,(uint8_t)voice))return 15;}
   if(!nba_setup_spc_resident_accept(&s,&b))return 16;
  }
  if(voice==5){if(w.kind!=NBA_SPC_DSP||s.cycles!=53)return 17;}
  else if(w.kind!=NBA_SPC_TIMER||s.cycles!=26||s.a!=5||s.ps!=0x49||s.sp!=254||b.aram[0x100]!=4||b.aram[0x1ff]!=0x4d||b.spc_to_cpu[0]!=0x77)return 18;
  cases++;
 }
 printf("{\"passed\":true,\"arithmetic_and_live_read_cases\":%u,\"pending_DSP_refusals\":%u}\n",cases,refusals);return 0;
}
