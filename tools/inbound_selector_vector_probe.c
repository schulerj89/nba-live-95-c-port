#include <stdio.h>
#include "nba_gameplay_ai.h"
int main(void) {
    unsigned v[55];
    for (;;) {
        for (unsigned i=0;i<55u;++i)
            if (scanf("%x",&v[i])!=1) return i==0u ? 0 : 2;
        NbaGameplayReceiverState actors[10];
        for (unsigned i=0;i<10u;++i) {
            unsigned j=5u+i*5u;
            actors[i].x=(int16_t)(uint16_t)v[j];
            actors[i].y=(int16_t)(uint16_t)v[j+1u];
            actors[i].control_mode=(uint8_t)v[j+2u];
            actors[i].travel_direction=(uint8_t)v[j+3u];
            actors[i].travel_distance=(uint16_t)v[j+4u];
        }
        int16_t selectors[3]={(int16_t)(uint16_t)v[2],
            (int16_t)(uint16_t)v[3],(int16_t)(uint16_t)v[4]};
        printf("%x\n",(uint8_t)nba_gameplay_select_inbound_receiver_cpu(
            (uint8_t)v[0],(uint16_t)v[1],selectors,actors,10u));
    }
}
