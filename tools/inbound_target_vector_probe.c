/* Replay `$85:C37D-$C5C0` through the portable inbound-target routine. */
#include <stdint.h>
#include <stdio.h>

#include "nba_gameplay_ai.h"

int main(void) {
    unsigned layout, source_x, source_y, anchor_x, ball_x, rng_state;
    while (scanf("%x %x %x %x %x %x", &layout, &source_x, &source_y,
                 &anchor_x, &ball_x, &rng_state) == 6) {
        NbaGameplayRng rng = {(uint16_t)rng_state};
        NbaGameplayInboundTarget target = {0};
        if (!nba_gameplay_inbound_target(
                (int16_t)(uint16_t)layout,
                (int16_t)(uint16_t)source_x,
                (int16_t)(uint16_t)source_y,
                (int16_t)(uint16_t)anchor_x,
                (int16_t)(uint16_t)ball_x, &rng, &target)) return 2;
        printf("%04x %04x %04x %04x %04x %04x\n",
               (uint16_t)target.x, (uint16_t)target.y,
               (uint16_t)target.direction, (uint16_t)target.play_code,
               target.play_requested ? 1u : 0u, rng.state);
    }
    return ferror(stdin) ? 1 : 0;
}
