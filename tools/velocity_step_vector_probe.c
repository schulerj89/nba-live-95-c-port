/* Replays `$85:A82C-$AB16` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned vx, vy, boost, direction, profile, dt, blocked, global_actor;
    while (scanf("%x %x %x %x %x %x %x %x", &vx, &vy, &boost,
                 &direction, &profile, &dt, &blocked, &global_actor) == 8) {
        int16_t out_x = (int16_t)(uint16_t)vx;
        int16_t out_y = (int16_t)(uint16_t)vy;
        uint16_t out_boost = (uint16_t)boost;
        nba_gameplay_velocity_step(
            &out_x, &out_y, &out_boost, (uint8_t)direction,
            (uint8_t)profile, (uint16_t)dt, blocked != 0u,
            (int16_t)(uint16_t)global_actor);
        printf("%04x %04x %04x\n", (uint16_t)out_x,
               (uint16_t)out_y, out_boost);
    }
    return 0;
}
