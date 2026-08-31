#define _CRT_SECURE_NO_WARNINGS
#include "nba_setup_spc_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t data[65552];static NbaSetupSpcResidentBus bus;
static int bytes(FILE*f,uint64_t value,unsigned n){unsigned i;for(i=0;i<n;i++)if(fputc((int)((value>>(8*i))&255),f)==EOF)return 0;return 1;}
static int load(const char*p){FILE*f=fopen(p,"rb");size_t n;if(!f)return 0;n=fread(data,1,sizeof(data),f);if(n!=sizeof(data)||fgetc(f)!=EOF){fclose(f);return 0;}fclose(f);return 1;}
int main(int argc,char**argv){
 NbaSetupSpcInit s;NbaSetupSpcInitWork q;FILE *ins,*writes,*output;unsigned count=0,wc=0;
 if(argc!=5||!load(argv[1]))return 2;
 memcpy(bus.aram,data+16,65536);memcpy(bus.cpu_to_spc,data+7,4);memcpy(bus.spc_to_cpu,data+11,4);bus.dsp_address=data[15];
 if(!nba_setup_spc_init_begin(&s,(uint16_t)(data[0]|((uint16_t)data[1]<<8)),data[2],data[3],data[4],data[5],data[6]))return 3;
 ins=fopen(argv[2],"wb");writes=fopen(argv[3],"wb");if(!ins||!writes)return 4;
 for(count=0;count<1000000;count++){
  if(!s.phase){if(!bytes(ins,s.pc,2)||!bytes(ins,s.a,1)||!bytes(ins,s.x,1)||!bytes(ins,s.y,1)||!bytes(ins,s.sp,1)||!bytes(ins,s.ps,1)||!bytes(ins,s.cycles,8))return 5;}
  q=nba_setup_spc_init_peek(&s);if(q.control_publication||q.bus.kind>=NBA_SPC_TIMER)break;
  if(!nba_setup_spc_init_accept(&s,&bus))return 6;
  if(q.bus.kind==NBA_SPC_WRITE){wc++;if(!bytes(writes,q.bus.pc,2)||!bytes(writes,q.bus.address,2)||!bytes(writes,q.bus.value,1)||!bytes(writes,s.cycles,8))return 7;}
 }
 if(fclose(ins)||fclose(writes))return 8;if(count==1000000)return 9;
 /* Repeated pending acceptance must be side-effect free, including bus. */
 {NbaSetupSpcInit old=s;NbaSetupSpcResidentBus *prior=malloc(sizeof(bus));if(!prior)return 10;*prior=bus;
  if(nba_setup_spc_init_accept(&s,&bus)||memcmp(&old,&s,sizeof(s))||memcmp(prior,&bus,sizeof(bus))){free(prior);return 11;}free(prior);}
 data[0]=(uint8_t)s.pc;data[1]=(uint8_t)(s.pc>>8);data[2]=s.a;data[3]=s.x;data[4]=s.y;data[5]=s.sp;data[6]=s.ps;
 memcpy(data+7,bus.cpu_to_spc,4);memcpy(data+11,bus.spc_to_cpu,4);data[15]=bus.dsp_address;memcpy(data+16,bus.aram,65536);
 output=fopen(argv[4],"wb");if(!output)return 12;if(fwrite(data,1,sizeof(data),output)!=sizeof(data)||fclose(output))return 13;
 printf("{\"pc\":%u,\"phase\":%u,\"cycles\":%llu,\"instructions\":%llu,\"writes\":%u,\"control\":%s}\n",s.pc,s.phase,(unsigned long long)s.cycles,(unsigned long long)s.instructions,wc,q.control_publication?"true":"false");return 0;
}
