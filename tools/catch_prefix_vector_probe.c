/* Replays normalized `$86:BAA2-$BAFA` player-grab vectors through the
 * actual C port. Inputs begin with catcher/controller, followed by the 19
 * mutable fields emitted in the same order as the output. */
#include <stdint.h>
#include <stdio.h>
#include "nba_gameplay_ball.h"

int main(void) {
    unsigned v[21];
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                 &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6],
                 &v[7], &v[8], &v[9], &v[10], &v[11], &v[12], &v[13],
                 &v[14], &v[15], &v[16], &v[17], &v[18], &v[19],
                 &v[20]) == 21) {
        NbaGameplayCatchPrefixState s = {
            .catcher = (uint8_t)v[0],
            .controller_actor = (int16_t)(uint16_t)v[1],
            .velocity_x = (int16_t)(uint16_t)v[2],
            .velocity_y = (int16_t)(uint16_t)v[3],
            .movement_magnitude = (uint16_t)v[4],
            .catcher_latch = (uint16_t)v[5],
            .rim_force_raw_1866 = (uint16_t)v[6],
            .dead_ball_raw_0968 = (uint16_t)v[7],
            .rim_raw_096a = (uint16_t)v[8],
            .context_actor_raw_3f = (uint16_t)v[9],
            .context_controller_raw_41 = (int16_t)(uint16_t)v[10],
            .context_previous_actor_raw_43 = (uint16_t)v[11],
            .context_previous_controller_raw_45 = (int16_t)(uint16_t)v[12],
            .special_actor_raw_09a2 = (int16_t)(uint16_t)v[13],
            .play_aux_raw_09a6 = (int16_t)(uint16_t)v[14],
            .play_selector_raw = {
                (int16_t)(uint16_t)v[15], (int16_t)(uint16_t)v[16],
                (int16_t)(uint16_t)v[17]
            },
            .possession_actor_raw_093e = (int16_t)(uint16_t)v[18],
            .actor_record_raw_0910 = (uint16_t)v[19],
            .context_record_raw_0912 = (uint16_t)v[20],
        };
        nba_gameplay_apply_catch_prefix(&s);
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x "
               "%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
               (uint16_t)s.velocity_x, (uint16_t)s.velocity_y,
               s.movement_magnitude, s.catcher_latch,
               s.rim_force_raw_1866, s.dead_ball_raw_0968, s.rim_raw_096a,
               s.context_actor_raw_3f, (uint16_t)s.context_controller_raw_41,
               s.context_previous_actor_raw_43,
               (uint16_t)s.context_previous_controller_raw_45,
               (uint16_t)s.special_actor_raw_09a2,
               (uint16_t)s.play_aux_raw_09a6,
               (uint16_t)s.play_selector_raw[0],
               (uint16_t)s.play_selector_raw[1],
               (uint16_t)s.play_selector_raw[2],
               (uint16_t)s.possession_actor_raw_093e,
               s.actor_record_raw_0910, s.context_record_raw_0912);
    }
    return 0;
}
