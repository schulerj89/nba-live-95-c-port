/* Replays `$85:F3C3-$F472` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned dx, dy;
    while (scanf("%x %x", &dx, &dy) == 2) {
        uint16_t distance = 0u;
        uint8_t direction = nba_gameplay_pass_direction(
            (int16_t)(uint16_t)dx, (int16_t)(uint16_t)dy, &distance);
        printf("%04x %04x\n", distance, (uint16_t)direction);
    }
    return 0;
}
