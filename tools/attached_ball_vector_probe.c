/* Replay the live phase>=3 `$85:A4F2-$A517/$85:A532-$A597` path. */
#include <stdint.h>
#include <stdio.h>

#include "nba_gameplay_ball.h"

int main(void) {
    unsigned state_09f6, dead_0968, velocity, fraction, z, impact, bits;
    while (scanf("%x %x %x %x %x %x %x", &state_09f6, &dead_0968,
                 &velocity, &fraction, &z, &impact, &bits) == 7) {
        NbaGameplayAttachedVerticalState state = {
            (uint16_t)state_09f6, (uint16_t)dead_0968,
            (int16_t)(uint16_t)velocity, (uint16_t)fraction,
            (int16_t)(uint16_t)z, (uint16_t)impact, (uint16_t)bits
        };
        nba_gameplay_ball_apply_attached_vertical(&state);
        printf("%04x %04x %04x %04x %04x %04x %04x\n",
               state.attachment_state_raw_09f6, state.dead_ball_raw_0968,
               (uint16_t)state.velocity_z, state.z_fraction,
               (uint16_t)state.z, state.impact_raw_13e5,
               state.event_bits_raw_13e7);
    }
    return ferror(stdin) ? 1 : 0;
}
