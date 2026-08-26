#include "nba_gameplay_ball.h"
#include <stdlib.h>

void nba_gameplay_apply_catch_prefix(NbaGameplayCatchPrefixState *state) {
    if (!state || state->catcher >= 10u) return;
    state->rim_force_raw_1866 = 0u;
    state->dead_ball_raw_0968 = 0u;
    state->rim_raw_096a = 0u;
    state->catcher_latch = 1u;
    if (state->movement_magnitude < 0x0080u) {
        state->velocity_x = 0;
        state->velocity_y = 0;
        state->movement_magnitude = 0u;
    }
    state->context_previous_actor_raw_43 = state->context_actor_raw_3f;
    state->context_previous_controller_raw_45 =
        state->context_controller_raw_41;
    state->special_actor_raw_09a2 = -1;
    state->play_aux_raw_09a6 = -1;
    for (unsigned i = 0; i < 3u; ++i) state->play_selector_raw[i] = -1;
    state->possession_actor_raw_093e = state->catcher;
    state->actor_record_raw_0910 =
        (uint16_t)(0x34EBu + (uint16_t)state->catcher * 0x0100u);
    state->context_record_raw_0912 =
        (uint16_t)(0x46EBu + (uint16_t)(state->catcher / 5u) * 0x0080u);
    state->context_actor_raw_3f = state->catcher;
    state->context_controller_raw_41 = state->controller_actor;
}

/* `$86:BAFD-$BB14`: publish the catch-time clock, preserve the special
 * mode-14 finish, or install ordinary CPU-owner mode 11 with an immediately
 * due decision and cleared behavior flags. */
void nba_gameplay_apply_catch_mode(uint16_t match_clock,
                                   uint16_t *context_match_clock,
                                   uint16_t *control_mode,
                                   uint16_t *decision_timer,
                                   uint16_t *behavior_flags) {
    if (!context_match_clock || !control_mode || !decision_timer ||
        !behavior_flags) return;
    *context_match_clock = match_clock;
    if (*control_mode == 14u) return;
    *control_mode = 11u;
    *decision_timer = 0u;
    *behavior_flags = 0u;
}

/* `$86:E593-$E5AA`: terminal mode-11 owner pose fallback. */
uint8_t nba_gameplay_owner_dribble_fallback_pose(
        uint16_t dead_ball_raw_0968, uint16_t catcher_latch_raw_ae) {
    return dead_ball_raw_0968 != 0u || catcher_latch_raw_ae != 0u ? 12u : 5u;
}

/* `$86:E4A7-$E4C4`: opening owner-pose gates. The signed BMI after CMP is
 * preserved rather than replaced with an unsigned host comparison. */
NbaGameplayOwnerDribbleGate nba_gameplay_owner_dribble_gate(
        int16_t actor_z, uint16_t free_throw_state_raw_0978,
        uint16_t live_state_raw_0936, uint16_t movement_magnitude_raw_4c) {
    if (actor_z != 0) return NBA_GAMEPLAY_OWNER_DRIBBLE_SKIP;
    if (free_throw_state_raw_0978 != 0u || live_state_raw_0936 == 0x82u)
        return NBA_GAMEPLAY_OWNER_DRIBBLE_FALLBACK;
    return (int16_t)(uint16_t)(movement_magnitude_raw_4c - 0x0200u) < 0 ?
        NBA_GAMEPLAY_OWNER_DRIBBLE_CONTINUE :
        NBA_GAMEPLAY_OWNER_DRIBBLE_FALLBACK;
}

/* `$86:E4C7-$E4F3`: nearby-pair owner-pose gates and facing handoff. */
NbaGameplayOwnerProximityResult nba_gameplay_owner_dribble_proximity(
        int16_t context_anchor_x, int16_t actor_x,
        uint16_t paired_movement_magnitude, uint16_t assignment_distance,
        uint8_t paired_direction, uint16_t dead_ball_raw_0968,
        uint16_t catcher_latch_raw_ae, uint8_t *requested_direction) {
    if (((uint16_t)context_anchor_x ^ (uint16_t)actor_x) & 0x8000u)
        return NBA_GAMEPLAY_OWNER_PROXIMITY_FALLBACK;
    if ((int16_t)(uint16_t)(paired_movement_magnitude - 0x0200u) >= 0 ||
        (int16_t)(uint16_t)(assignment_distance - 0x0021u) >= 0)
        return NBA_GAMEPLAY_OWNER_PROXIMITY_FALLBACK;
    if (requested_direction) *requested_direction = paired_direction;
    return dead_ball_raw_0968 != 0u || catcher_latch_raw_ae != 0u ?
        NBA_GAMEPLAY_OWNER_PROXIMITY_LATCHED :
        NBA_GAMEPLAY_OWNER_PROXIMITY_UNLATCHED;
}

/* Three-point arc table `$85:ABFB`, indexed by even Y offsets 0..356. */
static const int16_t three_point_arc[179] = {
    259,251,246,243,241,238,236,234,232,229,226,224,222,218,215,212,
    209,206,204,202,199,196,194,192,190,189,187,185,183,182,180,177,
    174,172,170,168,166,164,163,161,160,158,157,156,154,153,151,150,
    148,147,147,145,143,141,139,137,136,135,134,133,132,131,130,129,
    128,127,127,126,126,125,125,124,123,122,122,121,121,121,120,120,
    120,119,119,118,118,118,117,117,116,116,116,115,115,114,114,114,
    113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,
    113,113,113,113,113,113,113,113,113,113,113,114,
    115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,
    131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,
    147,148,149,151,152,154,157,159,162,164,167,170,173,176,178,181,
    184,189,194,200,208,216,224
};

/* `$85:F1C1`: high + low/4 for axis-dominant deltas, otherwise
 * high + 3*low/8. */
uint16_t nba_gameplay_hoop_distance(int16_t dx, int16_t dy) {
    unsigned ax = (unsigned)abs((int)dx);
    unsigned ay = (unsigned)abs((int)dy);
    unsigned high = ax > ay ? ax : ay;
    unsigned low = ax > ay ? ay : ax;
    return (uint16_t)(high + (high >= low * 2u ? low / 4u :
                             (low * 3u) / 8u));
}

static uint16_t magnitude16(int16_t value) {
    uint16_t raw = (uint16_t)value;
    return value < 0 ? (uint16_t)(0u - raw) : raw;
}

static int16_t negate16(int16_t value) {
    return (int16_t)(uint16_t)(0u - (uint16_t)value);
}

static int16_t halve_if_magnitude_at_least(int16_t value,
                                           uint16_t threshold) {
    return magnitude16(value) >= threshold ?
           nba_gameplay_arithmetic_shift_right(value, 1u) : value;
}

static void snap_outer_x(NbaGameplayRimState *state) {
    if (state->velocity_x >= 0)
        state->x = state->x < 0 ? (int16_t)-349 : (int16_t)343;
    else
        state->x = state->x < 0 ? (int16_t)-343 : (int16_t)349;
}

