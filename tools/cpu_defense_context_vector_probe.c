/* Replay native `$85:B130-$B176` defense-context witnesses through the
 * production RNG and CPU/AI helper. */
#include <stdint.h>
#include <stdio.h>

#include "nba_gameplay_ai.h"

int main(void) {
    unsigned current, opponent, period, activity, seed, initial;
    while (scanf_s("%x %x %x %x %x %x", &current, &opponent, &period,
                   &activity, &seed, &initial) == 6) {
        NbaGameplayRng rng = {(uint16_t)seed};
        uint16_t mode = (uint16_t)initial;
        uint16_t random_word = nba_gameplay_rng_next(&rng);
        bool selected = nba_gameplay_defense_context_reselect(
            (uint16_t)current, (uint16_t)opponent, (uint16_t)period,
            (uint16_t)activity, random_word, &mode);
        printf("%04x %u %04x %04x\n", random_word,
               selected ? 1u : 0u, mode, rng.state);
    }
    return ferror(stdin) ? 1 : 0;
}
