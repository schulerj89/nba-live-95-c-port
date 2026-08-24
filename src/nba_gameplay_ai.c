#include "nba_gameplay_ai.h"
#include <stdlib.h>

static int16_t arithmetic_shift_right_3(int16_t value) {
    if (value >= 0) return (int16_t)(value >> 3);
    return (int16_t)(-(((-(int)value) + 7) >> 3));
}

static int16_t wrap16(int32_t value) {
    return (int16_t)(uint16_t)value;
}

static int16_t trunc_div_pow2(int16_t value, unsigned shift) {
    uint16_t magnitude = value < 0 ?
        (uint16_t)(0u - (uint16_t)value) : (uint16_t)value;
    int16_t quotient = (int16_t)(magnitude >> shift);
    return value < 0 ? (int16_t)(uint16_t)(0u - (uint16_t)quotient) : quotient;
}

static uint16_t magnitude16(int16_t value) {
    return value < 0 ? (uint16_t)(0u - (uint16_t)value) : (uint16_t)value;
}

static bool subtract16_is_negative(uint16_t left, uint16_t right) {
    return ((uint16_t)(left - right) & 0x8000u) != 0u;
}

/* `$85:F34F-$F3B7`: exact direction key consumed by `$87:B832`, plus the
 * major+minor/4 distance. The
 * y<=x+1 swap and diagonal boundary intentionally differ from a conventional
 * octant quantizer. */
uint8_t nba_gameplay_target_direction(int16_t dx, int16_t dy,
                                      uint16_t *distance) {
    static const uint8_t direction_map[16] = {
        0,1,2,1, 4,3,2,3, 0,7,6,7, 4,5,6,5
    };
    if (dx == 0 && dy == 0) {
        if (distance) *distance = 0u;
        return 8u;
    }
    uint16_t key = 0u;
    uint16_t x = (uint16_t)dx, y = (uint16_t)dy;
    if (dx < 0) { x = (uint16_t)(0u - x); key |= 8u; }
    if (dy < 0) { y = (uint16_t)(0u - y); key |= 4u; }
    if ((int16_t)(uint16_t)(y - 1u) <= (int16_t)x) {
        uint16_t swap = x; x = y; y = swap; key |= 2u;
    }
    uint16_t doubled_x = (uint16_t)(x << 1);
    if ((int16_t)(uint16_t)(y - 1u) < (int16_t)doubled_x) key |= 1u;
    if (distance) *distance = (uint16_t)(y + (doubled_x >> 3));
    return direction_map[key];
}

/* `$85:F3C3-$F472`: the pass/catch initializer's 16-direction quantizer.
 * `$B2` returns the fine direction and `$AA` returns major+minor/4. The
 * instruction stream deliberately uses three slope bands and a 32-byte ROM
 * octant/sign map; it is not interchangeable with `$85:F34F`. */
uint8_t nba_gameplay_pass_direction(int16_t dx, int16_t dy,
                                    uint16_t *distance) {
    static const uint8_t direction_map[32] = {
        0,1,2,0xFF, 4,3,2,0xFF, 8,7,6,0xFF, 4,5,6,0xFF,
        0,15,14,0xFF, 12,13,14,0xFF, 8,9,10,0xFF, 12,11,10,0xFF
    };
    if (dx == 0 && dy == 0) {
        if (distance) *distance = 0u;
        return 0x10u; /* `$85:F3BB-$F3C2`, unlike `$85:F34F`'s 8. */
    }
    uint16_t key = 0u;
    uint16_t low = (uint16_t)dx, high = (uint16_t)dy;
    if (dx < 0) { low = (uint16_t)(0u - low); key |= 0x10u; }
    if (dy < 0) { high = (uint16_t)(0u - high); key |= 0x08u; }
    if (subtract16_is_negative((uint16_t)(high - 1u), low)) {
        uint16_t swap = low; low = high; high = swap; key |= 0x04u;
    }
    uint16_t five_low = (uint16_t)(low * 5u);
    if (subtract16_is_negative((uint16_t)(high - 1u), five_low)) {
        uint16_t three_half_low = (uint16_t)(low * 3u);
        three_half_low >>= 1;
        key |= subtract16_is_negative((uint16_t)(high - 1u), three_half_low) ?
               0x02u : 0x01u;
    }
    if (distance) *distance = (uint16_t)(high + (low >> 2));
    return direction_map[key];
}

