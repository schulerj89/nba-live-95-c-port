/* Replays `$85:F1C1-$F228` function vectors through the actual C port.
 * Each input line supplies signed DP `$AA`/`$AE`; output is the distance
 * word the ROM returns in DP `$AA`. */
#include <stdint.h>
#include <stdio.h>
#include "nba_gameplay_ball.h"

int main(void) {
    unsigned dx_raw, dy_raw;
    while (scanf("%x %x", &dx_raw, &dy_raw) == 2) {
        uint16_t distance = nba_gameplay_hoop_distance(
            (int16_t)(uint16_t)dx_raw, (int16_t)(uint16_t)dy_raw);
        printf("%04x\n", distance);
    }
    return 0;
}
