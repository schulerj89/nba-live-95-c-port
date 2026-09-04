/* Replays `$85:B3C9-$85:B401` direct arrival/velocity calls through compiled C. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned x, y, vx, vy, target_x, target_y, tolerance;
    unsigned boost, profile, dt, blocked, global_actor;
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x %x",
                 &x, &y, &vx, &vy, &target_x, &target_y, &tolerance,
                 &boost, &profile, &dt, &blocked, &global_actor) == 12) {
        int16_t out_x = (int16_t)(uint16_t)vx;
        int16_t out_y = (int16_t)(uint16_t)vy;
        uint16_t out_boost = (uint16_t)boost;
        uint8_t steering = 8u;
        bool arrived = nba_gameplay_direct_arrival(
            (int16_t)(uint16_t)x, (int16_t)(uint16_t)y,
            (int16_t)(uint16_t)target_x, (int16_t)(uint16_t)target_y,
            (uint16_t)tolerance, &steering, NULL);
        nba_gameplay_velocity_step(
            &out_x, &out_y, &out_boost, steering, (uint8_t)profile,
            (uint16_t)dt, blocked != 0u,
            (int16_t)(uint16_t)global_actor);
        printf("%04x %04x %04x %04x %04x\n",
               arrived ? 1u : 0u, (unsigned)steering,
               (uint16_t)out_x, (uint16_t)out_y, out_boost);
    }
    return 0;
}
