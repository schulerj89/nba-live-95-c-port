/* Replays `$85:F347-$F3BA` function vectors through the actual C port.
 * Each input line supplies signed DP `$AA`/`$AE`; outputs are distance in
 * DP `$AA` and the eight-direction result in DP `$B2`. */
#include <stdint.h>
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned dx_raw, dy_raw;
    while (scanf("%x %x", &dx_raw, &dy_raw) == 2) {
        uint16_t distance = 0u;
        uint8_t direction = nba_gameplay_target_direction(
            (int16_t)(uint16_t)dx_raw, (int16_t)(uint16_t)dy_raw,
            &distance);
        printf("%04x %04x\n", distance, (uint16_t)direction);
    }
    return 0;
}
