#include "nba_gameplay_ai.h"
#include <stdio.h>

int main(void) {
    unsigned v[21];
    while (scanf_s("%x", &v[0]) == 1) {
        for (unsigned i = 1; i < 21; ++i)
            if (scanf_s("%x", &v[i]) != 1) return 2;
        NbaGameplayDefensivePoseInput in = {
            (int16_t)v[1], (uint16_t)v[2], (uint16_t)v[3],
            (int16_t)v[4], (int16_t)v[5], (int16_t)v[6], (int16_t)v[7],
            (uint8_t)v[8], (uint16_t)v[9], (uint16_t)v[10],
            (uint16_t)v[11], (uint8_t)v[12], (uint16_t)v[13],
            (uint16_t)v[14], (int16_t)v[15], (int16_t)v[16],
            (uint8_t)v[17], (uint8_t)v[18], 0u, 0u, (uint16_t)v[20]
        };
        /* Facing/requested share the native +4E/+50 input in normalized
         * vectors; callers that need distinct values use controlled cases. */
        in.requested_direction_raw_50 = (uint8_t)(v[19] >> 8);
        in.facing_raw_4e = (uint8_t)v[19];
        NbaGameplayDefensivePoseOutput out;
        bool ok = v[0] ? nba_gameplay_defensive_pose(&in, &out) :
                         nba_gameplay_stationary_defensive_pose(&in, &out);
        if (!ok) return 3;
        printf("%02x %02x %02x %04x %02x %u %02x\n",
               out.base_state_raw_38, out.facing_raw_4e,
               out.requested_direction_raw_50,
               out.selected_count_raw_1868, out.selector_result_raw_aa,
               out.install_both ? 1u : 0u, out.install_state);
    }
    return 0;
}
