/* Replays `$87:B572-$B5ED` state-table selection through production C. */
#include <stdio.h>
#include "nba_player_lab.h"

int main(void) {
    unsigned state, stationary, boosted, owner, airborne;
    while (scanf_s("%x %x %x %x %x", &state, &stationary, &boosted,
                   &owner, &airborne) == 5)
        printf("%02x\n", nba_player_locomotion_state(
            (uint8_t)state, stationary != 0u, boosted != 0u,
            owner != 0u, airborne != 0u));
    return 0;
}
