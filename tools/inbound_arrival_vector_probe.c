#include <stdio.h>
#include "nba_gameplay_ai.h"
int main(void) {
    unsigned v[12];
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x %x",
        &v[0],&v[1],&v[2],&v[3],&v[4],&v[5],&v[6],&v[7],&v[8],
        &v[9],&v[10],&v[11])==12) {
        NbaGameplayInboundArrival s={
            (uint16_t)v[0],(uint16_t)v[1],(uint16_t)v[2],
            (int16_t)(uint16_t)v[3],(int16_t)(uint16_t)v[4],
            (uint16_t)v[5],(uint16_t)v[6],(uint16_t)v[7],
            (uint16_t)v[8],(int16_t)(uint16_t)v[9],(uint16_t)v[10],
            (uint8_t)v[11]};
        nba_gameplay_inbound_arrival_prepare(&s);
        printf("%x %x %x %x %x %x %x %x %x %x\n",
            s.dead_ball_raw_0968,s.attachment_raw_09f6,
            s.behavior_flags_raw_7e,(uint16_t)s.velocity_x_raw_0e,
            (uint16_t)s.velocity_y_raw_10,s.inbound_ready_raw_09ba,
            s.whistle_raw_09b6,s.foul_event_raw_0964,s.transfer_raw_09b8,
            s.draw_direction_raw_4e);
    }
    return 0;
}
