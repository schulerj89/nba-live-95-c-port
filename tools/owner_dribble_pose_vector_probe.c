/* Replays `$86:E593-$E5AA` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ball.h"

int main(void) {
    unsigned dead_ball, catcher_latch;
    while (scanf("%x %x", &dead_ball, &catcher_latch) == 2)
        printf("%04x\n", (unsigned)nba_gameplay_owner_dribble_fallback_pose(
            (uint16_t)dead_ball, (uint16_t)catcher_latch));
    return 0;
}