/* `$85:B60B-$B677`: reject self, passive/special executor modes, a zero
 * direction vector, and the two head-on low-motion conflicts. Carry clear
 * is the accepting return in the original routine. */
bool nba_gameplay_receiver_candidate_valid(
    uint8_t passer_actor, uint8_t candidate_actor,
    const NbaGameplayReceiverState *actors, uint8_t actor_count) {
    if (!actors || passer_actor >= actor_count ||
        candidate_actor >= actor_count || candidate_actor == passer_actor)
        return false;
    const NbaGameplayReceiverState *passer = &actors[passer_actor];
    const NbaGameplayReceiverState *candidate = &actors[candidate_actor];
    if (candidate->control_mode >= 7u) return false;
    uint8_t direction = nba_gameplay_target_direction(
        (int16_t)(candidate->x - passer->x),
        (int16_t)(candidate->y - passer->y), NULL);
    if (direction == 8u) return false;
    if (direction == passer->travel_direction &&
        passer->travel_distance < 0x20u) return false;
    if ((uint8_t)(direction ^ 4u) == candidate->travel_direction &&
        candidate->travel_distance < 0x20u) return false;
    return true;
}

static bool receiver_is_in_forward_window(
    const NbaGameplayReceiverState *passer,
    const NbaGameplayReceiverState *receiver, bool attack_right) {
    /* `$85:B5B0-$B5D9`: normalize X by team direction and reject only a
     * receiver 201 or more units ahead. Signed values behind the passer are
     * deliberately retained, matching CMP #$00C9/BPL. */
    int16_t delta = attack_right ? (int16_t)(receiver->x - passer->x) :
                                   (int16_t)(passer->x - receiver->x);
    return delta < 0x00C9;
}

/* `$85:B50E-$B60A`: `$09A2` has priority. A negative `$09AA` or one naming
 * the owner returns immediately. Otherwise AA, AC and AE are tried in order,
 * with each later selector reached only when `$85:B60B` rejects the prior
 * candidate. None of these selector words is consumed by this routine. */
int8_t nba_gameplay_select_pass_receiver(
    uint8_t passer_actor, int16_t special_actor,
    const int16_t selectors[3], const NbaGameplayReceiverState *actors,
    uint8_t actor_count, bool attack_right) {
    if (!actors || !selectors || passer_actor >= actor_count) return -1;
    if (special_actor >= 0 && special_actor < actor_count &&
        actors[special_actor].control_mode != 8u &&
        receiver_is_in_forward_window(&actors[passer_actor],
                                      &actors[special_actor], attack_right))
        return (int8_t)special_actor;
    if (selectors[0] < 0 || selectors[0] == passer_actor) return -1;
    for (unsigned i = 0; i < 3u; ++i) {
        int16_t candidate = selectors[i];
        if (candidate < 0 || candidate >= actor_count) continue;
        if (nba_gameplay_receiver_candidate_valid(
                passer_actor, (uint8_t)candidate, actors, actor_count) &&
            receiver_is_in_forward_window(&actors[passer_actor],
                                          &actors[candidate], attack_right))
            return (int8_t)candidate;
    }
    return -1;
}

/* `$85:B402-$B4B8`: predict target residual by velocity/8 with the ROM's
 * negative-quotient +1 bias, then use the inclusive caller tolerance. */
