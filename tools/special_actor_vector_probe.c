/* Replays `$85:B4B9-$B50D` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned timer, mode, cycle, possession, lane, distance, actor, special;
    while (scanf("%x %x %x %x %x %x %x %x", &timer, &mode, &cycle,
                 &possession, &lane, &distance, &actor, &special) == 8) {
        uint16_t out_timer = (uint16_t)timer;
        uint16_t out_special = (uint16_t)special;
        nba_gameplay_special_actor_step(
            &out_timer, (uint8_t)mode, (uint16_t)cycle, possession != 0u,
            lane != 0u, (uint16_t)distance, (uint8_t)actor, &out_special);
        printf("%04x %04x\n", out_timer, out_special);
    }
    return 0;
}
