#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned controller, boost, held, current;
    while (scanf("%x %x %x %x", &controller, &boost, &held, &current) == 4) {
        printf("%x\n", (uint16_t)nba_gameplay_human_inbound_direction(
            (int8_t)(uint8_t)controller, (uint16_t)boost,
            (uint16_t)held, (int16_t)(uint16_t)current));
    }
    return 0;
}