static void reflect_x_from_current_side(NbaGameplayRimState *state) {
    if ((state->x >= 0 && state->velocity_x >= 0) ||
        (state->x < 0 && state->velocity_x < 0))
        state->velocity_x = negate16(state->velocity_x);
}

static void reflect_y_to_matching_sign(NbaGameplayRimState *state) {
    if ((state->y >= 0 && state->velocity_y < 0) ||
        (state->y < 0 && state->velocity_y >= 0))
        state->velocity_y = negate16(state->velocity_y);
}

static int16_t signed_magnitude_with_sign(int16_t value, bool negative) {
    uint16_t magnitude = magnitude16(value);
    return negative ? (int16_t)(uint16_t)(0u - magnitude) :
                      (int16_t)magnitude;
}

static int16_t double16(int16_t value) {
    return (int16_t)(uint16_t)((uint16_t)value << 1u);
}

/* `$85:9ACB-$A081`: exact signed-16-bit rim/backboard shell. Inner edge and
 * miss impulses consume unrelated WRAM/RNG inputs later in the ROM routine,
 * so this deterministic split classifies those paths without inventing them. */
NbaGameplayRimResult nba_gameplay_rim_step(NbaGameplayRimState *state,
                                           uint16_t live_state,
                                           bool alternate_height,
                                           bool inner_veto,
                                           bool correct_basket_side) {
    uint16_t abs_x;
    if (!state || (!alternate_height && state->z < 73) ||
        (alternate_height && state->z < 68) || state->z >= 123 ||
        state->y < -27 || state->y >= 27)
        return NBA_GAMEPLAY_RIM_FLIGHT;
    abs_x = magnitude16(state->x);
    if (abs_x < 327u || abs_x >= 349u)
        return NBA_GAMEPLAY_RIM_FLIGHT;

    if (abs_x <= 343u) {
        int16_t dx;
        uint16_t distance;
        if (live_state != 1u || state->z >= 83 ||
            (!alternate_height && state->z < 74) ||
            (alternate_height && state->z < 68) ||
            !correct_basket_side)
            return NBA_GAMEPLAY_RIM_FLIGHT;
        dx = (int16_t)(abs_x - 336u);
        distance = nba_gameplay_hoop_distance(dx, state->y);
        if (distance >= 11u) return NBA_GAMEPLAY_RIM_FLIGHT;
        if (distance >= 8u) return NBA_GAMEPLAY_RIM_MISS;
        if (distance == 7u) return NBA_GAMEPLAY_RIM_EDGE_CONTACT;
        return inner_veto ? NBA_GAMEPLAY_RIM_MISS : NBA_GAMEPLAY_RIM_MAKE;
    }

    state->raw_092c = 0x05A0u;
    state->raw_0962 = 0x05A0u;
    state->raw_096a = 0u;
    state->raw_097c = 0x05A0u;
    if (state->velocity_x < -300) state->velocity_x = -300;
    if (state->velocity_x > 300) state->velocity_x = 300;

    if (magnitude16(state->velocity_x) < 20u &&
        magnitude16(state->velocity_y) < 20u &&
        magnitude16(state->velocity_z) < 150u) {
        if (state->x >= 0) {
            state->x = (int16_t)(uint16_t)((uint16_t)state->x - 1u);
            state->velocity_x = -30;
        } else {
            state->x = (int16_t)(uint16_t)((uint16_t)state->x + 1u);
            state->velocity_x = 30;
        }
        return NBA_GAMEPLAY_RIM_OUTER_CONTACT;
    }

    /* `$85:9B90-$9BB3`: both signed lip boundaries are inclusive. */
    /* `$85:9B8C-$9B97` arms this timer once. A live countdown must not be
     * restarted by every frame that remains inside the outer shell. */
    if (state->raw_096e == 0u) {
        state->raw_096e = 0x000Fu;
        state->raw_13e7 |= 0x0008u;
    }
    if (state->y >= 24 || state->y <= -24) {
        state->y = state->y < 0 ? (int16_t)-27 : (int16_t)27;
        state->velocity_y = halve_if_magnitude_at_least(state->velocity_y, 30u);
        reflect_y_to_matching_sign(state);
        return NBA_GAMEPLAY_RIM_OUTER_CONTACT;
    }

    if (state->z <= 76) {
        snap_outer_x(state);
        if (state->velocity_z >= 0)
            state->velocity_z = negate16(state->velocity_z);
        state->velocity_x = halve_if_magnitude_at_least(state->velocity_x, 30u);
        state->velocity_x = negate16(state->velocity_x);
        return NBA_GAMEPLAY_RIM_OUTER_CONTACT;
    }

    if (state->z >= 120) {
        state->velocity_z = halve_if_magnitude_at_least(state->velocity_z, 30u);
        if (state->velocity_z < 0)
            state->velocity_z = negate16(state->velocity_z);
        return NBA_GAMEPLAY_RIM_OUTER_CONTACT;
    }

    if (state->x >= 346) {
        state->x = 349;
        state->velocity_x = halve_if_magnitude_at_least(state->velocity_x, 60u);
        if (state->velocity_x < 0)
            state->velocity_x = negate16(state->velocity_x);
    } else if (state->x < -346) {
        state->x = -349;
        state->velocity_x = halve_if_magnitude_at_least(state->velocity_x, 60u);
        if (state->velocity_x >= 0)
            state->velocity_x = negate16(state->velocity_x);
    } else {
        snap_outer_x(state);
        state->velocity_x = halve_if_magnitude_at_least(state->velocity_x, 30u);
        reflect_x_from_current_side(state);
    }
    return NBA_GAMEPLAY_RIM_OUTER_CONTACT;
}

/* Host gameplay keeps the original per-axis units but centers each court at
 * its rendered hoop coordinates. This local translation is the complete
 * bridge into `$85:9ACB`'s signed +/-336 hoop domain; it does not use the
 * separate full-court scaling required by the three-point arc lookup. */
NbaGameplayRimResult nba_gameplay_rim_world_step(
    NbaGameplayRimState *state, int16_t hoop_x, int16_t hoop_y,
    bool right_basket, uint16_t live_state, bool alternate_height,
    bool inner_veto, bool correct_basket_side) {
    if (!state) return NBA_GAMEPLAY_RIM_FLIGHT;
    int16_t shell_center = right_basket ? 336 : -336;
    state->x = (int16_t)(shell_center + state->x - hoop_x);
    state->y = (int16_t)(state->y - hoop_y);
    NbaGameplayRimResult result = nba_gameplay_rim_step(
        state, live_state, alternate_height, inner_veto,
        correct_basket_side);
    state->x = (int16_t)(hoop_x + state->x - shell_center);
    state->y = (int16_t)(hoop_y + state->y);
    return result;
}

