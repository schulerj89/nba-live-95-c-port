/* Replays `$86:E923-$E96E` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned x, y, velocity_x, velocity_y, offset_x, offset_y;
    while (scanf("%x %x %x %x %x %x", &x, &y, &velocity_x, &velocity_y,
                 &offset_x, &offset_y) == 6) {
        int16_t target_x = 0, target_y = 0;
        nba_gameplay_target_from_pair(
            (int16_t)(uint16_t)x, (int16_t)(uint16_t)y,
            (int16_t)(uint16_t)velocity_x, (int16_t)(uint16_t)velocity_y,
            (int16_t)(uint16_t)offset_x, (int16_t)(uint16_t)offset_y,
            &target_x, &target_y);
        printf("%04x %04x\n", (uint16_t)target_x, (uint16_t)target_y);
    }
    return 0;
}
