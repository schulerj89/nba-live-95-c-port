/* Replays `$87:A52C-$A5FA` through the production presentation selector. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned current, mode, status, upper, anchor, valid, dx, dy;
    while (scanf("%x %x %x %x %x %x %x %x", &current, &mode, &status,
                 &upper, &anchor, &valid, &dx, &dy) == 8) {
        NbaGameplayDrawDirection input = {
            .current_direction = (uint8_t)current,
            .control_mode = (uint8_t)mode,
            .actor_status = (uint16_t)status,
            .upper_state = (uint16_t)upper,
            .anchor_direction = (uint16_t)anchor,
            .candidate_valid = valid != 0u,
            .candidate_dx = (int16_t)(uint16_t)dx,
            .candidate_dy = (int16_t)(uint16_t)dy,
        };
        printf("%x\n", nba_gameplay_draw_direction(&input));
    }
    return 0;
}