bool nba_gameplay_predictive_arrival(int16_t actor_x, int16_t actor_y,
                                     int16_t velocity_x, int16_t velocity_y,
                                     int16_t target_x, int16_t target_y,
                                     uint16_t tolerance,
                                     uint8_t *steering_direction,
                                     uint16_t *distance) {
    int16_t bias_x = arithmetic_shift_right_3(velocity_x);
    int16_t bias_y = arithmetic_shift_right_3(velocity_y);
    if (bias_x < 0) bias_x = wrap16((int32_t)bias_x + 1);
    if (bias_y < 0) bias_y = wrap16((int32_t)bias_y + 1);
    int16_t dx = wrap16((int32_t)target_x - actor_x - bias_x);
    int16_t dy = wrap16((int32_t)target_y - actor_y - bias_y);
    uint16_t predicted_distance = 0u;
    uint8_t direction = nba_gameplay_target_direction(
        dx, dy, &predicted_distance);
    bool arrived = predicted_distance <= tolerance;
    if (steering_direction) *steering_direction = arrived ? 8u : direction;
    if (distance) *distance = predicted_distance;
    return arrived;
}

/* `$85:A82C-$AB16`: profile-scaled integer acceleration, damping and cap
 * rejection. Inputs and outputs remain signed 8.8 actor velocity words. */
