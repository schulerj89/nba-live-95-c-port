/* Replays `$85:B60B-$B677` through the actual C port. */
#include <stdio.h>
#include <string.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned passer, candidate;
    unsigned px, py, pmode, pdir, pdistance;
    unsigned cx, cy, cmode, cdir, cdistance;
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x %x",
                 &passer, &candidate, &px, &py, &pmode, &pdir, &pdistance,
                 &cx, &cy, &cmode, &cdir, &cdistance) == 12) {
        NbaGameplayReceiverState actors[10];
        memset(actors, 0, sizeof(actors));
        if (passer < 10u) {
            actors[passer].x = (int16_t)(uint16_t)px;
            actors[passer].y = (int16_t)(uint16_t)py;
            actors[passer].control_mode = (uint8_t)pmode;
            actors[passer].travel_direction = (uint8_t)pdir;
            actors[passer].travel_distance = (uint16_t)pdistance;
        }
        if (candidate < 10u) {
            actors[candidate].x = (int16_t)(uint16_t)cx;
            actors[candidate].y = (int16_t)(uint16_t)cy;
            actors[candidate].control_mode = (uint8_t)cmode;
            actors[candidate].travel_direction = (uint8_t)cdir;
            actors[candidate].travel_distance = (uint16_t)cdistance;
        }
        printf("%04x\n", nba_gameplay_receiver_candidate_valid(
            (uint8_t)passer, (uint8_t)candidate, actors, 10u) ? 1u : 0u);
    }
    return 0;
}
