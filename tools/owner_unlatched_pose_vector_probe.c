/* Replays `$86:E545-$E592` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ball.h"

int main(void) {
    unsigned velocity_x, velocity_y, requested, display;
    while (scanf("%x %x %x %x", &velocity_x, &velocity_y,
                 &requested, &display) == 4) {
        uint8_t out_display = (uint8_t)display;
        uint8_t pose = nba_gameplay_owner_unlatched_pose(
            (int16_t)(uint16_t)velocity_x, (int16_t)(uint16_t)velocity_y,
            (uint8_t)requested, &out_display);
        printf("%04x %04x\n", (unsigned)pose, (unsigned)out_display);
    }
    return 0;
}
