/* Replays `$85:B734-$B820` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned step, cycle, hold, dead_ball, shot_clock, difficulty;
    unsigned assignment, anchor, two, three, range, rng_state;
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x %x", &step,
                 &cycle, &hold, &dead_ball, &shot_clock, &difficulty,
                 &assignment, &anchor, &two, &three, &range, &rng_state) == 12) {
        NbaGameplayMode11ShotInput input = {
            (int16_t)(uint16_t)step, (uint16_t)cycle, (uint16_t)hold,
            (uint16_t)dead_ball, (uint16_t)shot_clock, (uint16_t)difficulty,
            (uint16_t)assignment, (uint16_t)anchor, (uint8_t)two,
            (uint8_t)three, (uint8_t)range, 0, true
        };
        NbaGameplayRng rng = {(uint16_t)rng_state};
        bool accepted = nba_gameplay_mode11_shot_decision(&input, &rng);
        printf("%04x %04x\n", accepted ? 1u : 0u, rng.state);
    }
    return 0;
}
