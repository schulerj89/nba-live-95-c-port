/* Replays `$86:E4C7-$E4F3` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ball.h"

int main(void) {
    unsigned anchor, actor_x, pair_motion, distance, pair_direction;
    unsigned dead_ball, catcher_latch, requested;
    while (scanf("%x %x %x %x %x %x %x %x", &anchor, &actor_x,
                 &pair_motion, &distance, &pair_direction, &dead_ball,
                 &catcher_latch, &requested) == 8) {
        uint8_t out_requested = (uint8_t)requested;
        NbaGameplayOwnerProximityResult result =
            nba_gameplay_owner_dribble_proximity(
                (int16_t)(uint16_t)anchor, (int16_t)(uint16_t)actor_x,
                (uint16_t)pair_motion, (uint16_t)distance,
                (uint8_t)pair_direction, (uint16_t)dead_ball,
                (uint16_t)catcher_latch, &out_requested);
        printf("%04x %04x\n", (unsigned)result, (unsigned)out_requested);
    }
    return 0;
}