void nba_gameplay_velocity_step(int16_t *velocity_x, int16_t *velocity_y,
                                uint16_t *boost_timer, uint8_t direction,
                                uint8_t profile_42, uint16_t dispatch_dt,
                                bool movement_blocked,
                                int16_t global_093e) {
    static const int16_t normal[8][2] = {
        {0,60},{42,42},{60,0},{42,-42},
        {0,-60},{-42,-42},{-60,0},{-42,42}
    };
    static const int16_t boosted_vectors[8][2] = {
        {0,75},{53,53},{75,0},{53,-53},
        {0,-75},{-53,-53},{-75,0},{-53,53}
    };
    static const uint8_t scales[16] = {
        196,200,204,208,212,216,220,224,
        228,232,236,240,244,248,252,252
    };
    static const uint32_t caps[16] = {
        338000u,348480u,359120u,369920u,
        380880u,392000u,403280u,414720u,
        426320u,438080u,450000u,462080u,
        474320u,486720u,499280u,512000u
    };
    if (!velocity_x || !velocity_y || !boost_timer) return;
    bool boosted = *boost_timer != 0u;
    int16_t old_x = *velocity_x, old_y = *velocity_y;
    if (!movement_blocked && direction < 8u) {
        unsigned profile = (profile_42 >> 4) & 15u;
        const int16_t (*vectors)[2] = boosted ? boosted_vectors : normal;
        uint8_t scale = scales[profile];
        int16_t raw_x = vectors[direction][0], raw_y = vectors[direction][1];
        int16_t accel_x = (int16_t)(((uint8_t)(raw_x < 0 ? -raw_x : raw_x) *
                                     scale) >> 8);
        int16_t accel_y = (int16_t)(((uint8_t)(raw_y < 0 ? -raw_y : raw_y) *
                                     scale) >> 8);
        if (raw_x < 0) accel_x = (int16_t)-accel_x;
        if (raw_y < 0) accel_y = (int16_t)-accel_y;
        int16_t candidate_x = old_x, candidate_y = old_y;
        int16_t counter = (int16_t)(uint16_t)(dispatch_dt - 1u);
        do {
            candidate_x = wrap16((int32_t)candidate_x -
                                  trunc_div_pow2(candidate_x, 4u) + accel_x);
            candidate_y = wrap16((int32_t)candidate_y -
                                  trunc_div_pow2(candidate_y, 4u) + accel_y);
            counter = wrap16((int32_t)counter - 1);
        } while (counter >= 0);
        uint16_t qx = magnitude16(candidate_x) >> 2;
        uint16_t qy = magnitude16(candidate_y) >> 2;
        uint8_t mx = (uint8_t)(qx | (qx >> 8));
        uint8_t my = (uint8_t)(qy | (qy >> 8));
        uint32_t metric = ((uint32_t)mx * mx << 2) +
                          ((uint32_t)my * my << 2);
        unsigned cap_profile = profile;
        if (counter == global_093e) cap_profile = profile >= 2u ? profile - 2u : 0u;
        uint32_t cap = caps[cap_profile];
        if (boosted) cap += cap >> 1;
        if (metric <= cap) {
            *velocity_x = candidate_x;
            *velocity_y = candidate_y;
            goto finish;
        }
    }
    if (!movement_blocked) {
        uint16_t amount = (uint16_t)((uint8_t)dispatch_dt * 25u);
        *velocity_x = old_x >= (int16_t)amount ? wrap16(old_x - amount) :
            old_x < -(int16_t)amount ? wrap16(old_x + amount) : 0;
        *velocity_y = old_y >= (int16_t)amount ? wrap16(old_y - amount) :
            old_y < -(int16_t)amount ? wrap16(old_y + amount) : 0;
    }
finish:
    if (*boost_timer != 0u) {
        int16_t remaining = wrap16((int32_t)(int16_t)*boost_timer - dispatch_dt);
        *boost_timer = remaining >= 0 ? (uint16_t)remaining : 0u;
    }
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

/* `$85:B714-$B833`: exact symmetric mode-11 direct-shot rectangle. The
 * negative X path explicitly computes two's-complement magnitude. */
bool nba_gameplay_mode11_shot_rectangle(int16_t rom_x, int16_t y, int16_t z) {
    if (rom_x < -338 || rom_x >= 338 || z != 0) return false;
    uint16_t abs_x = rom_x < 0 ? (uint16_t)(0u - (uint16_t)rom_x) :
                                 (uint16_t)rom_x;
    return abs_x >= 0x00E2u && y >= -0x0040 && y < 0x0040;
}

static int32_t fixed_integer_floor(int32_t value) {
    return value >= 0 ? value / 256 : -(((-value) + 255) / 256);
}

static void fixed_replace_integer(int32_t *value, int32_t integer) {
    int32_t old_integer = fixed_integer_floor(*value);
    int32_t fraction = *value - old_integer * 256;
    *value = integer * 256 + fraction;
}

/* `$85:A656-$A755`: the common actor/free-ball court integrator clamps the
 * signed rectangle first, cancelling only outward velocity. It then applies
 * the asymmetric isometric edge by replacing integer X while preserving the
 * fractional word and velocity. The return value identifies the rectangular
 * `$86:A613` cancellation path; diagonal-only correction does not take it. */
bool nba_gameplay_court_clamp(int32_t *x_fp, int32_t *y_fp,
                              int16_t *velocity_x, int16_t *velocity_y) {
    if (!x_fp || !y_fp || !velocity_x || !velocity_y) return false;
    bool rectangle_contact = false;
    int32_t x = fixed_integer_floor(*x_fp);
    if (x >= 394) {
        fixed_replace_integer(x_fp, 394);
        if (*velocity_x >= 0) *velocity_x = 0;
        rectangle_contact = true;
    } else if (x <= -394) {
        fixed_replace_integer(x_fp, -394);
        if (*velocity_x < 0) *velocity_x = 0;
        rectangle_contact = true;
    }
    int32_t y = fixed_integer_floor(*y_fp);
    if (y >= 224) {
        fixed_replace_integer(y_fp, 224);
        if (*velocity_y >= 0) *velocity_y = 0;
        rectangle_contact = true;
    } else if (y <= -224) {
        fixed_replace_integer(y_fp, -224);
        if (*velocity_y < 0) *velocity_y = 0;
        rectangle_contact = true;
    }

    y = fixed_integer_floor(*y_fp);
    x = fixed_integer_floor(*x_fp);
    if (y < 0) {
        int32_t minimum_x = -556 - y;
        if (x <= minimum_x) fixed_replace_integer(x_fp, minimum_x);
    } else {
        int32_t maximum_x = 561 - y;
        if (x > maximum_x) fixed_replace_integer(x_fp, maximum_x);
    }
    return rectangle_contact;
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
    if (!nba_gameplay_decision_timer_step(&timer, 11u, 0x30u, false) ||
        timer != 47u) return false;
    static const struct { int16_t x, y, z; bool expected; } shot_edges[] = {
        {-338, 0, 0, true}, {-226, -64, 0, true}, {-225, 0, 0, false},
        {225, 0, 0, false}, {226, 63, 0, true}, {337, 0, 0, true},
        {338, 0, 0, false}, {226, -65, 0, false}, {226, 64, 0, false},
        {226, 0, 1, false}
    };
    for (unsigned i = 0; i < sizeof(shot_edges) / sizeof(shot_edges[0]); ++i)
        if (nba_gameplay_mode11_shot_rectangle(
                shot_edges[i].x, shot_edges[i].y, shot_edges[i].z) !=
            shot_edges[i].expected) return false;
    static const struct {
        int32_t x, y;
        int16_t vx, vy;
        int32_t expected_x, expected_y;
        int16_t expected_vx, expected_vy;
        bool rectangle;
    } court_edges[] = {
        {395 * 256, 0, 16, 0, 394 * 256, 0, 0, 0, true},
        {394 * 256, 0, -16, 0, 394 * 256, 0, -16, 0, true},
        {-395 * 256, 0, -16, 0, -394 * 256, 0, 0, 0, true},
        {0, 225 * 256, 0, 16, 0, 224 * 256, 0, 0, true},
        {0, -225 * 256, 0, -16, 0, -224 * 256, 0, 0, true},
        {-333 * 256, -224 * 256, -16, 0,
         -332 * 256, -224 * 256, -16, 0, true},
        {338 * 256, 224 * 256, 16, 0,
         337 * 256, 224 * 256, 16, 0, true},
        {-390 * 256 + 73, -167 * 256 + 91, -9, 4,
         -389 * 256 + 73, -167 * 256 + 91, -9, 4, false}
    };
    for (unsigned i = 0; i < sizeof(court_edges) / sizeof(court_edges[0]); ++i) {
        int32_t court_x = court_edges[i].x, court_y = court_edges[i].y;
        int16_t court_vx = court_edges[i].vx, court_vy = court_edges[i].vy;
        if (nba_gameplay_court_clamp(
                &court_x, &court_y, &court_vx, &court_vy) !=
                court_edges[i].rectangle ||
            court_x != court_edges[i].expected_x ||
            court_y != court_edges[i].expected_y ||
            court_vx != court_edges[i].expected_vx ||
            court_vy != court_edges[i].expected_vy) return false;
    }
    static const struct { int16_t x, y; uint8_t direction; uint16_t distance; } cases[] = {
        {0,0,8,0},{10,0,2,10},{0,10,0,10},{-10,-10,5,12},
        {10,20,1,22},{10,21,0,23},{10,11,1,12}
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        uint16_t distance = 0u;
        if (nba_gameplay_target_direction(cases[i].x, cases[i].y, &distance) !=
                cases[i].direction || distance != cases[i].distance) return false;
    }
    static const struct { int16_t x, y; uint8_t direction; uint16_t distance; }
        pass_cases[] = {
            {0,0,0x10,0}, {10,0,4,10}, {0,10,0,10}, {-10,-10,10,12},
            {10,20,1,22}, {10,21,1,23}, {10,11,2,13},
            {20,10,3,22}, {-20,10,13,22}, {-20,-10,11,22},
            {6576,6556,2,8215}, {20000,30000,2,35000},
            {-32768,-32768,11,0xA000}, {10,15,2,17}, {10,16,1,18},
            {10,50,1,52}, {10,51,0,53}
        };
    for (unsigned i = 0; i < sizeof(pass_cases) / sizeof(pass_cases[0]); ++i) {
        uint16_t distance = 0u;
        if (nba_gameplay_pass_direction(pass_cases[i].x, pass_cases[i].y,
                &distance) != pass_cases[i].direction ||
            distance != pass_cases[i].distance) return false;
    }
    NbaGameplayReceiverState receivers[10] = {0};
    for (unsigned i = 0; i < 10u; ++i) {
        receivers[i].x = (int16_t)(i * 20);
        receivers[i].y = (int16_t)(i * 3);
        receivers[i].control_mode = 1u;
        receivers[i].travel_direction = 8u;
        receivers[i].travel_distance = 0x40u;
    }
    int16_t selectors[3] = {9, 7, -1};
    if (nba_gameplay_select_pass_receiver(
            8u, -1, selectors, receivers, 10u, true) != 9) return false;
    receivers[9].control_mode = 7u;
    if (nba_gameplay_select_pass_receiver(
            8u, -1, selectors, receivers, 10u, true) != 7) return false;
    receivers[9].control_mode = 1u;
    receivers[9].travel_direction = 6u;
    receivers[9].travel_distance = 0x1Fu;
    if (nba_gameplay_select_pass_receiver(
            8u, -1, selectors, receivers, 10u, true) != 7) return false;
    receivers[9].travel_direction = 8u;
    receivers[9].travel_distance = 0x40u;
    selectors[2] = 6;
    receivers[9].control_mode = 7u;
    receivers[7].control_mode = 7u;
    if (nba_gameplay_select_pass_receiver(
            8u, -1, selectors, receivers, 10u, true) != 6) return false;
    selectors[0] = -1;
    if (nba_gameplay_select_pass_receiver(
            8u, -1, selectors, receivers, 10u, true) != -1) return false;
    selectors[0] = 9;
    selectors[2] = -1;
    receivers[9].control_mode = 1u;
    receivers[7].control_mode = 1u;
    receivers[5].x = 500;
    if (nba_gameplay_select_pass_receiver(
            8u, 5, selectors, receivers, 10u, true) != 9) return false;
    receivers[5].x = 190;
    if (nba_gameplay_select_pass_receiver(
            8u, 5, selectors, receivers, 10u, true) != 5) return false;
    uint8_t steering = 0u;
    uint16_t predicted = 0u;
    if (!nba_gameplay_predictive_arrival(
            275, -53, 310, -161, 320, -88, 16u,
            &steering, &predicted) || steering != 8u || predicted != 16u)
        return false;
    if (nba_gameplay_predictive_arrival(
            194, 131, 557, -157, 320, 88, 16u,
            &steering, &predicted) || predicted != 63u)
        return false;
    if (!nba_gameplay_predictive_arrival(
            0, 0, -8, -1, 16, 0, 16u, &steering, &predicted) ||
        predicted != 16u || steering != 8u)
        return false;
    if (nba_gameplay_predictive_arrival(
            0, 0, -9, 0, 16, 0, 16u, &steering, &predicted) ||
        predicted != 17u) return false;
    uint16_t boost = 0u; x = 0; y = 0;
    nba_gameplay_velocity_step(&x, &y, &boost, 2u, 0x58u, 2u, false, 8);
    if (x != 97 || y != 0) return false;
    x = 100; y = -100;
    nba_gameplay_velocity_step(&x, &y, &boost, 1u, 0x58u, 2u, false, 8);
    if (x != 156 || y != -21) return false;
    x = 100; y = -100;
    nba_gameplay_velocity_step(&x, &y, &boost, 8u, 0x58u, 2u, false, 8);
    if (x != 50 || y != -50) return false;
    x = 0; y = 0; boost = 2u;
    nba_gameplay_velocity_step(&x, &y, &boost, 0u, 0x58u, 2u, false, 8);
    if (x != 0 || y != 123 || boost != 0u) return false;
    x = 910; y = 910;
    nba_gameplay_velocity_step(&x, &y, &boost, 1u, 0x58u, 2u, false, -1);
    return x == 860 && y == 860;
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