/* `$85:9DAC-$9EFE` (distance 7) and `$85:9F01-$A006` (distance 8..10 or
 * `$09F8` veto). Both paths mutate the ball before tail-jumping to `$A3B7`
 * gravity. `$87:A9E3(A=3)` is represented by effect_raw_401b=3; its graphics
 * dispatcher remains outside this pure physics component. */
void nba_gameplay_rim_apply_inner_response(
    NbaGameplayRimState *state, NbaGameplayRimResult result,
    NbaGameplayRimContext *context, NbaGameplayRng *rng) {
    if (!state || !context ||
        (result != NBA_GAMEPLAY_RIM_EDGE_CONTACT &&
         result != NBA_GAMEPLAY_RIM_MISS)) return;

    if (context->raw_0978 == 0u) context->raw_09f8 = 0u;
    context->raw_0920 = (uint16_t)(context->raw_0920 + 1u);
    state->raw_092c = 0x05A0u;
    state->raw_097c = 0x05A0u;
    context->raw_0948 = 0u;
    context->raw_094a = 0u;
    state->raw_096a = 0u;

    if (result == NBA_GAMEPLAY_RIM_EDGE_CONTACT) {
        state->velocity_z = (int16_t)magnitude16(state->velocity_z);
        state->velocity_y = signed_magnitude_with_sign(
            state->velocity_y, state->y >= 0);
        state->velocity_y = halve_if_magnitude_at_least(
            state->velocity_y, 80u);
        state->velocity_y = halve_if_magnitude_at_least(
            state->velocity_y, 80u);
        bool outside_x = state->x >= 336 || state->x < -336;
        state->velocity_x = signed_magnitude_with_sign(
            state->velocity_x, outside_x);
        state->velocity_x = halve_if_magnitude_at_least(
            state->velocity_x, 80u);
        state->velocity_x = halve_if_magnitude_at_least(
            state->velocity_x, 80u);
    } else {
        if (context->raw_1866 != 0u) {
            state->velocity_x = double16(state->velocity_x);
            state->velocity_y = double16(state->velocity_y);
            state->velocity_z = -960;
        }
        state->velocity_x = halve_if_magnitude_at_least(
            state->velocity_x, 60u);
        state->velocity_y = halve_if_magnitude_at_least(
            state->velocity_y, 60u);
        state->velocity_x = signed_magnitude_with_sign(
            state->velocity_x, state->x >= 0);
        state->velocity_y = signed_magnitude_with_sign(
            state->velocity_y, state->y >= 0);
        if (state->velocity_x == 0 && state->velocity_y == 0) {
            if (state->y >= 0) {
                state->y = (int16_t)(uint16_t)((uint16_t)state->y + 2u);
                state->velocity_y = 100;
            } else {
                state->y = (int16_t)(uint16_t)((uint16_t)state->y - 2u);
                state->velocity_y = -100;
            }
        } else {
            state->velocity_z = negate16(state->velocity_z);
            state->velocity_z = halve_if_magnitude_at_least(
                state->velocity_z, 60u);
        }
        context->raw_0936 = 0u;
        context->effect_raw_401b = 3u;
    }

    if (context->raw_0970 == 0u) {
        context->raw_0970 = 0x000Fu;
        state->raw_13e7 |= 0x0002u;
    }
    if (magnitude16(state->velocity_x) < 10u)
        state->velocity_x = (int16_t)((context->raw_07f6 & 0x001Fu) + 15u);
    if (magnitude16(state->velocity_z) < 193u) {
        uint16_t random_word = rng ? nba_gameplay_rng_next(rng) : 0u;
        state->velocity_x = (int16_t)((random_word & 0x001Fu) +
            (result == NBA_GAMEPLAY_RIM_EDGE_CONTACT ? 31u : 60u));
    }
}

/* `$85:A0EB-$A10D,$A34A-$A3B3`: after awarding the basket, the ROM clears
 * the live ball latches, anchors the ball in the scoring cylinder, removes
 * planar motion, and retains one eighth of signed VZ. The branch then enters
 * `$85:A3C8`, deliberately skipping the normal `$85:A3B7` gravity update. */
void nba_gameplay_rim_apply_made_response(
    NbaGameplayRimState *state, bool right_basket,
    NbaGameplayRimContext *context) {
    if (!state || !context) return;
    state->raw_092c = 0x05A0u;
    state->raw_0962 = 0u;
    state->raw_096a = 0u;
    context->raw_0948 = 0u;
    context->raw_094a = 0u;
    context->raw_09b8 = 0u;
    state->x = right_basket ? 336 : -336;
    state->y = 0;
    state->velocity_x = 0;
    state->velocity_y = 0;
    state->velocity_z = nba_gameplay_arithmetic_shift_right(
        state->velocity_z, 3u);
}

/* `$85:A5F4-$A655` plus its exact `$86:A613-$A628` state clear. This runs
 * only for a grounded record whose unsigned VZ is below $18. Planar axes are
 * thresholded independently and then arithmetic-halved even when retained. */
bool nba_gameplay_ball_apply_settle(
    NbaGameplayRimState *state, NbaGameplaySettleContext *context) {
    if (!state || !context || state->z != 0 ||
        (uint16_t)state->velocity_z >= 0x0018u) return false;

    if (context->raw_09b8 != 0u) context->raw_0936 = 0u;
    context->raw_0942 = 0xFFFFu;
    context->raw_0944 = 0xFFFFu;
    context->raw_0946 = 0xFFFFu;
    context->raw_0948 = 0u;
    context->raw_094a = 0u;
    context->raw_09b8 = 0u;
    if (context->raw_0978 == 0x000Au) state->raw_097c = 1u;

    state->velocity_z = 0;
    if (magnitude16(state->velocity_x) < 0x0018u)
        state->velocity_x = 0;
    state->velocity_x = nba_gameplay_arithmetic_shift_right(
        state->velocity_x, 1u);
    if (magnitude16(state->velocity_y) < 0x0018u)
        state->velocity_y = 0;
    state->velocity_y = nba_gameplay_arithmetic_shift_right(
        state->velocity_y, 1u);
    return true;
}

/* `$85:A3B7-$A4DA`, specifically `$85:A43A-$A44B`: ground restitution is
 * stored in `$13E5`, and an impact of at least `$0048` raises event bit 0.
 * The 15/16 planar damping belongs to the same impact branch. */
void nba_gameplay_ball_apply_ground_impact(
    NbaGameplayRimState *state, uint16_t *impact_raw_13e5) {
    if (!state) return;
    int16_t original_vz = state->velocity_z;
    int rebound = -(int)original_vz +
        nba_gameplay_arithmetic_shift_right(original_vz, 3u);
    if (rebound > 0x0400) rebound = 0x0400;
    state->velocity_z = (int16_t)rebound;
    if (impact_raw_13e5) *impact_raw_13e5 = (uint16_t)state->velocity_z;
    if ((uint16_t)state->velocity_z >= 0x0048u)
        state->raw_13e7 |= 0x0001u;
    state->velocity_x = (int16_t)(state->velocity_x -
        nba_gameplay_arithmetic_shift_right(state->velocity_x, 4u));
    state->velocity_y = (int16_t)(state->velocity_y -
        nba_gameplay_arithmetic_shift_right(state->velocity_y, 4u));
}

