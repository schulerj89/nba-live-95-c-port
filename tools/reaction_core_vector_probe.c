/* Replays `$85:B971-$B9D1` through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned actor_x, actor_y, ball_x, ball_y, state;
    while (scanf("%x %x %x %x %x", &actor_x, &actor_y, &ball_x,
                 &ball_y, &state) == 5) {
        NbaGameplayRng rng = {(uint16_t)state};
        uint16_t result = nba_gameplay_reaction_threshold(
            &rng, (int16_t)(uint16_t)actor_x, (int16_t)(uint16_t)actor_y,
            (int16_t)(uint16_t)ball_x, (int16_t)(uint16_t)ball_y);
        printf("%04x %04x\n", result, rng.state);
    }
    return 0;
}
