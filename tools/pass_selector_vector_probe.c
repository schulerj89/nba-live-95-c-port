/* Replays `$85:B50E-$B5FE` through the actual C port. */
#include <stdio.h>
#include <string.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned passer, special, selector0, selector1, selector2, attack_right;
    for (;;) {
        NbaGameplayReceiverState actors[10];
        memset(actors, 0, sizeof(actors));
        if (scanf("%x %x %x %x %x %x", &passer, &special, &selector0,
                  &selector1, &selector2, &attack_right) != 6) break;
        for (unsigned i = 0; i < 10u; ++i) {
            unsigned x, y, mode, direction, distance;
            if (scanf("%x %x %x %x %x", &x, &y, &mode, &direction,
                      &distance) != 5) return 2;
            actors[i].x = (int16_t)(uint16_t)x;
            actors[i].y = (int16_t)(uint16_t)y;
            actors[i].control_mode = (uint8_t)mode;
            actors[i].travel_direction = (uint8_t)direction;
            actors[i].travel_distance = (uint16_t)distance;
        }
        const int16_t selectors[3] = {
            (int16_t)(uint16_t)selector0, (int16_t)(uint16_t)selector1,
            (int16_t)(uint16_t)selector2
        };
        int8_t selected = nba_gameplay_select_pass_receiver(
            (uint8_t)passer, (int16_t)(uint16_t)special, selectors, actors,
            10u, attack_right != 0u);
        printf("%04x\n", (uint16_t)(int16_t)selected);
    }
    return 0;
}