/* Final made-basket predicate, expressed through the complete ROM shell. The
 * caller's dx/dy are hoop-relative, so +336 reconstructs the right-rim raw X. */
bool nba_gameplay_ball_is_make(uint16_t live_state, bool alternate_height,
                               bool inner_veto, bool correct_basket_side,
                               int16_t dx, int16_t dy, int16_t z) {
    NbaGameplayRimState state = {0};
    state.x = (int16_t)(uint16_t)(336u + (uint16_t)dx);
    state.y = dy;
    state.z = z;
    return nba_gameplay_rim_step(&state, live_state, alternate_height,
                                  inner_veto, correct_basket_side) ==
           NBA_GAMEPLAY_RIM_MAKE;
}

/* `$86:9DED-$9DFF` chooses 1/2, then `$86:A561-$A5AF` upgrades to three
 * using the ROM arc table. */
uint16_t nba_gameplay_shot_value(bool one_point_attempt, int16_t shooter_x,
                                 int16_t shooter_y, bool right_basket) {
    if (one_point_attempt) return 1u;
    if (shooter_y < -178 || shooter_y >= 179) return 3u;
    unsigned index = right_basket ? (unsigned)(shooter_y + 178) / 2u :
                                    (unsigned)(178 - shooter_y) / 2u;
    int16_t threshold = three_point_arc[index];
    if (right_basket ? shooter_x <= threshold : shooter_x > -threshold)
        return 3u;
    return 2u;
}

uint8_t nba_gameplay_shot_chance(uint8_t rating, uint8_t raw_actor_8c,
                                 uint8_t difficulty,
                                 bool raw_actor_16_nonnegative) {
    /* Base tier of `$86:9ED8-$A11D`; `$86:A110` compares its result to the
     * low byte from the exact gameplay LFSR. */
    static const int8_t high_adjust[3] = {50, 15, 0}; /* `$86:9F32` */
    static const int8_t low_adjust[3] = {40, 23, 0};  /* `$86:9F38` */
    bool high_branch = raw_actor_8c >= rating;
    int chance;
    if (high_branch)
        chance = rating >= 0xD9 ? 0xDC : rating >= 0xC0 ? 0xA0 :
                 rating >= 0xA8 ? 0x82 : 0x6E;
    else
        chance = rating >= 0xD9 ? 0xE6 : rating >= 0xC0 ? 0xC0 :
                 rating >= 0xA8 ? 0x99 : 0x73;
    if (raw_actor_16_nonnegative)
        chance += (high_branch ? high_adjust : low_adjust)[difficulty < 3u ?
                                                           difficulty : 2u];
    if (chance < 5) chance = 5;
    if (chance > 255) chance = 255;
    return (uint8_t)chance;
}

void nba_gameplay_miss_offset(uint8_t index, bool left_basket,
                              int16_t *dx, int16_t *dy) {
    /* `$86:A17D`, selected with the second `$80:CEE7` result & $0F. */
    static const int8_t offsets[16][2] = {
        {0,7},{0,-7},{0,-7},{-8,0},{6,0},{6,0},{0,7},{0,7},
        {4,-8},{4,-8},{3,7},{3,8},{5,7},{5,8},{2,-7},{2,-7}
    };
    int x = offsets[index & 15u][0], y = offsets[index & 15u][1];
    if (left_basket) { x = -x; y = -y; }
    if (dx) *dx = (int16_t)x;
    if (dy) *dy = (int16_t)y;
}

uint16_t nba_gameplay_shot_flight_duration(int16_t dx, int16_t dy) {
    /* First two words of each `$86:A4AB` record. The third word is not read by
     * the normal launch path and intentionally remains unlabeled. */
    static const uint16_t records[13][2] = {
        {0x0040,0x0028},{0x0060,0x002E},{0x0080,0x0032},
        {0x00A0,0x0038},{0x00B8,0x003C},{0x00E8,0x0042},
        {0x0118,0x0046},{0x0150,0x0049},{0x0180,0x004C},
        {0x01B0,0x004E},{0x01E0,0x0050},{0x0250,0x0056},
        {0x0640,0x005A}
    };
    uint16_t distance = nba_gameplay_hoop_distance(dx, dy);
    for (unsigned i = 0; i < 13u; ++i)
        if (distance < records[i][0]) return records[i][1];
    return records[12][1];
}

void nba_gameplay_shot_launch(int32_t ball_x_fp, int32_t ball_y_fp,
                              int32_t ball_z_fp, int16_t target_x,
                              int16_t target_y, int16_t *velocity_x,
                              int16_t *velocity_y, int16_t *velocity_z) {
    /* `$86:A1BD-$A292`. Host positions are 24.8, which exactly preserve the
     * ROM's signed 8.8 velocity increments used by this base launch branch. */
    int32_t dx_fp = (int32_t)target_x * 256 - ball_x_fp;
    int32_t dy_fp = (int32_t)target_y * 256 - ball_y_fp;
    uint16_t duration = nba_gameplay_shot_flight_duration(
        (int16_t)(dx_fp / 256), (int16_t)(dy_fp / 256));
    int32_t dz_fp = 80 * 256 - ball_z_fp;
    if (velocity_x) *velocity_x = (int16_t)(dx_fp / (int32_t)duration);
    if (velocity_y) *velocity_y = (int16_t)(dy_fp / (int32_t)duration);
    if (velocity_z) *velocity_z = (int16_t)(
        dz_fp / (int32_t)duration + 12 * (int32_t)duration + 0x18);
}

int16_t nba_gameplay_arithmetic_shift_right(int16_t value, unsigned amount) {
    /* Make the 65816 signed `ROR` result explicit instead of relying on the
     * implementation-defined result of shifting a negative C integer. */
    if (amount == 0u) return value;
    if (amount >= 15u) return value < 0 ? -1 : 0;
    uint16_t raw = (uint16_t)value;
    uint16_t shifted = (uint16_t)(raw >> amount);
    if (value < 0) shifted |= (uint16_t)(0xFFFFu << (16u - amount));
    return (int16_t)shifted;
}

/* `$86:CCCD-$CCFB`: inclusive coarse actor/ball box. Y is intentionally
 * asymmetric and only the designated `$0946` pass candidate receives the
 * extended 95-unit vertical window. */
