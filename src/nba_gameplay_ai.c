#include "nba_gameplay_ai.h"
#include <string.h>

static int16_t arithmetic_shift_right_3(int16_t value) {
    if (value >= 0) return (int16_t)(value >> 3);
    return (int16_t)(-(((-(int)value) + 7) >> 3));
}

static int16_t wrap16(int32_t value) {
    return (int16_t)(uint16_t)value;
}

/* `$85:A930-$A969/$A971-$A9AA`: four sign-preserving CMP/ROR shifts,
 * followed by subtracting the sign word. Negative values therefore receive
 * a +1 bias even when exactly divisible by 16 (e.g. -128 becomes -7). */
static int16_t velocity_damping_div16(int16_t value) {
    if (value >= 0) return (int16_t)(value >> 4);
    return (int16_t)(-(((-(int)value) + 15) >> 4) + 1);
}

static uint16_t magnitude16(int16_t value) {
    return value < 0 ? (uint16_t)(0u - (uint16_t)value) : (uint16_t)value;
}

static const int16_t defense_tables[5][20] = {
    {0,64,118,155,168,155,118,64,0,-64,-118,-155,-168,-155,-118,-64,0,64,118,155},
    {0,24,45,59,64,59,45,24,0,-24,-45,-59,-64,-59,-45,-24,0,24,45,59},
    {0,18,33,44,48,44,33,18,0,-18,-33,-44,-48,-44,-33,-18,0,18,33,44},
    {0,9,16,22,24,22,16,9,0,-9,-16,-22,-24,-22,-16,-9,0,9,16,22},
    {0,7,13,17,19,17,13,7,0,-7,-13,-17,-19,-17,-13,-7,0,7,13,17}
};

static unsigned defense_table_index(uint16_t mode_raw_30,
                                    uint16_t flags_raw_32) {
    if (mode_raw_30 == 2u) return 4u;
    switch (flags_raw_32 & 3u) {
        case 0u: return 1u;
        case 1u: return 2u;
        default: return 3u;
    }
}

static void defense_target_with_table(
    const NbaGameplayDefenseTargetInput *in, unsigned table,
    uint8_t direction, NbaGameplayDefenseTargetOutput *out) {
    if (direction >= 16u) return;
    nba_gameplay_target_from_pair(
        in->paired_x, in->paired_y,
        in->paired_velocity_x, in->paired_velocity_y,
        defense_tables[table][direction],
        defense_tables[table][direction + 4u],
        &out->target_x, &out->target_y);
    out->target_written = true;
}

static bool subtract16_is_negative(uint16_t left, uint16_t right) {
    return ((uint16_t)(left - right) & 0x8000u) != 0u;
}

/* `$85:F347-$F3BA` (entry `$85:F34F`): exact direction key consumed by
 * `$87:B832`, plus the
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
    /* Preserve F37D/F399's wrapped CMP sign tests. Comparing separately
     * signed operands changes the original result when subtraction overflows.
     * F37D also accepts equality; F399 does not. For dx=$8000,dy=1 the
     * original swaps and returns direction6/distance$8000. This full-word
     * edge is source-verified; ordinary-court reachability is not established. */
    uint16_t y_minus_one = (uint16_t)(y - 1u);
    if (y_minus_one == x || subtract16_is_negative(y_minus_one, x)) {
        uint16_t swap = x; x = y; y = swap; key |= 2u;
    }
    uint16_t doubled_x = (uint16_t)(x << 1);
    if (subtract16_is_negative((uint16_t)(y - 1u), doubled_x)) key |= 1u;
    if (distance) *distance = (uint16_t)(y + (doubled_x >> 3));
    return direction_map[key];
}

/* $86:C217-$C222 / $86:CB5E-$CB69: contact facing uses F02D, not the
 * fine pass quantizer. Preserve the native xor4 even for zero direction8. */
