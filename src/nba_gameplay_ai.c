#include "nba_gameplay_ai.h"
#include <stdlib.h>

static int16_t arithmetic_shift_right_3(int16_t value) {
    if (value >= 0) return (int16_t)(value >> 3);
    return (int16_t)(-(((-(int)value) + 7) >> 3));
}

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

/* `$86:E923-$E96E`: predict the paired actor by one eighth of signed
 * velocity, then add the caller-selected formation-table displacement.
 * The 65816 stores the wrapped 16-bit results in actor +$56/+$58. */
void nba_gameplay_target_from_pair(int16_t paired_x, int16_t paired_y,
                                   int16_t paired_velocity_x,
                                   int16_t paired_velocity_y,
                                   int16_t offset_x, int16_t offset_y,
                                   int16_t *target_x, int16_t *target_y) {
    if (target_x)
        *target_x = (int16_t)(paired_x +
            arithmetic_shift_right_3(paired_velocity_x) + offset_x);
    if (target_y)
        *target_y = (int16_t)(paired_y +
            arithmetic_shift_right_3(paired_velocity_y) + offset_y);
}

bool nba_gameplay_ai_self_test(void) {
    int16_t x = 0, y = 0;
    nba_gameplay_target_from_pair(100, -50, 31, -31, -20, 12, &x, &y);
    if (x != 83 || y != -42) return false;
    nba_gameplay_target_from_pair(32760, -32760, 64, -64, 16, -16, &x, &y);
    if (x != -32752 || y != 32752) return false;
    uint16_t timer = 49u;
    if (nba_gameplay_decision_timer_step(&timer, 15u, 0x40u, false) ||
        timer != 17u) return false;
    if (!nba_gameplay_decision_timer_step(&timer, 15u, 0x40u, false) ||
        timer != 64u) return false;
    timer = 20u;
    if (!nba_gameplay_decision_timer_step(&timer, 11u, 0x20u, true) ||
        timer != 63u) return false;
    timer = 20u;
    return nba_gameplay_decision_timer_step(&timer, 11u, 0x30u, false) &&
           timer == 47u;
}

/* Modes 1-6 at `$86:F1CF/$F25E/$F6EE/$F7AA/$F8D8` subtract DP `$C8`.
 * Live dispatch proves `$C8=$20`; signed values <=0 reload from the mode's
 * exact base, its ROM profile byte, and an optional same-half `$20`. */
bool nba_gameplay_decision_timer_step(uint16_t *timer, uint8_t profile_byte,
                                      uint16_t reload_base,
                                      bool add_half_court_delay) {
    if (!timer) return false;
    int16_t remaining = (int16_t)(*timer - 0x20u);
    if (remaining > 0) {
        *timer = (uint16_t)remaining;
        return false;
    }
    *timer = (uint16_t)(remaining + (int)reload_base + profile_byte +
                        (add_half_court_delay ? 0x20 : 0));
    return true;
}
