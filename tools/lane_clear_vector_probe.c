/* Replays `$85:F5E4-$F727` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned subject, basket_x;
    while (scanf("%x %x", &subject, &basket_x) == 2) {
        NbaGameplayLaneActor actors[10];
        for (unsigned i = 0; i < 10u; ++i) {
            unsigned x, y, team;
            if (scanf("%x %x %x", &x, &y, &team) != 3) return 1;
            actors[i].x = (int16_t)(uint16_t)x;
            actors[i].y = (int16_t)(uint16_t)y;
            actors[i].team_group = (uint8_t)team;
        }
        bool clear = nba_gameplay_lane_to_basket_clear(
            (uint8_t)subject, (int16_t)(uint16_t)basket_x, actors, 10u);
        printf("%04x\n", clear ? 0u : 1u);
    }
    return 0;
}
