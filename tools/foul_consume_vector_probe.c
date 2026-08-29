#include <stdio.h>
#include "nba_gameplay_foul.h"
int main(void){unsigned v[16];for(;;){for(unsigned i=0;i<16u;++i)if(scanf("%x",&v[i])!=1)return i?2:0;
 NbaGameplayFoulState s; nba_gameplay_foul_init(&s);s.whistle_active_raw_09b6=v[0];s.foul_event_raw_0964=v[1];s.latched_event_raw_08f0=v[2];
 uint16_t ready=v[3];s.contact_context_raw_497f=v[4];s.presentation_pending_raw_4937=v[5];s.shooting_foul_raw_09bc=v[6];uint16_t bits=v[7];
 s.side_event_bits_raw_13e9=v[9];s.whistle_timer_raw_08de=(int16_t)(uint16_t)v[10];s.presentation_gate_raw_08e2=v[11];s.whistle_state_raw_08e6=v[12];s.whistle_state_mirror_raw_08e8=v[13];
 nba_gameplay_foul_consume_pending(&s,(uint8_t)v[8],&bits,&ready,(v[14]|v[15])!=0);
 printf("%x %x %x %x %x %x %x %x %x %x %x %x %x\n",s.whistle_active_raw_09b6,s.foul_event_raw_0964,s.latched_event_raw_08f0,ready,s.contact_context_raw_497f,s.presentation_pending_raw_4937,s.shooting_foul_raw_09bc,bits,s.side_event_bits_raw_13e9,(uint16_t)s.whistle_timer_raw_08de,s.presentation_gate_raw_08e2,s.whistle_state_raw_08e6,s.whistle_state_mirror_raw_08e8);
 }}
