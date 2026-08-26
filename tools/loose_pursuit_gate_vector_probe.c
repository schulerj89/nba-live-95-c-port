/* Replays `$86:F0FD-$F1AF` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned values[11];
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x", &values[0],
                 &values[1], &values[2], &values[3], &values[4], &values[5],
                 &values[6], &values[7], &values[8], &values[9],
                 &values[10]) == 11) {
        NbaGameplayLoosePursuitGateInput input = {
            (uint16_t)values[0], (uint16_t)values[1], (uint16_t)values[2],
            (uint16_t)values[3], (uint16_t)values[4],
            (int16_t)(uint16_t)values[5], (uint8_t)values[6],
            (uint8_t)values[7], (uint8_t)values[8], (uint8_t)values[9],
            (uint8_t)values[10]
        };
        printf("%04x\n", nba_gameplay_loose_ball_pursuit_allowed(&input) ? 1u : 0u);
    }
    return 0;
}
