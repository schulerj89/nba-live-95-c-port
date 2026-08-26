/* Replay `$85:B0A8-$B128` five-player loose-ball pursuit selection. */
#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned px, py, value[15];
    while (scanf("%x %x", &px, &py) == 2) {
        NbaGameplayLoosePursuitActor actors[5];
        for (unsigned i = 0; i < 5; ++i) {
            if (scanf("%x %x %x", &value[i * 3], &value[i * 3 + 1],
                      &value[i * 3 + 2]) != 3) return 2;
            actors[i].x = (int16_t)(uint16_t)value[i * 3];
            actors[i].y = (int16_t)(uint16_t)value[i * 3 + 1];
            actors[i].control_mode = (uint8_t)value[i * 3 + 2];
        }
        uint8_t modes[5];
        int8_t selected = nba_gameplay_select_no_owner_pursuer(
            actors, (int16_t)(uint16_t)px, (int16_t)(uint16_t)py, modes);
        printf("%04x", (uint16_t)(selected < 0 ? 0xFFFFu :
               (uint16_t)selected));
        for (unsigned i = 0; i < 5; ++i) printf(" %04x", modes[i]);
        putchar('\n');
    }
    return 0;
}