uint8_t nba_gameplay_contact_facing(int16_t dx, int16_t dy) {
    return nba_gameplay_target_direction(dx,dy,NULL)^4u;
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

static bool lane_half_open_between(int16_t value, int16_t endpoint_a,
                                   int16_t endpoint_b) {
    int16_t da = wrap16((int32_t)value - endpoint_a);
    int16_t db = wrap16((int32_t)value - endpoint_b);
    return (int16_t)(uint16_t)((uint16_t)da ^ (uint16_t)db) < 0;
}

/* `$85:F5E4-$F727`: construct the asymmetric actor-to-basket rectangle and
 * reject it when an opposing center lies in both half-open axis intervals.
 * The XOR-sign tests include the numeric lower edge and exclude the upper.
 * The ROM traverses the active linked list and skips the ball record; this
 * normalized actor array contains the same ten eligible player records. */
bool nba_gameplay_lane_to_basket_clear(
    uint8_t subject_actor, int16_t basket_x,
    const NbaGameplayLaneActor *actors, uint8_t actor_count) {
    if (!actors || subject_actor >= actor_count) return false;
    const NbaGameplayLaneActor *subject = &actors[subject_actor];
    int16_t x_a, x_b, y_a, y_b;
    if (subject->x < basket_x) {
        x_a = wrap16((int32_t)subject->x - 8);
        x_b = wrap16((int32_t)basket_x + 24);
    } else {
        x_a = wrap16((int32_t)subject->x + 8);
        x_b = wrap16((int32_t)basket_x - 24);
    }
    if (subject->y < 0) {
        y_a = wrap16((int32_t)subject->y - 24);
        y_b = 24;
    } else {
        y_a = wrap16((int32_t)subject->y + 24);
        y_b = -24;
    }
    for (uint8_t other = 0; other < actor_count; ++other) {
        if (actors[other].team_group == subject->team_group) continue;
        if (lane_half_open_between(actors[other].x, x_a, x_b) &&
            lane_half_open_between(actors[other].y, y_a, y_b))
            return false;
    }
    return true;
}

static uint8_t inbound_edge_play(int16_t x, int16_t y,
                                 int16_t context_anchor_x,
                                 NbaGameplayRng *rng) {
    if ((int16_t)(context_anchor_x ^ x) < 0)
        return y < -72 ? 0u : y >= 72 ? 2u : 1u;
    return (uint8_t)(14u + (nba_gameplay_rng_next(rng) & 3u));
}

static uint8_t inbound_endline_play(int16_t x, int16_t context_anchor_x,
                                    NbaGameplayRng *rng) {
    uint16_t magnitude = x < 0 ? (uint16_t)(0u - (uint16_t)x) : (uint16_t)x;
    if ((int16_t)(context_anchor_x ^ x) < 0)
        return magnitude < 136u ? 5u : magnitude >= 272u ? 3u : 4u;
    return (uint8_t)((magnitude < 136u ? 6u : 10u) +
                     (nba_gameplay_rng_next(rng) & 3u));
}

/* `$85:C37D-$C5C0`: derive the designated inbound actor's raw court target,
 * facing, and any play request. Layout values above five are ROM-impossible. */
bool nba_gameplay_inbound_target(
    int16_t layout_state, int16_t source_x, int16_t source_y,
    int16_t context_anchor_x, int16_t ball_x, NbaGameplayRng *rng,
    NbaGameplayInboundTarget *target) {
    if (!rng || !target || layout_state > 5) return false;
    int16_t x = 0, y = 0;
    uint8_t direction = 0u, play = 0u;
    bool request = false;
    if (layout_state == 0) {
        x = 394; y = -64; direction = 6u;
        if (context_anchor_x >= 0) { x = -394; y = 64; direction = 2u; }
        play = inbound_edge_play(x, y, context_anchor_x, rng);
        request = true;
    } else if (layout_state < 0 || layout_state == 1) {
        /* $85:C39C CMP #2 -> C3A4 BPL tests the subtraction's N flag.
         * Layout 1 therefore takes C50B, like the negative edge layout;
         * it must not reach C450's ball-X/endline path for layout 4.
         * The old translation could request unreachable X=404 from a
         * player capped at394 and wait forever at F4F2. */
        x = source_x < 0 ? -394 : 394;
        y = source_y < -160 ? -160 : source_y > 160 ? 160 : source_y;
        direction = source_x < 0 ? 2u : 6u;
        play = inbound_edge_play(x, y, context_anchor_x, rng);
        request = true;
    } else if (layout_state == 2) {
        /* `$85:C49E-$C4D1`: unlike the negative-state edge inbound, state 2
         * is an endline layout. X comes only from side context +$0A; `$09B2`
         * selects the fixed +/-224 Y edge and its 0/4 facing. */
        x = context_anchor_x >= 0 ? -226 : 226;
        y = source_y < 0 ? -224 : 224;
        direction = source_y < 0 ? 0u : 4u;
        play = inbound_endline_play(x, context_anchor_x, rng);
        request = true;
    } else if (layout_state == 4) {
        if ((int16_t)(context_anchor_x ^ source_x) < 0)
            x = context_anchor_x < 0 ? -40 : 40;
        else x = ball_x;
        y = source_y < 0 ? -224 : 224;
        direction = source_y < 0 ? 0u : 4u;
        play = inbound_endline_play(x, context_anchor_x, rng);
        request = true;
    } else if (layout_state == 3) {
        x = source_x < -332 ? -332 : source_x > 337 ? 337 : source_x;
        y = source_y < 0 ? -224 : 224;
        direction = source_y < 0 ? 0u : 4u;
        play = inbound_endline_play(x, context_anchor_x, rng);
        request = true;
    } else { /* state 5 */
        x = source_x; y = source_y;
        if (x >= 362) {
            play = inbound_edge_play(x, y, context_anchor_x, rng);
            request = true; x = 394;
        } else if (x < -362) {
            play = inbound_edge_play(x, y, context_anchor_x, rng);
            request = true; x = -394;
        }
        if (y >= 192) {
            play = inbound_endline_play(x, context_anchor_x, rng);
            request = true; y = 224;
        } else if (y < -192) {
            play = inbound_endline_play(x, context_anchor_x, rng);
            request = true; y = -224;
        }
        direction = y >= 208 ? 4u : y < -208 ? 0u : x >= 378 ? 6u : 2u;
    }
    /* `$85:C579-$C59F`: the same diagonal court correction used by actors. */
    if (y < 0) {
        int16_t minimum = (int16_t)(-556 - y);
        if (x < minimum) x = minimum;
    } else {
        int16_t maximum = (int16_t)(561 - y);
        if (x > maximum) x = maximum;
    }
    target->x = x; target->y = y; target->direction = direction;
    target->play_code = play; target->play_requested = request;
    return true;
}

/* `$86:F4F2-$F51D`: signed comparisons accept exactly [-9,+8]. */
bool nba_gameplay_inbound_arrived(int16_t actor_x, int16_t actor_y,
                                  int16_t target_x, int16_t target_y) {
    int16_t dx = (int16_t)(target_x - actor_x);
    int16_t dy = (int16_t)(target_y - actor_y);
    return dx >= -9 && dx < 9 && dy >= -9 && dy < 9;
}

static void inbound_compensated_target(
        int16_t target_x, int16_t target_y,
        int16_t velocity_x, int16_t velocity_y,
        int16_t *steering_x, int16_t *steering_y) {
    *steering_x = wrap16((int32_t)target_x -
                         velocity_damping_div16(velocity_x));
    *steering_y = wrap16((int32_t)target_y -
                         velocity_damping_div16(velocity_y));
}

/* `$86:F59F-$F5BB`: 240+ waits, 120..239 is RNG-gated, below 120 is due. */
bool nba_gameplay_inbound_pass_due(uint16_t timer, uint16_t random_word) {
    if ((int16_t)timer >= 240) return false;
    return timer < 120u || (random_word & 0x003Cu) == 0u;
}

/* `$86:F5C7-$F648`: ordered play selectors, late fallback, then the
 * baseline-side gate. `$85:B60B` supplies candidate validity; the fallback
 * intentionally bypasses that call exactly as the native branch does. */
int8_t nba_gameplay_select_inbound_receiver_cpu(
    uint8_t inbounder, uint16_t timer, int16_t context_anchor_x,
    const int16_t selectors[3],
    const NbaGameplayReceiverState *actors, uint8_t actor_count) {
    if (!selectors || !actors || inbounder >= actor_count) return -1;
    int candidate=-1;
    for (unsigned i=0;i<3u;++i) {
        int16_t proposed=selectors[i];
        if (proposed>=0 && proposed<actor_count &&
            nba_gameplay_receiver_candidate_valid(
                inbounder,(uint8_t)proposed,actors,actor_count)) {
            candidate=proposed; break;
        }
    }
    if (candidate<0) {
        if (timer>=60u) return -1;
        candidate=(int)(inbounder/5u)*5+4;
        if (candidate==inbounder) --candidate;
    }
    int16_t owner_x=actors[inbounder].x;
    int16_t receiver_x=actors[candidate].x;
    return nba_gameplay_inbound_side_allows(
        context_anchor_x, owner_x, receiver_x) ? (int8_t)candidate : -1;
}

/* `$86:F61F-$F647` reads the active context's +$0A sign. Team slots do not
 * encode basket direction: halftime reverses the two context anchors. */
bool nba_gameplay_inbound_side_allows(int16_t context_anchor_x,
                                     int16_t owner_x, int16_t receiver_x) {
    return context_anchor_x >= 0 ? owner_x < -20 || receiver_x >= 0
                                 : owner_x >= 20 || receiver_x < 0;
}

/* `$86:F54F-$F58E`: the first arrived pass owns both ball-state words.  The
 * old host path set `$0968` but omitted `$09F6`, leaving a visually detached
 * inbound ball, and also retained stale `$09B8` when `$0946` was negative. */
void nba_gameplay_inbound_arrival_prepare(NbaGameplayInboundArrival *state) {
    if (!state) return;
    state->dead_ball_raw_0968 = 2u;
    state->attachment_raw_09f6 = 2u;
    state->behavior_flags_raw_7e |= 0x0040u;
    state->velocity_x_raw_0e = 0;
    state->velocity_y_raw_10 = 0;
    if (state->inbound_ready_raw_09ba == 0u) {
        state->whistle_raw_09b6 = 0u;
        state->foul_event_raw_0964 = 0u;
        state->inbound_ready_raw_09ba = 1u;
    }
    if (state->receiver_actor_raw_0946 < 0)
        state->transfer_raw_09b8 = 0u;
    state->draw_direction_raw_4e =
        (uint8_t)state->inbound_direction_raw_095c;
}

void nba_gameplay_dead_ball_reset(NbaGameplayDeadBallReset *state) {
    if (!state) return;
    state->award_side_group = state->camera_side_group ^ 5u;
    state->live_state = 0x82u;
    state->inbound_timer = 300u;
    state->role_rebuild_timer = 300u;
    state->game_clock = 0x05a0u;
    state->shot_clock_mirror = 0x05a0u;
    state->dead_ball = 0u;
    state->ball_aux = 0u;
    state->dead_ball_x = state->ball_x;
    state->dead_ball_y = state->ball_y;
    if (state->owner_actor < 10u) {
        state->owner_mode = 2u;
        state->ball_velocity_x = 0;
        state->ball_velocity_y = 0;
    }
    state->owner_actor = 0xffffu;
    state->rim_state = 0u;
    state->ball_record = 0x3eebu;
    state->selector = 0u;
    state->scene_phase = 0u;
}

static void defensive_pose_output_begin(
    const NbaGameplayDefensivePoseInput *input,
    NbaGameplayDefensivePoseOutput *output) {
    memset(output, 0, sizeof(*output));
    output->base_state_raw_38 = input->base_state_raw_38;
    output->facing_raw_4e = input->facing_raw_4e;
    output->requested_direction_raw_50 = input->requested_direction_raw_50;
    output->selected_count_raw_1868 = input->selected_count_raw_1868;
}

/* `$86:E39A-$E3CA`: stationary close-pair pose selector. CMP/BPL tests are
 * expressed as wrapped signed differences to retain 65816 N-flag behavior. */
bool nba_gameplay_stationary_defensive_pose(
    const NbaGameplayDefensivePoseInput *input,
    NbaGameplayDefensivePoseOutput *output) {
    if (!input || !output) return false;
    defensive_pose_output_begin(input, output);
    if ((input->actor_movement_raw_4c | input->paired_movement_raw_4c) != 0u ||
        (int16_t)(uint16_t)(input->actor_pair_distance_raw_8a - 0x31u) >= 0)
        return true;
    output->requested_direction_raw_50 = input->actor_pair_direction_raw_86;
    output->facing_raw_4e = input->actor_pair_direction_raw_86;
    output->selected_count_raw_1868 =
        (uint16_t)(input->selected_count_raw_1868 + 1u);
    output->base_state_raw_38 = 7u;
    output->selector_result_raw_aa = 1u;
    return true;
}

/* `$86:E3E1-$E4A6`: choose the defensive idle/side-step pose after the
 * defense target pass. `$87:B37C` is returned as an explicit composed child
 * request because that animation installer has its own native proof. */
bool nba_gameplay_defensive_pose(
    const NbaGameplayDefensivePoseInput *input,
    NbaGameplayDefensivePoseOutput *output) {
    if (!input || !output) return false;
    defensive_pose_output_begin(input, output);
    if (input->actor_z != 0) return true; /* `$86:E3E8` */
    bool fallback = input->free_throw_state_raw_0978 != 0u ||
        input->live_state_raw_0936 == 0x81u ||
        input->live_state_raw_0936 == 0x82u ||
        (int16_t)(uint16_t)(input->paired_anchor_distance_raw_8c -
                            input->actor_anchor_distance_raw_8c) < 0 ||
        (input->owner_actor_raw_093e < 0 &&
         input->receiver_actor_raw_0946 < 0) ||
        (((uint16_t)input->context_anchor_x_raw_0a ^
          (uint16_t)input->actor_x) & 0x8000u) == 0u;
    if (fallback) {
        output->base_state_raw_38 = 3u;
        return true;
    }
    /* `$86:E41E` clears the selector counter before either the state-7
     * child or the lateral-pose continuation. */
    output->selected_count_raw_1868 = 0u;
    if (input->control_mode == 4u) {
        NbaGameplayDefensivePoseInput selector_input = *input;
        selector_input.selected_count_raw_1868 = 0u;
        NbaGameplayDefensivePoseOutput selected;
        if (!nba_gameplay_stationary_defensive_pose(&selector_input, &selected))
            return false;
        if (selected.selector_result_raw_aa != 0u) {
            *output = selected;
            return true;
        }
    }
    if ((int16_t)(uint16_t)(input->actor_movement_raw_4c - 0x180u) >= 0 ||
        (int16_t)(uint16_t)(input->paired_movement_raw_4c - 0x180u) >= 0 ||
        (int16_t)(uint16_t)(input->actor_pair_distance_raw_8a - 0x39u) >= 0) {
        output->base_state_raw_38 = 3u;
        return true;
    }
    output->requested_direction_raw_50 = input->actor_pair_direction_raw_86;
    output->facing_raw_4e = input->actor_pair_direction_raw_86;
    uint8_t velocity_direction = nba_gameplay_target_direction(
        input->velocity_x, input->velocity_y, NULL);
    uint8_t delta = (uint8_t)((velocity_direction -
                               input->actor_pair_direction_raw_86) & 7u);
    uint8_t state = delta >= 4u ? 8u : 10u;
    uint8_t opposite = delta >= 4u ? 10u : 8u;
    if (input->upper_state_raw_30 == opposite) {
        output->install_both = true;
        output->install_state = state;
    } else {
        output->base_state_raw_38 = state;
    }
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

/* `$85:B4B9-$B50D`: the signed actor +$64 cadence reloads by 47 before
 * applying the mode/play/lane/distance cutter gates. The lane predicate is
 * kept separate because `$85:F5E4-$F727` is independently reusable. */
void nba_gameplay_special_actor_step(
    uint16_t *behavior_timer, uint8_t control_mode,
    uint16_t play_cycle_raw_09a4, bool possession_active, bool lane_clear,
    uint16_t owner_distance, uint8_t actor_id,
    uint16_t *special_actor_raw_09a2) {
    if (!behavior_timer || !special_actor_raw_09a2) return;
    int16_t remaining = (int16_t)(uint16_t)(*behavior_timer - 2u);
    *behavior_timer = (uint16_t)remaining;
    if (remaining >= 0) return;
    *behavior_timer = (uint16_t)(remaining + 0x2Fu);
    if ((control_mode == 1u || control_mode == 3u) &&
        play_cycle_raw_09a4 != 0u && possession_active && lane_clear &&
        owner_distance < 0x00A0u)
        *special_actor_raw_09a2 = actor_id;
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
                                  velocity_damping_div16(candidate_x) + accel_x);
            candidate_y = wrap16((int32_t)candidate_y -
                                  velocity_damping_div16(candidate_y) + accel_y);
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

/* `$86:F45F-$F4E5` followed by `$85:A82C`: compensate the inbound target
 * with the same sign-biased /16 operation used by native velocity damping,
 * choose an eight-way direction, then install next-pass velocity. F43A does
 * not commit coordinates; `$85:963D` already did that before dispatch. */
void nba_gameplay_inbound_motion_step(NbaGameplayInboundMotion *motion) {
    if (!motion) return;
    int16_t steering_x, steering_y;
    inbound_compensated_target(
        motion->target_x, motion->target_y,
        motion->velocity_x, motion->velocity_y,
        &steering_x, &steering_y);
    uint16_t distance = 0u;
    motion->direction = nba_gameplay_target_direction(
        wrap16((int32_t)steering_x - motion->actor_x),
        wrap16((int32_t)steering_y - motion->actor_y), &distance);
    /* F4DD supplies B6=8 to B3C9. Residuals inside that inclusive distance
     * become direction 8, causing A82C's native damping path rather than a
     * new directional acceleration. */
    if (distance <= 8u) motion->direction = 8u;
    nba_gameplay_velocity_step(
        &motion->velocity_x, &motion->velocity_y, &motion->boost_timer,
        motion->direction, motion->profile_42, motion->dispatch_dt,
        motion->movement_blocked, motion->owner_actor_raw_093e);
}

/* `$87:A5B6` calls `$85:F02D`, whose first slope comparison takes only N.
 * F34F's extra equality swap is a different routine (e.g. dx=0,dy=1).
 * Keep this draw-specific correction separate from other gameplay callers. */
static uint8_t draw_facing_f02d(int16_t dx,int16_t dy) {
    static const uint8_t map[16]={0,1,2,1,4,3,2,3,0,7,6,7,4,5,6,5};
    if (!(dx|dy))return 8u;
    uint16_t x=(uint16_t)dx,y=(uint16_t)dy,key=0u;
    if(dx<0){x=(uint16_t)(0u-x);key|=8u;}
    if(dy<0){y=(uint16_t)(0u-y);key|=4u;}
    if(subtract16_is_negative((uint16_t)(y-1u),x)) {
        uint16_t swap=x;x=y;y=swap;key|=2u;
    }
    if(subtract16_is_negative((uint16_t)(y-1u),(uint16_t)(x*2u)))key|=1u;
    return map[key];
}

uint8_t nba_gameplay_draw_direction(const NbaGameplayDrawDirection *input) {
    if (!input) return 0u;
    uint8_t current = input->current_direction & 7u;
    /* `$87:A52F-$A555`: mode 8's two status bits rotate the visible pose by
     * one step without changing actor +$52. */
    if (input->control_mode == 8u) {
        if (input->actor_status & 0x0008u) return (uint8_t)((current - 1u) & 7u);
        if (input->actor_status & 0x0010u) return (uint8_t)((current + 1u) & 7u);
        return current;
    }
    uint16_t candidate;
    if (input->candidate_valid) {
        candidate = draw_facing_f02d(input->candidate_dx,input->candidate_dy);
    } else if (input->upper_state == 20u || input->upper_state == 21u) {
        /* `$87:A59C-$A5A2`: logical word shift, then the common candidate
         * gate. Do not mask to three bits: values >=8 retain movement facing. */
        candidate = input->anchor_direction >> 1;
    } else {
        return current;
    }
    if (candidate >= 8u) return current;
    /* `$87:A5C1-$A5F5`: large quarter-turn differences are eased by two
     * directions; adjacent and wrap-adjacent targets adopt immediately. */
    int16_t delta = (int16_t)candidate - (int16_t)current;
    uint16_t magnitude = (uint16_t)(delta < 0 ? -delta : delta);
    if (magnitude < 3u || magnitude >= 6u) return (uint8_t)candidate;
    return (uint8_t)(((delta & 7) == 5 ? current - 2u : current + 2u) & 7u);
}

void nba_gameplay_prepare_player_draw(
        const NbaGameplayDrawPreparationInput *input,
        NbaGameplayDrawPreparation *output) {
    static const int16_t head_offset[8] = {3, 2, 1, 0, 1, 2, 3, 4};
    if (!output) return;
    NbaGameplayDrawPreparation next = {0};
    if (!input) { *output = next; return; }
    next.direction = nba_gameplay_draw_direction(&input->direction);
    next.status = (uint16_t)(input->status & 0xFFFBu);
    if (next.direction < 3u) next.status |= 0x0004u;
    next.upper_resource = input->upper_resource;
    next.lower_resource = input->lower_resource;
    next.head_resource = (uint16_t)(input->head_base +
        head_offset[next.direction]);
    uint16_t priority = 0x3000u;
    if (input->world_x < 0 ||
        (input->world_x >= 346 && (int16_t)(input->world_y + 8) < 0))
        priority = 0x2000u;
    next.attribute = (uint16_t)(input->palette_offset + priority);
    next.x = input->screen_x;
    next.y = (int16_t)(input->screen_y - input->world_z);
    *output = next;
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

/* `$85:B971-$B9D1`: wrapped max(|dx|,|dy|) + min/4, randomized by the
 * upper four useful RNG bits and clamped to `$96` with the original
 * subtraction-N tests. Widening either coordinate subtraction changes the
 * full-word edge cases before the absolute-value and clamp branches. */
uint16_t nba_gameplay_reaction_threshold(NbaGameplayRng *rng,
                                         int16_t actor_x, int16_t actor_y,
                                         int16_t ball_x, int16_t ball_y) {
    uint16_t dx = (uint16_t)((uint16_t)actor_x - (uint16_t)ball_x);
    uint16_t dy = (uint16_t)((uint16_t)actor_y - (uint16_t)ball_y);
    if ((dx & 0x8000u) != 0u) dx = (uint16_t)(0u - dx);
    if ((dy & 0x8000u) != 0u) dy = (uint16_t)(0u - dy);
    if (subtract16_is_negative(dx, dy)) {
        uint16_t swap = dx; dx = dy; dy = swap;
    }
    uint16_t result = (uint16_t)(dx + (dy >> 2));
    result = (uint16_t)(result + (nba_gameplay_rng_next(rng) & 0x78u));
    if (!subtract16_is_negative(result, 0x0096u)) result = 0x0096u;
    return result;
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

/* `$86:E923-$E96E`: predict the paired actor by one eighth of velocity,
 * then add the caller-selected formation-table displacement. X preserves
 * the 16-bit carry from `bias + position` into the table ADC; Y explicitly
 * clears carry before its table ADC. */
void nba_gameplay_target_from_pair(int16_t paired_x, int16_t paired_y,
                                   int16_t paired_velocity_x,
                                   int16_t paired_velocity_y,
                                   int16_t offset_x, int16_t offset_y,
                                   int16_t *target_x, int16_t *target_y) {
    if (target_x) {
        uint32_t first = (uint16_t)paired_x +
                         (uint16_t)arithmetic_shift_right_3(paired_velocity_x);
        *target_x = (int16_t)(uint16_t)(first + (uint16_t)offset_x +
                                       (first >> 16));
    }
    if (target_y)
        *target_y = (int16_t)(paired_y +
            arithmetic_shift_right_3(paired_velocity_y) + offset_y);
}

/* `$86:E82F-$E8F6` circular spacing tables. `$86:E8F7-$E922` selects the
 * ordinary table from side-context +$30/+$32; `$86:E7DC` can force the
 * closest `$E8CF` table before `$86:E923-$E96E` predicts pair motion. */
void nba_gameplay_defense_pair_target(
    int16_t paired_x, int16_t paired_y,
    int16_t paired_velocity_x, int16_t paired_velocity_y,
    uint8_t paired_direction, uint16_t context_raw_30,
    uint16_t context_raw_32, bool force_close_table,
    int16_t *target_x, int16_t *target_y) {
    unsigned table_index;
    if (force_close_table || context_raw_30 == 2u) table_index = 4u;
    else table_index = defense_table_index(context_raw_30, context_raw_32);
    if (paired_direction >= 16u) return;
    unsigned direction = paired_direction;
    nba_gameplay_target_from_pair(
        paired_x, paired_y, paired_velocity_x, paired_velocity_y,
        defense_tables[table_index][direction],
        defense_tables[table_index][direction + 4u],
        target_x, target_y);
}

/* Portable target-writing core of defensive modes `$86:F6CD/$F794/$F8CD`.
 * Caller owns their timer/lock/action gates; this function translates the
 * exact `$E7B3/$E7DC/$E96F/$E6B7/$E9B3` geometry dispatcher. */
bool nba_gameplay_defense_mode_target(
    uint8_t actor_mode, const NbaGameplayDefenseTargetInput *in,
    NbaGameplayDefenseTargetOutput *out) {
    if (!in || !out || (actor_mode != 2u && actor_mode != 4u &&
                        actor_mode != 6u)) return false;
    out->target_written = false;
    out->stop_velocity = false;
    uint8_t d = in->paired_anchor_direction_raw_88;
    if (d >= 16u) return true;

    if (actor_mode == 6u) { /* `$86:E9B3-$EA03` */
        static const uint8_t negative_map[16] = {
            14,15,0,1,2,7,8,9,10,11,12,13,10,11,12,13
        };
        static const uint8_t positive_map[16] = {
            2,3,4,5,6,3,4,5,6,7,8,9,14,15,0,1
        };
        uint8_t mapped = in->context_anchor_x < 0 ?
            negative_map[d] : positive_map[d];
        unsigned table = in->paired_anchor_distance_raw_8c < 0x50u ? 4u : 3u;
        defense_target_with_table(in, table, mapped, out);
        return true;
    }

    unsigned ordinary = defense_table_index(
        in->context_mode_raw_30, in->context_flags_raw_32);
    if (in->context_mode_raw_30 == 3u) { /* `$86:E6B7-$E7B0` */
        defense_target_with_table(in, ordinary, d, out);
        bool opposite_half =
            (int16_t)(in->context_anchor_x ^ in->paired_x) < 0;
        if (in->paired_on_three_point_arc &&
            !(opposite_half && in->paired_three_point_rating >= 0xC2u)) {
            if (d < 16u) {
                out->target_x = wrap16(
                    -(int32_t)in->context_anchor_x - defense_tables[0][d] +
                    arithmetic_shift_right_3(in->paired_velocity_x));
                out->target_y = wrap16(
                    -(int32_t)defense_tables[0][d + 4u] +
                    arithmetic_shift_right_3(in->paired_velocity_y));
                out->target_written = true;
            }
        }
        return true;
    }

    if (in->context_mode_raw_30 != 0u &&
        (int16_t)(in->context_anchor_x ^ in->paired_x) >= 0) {
        /* `$86:E7B3`: normal Y geometry, absolute weak-side X override. */
        defense_target_with_table(in, ordinary, d, out);
        if (out->target_written)
            out->target_x = in->context_anchor_x < 0 ? 48 : -48;
        return true;
    }

    if (in->paired_position_raw_92 >= 3u) { /* `$86:E96F` */
        if (in->paired_anchor_distance_raw_8c < 0x60u &&
            in->actor_pair_distance_raw_8a < 0x28u) {
            uint8_t relative = (uint8_t)((
                (((uint16_t)in->actor_pair_direction_raw_86 << 1) ^ 8u) -
                d + 1u) & 15u);
            if (relative < 3u) {
                out->stop_velocity = true;
                return true;
            }
        }
        unsigned table = in->paired_anchor_distance_raw_8c < 0x50u ? 4u : 3u;
        defense_target_with_table(in, table, d, out);
        return true;
    }

    /* `$86:E7DC`: E8F7 selection with close-basket radius override. */
    if (in->paired_anchor_distance_raw_8c < 0x50u) ordinary = 4u;
    defense_target_with_table(in, ordinary, d, out);
    return true;
}

/* `$85:AFC2-$B103` uses the same major-plus-minor-quarter metric for its
 * predicted-ball scan as the rest of the formation planner. Arithmetic is
 * deliberately wrapped to a 16-bit word. */
uint16_t nba_gameplay_weighted_distance(int16_t dx, int16_t dy) {
    uint16_t x = magnitude16(dx), y = magnitude16(dy);
    uint16_t high = x >= y ? x : y;
    uint16_t low = x >= y ? y : x;
    return (uint16_t)(high + (low >> 2));
}

/* `$85:AFB2-$B128`: AF5C first normalizes only this side's modes below
 * seven to mode 1. With no owner/receiver, it scans the five records against
 * predicted ball `$0918/$091A`; a strictly smaller distance replaces the
 * winner, so equal distances retain the earliest record. */
int8_t nba_gameplay_select_no_owner_pursuer(
    const NbaGameplayLoosePursuitActor actors[5],
    int16_t predicted_ball_x, int16_t predicted_ball_y,
    uint8_t normalized_modes[5]) {
    if (!actors) return -1;
    uint16_t best = 0x7FFFu;
    int8_t selected = -1;
    for (unsigned i = 0; i < 5u; ++i) {
        uint8_t mode = actors[i].control_mode;
        if (normalized_modes) normalized_modes[i] = mode < 7u ? 1u : mode;
        if (mode >= 7u) continue;
        uint16_t distance = nba_gameplay_weighted_distance(
            wrap16((int32_t)actors[i].x - predicted_ball_x),
            wrap16((int32_t)actors[i].y - predicted_ball_y));
        /* Native `CMP best / BPL` is a wrapped N-bit test, not unsigned `<`. */
        if ((int16_t)(uint16_t)(distance - best) < 0) {
            best = distance;
            selected = (int8_t)i;
        }
    }
    if (normalized_modes && selected >= 0)
        normalized_modes[(unsigned)selected] = 3u;
    return selected;
}

/* Pure allow/decline result from `$86:F0FD-$F1AF`. The caller owns F134's
 * dead-ball clock/facing writes and B3AA's actual steering side effects. */
bool nba_gameplay_loose_ball_pursuit_allowed(
    const NbaGameplayLoosePursuitGateInput *in) {
    if (!in) return false;

    if (in->live_state_raw_0936 == 0x82u) {
        if (in->actor_team_group_raw_6e != in->inbound_group_raw_0952)
            return false;
        if (!(in->play_code_raw_0996 < 6u &&
              in->actor_control_mode == 3u) &&
            in->free_throw_state_raw_0978 == 0u &&
            in->actor_id != 2u && in->actor_id != 7u)
            return false;
    }

    /* `$F140-$F14F`: an active foul/free-throw state bypasses the ordinary
     * mode gate, but only the recorded `$7E492F` actor may pursue. */
    if (in->free_throw_state_raw_0978 != 0u)
        return (uint16_t)in->actor_id ==
               (uint16_t)in->foul_actor_raw_7e492f;

    if (in->live_state_raw_0936 != 0x82u &&
        in->actor_control_mode != 3u && in->actor_control_mode != 4u)
        return false;

    if (in->ball_activity_raw_0948 == 0u)
        return in->bounce_age_raw_094a == 0u ||
               in->bounce_age_raw_094a >= 0x1Eu;

    /* `$86:F197-$F1AE`: the non-offense group goes straight to pursuit.
     * The offense group is limited to its first three logical records. */
    if (in->actor_team_group_raw_6e != in->offense_group_raw_093a)
        return true;
    return (int16_t)(uint16_t)(in->actor_id -
           in->actor_team_group_raw_6e) < 3;
}

/* `$85:B714-$B833`: exact symmetric mode-11 direct-shot rectangle. The
 * negative X path explicitly computes two's-complement magnitude. */
bool nba_gameplay_mode11_shot_rectangle(int16_t rom_x, int16_t y, int16_t z) {
    if (rom_x < -338 || rom_x >= 338 || z != 0) return false;
    uint16_t abs_x = rom_x < 0 ? (uint16_t)(0u - (uint16_t)rom_x) :
                                 (uint16_t)rom_x;
    return abs_x >= 0x00E2u && y >= -0x0040 && y < 0x0040;
}

/* `$85:B734-$B820`: rating/distance tail of the CPU-owned mode-11 shot
 * decision. Calls to `$80:CEE7/$80:CEFD` are conditional and ordered; even a
 * rejected shot must leave the shared LFSR at the same state as the ROM. */
bool nba_gameplay_mode11_shot_decision(
    const NbaGameplayMode11ShotInput *in, NbaGameplayRng *rng) {
    static const uint16_t distance_minimum[3] = {0x18u, 0x20u, 0x20u};
    if (!in || !rng || !in->same_attack_half || in->difficulty_raw_17af > 2u)
        return false;

    bool accepted = in->play_hold_raw_09d0 != 0u &&
                    in->play_step_raw_0998 == 4;
    uint16_t difficulty = in->difficulty_raw_17af;
    uint16_t distance = distance_minimum[difficulty];
    uint16_t rating = (uint16_t)(0x94u + (difficulty << 5));

    if (!accepted && in->play_step_raw_0998 >= 2) {
        uint16_t random = nba_gameplay_rng_next(rng);
        accepted = (random & 0x000Fu) == 0u &&
                   in->anchor_distance_raw_8c >= 0x00A0u &&
                   in->assignment_distance_raw_8a >= distance &&
                   in->three_point_rating_raw_37 >= rating;
    }

    bool rating_gate = false;
    if (!accepted && in->shot_clock_rule_raw_17e1 == 0u &&
            in->play_cycle_raw_09a4 != 0u) {
        uint16_t random = nba_gameplay_rng_next(rng);
        rating_gate = (random & 0x001Fu) == 0u;
    }
    if (!accepted && !rating_gate) {
        uint16_t random = nba_gameplay_rng_next(rng);
        rating_gate = (random & 0x003Fu) == 0u &&
                      in->play_step_raw_0998 >= 3;
    }
    if (!accepted && rating_gate &&
            in->assignment_distance_raw_8a >= distance) {
        uint8_t selected_rating =
            in->shot_range_raw_49 < in->anchor_distance_raw_8c ?
            in->three_point_rating_raw_37 : in->two_point_rating_raw_36;
        accepted = selected_rating >= rating;
    }

    if (!accepted && in->dead_ball_raw_0968 != 0u) {
        uint16_t random = nba_gameplay_rng_next(rng) & 0x7FFFu;
        accepted = (random & 0x0020u) != 0u;
    }
    /* `$85:B820-$B825` checks actor +$0C only after a policy branch wins. */
    return accepted && in->actor_z == 0;
}

static int32_t fixed_integer_floor(int32_t value) {
    return value >= 0 ? value / 256 : -(((-value) + 255) / 256);
}

static void fixed_replace_integer(int32_t *value, int32_t integer) {
    int32_t old_integer = fixed_integer_floor(*value);
    int32_t fraction = *value - old_integer * 256;
    *value = integer * 256 + fraction;
}

/* `$85:A656-$A755`: the ownerless-ball court integrator clamps the
 * signed rectangle first, cancelling only outward velocity. It then applies
 * the asymmetric isometric edge by replacing integer X while preserving the
 * fractional word and velocity. The reusable geometry is also used to keep
 * host player records on court, but only the ball caller may interpret the
 * return value as the rectangular `$86:A613` cancellation path;
 * diagonal-only correction does not take it. */
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

/* `$85:A692-$A755`: X has already been integrated on entry. Finish the Y
 * fixed-point step, then apply the shared rectangular/isometric clamp. */
bool nba_gameplay_court_finish_y_step(
    int32_t *x_fp, int32_t *y_fp, int16_t *velocity_x,
    int16_t *velocity_y) {
    if (!x_fp || !y_fp || !velocity_x || !velocity_y) return false;
    *y_fp += *velocity_y;
    return nba_gameplay_court_clamp(
        x_fp, y_fp, velocity_x, velocity_y);
}

bool nba_gameplay_ai_self_test(void) {
    int16_t x = 0, y = 0;
    NbaGameplayRng reaction_rng = {0u};
    if (nba_gameplay_reaction_threshold(
            &reaction_rng, INT16_MAX, 0, INT16_MIN, 0) != 0x41u ||
        reaction_rng.state != 0x9146u) return false;
    if (nba_gameplay_weighted_distance(16, -16) != 20u ||
        nba_gameplay_weighted_distance(-20, 10) != 22u ||
        nba_gameplay_weighted_distance(INT16_MIN, INT16_MIN) != 0xA000u)
        return false;
    NbaGameplayLoosePursuitActor loose[5] = {
        {10, 0, 6u}, {5, 0, 4u}, {0, 0, 7u},
        {0, 0, 8u}, {-5, 0, 0u}
    };
    uint8_t loose_modes[5] = {0};
    if (nba_gameplay_select_no_owner_pursuer(
            loose, 0, 0, loose_modes) != 1 ||
        loose_modes[0] != 1u || loose_modes[1] != 3u ||
        loose_modes[2] != 7u || loose_modes[3] != 8u ||
        loose_modes[4] != 1u) return false; /* strict tie keeps side slot 1 */
    for (unsigned i = 0; i < 5u; ++i) loose[i].control_mode = 7u;
    if (nba_gameplay_select_no_owner_pursuer(
            loose, 0, 0, loose_modes) != -1)
        return false;
    for (unsigned i = 0; i < 5u; ++i)
        if (loose_modes[i] != 7u) return false;

    NbaGameplayLaneActor lane[10] = {0};
    lane[0] = (NbaGameplayLaneActor){100, 0, 0u};
    lane[1] = (NbaGameplayLaneActor){200, 0, 0u};
    lane[5] = (NbaGameplayLaneActor){200, 0, 5u};
    if (nba_gameplay_lane_to_basket_clear(0u, 336, lane, 10u))
        return false;
    lane[5].x = 92; /* numeric lower X endpoint is included */
    if (nba_gameplay_lane_to_basket_clear(0u, 336, lane, 10u))
        return false;
    lane[5].x = 360; /* numeric upper X endpoint is excluded */
    if (!nba_gameplay_lane_to_basket_clear(0u, 336, lane, 10u))
        return false;
    lane[5].x = 93;
    lane[5].y = 24; /* numeric upper Y endpoint is excluded */
    if (!nba_gameplay_lane_to_basket_clear(0u, 336, lane, 10u))
        return false;
    lane[5].y = -24; /* numeric lower Y endpoint is included */
    if (nba_gameplay_lane_to_basket_clear(0u, 336, lane, 10u))
        return false;
    if (nba_gameplay_lane_to_basket_clear(10u, 336, lane, 10u))
        return false;

    NbaGameplayLoosePursuitGateInput pursuit = {
        .live_state_raw_0936 = 0u,
        .actor_id = 3u,
        .actor_control_mode = 3u,
        .actor_team_group_raw_6e = 0u,
        .offense_group_raw_093a = 5u,
        .inbound_group_raw_0952 = 0u,
        .foul_actor_raw_7e492f = -1
    };
    if (!nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.bounce_age_raw_094a = 0x1Du;
    if (nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.bounce_age_raw_094a = 0x1Eu;
    if (!nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.actor_control_mode = 2u;
    if (nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.actor_control_mode = 4u;
    pursuit.ball_activity_raw_0948 = 1u;
    pursuit.actor_id = 4u;
    if (!nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.actor_team_group_raw_6e = 5u;
    pursuit.actor_id = 7u;
    if (!nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.actor_id = 8u;
    if (nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.free_throw_state_raw_0978 = 1u;
    pursuit.foul_actor_raw_7e492f = 7;
    if (nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.actor_id = 7u;
    if (!nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;

    pursuit = (NbaGameplayLoosePursuitGateInput){
        .live_state_raw_0936 = 0x82u,
        .play_code_raw_0996 = 5u,
        .actor_id = 4u,
        .actor_control_mode = 3u,
        .actor_team_group_raw_6e = 0u,
        .inbound_group_raw_0952 = 0u
    };
    if (!nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.actor_control_mode = 2u;
    if (nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.actor_id = 2u;
    if (!nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;
    pursuit.actor_team_group_raw_6e = 5u;
    if (nba_gameplay_loose_ball_pursuit_allowed(&pursuit)) return false;

    nba_gameplay_target_from_pair(100, -50, 31, -31, -20, 12, &x, &y);
    if (x != 83 || y != -42) return false;
    nba_gameplay_target_from_pair(32760, -32760, 64, -64, 16, -16, &x, &y);
    if (x != -32752 || y != 32752) return false;
    nba_gameplay_defense_pair_target(
        100, -50, 31, -31, 4u, 0u, 0u, false, &x, &y);
    if (x != 167 || y != -54) return false;
    nba_gameplay_defense_pair_target(
        100, -50, 31, -31, 4u, 0u, 0u, true, &x, &y);
    if (x != 122 || y != -54) return false;
    nba_gameplay_defense_pair_target(
        100, -50, 31, -31, 4u, 2u, 3u, false, &x, &y);
    if (x != 122 || y != -54) return false;
    NbaGameplayDefenseTargetInput defense = {
        .actor_pair_direction_raw_86 = 0u,
        .actor_pair_distance_raw_8a = 0x50u,
        .paired_x = 100, .paired_y = -50,
        .paired_velocity_x = 31, .paired_velocity_y = -31,
        .paired_anchor_direction_raw_88 = 4u,
        .paired_anchor_distance_raw_8c = 0x100u,
        .paired_position_raw_92 = 2u,
        .context_anchor_x = 336,
        .context_mode_raw_30 = 4u,
        .context_flags_raw_32 = 0u
    };
    NbaGameplayDefenseTargetOutput defense_out = {0};
    if (!nba_gameplay_defense_mode_target(2u, &defense, &defense_out) ||
        !defense_out.target_written || defense_out.stop_velocity ||
        defense_out.target_x != -48 || defense_out.target_y != -54)
        return false;
    defense.context_mode_raw_30 = 0u;
    defense.context_flags_raw_32 = 1u;
    if (!nba_gameplay_defense_mode_target(4u, &defense, &defense_out) ||
        defense_out.target_x != 151 || defense_out.target_y != -54)
        return false;
    defense.paired_position_raw_92 = 3u;
    defense.paired_anchor_direction_raw_88 = 9u;
    defense.paired_anchor_distance_raw_8c = 0x40u;
    defense.actor_pair_distance_raw_8a = 0x20u;
    if (!nba_gameplay_defense_mode_target(2u, &defense, &defense_out) ||
        defense_out.target_written || !defense_out.stop_velocity)
        return false;
    defense.paired_anchor_direction_raw_88 = 0u;
    defense.paired_anchor_distance_raw_8c = 0x100u;
    if (!nba_gameplay_defense_mode_target(6u, &defense, &defense_out) ||
        !defense_out.target_written || defense_out.target_x != 119 ||
        defense_out.target_y != -38) return false;
    defense.context_mode_raw_30 = 3u;
    defense.paired_anchor_direction_raw_88 = 4u;
    defense.paired_on_three_point_arc = true;
    defense.paired_three_point_rating = 0x80u;
    if (!nba_gameplay_defense_mode_target(2u, &defense, &defense_out) ||
        defense_out.target_x != -501 || defense_out.target_y != -4)
        return false;
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
    uint16_t cutter_timer = 47u, special_actor = 0xFFFFu;
    nba_gameplay_special_actor_step(
        &cutter_timer, 1u, 1u, true, true, 0x009Fu, 6u,
        &special_actor);
    if (cutter_timer != 45u || special_actor != 0xFFFFu) return false;
    cutter_timer = 1u;
    nba_gameplay_special_actor_step(
        &cutter_timer, 1u, 1u, true, true, 0x009Fu, 6u,
        &special_actor);
    if (cutter_timer != 46u || special_actor != 6u) return false;
    cutter_timer = 0u;
    special_actor = 0xFFFFu;
    nba_gameplay_special_actor_step(
        &cutter_timer, 1u, 1u, true, true, 0x00A0u, 6u,
        &special_actor);
    if (cutter_timer != 45u || special_actor != 0xFFFFu) return false;
    if (nba_gameplay_same_x_half(200, -336) ||
        !nba_gameplay_same_x_half(-200, -336) ||
        !nba_gameplay_same_x_half(200, 336) ||
        nba_gameplay_same_x_half(-200, 336)) return false;
    uint16_t context_mode = 4u;
    if (!nba_gameplay_defense_context_reselect(
            2u, 4u, 2u, 1u, 1u, &context_mode) || context_mode != 1u)
        return false;
    context_mode = 4u;
    if (!nba_gameplay_defense_context_reselect(
            2u, 4u, 3u, 1u, 0u, &context_mode) || context_mode != 3u)
        return false;
    context_mode = 4u;
    if (!nba_gameplay_defense_context_reselect(
            4u, 4u, 0u, 1u, 1u, &context_mode) || context_mode != 1u)
        return false;
    context_mode = 4u;
    if (nba_gameplay_defense_context_reselect(
            0u, 1u, 0u, 0u, 0u, &context_mode) || context_mode != 4u)
        return false;
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
    NbaGameplayMode11ShotInput shot = {
        .play_step_raw_0998 = 2,
        .shot_clock_rule_raw_17e1 = 1u,
        .difficulty_raw_17af = 0u,
        .assignment_distance_raw_8a = 0x18u,
        .anchor_distance_raw_8c = 0xA0u,
        .two_point_rating_raw_36 = 0x94u,
        .three_point_rating_raw_37 = 0x94u,
        .shot_range_raw_49 = 0xA0u,
        .same_attack_half = true
    };
    NbaGameplayRng shot_rng = {0x0008u};
    if (!nba_gameplay_mode11_shot_decision(&shot, &shot_rng) ||
        shot_rng.state != 0x0010u) return false;
    shot.anchor_distance_raw_8c = 0x009Fu;
    shot_rng.state = 0x0008u;
    if (nba_gameplay_mode11_shot_decision(&shot, &shot_rng) ||
        shot_rng.state != 0x0020u) return false;
    shot.play_step_raw_0998 = 3;
    shot.anchor_distance_raw_8c = 100u;
    shot.play_cycle_raw_09a4 = 1u;
    shot.shot_clock_rule_raw_17e1 = 0u;
    shot.shot_range_raw_49 = 100u;
    shot_rng.state = 0x0008u;
    if (!nba_gameplay_mode11_shot_decision(&shot, &shot_rng) ||
        shot_rng.state != 0x0020u) return false;
    shot.shot_range_raw_49 = 99u;
    shot.two_point_rating_raw_36 = 0u;
    shot.three_point_rating_raw_37 = 0x94u;
    shot_rng.state = 0x0008u;
    if (!nba_gameplay_mode11_shot_decision(&shot, &shot_rng)) return false;
    shot.shot_clock_rule_raw_17e1 = 1u;
    shot.play_cycle_raw_09a4 = 0u;
    shot.dead_ball_raw_0968 = 2u;
    shot.assignment_distance_raw_8a = 0u;
    shot_rng.state = 0x0004u;
    if (!nba_gameplay_mode11_shot_decision(&shot, &shot_rng) ||
        shot_rng.state != 0x0020u) return false;
    shot.dead_ball_raw_0968 = 0u;
    shot_rng.state = 0x0004u;
    if (nba_gameplay_mode11_shot_decision(&shot, &shot_rng) ||
        shot_rng.state != 0x0010u) return false;
    shot.play_hold_raw_09d0 = 1u;
    shot.play_step_raw_0998 = 4;
    shot_rng.state = 0x1234u;
    if (!nba_gameplay_mode11_shot_decision(&shot, &shot_rng) ||
        shot_rng.state != 0x1234u) return false;
    shot.actor_z = 1;
    if (nba_gameplay_mode11_shot_decision(&shot, &shot_rng) ||
        shot_rng.state != 0x1234u) return false;
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
    NbaGameplayRng inbound_rng = {0x9146u};
    NbaGameplayInboundTarget inbound;
    if (!nba_gameplay_inbound_target(
            0, 0, 0, -80, 0, &inbound_rng, &inbound) ||
        inbound.x != 394 || inbound.y != -64 || inbound.direction != 6u ||
        !inbound.play_requested || inbound.play_code != 1u) return false;
    if (!nba_gameplay_inbound_target(
            0, 0, 0, 80, 0, &inbound_rng, &inbound) ||
        inbound.x != -394 || inbound.y != 64 || inbound.direction != 2u ||
        !inbound.play_requested || inbound.play_code != 1u) return false;
    if (!nba_gameplay_inbound_target(
            5, 378, -52, -80, 0, &inbound_rng, &inbound) ||
        inbound.x != 394 || inbound.y != -52 || inbound.direction != 6u)
        return false;
    if (!nba_gameplay_inbound_target(
            5, -378, 52, 80, 0, &inbound_rng, &inbound) ||
        inbound.x != -394 || inbound.y != 52 || inbound.direction != 2u)
        return false;
    if (!nba_gameplay_inbound_target(
            2, 500, -12, -80, 0, &inbound_rng, &inbound) ||
        inbound.x != 226 || inbound.y != -224 || inbound.direction != 0u ||
        !inbound.play_requested || inbound.play_code != 4u) return false;
    if (!nba_gameplay_inbound_target(
            2, -500, 12, 80, 0, &inbound_rng, &inbound) ||
        inbound.x != -226 || inbound.y != 224 || inbound.direction != 4u ||
        !inbound.play_requested || inbound.play_code != 4u) return false;
    if (!nba_gameplay_inbound_target(
            3, 500, -300, -80, 0, &inbound_rng, &inbound) ||
        inbound.x != 337 || inbound.y != -224 || inbound.direction != 0u)
        return false;
    if (!nba_gameplay_inbound_arrived(0, 0, 8, 8) ||
        !nba_gameplay_inbound_arrived(0, 0, -9, -9) ||
        nba_gameplay_inbound_arrived(0, 0, 9, 0) ||
        nba_gameplay_inbound_arrived(0, 0, -10, 0)) return false;
    /* `$86:F4E6-$F4F0` restores the raw target before `$86:F4F2`.
     * A raw +9 X delta stays outside the box even when the temporary
     * steering compensation would make that delta +8. */
    if (nba_gameplay_inbound_arrived(394, -219, 403, -224)) return false;
    if (nba_gameplay_inbound_pass_due(240u, 0u) ||
        !nba_gameplay_inbound_pass_due(239u, 0u) ||
        nba_gameplay_inbound_pass_due(239u, 4u) ||
        !nba_gameplay_inbound_pass_due(119u, 0x003Cu)) return false;
    NbaGameplayInboundArrival arrival={
        9u,3u,6u,123,-456,0u,1u,7u,5u,-1,6u,1u};
    nba_gameplay_inbound_arrival_prepare(&arrival);
    if (arrival.dead_ball_raw_0968!=2u || arrival.attachment_raw_09f6!=2u ||
        arrival.behavior_flags_raw_7e!=0x46u || arrival.velocity_x_raw_0e!=0 ||
        arrival.velocity_y_raw_10!=0 || arrival.inbound_ready_raw_09ba!=1u ||
        arrival.whistle_raw_09b6!=0u || arrival.foul_event_raw_0964!=0u ||
        arrival.transfer_raw_09b8!=0u || arrival.draw_direction_raw_4e!=6u)
        return false;
    NbaGameplayReceiverState inbound_receivers[10]={0};
    for (unsigned i=0;i<10u;++i) {
        inbound_receivers[i].x=(int16_t)(-100+(int)i*25);
        inbound_receivers[i].y=(int16_t)(i*7);
        inbound_receivers[i].control_mode=1u;
        inbound_receivers[i].travel_direction=8u;
        inbound_receivers[i].travel_distance=0x40u;
    }
    inbound_receivers[7].x=-394;
    int16_t inbound_selectors[3]={7,9,6};
    if (nba_gameplay_select_inbound_receiver_cpu(
            7u,200u,336,inbound_selectors,inbound_receivers,10u)!=9)
        return false;
    inbound_receivers[9].control_mode=7u;
    if (nba_gameplay_select_inbound_receiver_cpu(
            7u,200u,336,inbound_selectors,inbound_receivers,10u)!=6)
        return false;
    inbound_receivers[6].control_mode=7u;
    if (nba_gameplay_select_inbound_receiver_cpu(
            7u,60u,336,inbound_selectors,inbound_receivers,10u)!=-1 ||
        nba_gameplay_select_inbound_receiver_cpu(
            7u,59u,336,inbound_selectors,inbound_receivers,10u)!=9)
        return false;
    inbound_receivers[7].x=0;
    inbound_receivers[9].x=-1;
    if (nba_gameplay_select_inbound_receiver_cpu(
            7u,59u,336,inbound_selectors,inbound_receivers,10u)!=-1)
        return false;
    if (!nba_gameplay_inbound_side_allows(336, -385, 298) ||
        nba_gameplay_inbound_side_allows(-336, -385, 298) ||
        !nba_gameplay_inbound_side_allows(-336, 20, 0) ||
        nba_gameplay_inbound_side_allows(-336, 19, 0) ||
        !nba_gameplay_inbound_side_allows(336, -21, -1) ||
        nba_gameplay_inbound_side_allows(336, -20, -1)) return false;
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

int16_t nba_gameplay_human_inbound_direction(
        int8_t controller_assignment, uint16_t movement_boost_timer,
        uint16_t pad_held, int16_t current_direction) {
    /* Direction zero is the ordinary right-facing value, so the table's
     * zero entries cover neutral and contradictory direction pairs too. */
    static const uint8_t direction_by_pad_nibble[16] = {
        0, 2, 6, 0, 4, 3, 5, 0, 0, 1, 7, 0, 0, 0, 0, 0
    };
    if (controller_assignment < 0 || movement_boost_timer == 0u)
        return current_direction;
    return direction_by_pad_nibble[(pad_held >> 4) & 0x0Fu];
}

/* `$86:F6EF-$F703/$86:F7BC-$F7D0/$86:F8EA-$F8FE`: M=0 makes both
 * operands signed 16-bit words. Truncating them to bytes changes the sign
 * for ordinary court X values such as +200. */
bool nba_gameplay_same_x_half(int16_t actor_x, int16_t context_anchor_x) {
    return (int16_t)((uint16_t)actor_x ^ (uint16_t)context_anchor_x) >= 0;
}

static void actor_commit_axis(uint16_t *fraction, int16_t *integer,
                              int16_t velocity, uint16_t dispatch_dt) {
    uint32_t raw = ((uint32_t)(uint16_t)*integer << 16) | *fraction;
    /* Velocity is signed 8.8 while the split position is signed 16.16. */
    raw += (uint32_t)((int32_t)velocity * (int32_t)dispatch_dt * 256);
    *fraction = (uint16_t)raw;
    *integer = (int16_t)(raw >> 16);
}

/* `$85:96B5-$9A13`: common actor Z/planar fixed-point integration and
 * movement-vector commit. The animation landing callbacks at `$85:9741`
 * remain with the mode executors; this function owns the arithmetic shared
 * by every ordinary actor pass. `$85:98F4` may additionally consume RNG and
 * set event `$13E7` bit $40; it never suppresses the later facing write.
 * That caller-owned event/RNG side effect is outside this pure helper. */
void nba_gameplay_actor_commit(NbaGameplayActorCommit *actor,
                               uint16_t dispatch_dt,
                               bool update_ground_facing) {
    if (!actor) return;
    if (actor->z_fraction != 0u || actor->z != 0 || actor->velocity_z != 0) {
        actor->velocity_z = (int16_t)(
            (uint16_t)actor->velocity_z - (uint16_t)(0x18u * dispatch_dt));
        actor_commit_axis(&actor->z_fraction, &actor->z,
                          actor->velocity_z, dispatch_dt);
        if (actor->z < 0) {
            actor->z_fraction = 0u;
            actor->z = 0;
            actor->velocity_z = 0;
        }
    }

    actor->previous_x_fraction_raw_94 = actor->x_fraction;
    actor->previous_x_raw_96 = actor->x;
    actor->previous_y_fraction_raw_98 = actor->y_fraction;
    actor->previous_y_raw_9a = actor->y;
    actor->planar_scratch_raw_a0 = 0u;
    /* `$85:97BC/$9810` skips a stationary axis, including its rectangle
     * check. Edge islands only replace the INTEGER word. Mode 8 still hits
     * the coordinate cap, but preserves its velocity and reaction timer. */
    if (actor->velocity_x != 0) {
        actor_commit_axis(&actor->x_fraction, &actor->x,
                          actor->velocity_x, dispatch_dt);
        bool positive_edge = actor->x >= 0x018A;
        bool negative_edge = actor->x <= -0x018A;
        if (positive_edge || negative_edge) {
            actor->x = positive_edge ? 0x018A : -0x018A;
            actor->planar_scratch_raw_a0 = positive_edge ? 7u : 3u;
            if (actor->control_mode_raw_5e != 8u) {
                actor->reaction_timer_raw_60 = 0u;
                if ((positive_edge && actor->velocity_x > 0) ||
                    (negative_edge && actor->velocity_x < 0))
                    actor->velocity_x = 0;
            }
        }
    }
    if (actor->velocity_y != 0) {
        actor_commit_axis(&actor->y_fraction, &actor->y,
                          actor->velocity_y, dispatch_dt);
        bool positive_edge = actor->y >= 0x00E0;
        bool negative_edge = actor->y <= -0x00E0;
        if (positive_edge || negative_edge) {
            /* Byte tables `$85:9A14/$9A1C` combine the X edge with Y. */
            static const uint8_t positive_codes[8] = {5,0,0,4,0,0,0,6};
            static const uint8_t negative_codes[8] = {1,0,0,2,0,0,0,8};
            actor->y = positive_edge ? 0x00E0 : -0x00E0;
            actor->planar_scratch_raw_a0 = positive_edge ?
                positive_codes[actor->planar_scratch_raw_a0] :
                negative_codes[actor->planar_scratch_raw_a0];
            if (actor->control_mode_raw_5e != 8u) {
                actor->reaction_timer_raw_60 = 0u;
                if ((positive_edge && actor->velocity_y > 0) ||
                    (negative_edge && actor->velocity_y < 0))
                    actor->velocity_y = 0;
            }
        }
    }
    /* `$85:9864-$988D` always applies the isometric diagonal after both
     * axes, even when stationary. It preserves fractions, velocity, timer
     * and the rectangle-only edge code already committed to +$A0. */
    if (actor->y < 0) {
        int16_t minimum_x = (int16_t)(-556 - actor->y);
        if (actor->x <= minimum_x) actor->x = minimum_x;
    } else {
        int16_t maximum_x = (int16_t)(561 - actor->y);
        if (actor->x > maximum_x) actor->x = maximum_x;
    }

    uint16_t distance = 0u;
    uint8_t direction = nba_gameplay_target_direction(
        actor->velocity_x, actor->velocity_y, &distance);
    actor->movement_distance_raw_4c = distance;
    actor->speed_raw_4a = (uint16_t)(distance * dispatch_dt);
    if (direction != 8u)
        actor->velocity_direction_raw_a2 = direction;
    if (actor->z_fraction == 0u && actor->z == 0 &&
        (actor->behavior_flags_raw_7e & 2u) == 0u &&
        direction != 8u && update_ground_facing)
        actor->facing_raw_4e = direction;
}

/* `$85:B13F-$B16A`: when a play request is consumed, update the opposing
 * `$46EB/$476B` context mode. The score comparison is the 65816 subtraction
 * N bit, not C's unsigned less-than and not an overflow-corrected compare. */
bool nba_gameplay_defense_context_reselect(
    uint16_t current_score, uint16_t opponent_score,
    uint16_t period_raw_0926, uint16_t opponent_activity_raw_39,
    uint16_t random_word, uint16_t *opponent_mode_raw_30) {
    if (!opponent_mode_raw_30 || opponent_activity_raw_39 == 0u) return false;
    if ((int16_t)(uint16_t)(current_score - opponent_score) < 0 &&
        period_raw_0926 < 3u)
        *opponent_mode_raw_30 = 1u;
    else
        *opponent_mode_raw_30 = (random_word & 1u) != 0u ? 1u : 3u;
    return true;
}
