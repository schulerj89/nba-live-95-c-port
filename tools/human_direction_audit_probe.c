/* Independent arithmetic observation only; no game state or native fixtures. */
#include "nba_gameplay_ai.h"
#include <stdio.h>
int main(void) {
    unsigned x, y;
    while (scanf_s("%x %x", &x, &y) == 2) {
        uint16_t distance = 0;
        unsigned direction = nba_gameplay_target_direction((int16_t)(uint16_t)x,
                                                          (int16_t)(uint16_t)y,
                                                          &distance);
        printf("%u %u\n", direction, distance);
    }
    return 0;
}
