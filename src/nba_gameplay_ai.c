#include "nba_gameplay_ai.h"
#include <stdlib.h>

void nba_gameplay_rng_seed(NbaGameplayRng *rng, uint16_t seed) {
    rng->state = seed;
}

/* `$80:CEE7`, state `$07F6`: 16-bit left-shift LFSR with the ROM's zero
 * recovery seed and feedback polynomial. */
uint16_t nba_gameplay_rng_next(NbaGameplayRng *rng) {
    uint16_t old = rng->state;
    if (old == 0u) {
        rng->state = 0x9146u;
    } else {
        rng->state = (uint16_t)(old << 1);
        if (old & 0x8000u) rng->state ^= 0x1D87u;
    }
    return rng->state;
}

bool nba_gameplay_rng_self_test(void) {
    static const uint16_t expected[] = {
        0x3F0Bu, 0x7E16u, 0xFC2Cu, 0xE5DFu, 0xD639u, 0xB1F5u,
        0x7E6Du, 0xFCDAu, 0xE433u, 0xD5E1u, 0xB645u, 0x710Du
    };
    NbaGameplayRng rng = { 0x9146u };
    for (unsigned i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i)
        if (nba_gameplay_rng_next(&rng) != expected[i]) return false;
    rng.state = 0u;
    return nba_gameplay_rng_next(&rng) == 0x9146u;
}

/* `$85:B95C-$B9D1`: max(|dx|,|dy|) + min/4, randomized by the upper four
 * useful RNG bits and clamped to `$96`. */
uint16_t nba_gameplay_reaction_threshold(NbaGameplayRng *rng,
                                         int16_t actor_x, int16_t actor_y,
                                         int16_t ball_x, int16_t ball_y) {
    unsigned dx = (unsigned)abs((int)ball_x - actor_x);
    unsigned dy = (unsigned)abs((int)ball_y - actor_y);
    unsigned high = dx > dy ? dx : dy;
    unsigned low = dx > dy ? dy : dx;
    unsigned result = high + (low >> 2) +
                      (nba_gameplay_rng_next(rng) & 0x78u);
    return (uint16_t)(result > 0x96u ? 0x96u : result);
}

/* `$87:9244 -> $87:9BD3 -> $87:9BD0` behavior-mode jump table. */
uint32_t nba_gameplay_behavior_routine(uint8_t mode) {
    static const uint32_t targets[18] = {
        0x879C1Bu, 0x86F1B0u, 0x86F6CDu, 0x86F23Fu, 0x86F794u,
        0x86F2CAu, 0x86F8CDu, 0x86994Cu, 0x86C6ADu, 0x86F0B7u,
        0x86A5B0u, 0x86F34Fu, 0x86B769u, 0x86A7DAu, 0x86B154u,
        0x86A6B3u, 0x86B0F7u, 0x86B979u
    };
    return mode < 18u ? targets[mode] : 0u;
}