bool nba_gameplay_ball_coarse_contact(int16_t actor_x, int16_t actor_y,
                                      int16_t actor_z, int16_t ball_x,
                                      int16_t ball_y, int16_t ball_z,
                                      bool intended_receiver) {
    int16_t dx = (int16_t)(actor_x - ball_x);
    int16_t dy = (int16_t)(actor_y - ball_y);
    int16_t dz = (int16_t)(ball_z - actor_z);
    int16_t max_z = intended_receiver ? 95 : 71;
    return dx >= -16 && dx <= 16 && dy >= -16 && dy <= 15 &&
           dz >= 0 && dz <= max_z;
}

/* `$86:D549-$D5DA`: either animation-resource attachment point may win.
 * Every axis is a strict cube comparison, so delta threshold itself fails. */
int nba_gameplay_ball_pose_contact_index(
    const NbaGameplayPosePoint points[2], int16_t ball_x, int16_t ball_y,
    int16_t ball_z, uint8_t threshold) {
    if (!points || threshold == 0u) return -1;
    for (unsigned i = 0; i < 2u; ++i) {
        int dx = abs((int)points[i].x - ball_x);
        int dy = abs((int)points[i].y - ball_y);
        int dz = abs((int)points[i].z - ball_z);
        if (dx < threshold && dy < threshold && dz < threshold)
            return (int)i;
    }
    return -1;
}

bool nba_gameplay_ball_pose_contact(const NbaGameplayPosePoint points[2],
                                    int16_t ball_x, int16_t ball_y,
                                    int16_t ball_z, uint8_t threshold) {
    return nba_gameplay_ball_pose_contact_index(
        points, ball_x, ball_y, ball_z, threshold) >= 0;
}

/* `$86:D4E3-$D544`: shared loose-ball deflection response. The routine
 * changes velocity only; the common integrator owns the following motion. */
void nba_gameplay_ball_apply_deflection(NbaGameplayRimState *state,
                                        NbaGameplayRng *rng) {
    if (!state) return;
    state->velocity_x = nba_gameplay_arithmetic_shift_right(
        (int16_t)(0u - (uint16_t)state->velocity_x), 2u);
    state->velocity_y = nba_gameplay_arithmetic_shift_right(
        state->velocity_y, 2u);
    if (state->z < 0x30 || !rng) return;
    uint16_t selector = (uint16_t)(nba_gameplay_rng_next(rng) & 3u);
    int16_t vz = state->velocity_z;
    uint16_t magnitude = vz < 0 ? (uint16_t)(0u - (uint16_t)vz) :
                                  (uint16_t)vz;
    if (selector == 0u) state->velocity_z = (int16_t)magnitude;
    else if (selector == 2u) state->velocity_z = (int16_t)(magnitude >> 1);
    else state->velocity_z = nba_gameplay_arithmetic_shift_right(vz, 1u);
}

/* `$86:D035-$D205`: owned-ball contact has two independent random stages
 * before the final roster +$3A strip roll. Ordinary poses first pass a 1/8
 * gate; animation $13 uses the difficulty-dependent bit gate instead. The
 * elaborate rating accumulated in DP $AA is dead for this ROM revision:
 * `$86:D11F-$D128` compares the next random byte with DP $00, the pose-point
 * selector written by `$86:D549`. Point zero can never pass and point one
 * passes only on random byte zero. `$86:D12D-$D1CE` then tests the first
 * committed Rules word before `$86:D1D9` performs the +$3A roll. */
NbaGameplayOwnedContactResult nba_gameplay_owned_contact_attempt(
    NbaGameplayRng *rng, uint8_t candidate_animation,
    uint8_t pose_point_index, uint8_t contact_rating_3a,
    uint16_t difficulty_raw_17af, uint16_t foul_rule_raw_17d1,
    bool foul_state_clear) {
    if (!rng || pose_point_index > 1u) return NBA_GAMEPLAY_OWNED_CONTACT_NONE;
    if (candidate_animation == 0x13u) {
        if (difficulty_raw_17af != 0u &&
            (nba_gameplay_rng_next(rng) & 1u) == 0u)
            return NBA_GAMEPLAY_OWNED_CONTACT_NONE;
    } else if ((nba_gameplay_rng_next(rng) & 7u) != 0u) {
        return NBA_GAMEPLAY_OWNED_CONTACT_NONE;
    }
    uint8_t point_roll = (uint8_t)nba_gameplay_rng_next(rng);
    if (point_roll >= pose_point_index)
        return NBA_GAMEPLAY_OWNED_CONTACT_NONE;
    if (foul_state_clear && foul_rule_raw_17d1 != 0u) {
        uint8_t foul_roll = (uint8_t)nba_gameplay_rng_next(rng);
        /* `$86:D151-$D167` is another destructive low-byte use of `$07F6`.
         * Preserve both the writeback and the signed BPL comparison. */
        rng->state = foul_roll;
        uint16_t delta = (uint16_t)((foul_rule_raw_17d1 >> 1) - foul_roll);
        if ((int16_t)delta >= 0)
            return NBA_GAMEPLAY_OWNED_CONTACT_FOUL;
    }
    return (uint8_t)nba_gameplay_rng_next(rng) < contact_rating_3a ?
        NBA_GAMEPLAY_OWNED_CONTACT_STRIP : NBA_GAMEPLAY_OWNED_CONTACT_NONE;
}

/* `$86:D078-$D128`: after a descending detached shot reaches an opponent's
 * strict pose point, a live rim context (`$097C`) acquires immediately.
 * Otherwise the ROM's literal DP-$00 bug compares the random low byte with
 * the pose-point selector instead of the rating accumulated in DP $AA.
 * Consequently point zero cannot catch and point one catches only on zero. */
bool nba_gameplay_detached_shot_contact_attempt(
    NbaGameplayRng *rng, uint8_t pose_point_index,
    bool rim_context_nonzero) {
    if (pose_point_index > 1u) return false;
    if (rim_context_nonzero) return true;
    return rng && (uint8_t)nba_gameplay_rng_next(rng) < pose_point_index;
}

