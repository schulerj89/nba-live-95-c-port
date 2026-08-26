/* Replays `$86:E4A7-$E4C4` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ball.h"

int main(void) {
    unsigned z, free_throw, live_state, movement;
    while (scanf("%x %x %x %x", &z, &free_throw, &live_state,
                 &movement) == 4)
        printf("%04x\n", (unsigned)nba_gameplay_owner_dribble_gate(
            (int16_t)(uint16_t)z, (uint16_t)free_throw,
            (uint16_t)live_state, (uint16_t)movement));
    return 0;
}
