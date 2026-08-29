#include <stdio.h>
#include "nba_gameplay_foul.h"
int main(void) {
 unsigned v[25];
 for (;;) {
  for (unsigned i=0;i<25u;++i) if(scanf("%x",&v[i])!=1) return i?2:0;
  NbaGameplayFoulState s; nba_gameplay_foul_init(&s);
  s.foul_event_raw_0964=(uint16_t)v[17];s.shooting_foul_raw_09bc=(uint16_t)v[18];
  s.free_throw_state_raw_0978=(uint16_t)v[19];s.free_throw_sequence_raw_097a=(uint16_t)v[20];
  s.whistle_active_raw_09b6=(uint16_t)v[21];s.team_fouls[0]=(uint16_t)v[22];s.team_fouls[1]=(uint16_t)v[23];
  NbaGameplayRng rng={(uint16_t)v[16]};uint16_t bits=(uint16_t)v[24];
  NbaGameplayContactFoulInput in={(uint8_t)v[0],(uint8_t)v[1],(uint8_t)v[2],(uint8_t)v[3],
   (uint8_t)v[4],(uint16_t)v[5],(uint16_t)v[6],(int8_t)(uint8_t)v[7],(int8_t)(uint8_t)v[8],
   (uint8_t)v[9],(uint16_t)v[10],(uint16_t)v[11],(uint16_t)v[12],(uint16_t)v[13],
   (uint16_t)v[14],(uint16_t)v[15]};
  nba_gameplay_foul_classify_contact(&s,&rng,&in,&bits);
  printf("%x %x %x %x %x %x %x %x %x\n",rng.state,s.foul_event_raw_0964,
   s.shooting_foul_raw_09bc,s.free_throw_state_raw_0978,s.free_throw_sequence_raw_097a,
   s.whistle_active_raw_09b6,s.team_fouls[0],s.team_fouls[1],bits);
 }
}
