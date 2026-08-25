/* Replays mesen_func_vectors.lua RNG vectors through the real C port
 * function. Reads one hex word (the $07F6 entry state) per stdin line and
 * prints the state nba_gameplay_rng_next leaves behind, matching the ROM
 * routine's $07F6 writeback. Build and drive with verify_func_vectors.py. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned state;
    while (scanf("%x", &state) == 1) {
        NbaGameplayRng rng;
        rng.state = (uint16_t)state;
        (void)nba_gameplay_rng_next(&rng);
        printf("%04x\n", rng.state);
    }
    return 0;
}
