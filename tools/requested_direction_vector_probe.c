#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nba_tipoff.h"
#define SIZE 0x4B00u
static uint16_t w(const uint8_t*r,unsigned a){return(uint16_t)(r[a]|(uint16_t)r[a+1]<<8);}
static int32_t f(const uint8_t*r,unsigned a){return(int32_t)(int16_t)w(r,a)*256+(w(r,a-2)>>8);}
int main(void){static uint8_t r[SIZE];_setmode(_fileno(stdin),_O_BINARY);
 while(fread(r,1,SIZE,stdin)==SIZE){NbaTipoff s;memset(&s,0,sizeof(s));unsigned slot=w(r,0xc2),b=0x34eb+slot*0x100;
  if(slot>=10)return 2;NbaTipoffActor*a=&s.actors[slot];a->x_fp=f(r,b+4);a->y_fp=f(r,b+8);a->target_x=(int16_t)w(r,b+0x56);a->target_y=(int16_t)w(r,b+0x58);
  a->velocity_x=(int16_t)w(r,b+0xe);a->velocity_y=(int16_t)w(r,b+0x10);a->lower_animation_state=(uint8_t)w(r,b+0x32);
  a->controller_assignment_raw=(int8_t)(int16_t)w(r,b+0x16);a->movement_direction=(uint8_t)w(r,b+0x4e);a->requested_direction=(uint8_t)w(r,b+0x50);
  s.ball.x_fp=f(r,0x3eef);s.ball.y_fp=f(r,0x3ef3);
  if(!nba_tipoff_replay_requested_direction(&s,(uint8_t)slot))return 3;printf("%04x\n",a->requested_direction);
 }return ferror(stdin)?1:0;}
