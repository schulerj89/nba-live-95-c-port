/* Replays `$86:F45F-$F4E5 -> $85:A82C` through the production C helper. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned ax, ay, tx, ty, vx, vy, boost, profile, dt, blocked, owner;
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x",
                 &ax, &ay, &tx, &ty, &vx, &vy, &boost, &profile,
                 &dt, &blocked, &owner) == 11) {
        NbaGameplayInboundMotion motion = {
            .actor_x = (int16_t)(uint16_t)ax,
            .actor_y = (int16_t)(uint16_t)ay,
            .target_x = (int16_t)(uint16_t)tx,
            .target_y = (int16_t)(uint16_t)ty,
            .velocity_x = (int16_t)(uint16_t)vx,
            .velocity_y = (int16_t)(uint16_t)vy,
            .boost_timer = (uint16_t)boost,
            .profile_42 = (uint8_t)profile,
            .dispatch_dt = (uint16_t)dt,
            .movement_blocked = blocked != 0u,
            .owner_actor_raw_093e = (int16_t)(uint16_t)owner
        };
        nba_gameplay_inbound_motion_step(&motion);
        printf("%04x %04x %04x %04x\n",
               (uint16_t)motion.velocity_x, (uint16_t)motion.velocity_y,
               motion.boost_timer, (uint16_t)motion.direction);
    }
    return 0;
}