bool nba_gameplay_ball_self_test(void) {
    int16_t vx = 0, vy = 0, vz = 0;
    NbaGameplayRimState rim = {0};
    nba_gameplay_shot_launch(0, 0, 20 * 256, 63, 0, &vx, &vy, &vz);
    bool launch_ok = vx == 403 && vy == 0 && vz == 888;
    rim.x = 344; rim.y = 0; rim.z = 77;
    rim.velocity_x = 301; rim.velocity_y = 20; rim.velocity_z = 150;
    bool outer_generic = nba_gameplay_rim_step(&rim, 1u, false, false, true) ==
                         NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
                         rim.x == 343 && rim.velocity_x == -150 &&
                         rim.raw_092c == 0x05A0u && rim.raw_0962 == 0x05A0u &&
                         rim.raw_096a == 0u && rim.raw_097c == 0x05A0u;
    rim = (NbaGameplayRimState){-344, -25, 80, -20, -31, 150, 0, 0, 1, 0};
    bool outer_y = nba_gameplay_rim_step(&rim, 1u, false, false, true) ==
                   NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
                   rim.y == -27 && rim.velocity_y == -16 &&
                   rim.raw_096e == 0x000Fu && rim.raw_13e7 == 0x0008u;
    rim = (NbaGameplayRimState){348, 0, 76, 45, 20, 31, 0, 0, 0, 0};
    bool outer_low = nba_gameplay_rim_step(&rim, 1u, false, false, true) ==
                     NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
                     rim.x == 343 && rim.velocity_x == -22 &&
                     rim.velocity_z == -31;
    rim = (NbaGameplayRimState){-348, 0, 120, -20, 20, -31, 0, 0, 0, 0};
    bool outer_high = nba_gameplay_rim_step(&rim, 1u, false, false, true) ==
                      NBA_GAMEPLAY_RIM_OUTER_CONTACT && rim.x == -348 &&
                      rim.velocity_z == 16;
    rim = (NbaGameplayRimState){394,0,80,20,20,150,1,2,3,4,5,0x20};
    bool right_world_bridge = nba_gameplay_rim_world_step(
        &rim, 386, 0, true, 1u, false, false, true) ==
            NBA_GAMEPLAY_RIM_OUTER_CONTACT && rim.x == 393 &&
        rim.raw_092c == 0x05A0u && rim.raw_13e7 == 0x20u;
    rim = (NbaGameplayRimState){-114,0,80,-20,20,150,1,2,3,4,5,0x40};
    bool left_world_bridge = nba_gameplay_rim_world_step(
        &rim, -106, 0, false, 1u, false, false, true) ==
            NBA_GAMEPLAY_RIM_OUTER_CONTACT && rim.x == -113 &&
        rim.raw_092c == 0x05A0u && rim.raw_13e7 == 0x40u;
    rim = (NbaGameplayRimState){344, -24, 80, 20, 31, 150,
                                0, 0, 0, 0, 7, 0};
    bool outer_negative_y_edge =
        nba_gameplay_rim_step(&rim, 1u, false, false, true) ==
            NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
        rim.y == -27 && rim.velocity_y == -15 && rim.raw_096e == 7u &&
        rim.raw_13e7 == 0u;
    bool shell_gates =
        nba_gameplay_rim_step(&(NbaGameplayRimState){326,0,80,0,0,150,0,0,0,0},
                               1u, false, false, true) == NBA_GAMEPLAY_RIM_FLIGHT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){327,0,80,0,0,150,0,0,0,0},
                               1u, false, false, true) == NBA_GAMEPLAY_RIM_MISS &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){348,0,80,20,20,150,0,0,0,0},
                               1u, false, false, true) ==
            NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){349,0,80,20,20,150,0,0,0,0},
                               1u, false, false, true) == NBA_GAMEPLAY_RIM_FLIGHT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){344,-28,80,20,20,150,0,0,0,0},
                               1u, false, false, true) == NBA_GAMEPLAY_RIM_FLIGHT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){344,-27,80,20,20,150,0,0,0,0},
                               1u, false, false, true) ==
            NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){344,-24,80,20,20,150,0,0,0,0},
                               1u, false, false, true) ==
            NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){344,26,80,20,20,150,0,0,0,0},
                               1u, false, false, true) ==
            NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){344,27,80,20,20,150,0,0,0,0},
                               1u, false, false, true) == NBA_GAMEPLAY_RIM_FLIGHT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){344,0,72,20,20,150,0,0,0,0},
                               1u, false, false, true) == NBA_GAMEPLAY_RIM_FLIGHT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){344,0,73,20,20,150,0,0,0,0},
                               1u, false, false, true) ==
            NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){344,0,122,20,20,150,0,0,0,0},
                               1u, false, false, true) ==
            NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
        nba_gameplay_rim_step(&(NbaGameplayRimState){344,0,123,20,20,150,0,0,0,0},
                               1u, false, false, true) == NBA_GAMEPLAY_RIM_FLIGHT;
    NbaGameplayPosePoint points[2] = {{100, 100, 80}, {140, 120, 60}};
    bool acquisition_gates =
        nba_gameplay_ball_coarse_contact(0, 0, 0, -16, -15, 71, false) &&
        nba_gameplay_ball_coarse_contact(0, 0, 0, 16, 16, 71, false) &&
        !nba_gameplay_ball_coarse_contact(0, 0, 0, -17, 0, 0, false) &&
        !nba_gameplay_ball_coarse_contact(0, 0, 0, 17, 0, 0, false) &&
        !nba_gameplay_ball_coarse_contact(0, 0, 0, 0, -16, 0, false) &&
        !nba_gameplay_ball_coarse_contact(0, 0, 0, 0, 17, 0, false) &&
        !nba_gameplay_ball_coarse_contact(0, 0, 0, 0, 0, 72, false) &&
        nba_gameplay_ball_coarse_contact(0, 0, 0, 0, 0, 95, true) &&
        !nba_gameplay_ball_coarse_contact(0, 0, 0, 0, 0, 96, true) &&
        nba_gameplay_ball_pose_contact(points, 115, 115, 95, 16u) &&
        !nba_gameplay_ball_pose_contact(points, 116, 100, 80, 16u) &&
        nba_gameplay_ball_pose_contact(points, 107, 107, 87, 8u) &&
        !nba_gameplay_ball_pose_contact(points, 108, 100, 80, 8u);
    bool contact_selector = nba_gameplay_ball_pose_contact_index(
        points, 115, 115, 95, 16u) == 0 &&
        nba_gameplay_ball_pose_contact_index(
            points, 140, 120, 60, 8u) == 1 &&
        nba_gameplay_ball_pose_contact_index(
            points, 108, 100, 80, 8u) == -1;
    NbaGameplayRng deflect_rng = {0u};
    NbaGameplayRimState deflect = {
        0, 0, 0x2F, 64, -101, -32, 0, 0, 0, 0, 0, 0
    };
    nba_gameplay_ball_apply_deflection(&deflect, &deflect_rng);
    bool deflection_vectors = deflect.velocity_x == -16 &&
        deflect.velocity_y == -26 && deflect.velocity_z == -32 &&
        deflect_rng.state == 0u;
    deflect.z = 0x30;
    deflect.velocity_x = 64;
    deflect.velocity_y = -64;
    deflect.velocity_z = -32;
    nba_gameplay_rng_seed(&deflect_rng, 0u);
    nba_gameplay_ball_apply_deflection(&deflect, &deflect_rng);
    deflection_vectors = deflection_vectors && deflect.velocity_x == -16 &&
        deflect.velocity_y == -16 && deflect.velocity_z == 16 &&
        deflect_rng.state == 0x9146u;
    NbaGameplayRng owned_rng;
    nba_gameplay_rng_seed(&owned_rng, 64u);
    bool owned_contact_vectors = nba_gameplay_owned_contact_attempt(
        &owned_rng, 0u, 1u, 128u, 0u, 45u, true) ==
            NBA_GAMEPLAY_OWNED_CONTACT_FOUL && owned_rng.state == 0x0000u;
    nba_gameplay_rng_seed(&owned_rng, 8192u);
    owned_contact_vectors = owned_contact_vectors &&
        nba_gameplay_owned_contact_attempt(
            &owned_rng, 0u, 1u, 128u, 0u, 45u, true) ==
            NBA_GAMEPLAY_OWNED_CONTACT_STRIP && owned_rng.state == 0x010Eu;
    nba_gameplay_rng_seed(&owned_rng, 8192u);
    owned_contact_vectors = owned_contact_vectors &&
        nba_gameplay_owned_contact_attempt(
            &owned_rng, 0u, 0u, 128u, 0u, 45u, true) ==
            NBA_GAMEPLAY_OWNED_CONTACT_NONE && owned_rng.state == 0x8000u;
    NbaGameplayRng detached_rng;
    nba_gameplay_rng_seed(&detached_rng, 0x0080u);
    bool detached_contact_vectors =
        nba_gameplay_detached_shot_contact_attempt(
            &detached_rng, 1u, false) &&
        detached_rng.state == 0x0100u;
    nba_gameplay_rng_seed(&detached_rng, 0u);
    detached_contact_vectors = detached_contact_vectors &&
        !nba_gameplay_detached_shot_contact_attempt(
            &detached_rng, 0u, false) &&
        detached_rng.state == 0x9146u &&
        nba_gameplay_detached_shot_contact_attempt(NULL, 0u, true);
    NbaGameplayRng edge_rng = {0x9146u};
    NbaGameplayRimContext edge_context = {
        .raw_0920 = 5u, .raw_0936 = 1u, .raw_0948 = 4u,
        .raw_094a = 5u, .raw_09f8 = 1u, .raw_07f6 = 0x0012u
    };
    rim = (NbaGameplayRimState){343, -4, 74, -160, -161, -100};
    nba_gameplay_rim_apply_inner_response(
        &rim, NBA_GAMEPLAY_RIM_EDGE_CONTACT, &edge_context, &edge_rng);
    bool edge_response =
        rim.velocity_x == 42 && rim.velocity_y == 40 &&
        rim.velocity_z == 100 && rim.raw_092c == 0x05A0u &&
        rim.raw_097c == 0x05A0u && rim.raw_096a == 0u &&
        (rim.raw_13e7 & 0x0002u) != 0u &&
        edge_context.raw_0920 == 6u && edge_context.raw_0948 == 0u &&
        edge_context.raw_094a == 0u && edge_context.raw_0970 == 0x000Fu &&
        edge_context.raw_09f8 == 0u && edge_context.effect_raw_401b == 0u &&
        edge_rng.state == 0x3F0Bu;
    NbaGameplayRng miss_rng = {0x9146u};
    NbaGameplayRimContext miss_context = {
        .raw_0920 = 9u, .raw_0936 = 1u, .raw_0948 = 7u,
        .raw_094a = 8u, .raw_09f8 = 1u, .raw_07f6 = 0x0012u
    };
    rim = (NbaGameplayRimState){336, 8, 74, 100, -61, -200};
    nba_gameplay_rim_apply_inner_response(
        &rim, NBA_GAMEPLAY_RIM_MISS, &miss_context, &miss_rng);
    bool miss_response =
        rim.velocity_x == 71 && rim.velocity_y == -31 &&
        rim.velocity_z == 100 && miss_context.raw_0920 == 10u &&
        miss_context.raw_0936 == 0u && miss_context.raw_0948 == 0u &&
        miss_context.raw_094a == 0u && miss_context.raw_0970 == 0x000Fu &&
        miss_context.raw_09f8 == 0u && miss_context.effect_raw_401b == 3u &&
        miss_rng.state == 0x3F0Bu;
    bool inner_distance_vectors = true;
    for (int side = -1; side <= 1; side += 2) {
        for (int distance = 8; distance <= 10; ++distance) {
            NbaGameplayRimState vector = {
                (int16_t)(side * 336), (int16_t)distance, 74,
                0, 0, -200
            };
            if (nba_gameplay_rim_step(&vector, 1u, false, false, true) !=
                NBA_GAMEPLAY_RIM_MISS) inner_distance_vectors = false;
        }
    }
    NbaGameplayRimContext make_context = {
        .raw_0948 = 0xFFFFu, .raw_094a = 7u, .raw_09b8 = 9u
    };
    rim = (NbaGameplayRimState){335, 3, 77, 95, -65, -73,
                                1u, 9u, 8u, 0u, 0u, 0u};
    nba_gameplay_rim_apply_made_response(&rim, true, &make_context);
    bool made_response =
        rim.x == 336 && rim.y == 0 && rim.z == 77 &&
        rim.velocity_x == 0 && rim.velocity_y == 0 &&
        rim.velocity_z == -10 && rim.raw_092c == 0x05A0u &&
        rim.raw_0962 == 0u && rim.raw_096a == 0u &&
        make_context.raw_0948 == 0u && make_context.raw_094a == 0u &&
        make_context.raw_09b8 == 0u;
    NbaGameplaySettleContext settle_context = {
        .raw_0936 = 2u, .raw_0942 = 4u, .raw_0944 = 5u,
        .raw_0946 = 6u, .raw_0948 = 7u, .raw_094a = 8u,
        .raw_0978 = 10u, .raw_09b8 = 1u
    };
    rim = (NbaGameplayRimState){0, 0, 0, 100, -101, 23,
                                0u, 0u, 0u, 9u, 0u, 0u};
    bool settle_response = nba_gameplay_ball_apply_settle(
        &rim, &settle_context) && rim.velocity_x == 50 &&
        rim.velocity_y == -51 && rim.velocity_z == 0 &&
        rim.raw_097c == 1u && settle_context.raw_0936 == 0u &&
        settle_context.raw_0942 == 0xFFFFu &&
        settle_context.raw_0944 == 0xFFFFu &&
        settle_context.raw_0946 == 0xFFFFu &&
        settle_context.raw_0948 == 0u && settle_context.raw_094a == 0u &&
        settle_context.raw_09b8 == 0u;
    rim = (NbaGameplayRimState){0, 0, 0, 100, -101, 24};
    bool settle_gates = !nba_gameplay_ball_apply_settle(
        &rim, &settle_context);
    rim.z = 1;
    rim.velocity_z = 0;
    settle_gates = settle_gates && !nba_gameplay_ball_apply_settle(
        &rim, &settle_context);
    uint16_t impact_raw = 0u;
    rim = (NbaGameplayRimState){0, 0, 0, 160, -161, -83,
                                0u, 0u, 0u, 0u, 0u, 8u};
    nba_gameplay_ball_apply_ground_impact(&rim, &impact_raw);
    bool ground_impact = rim.velocity_x == 150 &&
        rim.velocity_y == -150 && rim.velocity_z == 72 &&
        impact_raw == 72u && rim.raw_13e7 == 9u;
    rim = (NbaGameplayRimState){0, 0, 0, 0, 0, -82};
    nba_gameplay_ball_apply_ground_impact(&rim, &impact_raw);
    ground_impact = ground_impact && rim.velocity_z == 71 &&
        impact_raw == 71u && rim.raw_13e7 == 0u;
    uint16_t catch_clock = 0u, catch_mode = 3u;
    uint16_t catch_timer = 9u, catch_flags = 0x55AAu;
    nba_gameplay_apply_catch_mode(
        0x4321u, &catch_clock, &catch_mode, &catch_timer, &catch_flags);
    bool catch_modes = catch_clock == 0x4321u && catch_mode == 11u &&
        catch_timer == 0u && catch_flags == 0u;
    catch_mode = 14u;
    catch_timer = 7u;
    catch_flags = 0x1234u;
    nba_gameplay_apply_catch_mode(
        0x5678u, &catch_clock, &catch_mode, &catch_timer, &catch_flags);
    catch_modes = catch_modes && catch_clock == 0x5678u &&
        catch_mode == 14u && catch_timer == 7u && catch_flags == 0x1234u;
    bool dribble_gates = nba_gameplay_owner_dribble_gate(
            1, 0u, 0u, 0u) == NBA_GAMEPLAY_OWNER_DRIBBLE_SKIP &&
        nba_gameplay_owner_dribble_gate(
            0, 1u, 0u, 0u) == NBA_GAMEPLAY_OWNER_DRIBBLE_FALLBACK &&
        nba_gameplay_owner_dribble_gate(
            0, 0u, 0x82u, 0u) == NBA_GAMEPLAY_OWNER_DRIBBLE_FALLBACK &&
        nba_gameplay_owner_dribble_gate(
            0, 0u, 0u, 0x01FFu) == NBA_GAMEPLAY_OWNER_DRIBBLE_CONTINUE &&
        nba_gameplay_owner_dribble_gate(
            0, 0u, 0u, 0x0200u) == NBA_GAMEPLAY_OWNER_DRIBBLE_FALLBACK;
    uint8_t proximity_facing = 3u;
    bool proximity_gates = nba_gameplay_owner_dribble_proximity(
            336, -1, 0u, 0u, 5u, 0u, 0u, &proximity_facing) ==
            NBA_GAMEPLAY_OWNER_PROXIMITY_FALLBACK && proximity_facing == 3u;
    proximity_gates = proximity_gates &&
        nba_gameplay_owner_dribble_proximity(
            336, 1, 0x0200u, 0u, 5u, 0u, 0u, &proximity_facing) ==
            NBA_GAMEPLAY_OWNER_PROXIMITY_FALLBACK;
    proximity_gates = proximity_gates &&
        nba_gameplay_owner_dribble_proximity(
            336, 1, 0u, 0x20u, 6u, 0u, 1u, &proximity_facing) ==
            NBA_GAMEPLAY_OWNER_PROXIMITY_LATCHED && proximity_facing == 6u;
    proximity_gates = proximity_gates &&
        nba_gameplay_owner_dribble_proximity(
            336, 1, 0u, 0x20u, 7u, 0u, 0u, &proximity_facing) ==
            NBA_GAMEPLAY_OWNER_PROXIMITY_UNLATCHED && proximity_facing == 7u;
    return launch_ok && shell_gates && outer_generic && outer_y &&
           acquisition_gates && contact_selector && deflection_vectors &&
           owned_contact_vectors && detached_contact_vectors &&
           edge_response && miss_response &&
           inner_distance_vectors && made_response && settle_response &&
           settle_gates && ground_impact && catch_modes && dribble_gates &&
           proximity_gates &&
           right_world_bridge && left_world_bridge &&
           outer_negative_y_edge &&
           outer_low && outer_high &&
           nba_gameplay_rim_step(&(NbaGameplayRimState){343,0,74,0,0,0,0,0,0,0},
                                  1u, false, false, true) ==
               NBA_GAMEPLAY_RIM_EDGE_CONTACT &&
           nba_gameplay_rim_step(&(NbaGameplayRimState){343,4,74,0,0,0,0,0,0,0},
                                  1u, false, false, true) == NBA_GAMEPLAY_RIM_MISS &&
           nba_gameplay_rim_step(&(NbaGameplayRimState){343,8,74,0,0,0,0,0,0,0},
                                  1u, false, false, true) == NBA_GAMEPLAY_RIM_MISS &&
           nba_gameplay_rim_step(&(NbaGameplayRimState){343,9,74,0,0,0,0,0,0,0},
                                  1u, false, false, true) == NBA_GAMEPLAY_RIM_FLIGHT &&
           nba_gameplay_rim_step(&(NbaGameplayRimState){336,0,74,0,0,0,0,0,0,0},
                                  1u, false, false, true) == NBA_GAMEPLAY_RIM_MAKE &&
           nba_gameplay_rim_step(&(NbaGameplayRimState){342,0,74,0,0,0,0,0,0,0},
                                  1u, false, true, true) == NBA_GAMEPLAY_RIM_MISS &&
           nba_gameplay_ball_is_make(1, false, false, true, 0, 0, 74) &&
           nba_gameplay_ball_is_make(1, false, false, true, 6, 0, 82) &&
           !nba_gameplay_ball_is_make(1, false, false, true, 7, 0, 82) &&
           !nba_gameplay_ball_is_make(1, false, false, true, 0, 0, 73) &&
           !nba_gameplay_ball_is_make(1, false, true, true, 0, 0, 81) &&
           !nba_gameplay_ball_is_make(1, false, false, false, 0, 0, 81) &&
           nba_gameplay_ball_is_make(1, true, false, true, 0, 0, 68) &&
           !nba_gameplay_ball_is_make(1, true, false, true, 0, 0, 67) &&
           nba_gameplay_shot_value(false, 117, 0, true) == 2u &&
           nba_gameplay_shot_value(false, 116, 0, true) == 3u &&
           nba_gameplay_shot_value(false, -116, 0, false) == 2u &&
           nba_gameplay_shot_value(false, -115, 0, false) == 3u &&
           nba_gameplay_shot_value(false, 225, 178, true) == 2u &&
           nba_gameplay_shot_value(false, 224, 178, true) == 3u &&
           nba_gameplay_shot_value(true, 300, 0, true) == 1u &&
           nba_gameplay_shot_chance(0xC0, 0xC0, 0, true) == 210u &&
           nba_gameplay_shot_chance(0xA0, 0xA0, 2, true) == 110u &&
           nba_gameplay_shot_flight_duration(63, 0) == 40u &&
           nba_gameplay_shot_flight_duration(64, 0) == 46u &&
           nba_gameplay_shot_flight_duration(1599, 0) == 90u &&
           nba_gameplay_arithmetic_shift_right(-17, 4) == -2 &&
           nba_gameplay_arithmetic_shift_right(17, 4) == 1;
}
