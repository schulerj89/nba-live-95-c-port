/* Replay the `$86:E7DC-$E7FC` close-table defense target branch. */
#include <stdint.h>
#include <stdio.h>

#include "nba_gameplay_ai.h"

int main(void) {
    unsigned values[16];
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                 &values[0], &values[1], &values[2], &values[3],
                 &values[4], &values[5], &values[6], &values[7],
                 &values[8], &values[9], &values[10], &values[11],
                 &values[12], &values[13], &values[14], &values[15]) == 16) {
        NbaGameplayDefenseTargetInput input = {
            .actor_x = (int16_t)(uint16_t)values[1],
            .actor_y = (int16_t)(uint16_t)values[2],
            .actor_pair_direction_raw_86 = (uint8_t)values[3],
            .actor_pair_distance_raw_8a = (uint16_t)values[4],
            .paired_x = (int16_t)(uint16_t)values[5],
            .paired_y = (int16_t)(uint16_t)values[6],
            .paired_velocity_x = (int16_t)(uint16_t)values[7],
            .paired_velocity_y = (int16_t)(uint16_t)values[8],
            .paired_anchor_direction_raw_88 = (uint8_t)values[9],
            .paired_anchor_distance_raw_8c = (uint16_t)values[10],
            .paired_position_raw_92 = (uint16_t)values[11],
            .context_anchor_x = (int16_t)(uint16_t)values[12],
            .context_mode_raw_30 = (uint16_t)values[13],
            .context_flags_raw_32 = (uint16_t)values[14],
            .paired_on_three_point_arc = values[15] != 0u,
            .paired_three_point_rating = 0u
        };
        NbaGameplayDefenseTargetOutput output = {0};
        if (!nba_gameplay_defense_mode_target(
                (uint8_t)values[0], &input, &output) ||
            !output.target_written) return 2;
        printf("%04x %04x\n", (uint16_t)output.target_x,
               (uint16_t)output.target_y);
    }
    return ferror(stdin) ? 1 : 0;
}
