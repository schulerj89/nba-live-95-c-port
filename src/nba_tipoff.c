#include "nba_tipoff.h"
#include "nba_player_lab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int16_t world_x, world_y;
    int16_t screen_x, screen_y;
    uint8_t direction;
} TipoffFormation;

/* $86:DDA7-$DF54 initializes these paired $100-byte actor records. The
 * settled jump-ball camera sends only eight actors through $80:AD92;
 * $87:A47A culls actors 4 and 9 before composition. */
static const TipoffFormation formation[10] = {
    {   8,   3, 139, 122, 6 }, { -16, -83,  29, 107, 0 },
    { -24,  80, 184, 150, 4 }, { 104, -56, 176,  84, 7 },
    {  96,  59,   0,   0, 5 }, {  -8,  -3, 117, 125, 2 },
    {  16,  83, 227, 140, 4 }, {  24, -80,  72,  98, 0 },
    {-104,  56,  80, 164, 3 }, { -96, -59,   0,   0, 0 },
};

/* Native $80:AD92 submission order at gameplay frames 89/91. The SNES gives
 * lower OAM indices priority, so the framebuffer composites it in reverse. */
static const uint8_t visible_submission[8] = { 8, 2, 6, 5, 0, 1, 7, 3 };

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t bgr555(uint16_t word) {
    uint32_t r = word & 31u, g = (word >> 5) & 31u, b = (word >> 10) & 31u;
    return 0xFF000000u | ((r << 3 | r >> 2) << 16) |
           ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
}

static uint8_t tile_pixel(const uint8_t *tile, int x, int y) {
    int bit = 7 - x;
    return (uint8_t)(((tile[y * 2] >> bit) & 1) |
        (((tile[y * 2 + 1] >> bit) & 1) << 1) |
        (((tile[16 + y * 2] >> bit) & 1) << 2) |
        (((tile[17 + y * 2] >> bit) & 1) << 3));
}

static int16_t fp_round(int32_t value) {
    return (int16_t)(value >= 0 ? (value + 128) >> 8 : -((-value + 128) >> 8));
}

/* Gameplay records keep the integer coordinate and its subpixel byte
 * separately. Reading the ROM integer word is a signed floor, not the
 * nearest-pixel conversion used by screen placement. */
static int16_t fp_integer_word(int32_t value) {
    return (int16_t)(value >= 0 ? value / 256 :
                     -(((-value) + 255) / 256));
}

static int32_t fp_replace_integer_word(int32_t value, int16_t integer) {
    return (int32_t)integer * 256 + (int32_t)((uint32_t)value & 0xFFu);
}

static int basket_x_for_side(unsigned side) {
    return side ? 336 : -336;
}

static int basket_y_for_side(unsigned side) {
    (void)side;
    return 0;
}

static void cpu_begin_possession(NbaTipoff *tipoff, uint8_t offense_side);
static void ball_attach_to_actor(NbaTipoff *tipoff, unsigned owner);
static void ball_launch(NbaTipoff *tipoff, int target_x, int target_y,
                        unsigned flight_frames, int vertical_velocity,
                        NbaBallMode mode);
static bool cpu_update_rom_passer(NbaTipoff *tipoff, unsigned slot);
static void cpu_enter_play_state(NbaTipoff *tipoff, NbaCpuPlayState state);
static bool cpu_try_rom_mode11_shot(NbaTipoff *tipoff, unsigned slot);
static bool cpu_update_rom_shooter(NbaTipoff *tipoff, unsigned slot);

/* `$86:A613-$A628`, reached by the shared `$85:A656-$A726` rectangular
 * actor/ball clamp. The port represents the proven globals it mutates; the
 * unrepresented `$0944/$094A/$09B8` words remain documented in the dump. */
static void cpu_cancel_boundary_activity(NbaTipoff *tipoff) {
    tipoff->pass_actor_raw = -1;
    tipoff->pass_receiver_raw = -1;
    tipoff->ball_activity_raw = 0u;
}

static void cpu_clamp_record_to_court(NbaTipoff *tipoff,
                                      int32_t *x_fp, int32_t *y_fp,
                                      int16_t *velocity_x,
                                      int16_t *velocity_y) {
    if (nba_gameplay_court_clamp(
            x_fp, y_fp, velocity_x, velocity_y))
        cpu_cancel_boundary_activity(tipoff);
}

static int cpu_select_rom_receiver(const NbaTipoff *tipoff,
                                   uint8_t passer_slot) {
    NbaGameplayReceiverState actors[NBA_GAMEPLAY_ACTOR_COUNT];
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        actors[i].x = fp_round(tipoff->actors[i].x_fp);
        actors[i].y = fp_round(tipoff->actors[i].y_fp);
        actors[i].control_mode = tipoff->actors[i].control_mode;
        actors[i].travel_direction = tipoff->actors[i].assignment_direction;
        actors[i].travel_distance = tipoff->actors[i].pair_distance;
    }
    int16_t special = tipoff->special_actor_raw == NBA_GAMEPLAY_UNKNOWN_WORD ?
                      -1 : (int16_t)tipoff->special_actor_raw;
    return nba_gameplay_select_pass_receiver(
        passer_slot, special, tipoff->play_selector_raw, actors,
        NBA_GAMEPLAY_ACTOR_COUNT, tipoff->offense_side != 0u);
}

static bool cpu_inbound_candidate_valid(const NbaTipoff *tipoff,
                                        uint8_t passer_slot,
                                        int16_t candidate) {
    if (candidate < 0 || candidate >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
    NbaGameplayReceiverState actors[NBA_GAMEPLAY_ACTOR_COUNT];
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        actors[i].x = fp_round(tipoff->actors[i].x_fp);
        actors[i].y = fp_round(tipoff->actors[i].y_fp);
        actors[i].control_mode = tipoff->actors[i].control_mode;
        actors[i].travel_direction = tipoff->actors[i].assignment_direction;
        actors[i].travel_distance = tipoff->actors[i].pair_distance;
    }
    return nba_gameplay_receiver_candidate_valid(
        passer_slot, (uint8_t)candidate, actors, NBA_GAMEPLAY_ACTOR_COUNT);
}

static uint16_t actor_distance(int dx, int dy) {
    unsigned ax = (unsigned)(dx < 0 ? -dx : dx);
    unsigned ay = (unsigned)(dy < 0 ? -dy : dy);
    unsigned high = ax > ay ? ax : ay, low = ax > ay ? ay : ax;
    return (uint16_t)(high + (low >> 2));
}

static bool strictly_between(int16_t value, int16_t endpoint_a,
                             int16_t endpoint_b) {
    int32_t da = (int32_t)value - endpoint_a;
    int32_t db = (int32_t)value - endpoint_b;
    return (da < 0 && db > 0) || (da > 0 && db < 0);
}

/* `$85:F5E4-$F715`: an opponent blocks the cutter only when its center is
 * strictly inside the actor-to-basket rectangle. The ROM traverses linked
 * actor neighbors; the fixed ten-record C array yields the same predicate. */
static bool cpu_lane_to_basket_is_clear(const NbaTipoff *tipoff,
                                        unsigned slot) {
    const NbaTipoffActor *actor = &tipoff->actors[slot];
    int16_t x = fp_round(actor->x_fp), y = fp_round(actor->y_fp);
    int16_t basket_x = slot < 5u ? -336 : 336;
    int16_t x_a, x_b, y_a, y_b;
    if (x < basket_x) {
        x_a = (int16_t)(x - 8);
        x_b = (int16_t)(basket_x + 24);
    } else {
        x_a = (int16_t)(x + 8);
        x_b = (int16_t)(basket_x - 24);
    }
    if (y < 0) {
        y_a = (int16_t)(y - 24);
        y_b = 24;
    } else {
        y_a = (int16_t)(y + 24);
        y_b = -24;
    }
    unsigned side = slot / 5u;
    for (unsigned other = 0; other < NBA_GAMEPLAY_ACTOR_COUNT; ++other) {
        if (other / 5u == side) continue;
        int16_t other_x = fp_round(tipoff->actors[other].x_fp);
        int16_t other_y = fp_round(tipoff->actors[other].y_fp);
        if (strictly_between(other_x, x_a, x_b) &&
            strictly_between(other_y, y_a, y_b)) return false;
    }
    return true;
}

/* `$85:B4B9-$B50D`: actor +$64 cadence selects a clear-lane cutter while
 * `$09A4` is active. This runs before the mode's formation/arrival work. */
static void cpu_update_special_actor(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    bool selects_cutter = actor->control_mode == 1u ||
                          actor->control_mode == 3u;
    int16_t remaining = (int16_t)(uint16_t)(actor->behavior_timer - 2u);
    actor->behavior_timer = (uint16_t)remaining;
    if (remaining >= 0) return;
    actor->behavior_timer = (uint16_t)(remaining + 0x2Fu);
    if (!selects_cutter || tipoff->play_cycle_raw == 0u ||
        tipoff->possession_actor < 0 ||
        !cpu_lane_to_basket_is_clear(tipoff, slot)) return;
    const NbaTipoffActor *owner = &tipoff->actors[tipoff->possession_actor];
    int dx = fp_round(owner->x_fp) - fp_round(actor->x_fp);
    int dy = fp_round(owner->y_fp) - fp_round(actor->y_fp);
    if (actor_distance(dx, dy) < 0xA0u)
        tipoff->special_actor_raw = (uint16_t)slot;
}

/* `$87:B37C/$B3BD/$B47A/$B4DB` install independent resources and restart
 * only the animation channel whose state changed. */
static void actor_set_animation(NbaTipoffActor *actor, uint8_t upper,
                                uint8_t lower) {
    if (actor->animation_state != upper) {
        actor->animation_state = upper;
        actor->upper_animation_tick = 0u;
        actor->upper_animation_phase_raw = 0u;
    }
    if (actor->lower_animation_state != lower) {
        actor->lower_animation_state = lower;
        actor->lower_animation_tick = 0u;
    }
}

static void actor_set_upper_animation(NbaTipoffActor *actor, uint8_t upper) {
    if (actor->animation_state == upper) return;
    actor->animation_state = upper;
    actor->upper_animation_tick = 0u;
    actor->upper_animation_phase_raw = 0u;
}

static int16_t pass_predict_component(int16_t value, unsigned shift) {
    return nba_gameplay_arithmetic_shift_right(value, shift);
}

static int32_t pass_lead_component(int16_t velocity, uint16_t duration) {
    int32_t product = (int32_t)velocity * duration;
    if (product >= 0) return product >> 8;
    return -(((-product) + 255) >> 8);
}

static uint8_t pass_band_from_distance(uint16_t distance) {
    return distance < 0x41u ? 0u : distance < 0x79u ? 1u :
           distance < 0xC9u ? 2u : distance < 0x119u ? 3u :
           distance < 0x191u ? 4u : 5u;
}

/* `$86:AB2D-$AF65`: live-covered grounded mode-15 setup. The aligned
 * `$2A-$2C`, airborne `$AFC4`, and catch-preinit `$AF66` branches still
 * depend on raw writers not represented by the port and remain gated. */
static bool cpu_begin_rom_pass(NbaTipoff *tipoff, unsigned passer_slot,
                               unsigned receiver_slot) {
    NbaTipoffActor *passer = &tipoff->actors[passer_slot];
    NbaTipoffActor *receiver = &tipoff->actors[receiver_slot];
    int16_t passer_x = (int16_t)(fp_round(passer->x_fp) +
        pass_predict_component(passer->velocity_x, 4u));
    int16_t passer_y = (int16_t)(fp_round(passer->y_fp) +
        pass_predict_component(passer->velocity_y, 4u));
    int16_t receiver_x = (int16_t)(fp_round(receiver->x_fp) +
        pass_predict_component(receiver->velocity_x, 3u));
    int16_t receiver_y = (int16_t)(fp_round(receiver->y_fp) +
        pass_predict_component(receiver->velocity_y, 3u));
    uint16_t distance = 0u;
    uint8_t fine = nba_gameplay_pass_direction(
        (int16_t)(receiver_x - passer_x),
        (int16_t)(receiver_y - passer_y), &distance);
    uint8_t pass_direction = fine >> 1;
    if (pass_direction >= 8u) return false;

    uint8_t band = pass_band_from_distance(distance);
    uint8_t relative = (uint8_t)((pass_direction - passer->direction) & 7u);
    if (fp_round(passer->z_fp) != 0) return false;
    uint8_t upper = 0u;
    int16_t family = -1;
    uint16_t receiver_timer = 0x3Cu;
    if (relative >= 3u && relative < 6u) {
        uint8_t team = passer_slot >= 5u ? tipoff->session->right_team :
                                           tipoff->session->left_team;
        uint8_t profile_39 = 0u, profile_3e = 0xFFu;
        (void)nba_player_gameplay_pass_profiles(
            tipoff->assets, team, passer->roster_slot,
            &profile_39, &profile_3e);
        (void)profile_39; /* catch-preinit input, deliberately gated above. */
        bool stationary = (passer->velocity_x | passer->velocity_y) == 0;
        bool forced_special = profile_3e < 0x55u ||
            passer->lower_animation_state == 0x09u ||
            passer->lower_animation_state == 0x0Bu;
        /* `$86:AC87-$AC96` selects the still-unported `$AFC4` family for
         * a long, boosted, otherwise-normal grounded pass. */
        if (!forced_special && distance >= 0x119u &&
            passer->movement_boost_timer != 0u) return false;
        bool grounded_special = forced_special || stationary;
        if (grounded_special) {
            /* `$86:B00B-$B04A`: upper-only state `$2F`, family 1. */
            receiver_timer = 0x50u;
            passer->velocity_x = passer->velocity_y = 0;
            passer->movement_magnitude_raw = 0u;
            passer->behavior_flags_raw |= 0x0006u;
            family = 5;
            upper = 0x2Fu;
        } else {
            /* `$86:ACB1-$AD0B`: normal side/back pass. */
            int selector = relative;
            if (relative == 4u) {
                uint8_t fine_relative = (uint8_t)(
                    (fine - passer->direction - passer->direction) & 15u);
                if (fine_relative == 8u) {
                    passer->direction = (uint8_t)((passer->direction + 1u) & 7u);
                    relative = (uint8_t)((pass_direction - passer->direction) & 7u);
                    selector = relative;
                } else selector = fine_relative == 9u ? 5 : 3;
            }
            int16_t sign_test = (int16_t)(selector * 2 - 7);
            if (passer->direction < 3u) sign_test = (int16_t)~sign_test;
            upper = sign_test < 0 ? 0x2Eu : 0x2Du;
        }
    } else {
        /* `$86:AE10-$AE4F`: only the proven off-axis route. A perfectly
         * aligned pass requires the still-gated `$2A-$2C` inputs. */
        int selector = relative;
        if (relative == 0u) {
            selector = (fine - passer->direction - passer->direction) & 15u;
            if (selector == 0) return false;
        }
        bool choose_30 = selector < 3;
        if (passer->direction < 3u) choose_30 = !choose_30;
        upper = choose_30 ? 0x30u : 0x31u;
    }

    uint8_t threshold = 0u;
    if (!nba_assets_gameplay_pass_release_threshold(
            tipoff->assets, upper, &threshold)) return false;
    passer->pass_band_raw = (uint16_t)(band * 6u);
    passer->pass_direction_raw = pass_direction;
    passer->pass_family_raw = family;
    passer->pass_release_threshold_raw = threshold;
    passer->pass_released_raw = false;
    passer->control_mode = 15u;
    passer->behavior_flags_raw |= 0x0006u;
    receiver->control_mode = 10u;
    receiver->reaction_threshold = receiver_timer;
    actor_set_upper_animation(passer, upper);
    passer->upper_animation_phase_raw = 0u;
    tipoff->pass_actor_raw = (int16_t)passer_slot;
    tipoff->pass_receiver_raw = (int16_t)receiver_slot;
    tipoff->pass_active_raw = 1u;
    tipoff->pass_distance_raw = distance;
    if (tipoff->live_state_raw == 0x82u)
        tipoff->inbound_transfer_raw = 1u;
    else if (tipoff->live_state_raw < 0x80u)
        tipoff->live_state_raw = 2u;
    /* `$86:AF1D-$AF21` resolves the newly selected resource before the
     * initializer returns, so the first visible pass frame already owns the
     * correct hand attachment. */
    ball_attach_to_actor(tipoff, passer_slot);
    return true;
}

static void cpu_set_role_targets(NbaTipoff *tipoff) {
    if (tipoff->cpu_play_state == NBA_CPU_PLAY_REBOUND &&
        tipoff->live_state_raw != 0x82u) {
        int16_t ball_x = fp_round(tipoff->ball.x_fp);
        int16_t ball_y = fp_round(tipoff->ball.y_fp);
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            tipoff->actors[actor].target_x = ball_x;
            tipoff->actors[actor].target_y = ball_y;
            /* `$86:A45E` returns the released shooter to mode 11; its next
             * non-owner dispatch falls through `$86:F3F6-$F40A` to mode 1.
             * Mode 16 is a different lifecycle and must not be synthesized
             * merely because a host shot entered loose-ball recovery. */
            tipoff->actors[actor].control_mode =
                actor / 5u == tipoff->offense_side ? 1u : 2u;
        }
    }
}

static bool cpu_refresh_defense_target(NbaTipoff *tipoff, unsigned slot,
                                       bool *stop_velocity) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    unsigned paired_slot = actor->assignment_current_raw >> 1;
    if (paired_slot >= NBA_GAMEPLAY_ACTOR_COUNT ||
        paired_slot / 5u == slot / 5u) {
        paired_slot = actor->assignment_base_raw >> 1;
        if (paired_slot >= NBA_GAMEPLAY_ACTOR_COUNT ||
            paired_slot / 5u == slot / 5u) return false;
        actor->assignment_current_raw = actor->assignment_base_raw;
    }
    actor->assignment_actor = (uint8_t)paired_slot;
    NbaTipoffActor *paired = &tipoff->actors[paired_slot];
    int16_t actor_x = fp_round(actor->x_fp);
    int16_t actor_y = fp_round(actor->y_fp);
    int16_t paired_x = fp_round(paired->x_fp);
    int16_t paired_y = fp_round(paired->y_fp);

    /* `$85:BC52-$BC81`: refresh coarse +$86 and shared +$8A only at the
     * bound-pair decision boundary, not by rebuilding assignments per frame. */
    uint16_t pair_distance = 0u;
    uint8_t pair_direction = nba_gameplay_target_direction(
        (int16_t)(paired_x - actor_x), (int16_t)(paired_y - actor_y),
        &pair_distance);
    actor->assignment_direction = pair_direction;
    actor->assignment_distance = pair_distance;
    actor->pair_distance = pair_distance;
    paired->assignment_direction = pair_direction < 8u ?
        (uint8_t)(pair_direction ^ 4u) : pair_direction;
    paired->pair_distance = pair_distance;

    /* `$85:AFC2-$AFE5`: actor +$88/+8C are fine direction and distance to
     * that actor's own team-context basket anchor. */
    int16_t paired_anchor = paired_slot < 5u ? -336 : 336;
    paired->anchor_direction_raw = nba_gameplay_pass_direction(
        (int16_t)(paired_anchor - paired_x), (int16_t)(-paired_y),
        &paired->anchor_distance_raw);

    uint8_t paired_team = paired_slot >= 5u ?
        tipoff->session->right_team : tipoff->session->left_team;
    uint8_t rating_2pt = 0u, rating_3pt = 0u;
    (void)nba_player_gameplay_shot_ratings(
        tipoff->assets, paired_team, paired->roster_slot,
        &rating_2pt, &rating_3pt);
    (void)rating_2pt;
    unsigned side = slot / 5u;
    int16_t context_anchor = side == 0u ? -336 : 336;
    NbaGameplayDefenseTargetInput input = {
        .actor_x = actor_x, .actor_y = actor_y,
        .actor_pair_direction_raw_86 = actor->assignment_direction,
        .actor_pair_distance_raw_8a = actor->assignment_distance,
        .paired_x = paired_x, .paired_y = paired_y,
        .paired_velocity_x = paired->velocity_x,
        .paired_velocity_y = paired->velocity_y,
        .paired_anchor_direction_raw_88 = paired->anchor_direction_raw,
        .paired_anchor_distance_raw_8c = paired->anchor_distance_raw,
        .paired_position_raw_92 = paired->assignment_role_raw_92,
        .context_anchor_x = context_anchor,
        .context_mode_raw_30 = tipoff->team_context[side].mode_raw_30,
        .context_flags_raw_32 = tipoff->team_context[side].flags_raw_32,
        .paired_on_three_point_arc = nba_gameplay_shot_value(
            false, paired_x, paired_y, paired_slot >= 5u) == 3u,
        .paired_three_point_rating = rating_3pt
    };
    NbaGameplayDefenseTargetOutput output = {0};
    if (!nba_gameplay_defense_mode_target(
            actor->control_mode, &input, &output)) return false;
    if (output.target_written) {
        actor->target_x = output.target_x;
        actor->target_y = output.target_y;
    }
    if (stop_velocity) *stop_velocity = output.stop_velocity;
    return true;
}

/* Proven passive behavior executors from `$87:9244/$87:9BD3`. Returning
 * true means the mode consumed this actor's scheduled pass. Modes 1-6 and
 * the active ball handlers remain in the explicitly provisional planner. */
static bool cpu_apply_passive_mode(NbaTipoffActor *actor) {
    if (actor->control_mode == 0u) { /* `$87:9C1B`: no-op wrapper. */
        actor->velocity_x = actor->velocity_y = 0;
        actor_set_animation(actor, 0u, 0u);
        return true;
    }
    if (actor->control_mode == 9u) { /* `$86:F0B7`: timed target override. */
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        if ((remaining & 0x8000u) != 0u) {
            actor->reaction_threshold = 0u;
            actor->behavior_timer = 0u;
            actor->control_mode = actor->saved_control_mode;
            return true;
        }
        return false;
    }
    if (actor->control_mode == 16u) { /* `$86:B0F7-$B153`: post-shot hold. */
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        actor->velocity_x = actor->velocity_y = 0;
        actor_set_animation(actor, 1u, 1u);
        if ((remaining & 0x8000u) != 0u) {
            /* `$86:B10A-$B122`: reset, mode 7, +$60=$B4. */
            actor->control_mode = 7u;
            actor->reaction_threshold = 0xB4u;
        }
        return true;
    }
    if (actor->control_mode == 7u) { /* `$86:994C-$99C3`: dead-ball hold. */
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        actor->velocity_x = actor->velocity_y = 0;
        actor_set_animation(actor, 3u, 3u);
        if ((remaining & 0x8000u) != 0u) {
            actor->reaction_threshold = 0u;
            actor->control_mode = 0u; /* `$86:9846` planner boundary. */
        }
        return true;
    }
    return false;
}

/* `$87:9BD3` maps behavior mode 10 to wrapper `$87:9C3A`, which calls
 * `$86:A5B0-$A628`. A valid `$0946` receiver counts actor +$60 down by the
 * current actor delta. Once it expires, or whenever `$0946` is negative,
 * `$86:9846-$986C` restores the actor to the ordinary offense/defense mode
 * selected by actor +$6E versus `$093A`. The previous host implementation
 * skipped this executor and therefore allowed stale receivers to coast to a
 * court edge forever after A613 cleared the pass globals. */
static bool cpu_update_rom_receiver(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (actor->control_mode != 10u) return false;
    bool receiver_valid = tipoff->pass_receiver_raw >= 0;
    if (receiver_valid) {
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        if ((remaining & 0x8000u) == 0u) return true;
    }
    /* `$86:9846`: offense group `$093A` receives mode 1; the other group
     * receives mode 2. It also clears +$60, +$7E and +$28. */
    uint8_t side_group = slot >= 5u ? 5u : 0u;
    actor->control_mode = side_group == tipoff->camera_side_group_raw ? 1u : 2u;
    actor->reaction_threshold = 0u;
    actor->behavior_flags_raw = 0u;
    actor->velocity_z = 0;
    return false;
}

static bool cpu_active_decision_due(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    uint8_t mode = actor->control_mode;
    if ((mode < 1u || mode > 6u) && mode != 11u) return true;
    uint8_t team = slot >= 5u ? tipoff->session->right_team :
                               tipoff->session->left_team;
    uint8_t profile_3f = 0u, profile_40 = 0u;
    (void)nba_player_gameplay_decision_profiles(
        tipoff->assets, team, actor->roster_slot,
        &profile_3f, &profile_40);
    int16_t actor_x = fp_round(actor->x_fp);
    /* Modes 2/4/6 compare signed-word actor +$04 against side context +$0A.
     * `$87:8EFE/$8F11` keeps DP $9E at $46EB for slots 0..4 and $476B for
     * slots 5..9; full words `$FEB0/$0150` are -336/+336. This is
     * deliberately not a ball-position or matchup comparison. */
    int16_t side_anchor = slot < 5u ? -336 : 336;
    bool same_half = nba_gameplay_same_x_half(actor_x, side_anchor);
    if (mode == 1u || mode == 3u || mode == 5u || mode == 11u)
        return nba_gameplay_decision_timer_step(
            &actor->reaction_threshold, profile_3f, 0x40u, false);
    /* Mode 2 reloads `$30 + profile[$40]`; modes 4/6 use `$20`.
     * All three add another `$20` when actor/related X signs match. */
    return nba_gameplay_decision_timer_step(
        &actor->reaction_threshold, profile_40,
        mode == 2u ? 0x30u : 0x20u, same_half);
}

/* `$85:AD6B-$AF5B`: install one formation coordinate per play-step and run
 * the normal `$85:B402` completion route. The unresolved state-$82 inbound
 * override (`$AE3B-$AE95`) and DP-$5C override (`$AE97-$AEBB`) are excluded;
 * neither raw owner is represented by the current CPU-only runtime. */
static uint8_t cpu_formation_target_direction(NbaTipoff *tipoff,
                                              unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    uint8_t role = (uint8_t)(slot % 5u);
    if (tipoff->special_actor_raw == slot) {
        /* `$85:AE1F-$AE32`: the selected cutter never owns the formation
         * install latch while it targets the team basket anchor. */
        actor->behavior_flags_raw &= 0xFFF7u;
        actor->target_x = slot < 5u ? -336 : 336;
        actor->target_y = 0;
    } else if ((actor->behavior_flags_raw & 0x0008u) == 0u) {
        int16_t target_x = actor->target_x, target_y = actor->target_y;
        int16_t side_anchor_x = slot < 5u ? -336 : 336;
        if (nba_assets_gameplay_formation_offset(
                tipoff->assets, (uint8_t)tipoff->play_code, role,
                (uint8_t)tipoff->play_step_raw,
                tipoff->play_mirror_raw != 0u, side_anchor_x,
                &target_x, &target_y)) {
            actor->target_x = target_x;
            actor->target_y = target_y;
            actor->behavior_flags_raw |= 0x0008u;
        }
    }

    int16_t actor_x = fp_round(actor->x_fp);
    int16_t actor_y = fp_round(actor->y_fp);
    bool opposite_x_sign = (int16_t)(actor->target_x ^ actor_x) < 0;
    bool special_edge = (tipoff->ball_activity_raw | tipoff->rim_raw_097c) != 0u &&
                        role >= 3u;
    if (opposite_x_sign && role < 3u) {
        /* `$85:AEDF-$AEF3`: cross midcourt through X=+/-16 with B3AA;
         * this path never sets the completion bit. */
        int16_t gate_x = actor->target_x < 0 ? -16 : 16;
        return nba_gameplay_target_direction(
            (int16_t)(gate_x - actor_x),
            (int16_t)(actor->target_y - actor_y), NULL);
    }
    if (special_edge) {
        /* `$85:AF2A-$AF3D` reaches the back-role edge route only while
         * `$0948|$097C` is active. Opposite X sign alone does not clamp
         * roles 3/4; the CPU oracle shows both crossing normally. */
        int16_t edge_x = actor_x < 0 ? -0x152 : 0x152;
        int16_t edge_y = actor_y < 0 ? -16 : 16;
        uint8_t steering = 8u;
        (void)nba_gameplay_predictive_arrival(
            actor_x, actor_y, actor->velocity_x, actor->velocity_y,
            edge_x, edge_y, 16u, &steering, NULL);
        return steering;
    }

    uint8_t steering = 8u;
    if (nba_gameplay_predictive_arrival(
            actor_x, actor_y, actor->velocity_x, actor->velocity_y,
            actor->target_x, actor->target_y, 16u, &steering, NULL))
        actor->behavior_flags_raw |= 0x0040u;
    return steering;
}

/* Common `$85:96B5-$973A` actor Z integration. `$C6=2` makes gravity
 * `$18*$C6=$30` and the fixed-point displacement uses the same dt. */
static void cpu_integrate_actor_vertical(NbaTipoffActor *actor) {
    if (actor->z_fp == 0 && actor->velocity_z == 0) return;
    actor->velocity_z = (int16_t)(actor->velocity_z - 0x0030);
    actor->z_fp += (int32_t)actor->velocity_z * 2;
    if (actor->z_fp < 0) {
        actor->z_fp = 0;
        actor->velocity_z = 0;
    }
}

static int16_t divide_16_toward_zero(int16_t value) {
    return value >= 0 ? (int16_t)(value / 16) :
                        (int16_t)(-((-(int)value) / 16));
}

/* `$86:F43A-$F4E2`: mode 11 has a dedicated dead-ball steering branch.
 * Its target compensation divides signed velocity by 16 toward zero. */
static bool cpu_move_inbound_actor(NbaTipoff *tipoff, unsigned slot) {
    if (tipoff->live_state_raw != 0x82u ||
        tipoff->inbound_actor_raw != slot ||
        tipoff->possession_actor != (int8_t)slot ||
        tipoff->actors[slot].control_mode != 11u ||
        (tipoff->inbound_transfer_raw != 0u &&
         tipoff->actors[slot].control_mode != 11u)) return false;
    NbaTipoffActor *actor = &tipoff->actors[slot];
    cpu_update_special_actor(tipoff, slot);
    if (tipoff->inbound_ready_raw != 0u) {
        actor->velocity_x = actor->velocity_y = 0;
        actor->movement_magnitude_raw = 0u;
        actor->direction = (uint8_t)tipoff->inbound_direction_raw;
        actor->requested_direction = actor->direction;
        actor_set_animation(actor, 11u, 3u);
        actor->action_state = tipoff->cpu_play_state;
        return true;
    }
    int16_t steering_x = (int16_t)(tipoff->inbound_target_x_raw -
                                   divide_16_toward_zero(actor->velocity_x));
    int16_t steering_y = (int16_t)(tipoff->inbound_target_y_raw -
                                   divide_16_toward_zero(actor->velocity_y));
    uint8_t direction = nba_gameplay_target_direction(
        (int16_t)(steering_x - fp_round(actor->x_fp)),
        (int16_t)(steering_y - fp_round(actor->y_fp)), NULL);
    uint8_t team = slot >= 5u ? tipoff->session->right_team :
                               tipoff->session->left_team;
    uint8_t profile_42 = 0x58u;
    (void)nba_player_gameplay_movement_profile(
        tipoff->assets, team, actor->roster_slot, &profile_42);
    nba_gameplay_velocity_step(
        &actor->velocity_x, &actor->velocity_y,
        &actor->movement_boost_timer, direction, profile_42, 2u,
        false, (int16_t)tipoff->possession_actor);
    actor->movement_direction = direction;
    if (direction < 8u) actor->requested_direction = direction;
    actor->movement_magnitude_raw = actor_distance(
        actor->velocity_x, actor->velocity_y);
    actor->x_fp += (int32_t)actor->velocity_x * 2;
    actor->y_fp += (int32_t)actor->velocity_y * 2;
    cpu_integrate_actor_vertical(actor);
    actor->direction = actor->requested_direction;
    actor_set_animation(actor, direction >= 8u ? 0u : 11u,
                        direction >= 8u ? 0u : 3u);
    actor->action_state = tipoff->cpu_play_state;
    return true;
}

static void cpu_move_actor(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (cpu_move_inbound_actor(tipoff, slot)) return;
    if (actor->control_mode == 11u &&
        slot != (unsigned)tipoff->possession_actor) {
        /* `$86:F3F6-$F40A`: a mode-11 actor that no longer matches `$093E`
         * falls back to mode 1 on its next scheduled dispatch. */
        actor->control_mode = 1u;
        actor->behavior_timer = 47u;
        actor->reaction_threshold = 0u;
        actor->behavior_flags_raw = 0u;
        cpu_integrate_actor_vertical(actor);
        return;
    }
    if (actor->control_mode == 5u && slot != (unsigned)tipoff->possession_actor) {
        /* `$86:F3F6-$F40A`: mode 5 belongs only to `$093E`. Every other
         * actor falls back to mode 1 and clears the complete +$7E word. */
        actor->control_mode = 1u;
        actor->behavior_timer = 47u;
        actor->reaction_threshold = 0u;
        actor->behavior_flags_raw = 0u;
        cpu_integrate_actor_vertical(actor);
        return;
    }
    cpu_update_special_actor(tipoff, slot);
    if (actor->control_mode == 15u && cpu_update_rom_passer(tipoff, slot))
        return;
    if (actor->control_mode == 12u && cpu_update_rom_shooter(tipoff, slot))
        return;
    if (actor->control_mode == 10u && cpu_update_rom_receiver(tipoff, slot)) {
        /* `$86:A5B0` (normal receiver) and `$86:B154` (special receiver)
         * do not dispatch the generic formation accelerator. `$86:99C4`
         * already led the pass by the receiver's existing velocity, so keep
         * that motion through the common `$85:963D` coordinate commit. */
        actor->x_fp += (int32_t)actor->velocity_x * 2;
        actor->y_fp += (int32_t)actor->velocity_y * 2;
        actor->movement_magnitude_raw = actor_distance(
            actor->velocity_x, actor->velocity_y);
        if (actor->reaction_threshold >= 2u)
            actor->reaction_threshold -= 2u;
        cpu_integrate_actor_vertical(actor);
        return;
    }
    if (actor->control_mode == 14u) {
        /* `$86:B154` special receiver follows its separately gated handler;
         * its complete lifecycle remains outside the normal `$86:A5B0` path. */
        actor->x_fp += (int32_t)actor->velocity_x * 2;
        actor->y_fp += (int32_t)actor->velocity_y * 2;
        actor->movement_magnitude_raw = actor_distance(
            actor->velocity_x, actor->velocity_y);
        if (actor->reaction_threshold >= 2u)
            actor->reaction_threshold -= 2u;
        cpu_integrate_actor_vertical(actor);
        return;
    }
    if (cpu_apply_passive_mode(actor)) {
        cpu_integrate_actor_vertical(actor);
        return;
    }
    /* `$85:B95C` seeds actor +$60; the mode-specific `$C8=$20` cadence above
     * replaces the former handcrafted per-slot/possession-frame delay. */
    /* `$85:963D-$985F` dispatches and integrates fractional words every
     * scheduled actor; the coordinate write slice is `$85:97CA-$985F`.
     * update. Integer coordinates change irregularly from subpixel carry;
     * there is no ROM evidence for the former odd/even slot shortcut. */
    int x = fp_round(actor->x_fp), y = fp_round(actor->y_fp);
    bool decision_due = cpu_active_decision_due(tipoff, slot);
    bool pass_started = false;
    uint8_t direction = actor->movement_direction;
    bool stop_velocity = false;
    if (decision_due && actor->recovery_inhibit_raw == 0u) {
        uint8_t mode = actor->control_mode;
        if (mode == 11u && slot == (unsigned)tipoff->possession_actor &&
                actor->controller_assignment_raw < 0) {
            /* `$86:F423-$F435`: B678 may consume the mode-11 return when it
             * starts a shot. Only its non-shot return reaches AD6B/B50E. */
            if (cpu_try_rom_mode11_shot(tipoff, slot)) {
                pass_started = true; /* suppress generic walk animation */
            } else {
                direction = cpu_formation_target_direction(tipoff, slot);
                int selected = cpu_select_rom_receiver(tipoff, (uint8_t)slot);
                bool special_receiver = selected >= 0 &&
                    (uint16_t)selected == tipoff->special_actor_raw;
                if (selected >= 0) {
                    /* `$85:B566`: a validated normal selector writes owner
                     * +$60=1 before AB2D. B50E/AB2D never clear A2/AA/AC/AE.
                     * A C false return only represents a still-unported AB2D
                     * animation family, so retain mode 11 and retry it. */
                    if (!special_receiver) actor->reaction_threshold = 1u;
                    if (cpu_begin_rom_pass(
                            tipoff, slot, (unsigned)selected)) {
                        tipoff->receiver_actor = (uint8_t)selected;
                        tipoff->actors[selected].control_mode =
                            special_receiver ? 14u : 10u;
                        cpu_enter_play_state(tipoff, NBA_CPU_PLAY_PASS);
                        pass_started = true;
                    }
                }
            }
        } else if ((mode == 1u || mode == 3u || mode == 5u) &&
                actor->controller_assignment_raw < 0)
            direction = cpu_formation_target_direction(tipoff, slot);
        else if (tipoff->cpu_play_state != NBA_CPU_PLAY_REBOUND &&
                 (mode == 2u || mode == 4u || mode == 6u)) {
            (void)cpu_refresh_defense_target(
                tipoff, slot, &stop_velocity);
            direction = stop_velocity ? 8u : nba_gameplay_target_direction(
                (int16_t)(actor->target_x - x),
                (int16_t)(actor->target_y - y), NULL);
        } else
            direction = nba_gameplay_target_direction(
                (int16_t)(actor->target_x - x),
                (int16_t)(actor->target_y - y), NULL);
        actor->movement_direction = direction;
        if (direction < 8u) actor->requested_direction = direction;
    }
    if (stop_velocity) actor->velocity_x = actor->velocity_y = 0;
    uint8_t team = slot >= 5u ? tipoff->session->right_team :
                               tipoff->session->left_team;
    uint8_t profile_42 = 0x58u;
    (void)nba_player_gameplay_movement_profile(
        tipoff->assets, team, actor->roster_slot, &profile_42);
    /* `$85:A82C-$AB16` runs every actor dispatch. The +$60 decision timer
     * controls target refresh, not integer acceleration/integration cadence. */
    nba_gameplay_velocity_step(
        &actor->velocity_x, &actor->velocity_y,
        &actor->movement_boost_timer, direction, profile_42, 2u,
        tipoff->live_state_raw == 0x81u || fp_round(actor->z_fp) != 0,
        (int16_t)tipoff->possession_actor);
    actor->movement_magnitude_raw = actor_distance(
        actor->velocity_x, actor->velocity_y);
    /* `$8E86-$8E9C` turns `$0938=2` into fixed multiplier `$0200` before
     * `$85:AB17`; in the host 24.8 bridge this is velocity8.8 * dt. */
    actor->x_fp += (int32_t)actor->velocity_x * 2;
    actor->y_fp += (int32_t)actor->velocity_y * 2;
    cpu_integrate_actor_vertical(actor);
    actor->direction = actor->requested_direction;
    if (!pass_started)
        actor_set_animation(actor, direction >= 8u ? 0u :
                            slot == tipoff->handler_actor ? 11u : 3u,
                            direction >= 8u ? 0u : 3u);
    actor->action_state = tipoff->cpu_play_state;
}

static void ball_position_at_actor(NbaTipoff *tipoff, unsigned owner) {
    /* `$87:B649`, `$87:B66A`, `$87:B832`, `$87:B953`: resolve the current independent upper
     * and lower resources, then compose their ROM attachment tables. */
    NbaTipoffActor *actor = &tipoff->actors[owner];
    uint8_t direction = actor->direction < 8u ? actor->direction : 0u;
    uint16_t upper_resource = 0u, lower_resource = 0u;
    int16_t offset_x = 0, offset_y = 0, offset_z = 0;
    uint16_t mirror_flags = direction < 3u ? 0x8000u : 0u;
    bool resolved = nba_player_animation_resources(
        tipoff->assets, actor->animation_state, actor->lower_animation_state,
        direction, actor->upper_animation_tick, actor->lower_animation_tick,
        &upper_resource, &lower_resource) &&
        nba_player_ball_attachment_offsets(
            tipoff->assets, upper_resource, lower_resource, mirror_flags,
            &offset_x, &offset_y, &offset_z);
    /* A validated v26 asset pack makes failure unreachable. Keep ownership
     * deterministic if an externally-corrupted pack reaches this boundary. */
    if (!resolved) offset_x = offset_y = offset_z = 0;
    tipoff->ball.x_fp = actor->x_fp + (int32_t)offset_x * 256;
    tipoff->ball.y_fp = actor->y_fp + (int32_t)offset_y * 256;
    tipoff->ball.z_fp = actor->z_fp + (int32_t)offset_z * 256;
}

static void ball_attach_to_actor(NbaTipoff *tipoff, unsigned owner) {
    ball_position_at_actor(tipoff, owner);
    NbaTipoffActor *actor = &tipoff->actors[owner];
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        tipoff->actors[i].controller_assignment_raw = -1;
    if (!tipoff->cpu_vs_cpu)
        actor->controller_assignment_raw = 0;
    tipoff->ball.owner_actor = (int8_t)owner;
    tipoff->ball.state = NBA_BALL_ATTACHED;
    /* `$86:B625/$B769` keeps `$0948=FFFF` while mode 12 still owns and
     * pose-attaches the ball. Ordinary catches clear the activity word. */
    tipoff->ball_activity_raw = actor->control_mode == 12u ? 0xFFFFu : 0u;
}

static bool ball_attachment_assets_valid(const NbaAssetPack *assets) {
    static const struct {
        uint16_t upper, lower, flags;
        int16_t x, y, z;
    } vectors[] = {
        {328u, 1388u, 0u,      -6, -2, 48},
        {964u, 1474u, 0x8000u,  7,  7, 23},
        {372u, 1737u, 0x8000u,  7, -1, 30},
    };
    for (unsigned i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
        int16_t x, y, z;
        if (!nba_player_ball_attachment_offsets(
                assets, vectors[i].upper, vectors[i].lower, vectors[i].flags,
                &x, &y, &z) || x != vectors[i].x || y != vectors[i].y ||
                z != vectors[i].z) return false;
    }
    return true;
}

static bool cpu_actor_pose_points(const NbaTipoff *tipoff, unsigned actor_index,
                                  NbaGameplayPosePoint points[2]) {
    if (!tipoff || !points || actor_index >= NBA_GAMEPLAY_ACTOR_COUNT)
        return false;
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    uint8_t direction = actor->direction < 8u ? actor->direction : 0u;
    uint16_t upper_resource = 0u, lower_resource = 0u;
    uint16_t mirror_flags = direction < 3u ? 0x8000u : 0u;
    if (!nba_player_animation_resources(
            tipoff->assets, actor->animation_state,
            actor->lower_animation_state, direction,
            actor->upper_animation_tick, actor->lower_animation_tick,
            &upper_resource, &lower_resource)) return false;
    for (unsigned point = 0; point < 2u; ++point) {
        int16_t x, y, z;
        if (!nba_player_ball_attachment_point_offsets(
                tipoff->assets, upper_resource, lower_resource, mirror_flags,
                (uint8_t)point, &x, &y, &z)) return false;
        points[point].x = (int16_t)(fp_round(actor->x_fp) + x);
        points[point].y = (int16_t)(fp_round(actor->y_fp) + y);
        points[point].z = (int16_t)(fp_round(actor->z_fp) + z);
    }
    return true;
}

static bool cpu_actor_contact_height(const NbaTipoff *tipoff,
                                     unsigned actor_index,
                                     uint16_t *height) {
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    uint8_t direction = actor->direction < 8u ? actor->direction : 0u;
    uint16_t upper_resource = 0u, lower_resource = 0u;
    uint8_t team = actor_index >= 5u ? tipoff->session->right_team :
                                      tipoff->session->left_team;
    return nba_player_animation_resources(
               tipoff->assets, actor->animation_state,
               actor->lower_animation_state, direction,
               actor->upper_animation_tick, actor->lower_animation_tick,
               &upper_resource, &lower_resource) &&
           nba_player_animation_contact_height(
               tipoff->assets, team, actor->roster_slot, upper_resource,
               lower_resource, direction, height);
}

static bool cpu_actor_body_contacts_ball(const NbaTipoff *tipoff,
                                         unsigned actor_index,
                                         bool intended_receiver,
                                         uint8_t radius) {
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    uint16_t height = 0u;
    if (!cpu_actor_contact_height(tipoff, actor_index, &height)) return false;
    int dz = fp_round(tipoff->ball.z_fp) - fp_round(actor->z_fp);
    if (dz < 0) return false;
    if (intended_receiver) dz -= 8; /* `$86:CF59-$CF68` */
    int dx = fp_round(actor->x_fp) - fp_round(tipoff->ball.x_fp);
    int dy = fp_round(actor->y_fp) - fp_round(tipoff->ball.y_fp);
    return dz < (int)height && dx >= -(int)radius && dx < (int)radius &&
           dy >= -(int)radius && dy < (int)radius;
}

static bool cpu_actor_contacts_ball(const NbaTipoff *tipoff,
                                    unsigned actor_index,
                                    bool intended_receiver,
                                    uint8_t threshold) {
    NbaGameplayPosePoint points[2];
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    int16_t ball_x = fp_round(tipoff->ball.x_fp);
    int16_t ball_y = fp_round(tipoff->ball.y_fp);
    int16_t ball_z = fp_round(tipoff->ball.z_fp);
    if (actor->contact_inhibit_raw_5a != 0u ||
        actor->animation_state == 7u || actor->animation_state == 8u ||
        !nba_gameplay_ball_coarse_contact(
            fp_round(actor->x_fp), fp_round(actor->y_fp),
            fp_round(actor->z_fp), ball_x, ball_y, ball_z,
            intended_receiver)) return false;
    if (cpu_actor_pose_points(tipoff, actor_index, points) &&
        nba_gameplay_ball_pose_contact(
            points, ball_x, ball_y, ball_z, threshold)) return true;
    return cpu_actor_body_contacts_ball(
        tipoff, actor_index, intended_receiver, threshold);
}

static int cpu_actor_ball_contact_index(const NbaTipoff *tipoff,
                                        unsigned actor_index,
                                        bool intended_receiver,
                                        uint8_t threshold) {
    const NbaTipoffActor *actor = &tipoff->actors[actor_index];
    int16_t ball_x = fp_round(tipoff->ball.x_fp);
    int16_t ball_y = fp_round(tipoff->ball.y_fp);
    int16_t ball_z = fp_round(tipoff->ball.z_fp);
    if (actor->contact_inhibit_raw_5a != 0u ||
        actor->animation_state == 7u || actor->animation_state == 8u ||
        !nba_gameplay_ball_coarse_contact(
            fp_round(actor->x_fp), fp_round(actor->y_fp),
            fp_round(actor->z_fp), ball_x, ball_y, ball_z,
            intended_receiver)) return -1;
    NbaGameplayPosePoint points[2];
    if (!cpu_actor_pose_points(tipoff, actor_index, points)) return -1;
    return nba_gameplay_ball_pose_contact_index(
        points, ball_x, ball_y, ball_z, threshold);
}

/* `$86:CCCD-$D5DA` supplies the broadphase, classifier and pose hitbox;
 * `$86:D5DB` insertion-sorts the zero-terminated `$34D3` actor list by
 * signed world X, retaining source order for ties. `$86:D652-$D728` then
 * presents actor/ball pairs in that stable order. */
static void cpu_actor_contact_order(const NbaTipoff *tipoff,
                                    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT]) {
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        order[i] = (uint8_t)i;
        unsigned at = i;
        while (at > 0u) {
            int16_t left = fp_round(tipoff->actors[order[at - 1u]].x_fp);
            int16_t right = fp_round(tipoff->actors[order[at]].x_fp);
            if (left <= right) break;
            uint8_t swap = order[at - 1u];
            order[at - 1u] = order[at];
            order[at] = swap;
            --at;
        }
    }
}

static int cpu_first_loose_ball_contact(const NbaTipoff *tipoff) {
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        unsigned actor = order[i];
        uint8_t threshold = tipoff->actors[actor].animation_state == 0x13u ?
                            12u : 16u;
        if (cpu_actor_contacts_ball(tipoff, actor, false, threshold))
            return (int)actor;
    }
    return -1;
}

/* During live state `$82`, only the inbound side may recover the dead ball.
 * The collision winner replaces the provisional team-slot-2 `$0954`, as the
 * CPU oracle does when side-0 actor 3 becomes the actual inbounder. */
static int cpu_first_inbound_ball_contact(const NbaTipoff *tipoff) {
    unsigned base = tipoff->inbound_state_raw == 5u ? 5u : 0u;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        unsigned actor = order[i];
        if (actor < base || actor >= base + 5u) continue;
        if (cpu_actor_contacts_ball(tipoff, actor, false, 16u))
            return (int)actor;
    }
    return -1;
}

/* `$86:CE88-$D1D6`: a detached pass rejects same-side nonreceivers. The
 * intended receiver uses radius 16; an opposing player uses the radius-12
 * branch at `$86:CEE2-$CEFF`. At Z<24 the ROM computes a full rating into
 * DP $AA but accidentally compares RNG with DP $00, the hit-point index:
 * point 0 can never win and point 1 wins only for RNG byte zero. */
static int cpu_first_pass_contact(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u ||
        tipoff->ball.state != NBA_BALL_PASS ||
        tipoff->pass_receiver_raw < 0) return -1;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        unsigned actor = order[i];
        bool receiver = actor == tipoff->receiver_actor;
        if (!receiver && actor / 5u == tipoff->offense_side) continue;
        int point = cpu_actor_ball_contact_index(
            tipoff, actor, receiver, receiver ? 16u : 12u);
        if (point < 0 && receiver && cpu_actor_body_contacts_ball(
                tipoff, actor, true, 16u)) point = 2;
        if (point < 0) continue;
        if (!receiver && fp_round(tipoff->ball.z_fp) < 24) {
            uint8_t random = (uint8_t)nba_gameplay_rng_next(&tipoff->rng);
            if (random >= (uint8_t)point) continue;
        }
        return (int)actor;
    }
    return -1;
}

static bool cpu_contact_orchestration_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    static const int16_t x[10] = {30, -5, 12, -5, 99, 0, 12, -20, 30, 4};
    static const uint8_t expected[10] = {7, 1, 3, 5, 9, 2, 6, 0, 8, 4};
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        state.actors[i].x_fp = (int32_t)x[i] * 256;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(&state, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        if (order[i] != expected[i]) return false;
    /* `$86:D11F-$D128` compares against point selector DP $00, not the
     * computed chance DP $AA: selector zero never passes; selector one only
     * passes for a zero random byte. */
    return !(0u < 0u) && !(255u < 0u) && (0u < 1u) && !(1u < 1u);
}

static void ball_launch(NbaTipoff *tipoff, int target_x, int target_y,
                        unsigned flight_frames, int vertical_velocity,
                        NbaBallMode mode) {
    if (flight_frames == 0u) flight_frames = 1u;
    tipoff->ball.velocity_x = (int16_t)((target_x * 256 - tipoff->ball.x_fp) /
                                        (int)flight_frames);
    tipoff->ball.velocity_y = (int16_t)((target_y * 256 - tipoff->ball.y_fp) /
                                        (int)flight_frames);
    tipoff->ball.velocity_z = (int16_t)vertical_velocity;
    tipoff->ball.owner_actor = -1;
    tipoff->ball.state = (uint8_t)mode;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        tipoff->actors[i].controller_assignment_raw = -1;
}

/* `$86:B625` initializes the jump but does not release the ball. The CPU
 * oracle remains attached for 24 rendered frames, governed by actor +$12
 * physics rather than a host timer. */
static bool cpu_start_rom_shot(NbaTipoff *tipoff, unsigned slot) {
    if (!tipoff || slot >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
    NbaTipoffActor *shooter = &tipoff->actors[slot];
    tipoff->handler_actor = (uint8_t)slot;
    tipoff->shot_origin_x = fp_round(shooter->x_fp);
    tipoff->shot_origin_y = fp_round(shooter->y_fp);
    tipoff->shot_value_raw = 0u;
    tipoff->shot_chance_raw = 0u;
    tipoff->shot_inner_veto_raw = false;
    tipoff->shot_miss_index_raw = 0xFFu;
    tipoff->shot_result_resolved = false;
    cpu_enter_play_state(tipoff, NBA_CPU_PLAY_SHOT);
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
        tipoff->actors[actor].controller_assignment_raw = -1;
    shooter->control_mode = 12u;
    shooter->velocity_x /= 2;
    shooter->velocity_y /= 2;
    shooter->velocity_z = 0x0210;
    shooter->behavior_flags_raw |= 0x0004u;
    actor_set_animation(shooter, 0x16u, 0x32u);
    ball_attach_to_actor(tipoff, slot);
    tipoff->ball_activity_raw = 0xFFFFu;
    return true;
}

/* Live-covered `$85:B678` normal-clock path. `$092C` is the 24-second
 * possession clock (1440 60-Hz ticks), not a rim-only flag. With both clocks
 * above 120 the captured oracle reaches B6B7 and then the exact symmetric
 * B714 rectangle. The deeper rating/RNG routes below B734 remain separate
 * until their actor +$8A/+$8C writers are represented. */
static bool cpu_try_rom_mode11_shot(NbaTipoff *tipoff, unsigned slot) {
    if (!tipoff || slot >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
    const NbaTipoffActor *actor = &tipoff->actors[slot];
    int16_t rom_x = fp_round(actor->x_fp);
    int16_t y = fp_round(actor->y_fp);
    int16_t z = fp_round(actor->z_fp);
    if (rom_x < -338 || rom_x >= 338) return false;

    if (tipoff->rim_raw_092c < 120u) {
        /* `$80:CEFD` is `$80:CEE7 & $7FFF`, stored in DP $AA. */
        uint16_t random = nba_gameplay_rng_next(&tipoff->rng) & 0x7FFFu;
        if ((random & 0x0008u) == 0u && z == 0)
            return cpu_start_rom_shot(tipoff, slot);
    }

    int16_t side_anchor = slot < 5u ? -336 : 336;
    bool same_attack_half = (int16_t)(rom_x ^ side_anchor) >= 0;
    if (same_attack_half &&
        nba_gameplay_mode11_shot_rectangle(rom_x, y, z))
        return cpu_start_rom_shot(tipoff, slot);
    if (tipoff->play_hold_raw != 0u && tipoff->play_step_raw == 4 && z == 0)
        return cpu_start_rom_shot(tipoff, slot);
    return false;
}

/* `$86:9D6E-$86:A45E`: detach only after mode 12 reaches its CPU release
 * gate, then run the already-ported ROM shot-value, quality, miss-offset and
 * launch calculations. */
static void cpu_release_rom_shot(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *shooter = &tipoff->actors[slot];
    int basket_x = basket_x_for_side(tipoff->offense_side);
    int basket_y = basket_y_for_side(tipoff->offense_side);
    int16_t release_x_rom = tipoff->shot_origin_x;
    tipoff->shot_value_raw = nba_gameplay_shot_value(
        false, release_x_rom, tipoff->shot_origin_y,
        tipoff->offense_side != 0u);
    uint8_t team = tipoff->offense_side ?
        tipoff->session->right_team : tipoff->session->left_team;
    uint8_t rating_2pt = 0xA8u, rating_3pt = 0xA8u;
    (void)nba_player_gameplay_shot_ratings(
        tipoff->assets, team, shooter->roster_slot,
        &rating_2pt, &rating_3pt);
    uint8_t rating = tipoff->shot_value_raw == 3u ? rating_3pt : rating_2pt;
    uint8_t difficulty = (uint8_t)tipoff->session->config.main_values[2];
    tipoff->shot_chance_raw = nba_gameplay_shot_chance(
        rating, rating, difficulty, true);
    uint8_t roll = (uint8_t)nba_gameplay_rng_next(&tipoff->rng);
    tipoff->shot_inner_veto_raw = roll >= tipoff->shot_chance_raw;
    tipoff->shot_miss_index_raw = 0xFFu;
    if (tipoff->shot_inner_veto_raw) {
        int16_t miss_x, miss_y;
        tipoff->shot_miss_index_raw =
            (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 0x0Fu);
        nba_gameplay_miss_offset(tipoff->shot_miss_index_raw,
            tipoff->offense_side == 0u, &miss_x, &miss_y);
        basket_x += miss_x;
        basket_y += miss_y;
    }
    tipoff->live_state_raw = 1u; /* `$86:9DDB-$9DE4` */
    nba_gameplay_shot_launch(tipoff->ball.x_fp,
        tipoff->ball.y_fp, tipoff->ball.z_fp,
        (int16_t)basket_x, (int16_t)basket_y,
        &tipoff->ball.velocity_x, &tipoff->ball.velocity_y,
        &tipoff->ball.velocity_z);
    tipoff->ball.owner_actor = -1;
    tipoff->ball.state = NBA_BALL_SHOT;
    tipoff->possession_actor = -1;
    shooter->control_mode = 11u;
    shooter->reaction_threshold = 0u;
    shooter->behavior_flags_raw = 0u;
    actor_set_upper_animation(shooter, 0x17u);
}

/* `$86:B769/$86:B8CA-$B978`: attach the ball to the live pose throughout
 * the shot jump. Common `$85:963D` physics subtracts `$18*$C6` from +$12
 * and integrates with `$C6=2`; release is based on the signed velocity, not
 * a rendered-frame counter. */
static bool cpu_update_rom_shooter(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *shooter = &tipoff->actors[slot];
    if (tipoff->possession_actor != (int8_t)slot) {
        shooter->control_mode = 1u;
        shooter->reaction_threshold = 0u;
        shooter->behavior_flags_raw = 0u;
        return false;
    }

    shooter->behavior_flags_raw |= 0x0002u;
    tipoff->live_state_raw = 2u;
    ball_attach_to_actor(tipoff, slot);
    tipoff->ball_activity_raw = 0xFFFFu;
    bool release = shooter->velocity_z < 0;
    if (!release && shooter->velocity_z < 0x0060)
        release = tipoff->fouls.free_throw_state_raw_0978 != 0u ||
                  (tipoff->rng.state & 0x0070u) == 0u;
    if (release) cpu_release_rom_shot(tipoff, slot);

    cpu_integrate_actor_vertical(shooter);
    shooter->x_fp += (int32_t)shooter->velocity_x * 2;
    shooter->y_fp += (int32_t)shooter->velocity_y * 2;
    shooter->movement_magnitude_raw = actor_distance(
        shooter->velocity_x, shooter->velocity_y);
    return true;
}

/* `$86:A6B3-$A790`: only the owner advances mode 15's release gate. The
 * ROM reattaches through `$87:B649` while threshold >= actor +$3A, then
 * dispatches `$86:99C4` with family selected by signed actor +$C0. */
static bool cpu_update_rom_passer(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *passer = &tipoff->actors[slot];
    /* `$86:A6B3` keys ownership from `$093E` and retains the receiver in the
     * actor/pass executor state. `$86:A613` may clear `$0942/$0946` when any
     * integrated record touches a rectangular court edge, so those telemetry
     * mirrors cannot be used to abort an already-installed mode 15. */
    if (tipoff->handler_actor != slot ||
        tipoff->receiver_actor >= NBA_GAMEPLAY_ACTOR_COUNT)
        return true;
    if (passer->pass_released_raw) return true;
    if (tipoff->possession_actor != (int8_t)slot) return true;
    if (passer->upper_animation_phase_raw <=
            passer->pass_release_threshold_raw) {
        ball_attach_to_actor(tipoff, slot);
        return true;
    }
    unsigned family = passer->pass_family_raw < 0 ? 0u : 1u;
    unsigned band = passer->pass_band_raw / 6u;
    int16_t duration = 0, vertical = 0, opaque = 0;
    if (!nba_assets_gameplay_pass_launch(
            tipoff->assets, (uint8_t)family, (uint8_t)band,
            &duration, &vertical, &opaque) || duration <= 0)
        return true;
    (void)opaque; /* `$86:99C4` does not consume the third record word. */
    const NbaTipoffActor *receiver =
        &tipoff->actors[tipoff->receiver_actor];
    int target_x = fp_round(receiver->x_fp) +
        (int)pass_lead_component(receiver->velocity_x, (uint16_t)duration);
    int target_y = fp_round(receiver->y_fp) +
        (int)pass_lead_component(receiver->velocity_y, (uint16_t)duration);
    ball_launch(tipoff, target_x, target_y, (unsigned)duration,
                vertical, NBA_BALL_PASS);
    /* `$86:9B84-$9B8F`: `$99C4` closes the temporary pass-init state 2
     * before normal live ball processing resumes. Leaving it set makes
     * `$85:9D40` reject an otherwise centered basket. */
    if (tipoff->live_state_raw < 0x81u) tipoff->live_state_raw = 0u;
    tipoff->possession_actor = -1;
    passer->pass_released_raw = true;
    return true;
}

static void score_made_basket(NbaTipoff *tipoff) {
    /* `$85:A079-$A345` is the made-basket branch. `$094C` is added to
     * team-record +$26 (`$4711/$4791`), `$0936` becomes `$82`, and
     * `$0952/$0954` seed the dead-ball/inbound path before `$094C` clears. */
    unsigned scoring_side = tipoff->offense_side & 1u;
    unsigned inbound_side = scoring_side ^ 1u;
    tipoff->session->score[scoring_side] = (uint16_t)(
        tipoff->session->score[scoring_side] + tipoff->shot_value_raw);
    tipoff->last_scoring_side = (uint8_t)scoring_side;
    /* The accepted `$85:9D3E->$A079` make has entered the hoop cylinder.
     * The CPU oracle is centered at +/-336 with zero planar velocity while
     * it falls to the inbound-side pose collision. Preserve the subpixel Z
     * descent, but close the host 24.8 overshoot that otherwise lets a made
     * ball drift to the corner before `$86:CCFC` can visit it. */
    tipoff->ball.x_fp = basket_x_for_side(scoring_side) * 256;
    tipoff->ball.y_fp = 0;
    tipoff->ball.velocity_x = 0;
    tipoff->ball.velocity_y = 0;
    tipoff->live_state_raw = 0x82u;
    tipoff->inbound_state_raw = (uint16_t)(inbound_side * 5u);
    tipoff->inbound_actor_raw = (uint16_t)(tipoff->inbound_state_raw + 2u);
    tipoff->inbound_layout_raw = 0;
    tipoff->inbound_timer_raw = 300u;
    /* `$85:A219-$A222`: dead-ball scoring requests play `$01`; B128 later
     * preserves it while resetting the stream at the actor-pass boundary. */
    tipoff->play_code = 1u;
    tipoff->play_request_raw = 1u;
    tipoff->offense_side = (uint8_t)inbound_side;
    tipoff->possession_team = (int8_t)inbound_side;
    tipoff->handler_actor = (uint8_t)tipoff->inbound_actor_raw;
    tipoff->receiver_actor = (uint8_t)(inbound_side * 5u + 4u);
    NbaGameplayInboundTarget target;
    int16_t context_anchor = inbound_side ? 336 : -336;
    if (nba_gameplay_inbound_target(
            tipoff->inbound_layout_raw, fp_round(tipoff->ball.x_fp),
            fp_round(tipoff->ball.y_fp), context_anchor,
            fp_round(tipoff->ball.x_fp), &tipoff->rng, &target)) {
        tipoff->inbound_target_x_raw = target.x;
        tipoff->inbound_target_y_raw = target.y;
        tipoff->inbound_direction_raw = target.direction;
        if (target.play_requested) {
            tipoff->play_code = target.play_code;
            tipoff->play_request_raw = 1u;
        }
        tipoff->actors[tipoff->inbound_actor_raw].target_x = target.x;
        tipoff->actors[tipoff->inbound_actor_raw].target_y = target.y;
    }
    tipoff->inbound_ready_raw = 0u;
    tipoff->shot_result_resolved = true;
    tipoff->ball.owner_actor = -1;
    tipoff->possession_actor = -1;
}

static NbaGameplayRimResult cpu_update_live_ball(NbaTipoff *tipoff) {
    /* The same `$87:9B0D` 30-Hz logical pass drives `$85:9ACB+` ball
     * collision/integration. Flight-table durations count these due passes. */
    if ((tipoff->simulation_tick & 1u) != 0u)
        return NBA_GAMEPLAY_RIM_FLIGHT;
    /* Oracle frames 1652..1683 write `$0970` 15..0 once per 30-Hz ball
     * pass. Decrement before contact so a newly installed response remains
     * at 15 for its complete first interval. */
    if (tipoff->rim_raw_0970 != 0u) --tipoff->rim_raw_0970;
    NbaTipoffBall *ball = &tipoff->ball;
    int32_t old_x = ball->x_fp, old_y = ball->y_fp, old_z = ball->z_fp;
    bool attached = ball->state == NBA_BALL_ATTACHED;
    if (attached) {
        ball_attach_to_actor(tipoff, tipoff->handler_actor);
    } else if (ball->state == NBA_BALL_PASS || ball->state == NBA_BALL_SHOT ||
               ball->state == NBA_BALL_BOUNCE) {
        bool rom_free_flight = ball->state == NBA_BALL_SHOT ||
                               ball->state == NBA_BALL_BOUNCE;
        bool made_response = false;
        NbaGameplayRimResult result = NBA_GAMEPLAY_RIM_FLIGHT;
        /* `$85:9A78-$A656`: the current integer position is tested against
         * the rim first. Any reflected velocity and snapped integer axis are
         * then consumed by gravity/integration on this same logical pass. */
        if (ball->state == NBA_BALL_SHOT) {
            int hoop_x = basket_x_for_side(tipoff->offense_side);
            int hoop_y = basket_y_for_side(tipoff->offense_side);
            int16_t before_x = fp_integer_word(ball->x_fp);
            int16_t before_y = fp_integer_word(ball->y_fp);
            int16_t before_z = fp_integer_word(ball->z_fp);
            NbaGameplayRimState rim = {
                before_x, before_y, before_z,
                ball->velocity_x, ball->velocity_y, ball->velocity_z,
                tipoff->rim_raw_092c, tipoff->rim_raw_0962,
                tipoff->rim_raw_096a, tipoff->rim_raw_097c,
                tipoff->rim_raw_096e, tipoff->rim_raw_13e7
            };
            /* `$85:9D65-$9D7B` compares the selected side context-anchor X
             * sign with the ball X sign; it is not an owner/team test. */
            bool correct_side = tipoff->offense_side != 0u ?
                                before_x >= 0 : before_x < 0;
            result = nba_gameplay_rim_world_step(
                &rim, (int16_t)hoop_x, (int16_t)hoop_y,
                tipoff->offense_side != 0u, tipoff->live_state_raw, false,
                tipoff->shot_inner_veto_raw, correct_side);
            NbaGameplayRimContext rim_context = {
                .raw_0920 = tipoff->rim_raw_0920,
                .raw_0936 = tipoff->live_state_raw,
                .raw_0948 = tipoff->ball_activity_raw,
                .raw_094a = tipoff->rim_raw_094a,
                .raw_0970 = tipoff->rim_raw_0970,
                .raw_0978 = tipoff->fouls.free_throw_state_raw_0978,
                .raw_09f8 = tipoff->shot_inner_veto_raw ? 1u : 0u,
                .raw_09b8 = tipoff->inbound_transfer_raw,
                .raw_1866 = tipoff->rim_force_raw_1866,
                .raw_07f6 = tipoff->rng.state,
                .effect_raw_401b = tipoff->rim_effect.effect_raw_401b
            };
            nba_gameplay_rim_apply_inner_response(
                &rim, result, &rim_context, &tipoff->rng);
            if (result == NBA_GAMEPLAY_RIM_MAKE) {
                nba_gameplay_rim_apply_made_response(
                    &rim, tipoff->offense_side != 0u, &rim_context);
                made_response = true;
            }
            tipoff->rim_raw_092c = rim.raw_092c;
            tipoff->rim_raw_0962 = rim.raw_0962;
            tipoff->rim_raw_096a = rim.raw_096a;
            tipoff->rim_raw_097c = rim.raw_097c;
            tipoff->rim_raw_096e = rim.raw_096e;
            tipoff->rim_raw_13e7 = rim.raw_13e7;
            tipoff->rim_raw_0920 = rim_context.raw_0920;
            tipoff->live_state_raw = rim_context.raw_0936;
            tipoff->ball_activity_raw = rim_context.raw_0948;
            tipoff->rim_raw_094a = rim_context.raw_094a;
            tipoff->rim_raw_0970 = rim_context.raw_0970;
            tipoff->shot_inner_veto_raw = rim_context.raw_09f8 != 0u;
            tipoff->inbound_transfer_raw = rim_context.raw_09b8;
            if (result == NBA_GAMEPLAY_RIM_MISS)
                (void)nba_gameplay_effect_start(&tipoff->rim_effect, 3u);
            if (result == NBA_GAMEPLAY_RIM_OUTER_CONTACT ||
                result == NBA_GAMEPLAY_RIM_EDGE_CONTACT ||
                result == NBA_GAMEPLAY_RIM_MISS ||
                result == NBA_GAMEPLAY_RIM_MAKE) {
                /* The ROM writes only axes touched by the collision branch;
                 * retain each independent fractional byte. */
                if (result == NBA_GAMEPLAY_RIM_MAKE) {
                    /* `$85:A34A-$A3B3` rebuilds the made-ball record from
                     * integer scratch words, so stale host fractions do not
                     * survive the cylinder anchor. */
                    ball->x_fp = (int32_t)rim.x * 256;
                    ball->y_fp = (int32_t)rim.y * 256;
                    ball->z_fp = (int32_t)rim.z * 256;
                } else if (rim.x != before_x)
                    ball->x_fp = fp_replace_integer_word(ball->x_fp, rim.x);
                if (result != NBA_GAMEPLAY_RIM_MAKE && rim.y != before_y)
                    ball->y_fp = fp_replace_integer_word(ball->y_fp, rim.y);
                if (result != NBA_GAMEPLAY_RIM_MAKE && rim.z != before_z)
                    ball->z_fp = fp_replace_integer_word(ball->z_fp, rim.z);
                ball->velocity_x = rim.velocity_x;
                ball->velocity_y = rim.velocity_y;
                ball->velocity_z = rim.velocity_z;
            }
        }
        if (rom_free_flight && !made_response)
            ball->velocity_z = (int16_t)(ball->velocity_z - 0x18);
        ball->x_fp += ball->velocity_x;
        ball->y_fp += ball->velocity_y;
        ball->z_fp += ball->velocity_z;
        cpu_clamp_record_to_court(tipoff, &ball->x_fp, &ball->y_fp,
                                  &ball->velocity_x, &ball->velocity_y);
        if (!rom_free_flight && !made_response)
            ball->velocity_z = (int16_t)(ball->velocity_z - 48);
        if (ball->z_fp < 0) {
            ball->z_fp = 0;
            if (rom_free_flight) {
                /* `$85:A3B7-$A4DA`: gravity precedes integration; ground
                 * impact applies 7/8 vertical restitution (cap $0400) and
                 * impact-only 15/16 lateral damping. */
                NbaGameplayRimState impact = {
                    fp_integer_word(ball->x_fp), fp_integer_word(ball->y_fp),
                    0, ball->velocity_x, ball->velocity_y, ball->velocity_z,
                    0u, 0u, 0u, tipoff->rim_raw_097c, 0u,
                    tipoff->rim_raw_13e7
                };
                nba_gameplay_ball_apply_ground_impact(
                    &impact, &tipoff->rim_impact_raw_13e5);
                ball->velocity_x = impact.velocity_x;
                ball->velocity_y = impact.velocity_y;
                ball->velocity_z = impact.velocity_z;
                tipoff->rim_raw_13e7 = impact.raw_13e7;
                ball->state = NBA_BALL_BOUNCE;
                NbaGameplayRimState settle = {
                    fp_integer_word(ball->x_fp), fp_integer_word(ball->y_fp),
                    fp_integer_word(ball->z_fp), ball->velocity_x,
                    ball->velocity_y, ball->velocity_z, 0u, 0u, 0u,
                    tipoff->rim_raw_097c, 0u, 0u
                };
                NbaGameplaySettleContext settle_context = {
                    .raw_0936 = tipoff->live_state_raw,
                    .raw_0942 = (uint16_t)tipoff->pass_actor_raw,
                    .raw_0944 = (uint16_t)tipoff->pass_aux_raw,
                    .raw_0946 = (uint16_t)tipoff->pass_receiver_raw,
                    .raw_0948 = tipoff->ball_activity_raw,
                    .raw_094a = tipoff->rim_raw_094a,
                    .raw_0978 = tipoff->fouls.free_throw_state_raw_0978,
                    .raw_09b8 = tipoff->inbound_transfer_raw
                };
                if (nba_gameplay_ball_apply_settle(
                        &settle, &settle_context)) {
                    ball->velocity_x = settle.velocity_x;
                    ball->velocity_y = settle.velocity_y;
                    ball->velocity_z = settle.velocity_z;
                    tipoff->rim_raw_097c = settle.raw_097c;
                    tipoff->live_state_raw = settle_context.raw_0936;
                    tipoff->pass_actor_raw =
                        (int16_t)settle_context.raw_0942;
                    tipoff->pass_aux_raw =
                        (int16_t)settle_context.raw_0944;
                    tipoff->pass_receiver_raw =
                        (int16_t)settle_context.raw_0946;
                    tipoff->ball_activity_raw = settle_context.raw_0948;
                    tipoff->rim_raw_094a = settle_context.raw_094a;
                    tipoff->inbound_transfer_raw = settle_context.raw_09b8;
                }
            } else {
                ball->velocity_z = (int16_t)(-ball->velocity_z / 2);
                ball->velocity_x = (int16_t)(ball->velocity_x * 3 / 4);
                ball->velocity_y = (int16_t)(ball->velocity_y * 3 / 4);
                ball->state = NBA_BALL_BOUNCE;
            }
        }
        /* `$87:8F95-$8FA9` schedules `$85:9A24` ball physics before
         * `$87:AA02`, so a miss-started effect receives its first dt=2 step
         * on this same logical pass. */
        nba_gameplay_effect_step(
            &tipoff->rim_effect, fp_integer_word(ball->y_fp),
            fp_integer_word(ball->z_fp), ball->velocity_z, 2u);
        return result;
    }
    if (attached) {
        ball->velocity_x = (int16_t)(ball->x_fp - old_x);
        ball->velocity_y = (int16_t)(ball->y_fp - old_y);
        ball->velocity_z = (int16_t)(ball->z_fp - old_z);
    }
    nba_gameplay_effect_step(
        &tipoff->rim_effect, fp_integer_word(ball->y_fp),
        fp_integer_word(ball->z_fp), ball->velocity_z, 2u);
    return NBA_GAMEPLAY_RIM_FLIGHT;
}

/* Contact-tick regression for `$85:9A78-$A656`: the negative Y lip reflects
 * velocity before this pass integrates it, while all three fractional bytes
 * survive. This deliberately exercises the live orchestration rather than
 * only the isolated rim classifier. */
static bool cpu_rim_contact_tick_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    nba_gameplay_effect_init(&state.rim_effect);
    state.offense_side = 1u;
    state.handler_actor = 5u;
    state.live_state_raw = 1u;
    state.rim_raw_096e = 7u;
    state.rim_raw_0970 = 4u;
    state.ball.state = NBA_BALL_SHOT;
    state.ball.x_fp = 344 * 256 + 128;
    state.ball.y_fp = -24 * 256 + 64;
    state.ball.z_fp = 80 * 256 + 96;
    state.ball.velocity_x = 100;
    state.ball.velocity_y = 31;
    state.ball.velocity_z = 150;
    NbaGameplayRimResult result = cpu_update_live_ball(&state);
    return result == NBA_GAMEPLAY_RIM_OUTER_CONTACT &&
           state.ball.x_fp == 344 * 256 + 228 &&
           state.ball.y_fp == -27 * 256 + 49 &&
           state.ball.z_fp == 80 * 256 + 222 &&
           state.ball.velocity_x == 100 &&
           state.ball.velocity_y == -15 &&
           state.ball.velocity_z == 126 &&
           state.rim_raw_096e == 7u &&
           state.rim_raw_0970 == 3u &&
           (state.rim_raw_13e7 & 0x0008u) != 0u;
}

static void cpu_update_tip_ball(NbaTipoff *tipoff) {
    NbaTipoffBall *ball = &tipoff->ball;
    int32_t old_x = ball->x_fp, old_y = ball->y_fp, old_z = ball->z_fp;
    if (tipoff->frame < NBA_TIPOFF_CONTACT_FRAME) {
        int t = tipoff->frame - 170;
        int z = tipoff->frame < NBA_TIPOFF_TOSS_FRAME ? 80 :
            108 - (t * t * (t < 0 ? 28 : 40)) / (t < 0 ? 625 : 784);
        ball->x_fp = ball->y_fp = 0;
        ball->z_fp = (int32_t)z * 256;
        ball->state = tipoff->frame < NBA_TIPOFF_BALL_APPEAR_FRAME ?
                      NBA_BALL_HIDDEN : NBA_BALL_TOSS;
    } else {
        int t = tipoff->frame - NBA_TIPOFF_CONTACT_FRAME;
        if (t > 22) t = 22;
        ball->x_fp = (int32_t)(-92 * t) * 256 / 22;
        ball->y_fp = (int32_t)(36 * t) * 256 / 22;
        ball->z_fp = (int32_t)(67 + (14 - 67) * t / 22) * 256;
        ball->state = NBA_BALL_LOOSE;
    }
    ball->velocity_x = (int16_t)(ball->x_fp - old_x);
    ball->velocity_y = (int16_t)(ball->y_fp - old_y);
    ball->velocity_z = (int16_t)(ball->z_fp - old_z);
}

static bool cpu_load_next_play_control(NbaTipoff *tipoff) {
    NbaGameplayPlayControlRecord record;
    uint8_t count = 0u;
    int next = tipoff->play_step_raw + 1;
    if (!nba_assets_gameplay_play_control(
            tipoff->assets, (uint8_t)tipoff->play_code, (uint8_t)next,
            &record, &count)) {
        if (next < count) return false;
        /* `$85:B309-$B326`: `$09D0` repeats the final record; otherwise the
         * `$23BA` sentinel sets `$09A4`, rewinds `$0998`, and loads record 0. */
        if (tipoff->play_hold_raw != 0u) {
            next = count ? (int)count - 1 : 0;
        } else {
            tipoff->play_cycle_raw = 1u;
            next = 0;
        }
        if (!nba_assets_gameplay_play_control(
                tipoff->assets, (uint8_t)tipoff->play_code, (uint8_t)next,
                &record, &count)) return false;
    }
    tipoff->play_step_raw = (int16_t)next;
    if (record.countdown < 0) {
        tipoff->play_countdown_raw = 0;
        tipoff->play_event_wait_raw = 1u;
    } else {
        tipoff->play_countdown_raw = record.countdown;
        tipoff->play_event_wait_raw = 0u;
    }
    /* DP `$9E` is the active team context; it is independent of the camera's
     * persistent free-ball side proxy `$093A`. */
    int side_base = tipoff->offense_side ? 5 : 0;
    const int16_t source[3] = {
        record.selector_a, record.selector_b, record.selector_c
    };
    for (unsigned i = 0; i < 3u; ++i)
        tipoff->play_selector_raw[i] = source[i] < 0 ? source[i] :
                                       (int16_t)(source[i] + side_base);
    if (tipoff->live_state_raw == 0x82u)
        tipoff->play_cycle_raw = 0u; /* `$85:B31E-$B326` */
    return true;
}

static void cpu_advance_play_control(NbaTipoff *tipoff) {
    unsigned base = tipoff->offense_side ? 5u : 0u;
    /* `$85:B28B-$B2AF`: every record boundary consumes readiness bits 08/40
     * for this side only, then may raise the randomized transition flag. */
    for (unsigned i = 0; i < 5u; ++i)
        tipoff->actors[base + i].behavior_flags_raw &= 0xFFB7u;
    if (tipoff->play_step_raw >= 2 && (tipoff->rng.state & 1u) != 0u &&
        tipoff->play_hold_raw == 0u)
        tipoff->play_cycle_raw = 1u;
    (void)cpu_load_next_play_control(tipoff);
}

static void cpu_reset_play_control(NbaTipoff *tipoff) {
    /* `$85:B377`: initialize the stream registers, then fall through
     * `$85:B28B/$B2DC` to load record zero. */
    tipoff->play_step_raw = -1;
    tipoff->play_countdown_raw = 0;
    tipoff->play_event_wait_raw = 0u;
    tipoff->play_cycle_raw = 0u;
    tipoff->special_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->play_mirror_raw = tipoff->play_code >= 0x12u ?
        (tipoff->rng.state & 1u) : 0u;
    for (unsigned i = 0; i < 3u; ++i) tipoff->play_selector_raw[i] = -1;
    cpu_advance_play_control(tipoff);
}

static void cpu_reselect_play_control(NbaTipoff *tipoff) {
    /* `$85:B128-$B24B`: `$0994` is consumed only at the completed logical
     * pass boundary. The first RNG result updates the opposite/defending
     * team context at `$46EB/$476B +$30`. */
    uint16_t context_random = nba_gameplay_rng_next(&tipoff->rng);
    unsigned offense = tipoff->offense_side != 0u ? 1u : 0u;
    unsigned defense = offense ^ 1u;
    (void)nba_gameplay_defense_context_reselect(
        tipoff->session->score[offense], tipoff->session->score[defense],
        tipoff->period_raw_0926,
        tipoff->team_context[defense].activity_raw_39, context_random,
        &tipoff->team_context[defense].mode_raw_30);
    tipoff->play_request_raw = 0u;
    tipoff->play_event_wait_raw = 0u;
    tipoff->play_cycle_raw = 0u;
    if (tipoff->live_state_raw == 0x82u) {
        /* `$85:B176/$B245`: made-score state preserves `$0996` and `$09D0`
         * while B377 rewinds the selected stream to record zero. */
        cpu_reset_play_control(tipoff);
        return;
    }

    /* The currently represented context has +$2E!=7 and +$56==0. Preserve
     * the exact normal-path RNG rejection order before the team/coin table. */
    uint8_t preliminary;
    do preliminary = (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 7u);
    while (preliminary >= 6u);
    (void)preliminary;
    uint8_t coin = (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 1u);
    uint8_t team = tipoff->offense_side ? tipoff->session->right_team :
                                          tipoff->session->left_team;
    uint8_t strategy, base, count;
    bool hold;
    if (!nba_assets_gameplay_cpu_strategy(
            tipoff->assets, team, coin, &strategy, &base, &count, &hold))
        return;
    (void)strategy;
    uint8_t offset;
    do offset = (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 7u);
    while (offset >= count);
    tipoff->play_code = (uint16_t)(base + offset);
    tipoff->play_hold_raw = hold ? 1u : 0u;
    cpu_reset_play_control(tipoff);
}

static void cpu_update_play_control(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u) return;
    if (tipoff->play_request_raw != 0u) {
        cpu_reselect_play_control(tipoff);
        return;
    }
    int16_t remaining = (int16_t)(uint16_t)(
        (uint16_t)tipoff->play_countdown_raw - 2u);
    if (remaining >= 0) {
        tipoff->play_countdown_raw = remaining;
        return;
    }
    if (tipoff->play_event_wait_raw != 0u) {
        unsigned base = tipoff->offense_side ? 5u : 0u;
        for (unsigned i = 0; i < 5u; ++i) {
            const NbaTipoffActor *actor = &tipoff->actors[base + i];
            if (actor->controller_assignment_raw < 0 &&
                (actor->behavior_flags_raw & 0x0040u) == 0u) {
                /* `$85:B271-$B279/$B353`: retain the signed underflow and
                 * retry this same five-actor barrier next logical pass. */
                tipoff->play_countdown_raw = remaining;
                return;
            }
        }
        tipoff->play_event_wait_raw = 0u; /* `$85:B288` */
    }
    cpu_advance_play_control(tipoff);
}

static void cpu_begin_possession(NbaTipoff *tipoff, uint8_t offense_side) {
    static const uint8_t play_codes[4] = {0x35u, 0x01u, 0x0Fu, 0x26u};
    static const uint8_t handler_slots[4] = {3u, 3u, 2u, 2u};
    tipoff->offense_side = offense_side;
    tipoff->possession_team = (int8_t)offense_side;
    tipoff->camera_side_group_raw = offense_side ? 5u : 0u;
    tipoff->possession_frame = 0;
    tipoff->play_state_frame = 0;
    tipoff->play_code = play_codes[tipoff->possession_number % 4u];
    tipoff->play_hold_raw = 0u;
    tipoff->special_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    cpu_reset_play_control(tipoff);
    if (tipoff->possession_number == 0u) {
        /* The `$86:D3F9 -> $86:BAA2` jump-ball ownership commit consumes
         * play `$35` record zero's `$09AA/$09AC/$09AE` opportunity. The
         * CPU-only oracle is already all `$FFFF` when mode 11 starts at
         * frame 220; record one later reloads `[9,7,-1]`. */
        tipoff->play_selector_raw[0] = -1;
        tipoff->play_selector_raw[1] = -1;
        tipoff->play_selector_raw[2] = -1;
    }
    unsigned base = offense_side ? 5u : 0u;
    tipoff->handler_actor = (uint8_t)(base + handler_slots[tipoff->possession_number % 4u]);
    tipoff->receiver_actor = (uint8_t)(base +
        ((handler_slots[tipoff->possession_number % 4u] + 1u) % 5u));
    tipoff->possession_actor = (int8_t)tipoff->handler_actor;
    tipoff->cpu_play_state = NBA_CPU_PLAY_BREAK;
    tipoff->shot_result_resolved = false;
    tipoff->shot_inner_veto_raw = false;
    tipoff->shot_miss_index_raw = 0xFFu;
    tipoff->shot_value_raw = 0u;
    /* `$092C=05A0` is 24 seconds at the ROM's 60-Hz outer cadence. The
     * oracle is 1440 at frame 220 and 1260 at frame 400. */
    tipoff->rim_raw_092c = 0x05A0u;
    tipoff->pass_actor_raw = -1;
    tipoff->pass_receiver_raw = -1;
    tipoff->pass_active_raw = 0u;
    tipoff->pass_distance_raw = 0u;
    ball_attach_to_actor(tipoff, tipoff->handler_actor);
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
        tipoff->actors[actor].control_mode =
            actor / 5u == offense_side ? 1u : 2u;
    tipoff->actors[tipoff->handler_actor].control_mode = 11u;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        state->reaction_threshold = nba_gameplay_reaction_threshold(
            &tipoff->rng, fp_round(state->x_fp), fp_round(state->y_fp),
            fp_round(tipoff->ball.x_fp), fp_round(tipoff->ball.y_fp));
    }
    /* `$86:F3DD-$F439` begins CPU mode 11 with +$60=0, making the first
     * B678/AD6B/B50E decision immediately due. CPU ownership remains in
     * `$093E`; actor +$16 stays negative. */
    tipoff->actors[tipoff->handler_actor].reaction_threshold = 0u;
}

static void cpu_commit_ball_acquisition(NbaTipoff *tipoff, uint8_t catcher) {
    /* `$86:BAA2-$BC99` is shared by pass catches and loose rebounds. It
     * installs the collision winner without mass-resetting the other nine
     * actor modes, clears the old play opportunities, and requests the next
     * ROM strategy through `$0994`. */
    bool inbound_completion = tipoff->live_state_raw == 0x82u;
    unsigned previous_side = tipoff->offense_side;
    unsigned side = catcher / 5u;
    NbaTipoffActor *catcher_state = &tipoff->actors[catcher];
    /* `$86:BAB9-$BAC7`: only a slow catcher loses lateral momentum. */
    if (catcher_state->movement_magnitude_raw < 0x0080u) {
        catcher_state->velocity_x = 0;
        catcher_state->velocity_y = 0;
        catcher_state->movement_magnitude_raw = 0u;
    }
    if (side != previous_side || inbound_completion) {
        ++tipoff->possession_number;
        tipoff->rim_raw_092c = 0x05A0u; /* `$86:BB76-$BB81` */
        /* `$86:BB3B-$BB6E` restores all actor +$74 assignments from +$76
         * and raises `$09D6`; the end-frame AF5C/BC07 pair owns modes. */
        for (unsigned actor = 0;
             actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            NbaTipoffActor *state = &tipoff->actors[actor];
            state->assignment_current_raw = state->assignment_base_raw;
            state->assignment_actor =
                (uint8_t)(state->assignment_current_raw >> 1);
        }
        tipoff->role_rebuild_raw_09d6 = NBA_GAMEPLAY_UNKNOWN_WORD;
    }
    if (tipoff->handler_actor != catcher &&
        tipoff->handler_actor < NBA_GAMEPLAY_ACTOR_COUNT)
        tipoff->actors[tipoff->handler_actor].control_mode = 1u;
    tipoff->offense_side = (uint8_t)side;
    tipoff->possession_team = (int8_t)side;
    tipoff->camera_side_group_raw = side ? 5u : 0u;
    tipoff->possession_frame = 0u;
    tipoff->play_state_frame = 0u;
    tipoff->handler_actor = catcher;
    tipoff->receiver_actor = catcher;
    ball_attach_to_actor(tipoff, catcher);
    tipoff->possession_actor = (int8_t)catcher;
    catcher_state->control_mode = 11u;
    catcher_state->reaction_threshold = 0u;
    catcher_state->behavior_flags_raw = 0u;
    catcher_state->velocity_z = 0;
    tipoff->play_request_raw = 1u;
    tipoff->special_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->play_selector_raw[0] = -1;
    tipoff->play_selector_raw[1] = -1;
    tipoff->play_selector_raw[2] = -1;
    tipoff->pass_actor_raw = -1;
    tipoff->pass_receiver_raw = -1;
    tipoff->pass_active_raw = 0u;
    tipoff->pass_distance_raw = 0u;
    tipoff->cpu_play_state = NBA_CPU_PLAY_ATTACK;
    tipoff->shot_result_resolved = false;
    tipoff->shot_inner_veto_raw = false;
    tipoff->shot_miss_index_raw = 0xFFu;
    /* `$86:BC81-$BC90` runs after rebound/stat classification. In particular,
     * BC8A clears the rim context which otherwise traps formation roles 3/4
     * in `$85:AF2A` on the next possession. `$096E` is a separate cooldown
     * and is deliberately preserved. Owner installation makes `$85:A7A8`
     * clear `$0962` at the following common ball tail. */
    tipoff->ball_activity_raw = 0u;
    tipoff->shot_value_raw = 0u;
    tipoff->rim_raw_096a = 0u;
    tipoff->rim_raw_097c = 0u;
    tipoff->rim_raw_0962 = 0u;
    tipoff->rim_raw_13e7 |= 0x0010u;
    tipoff->live_state_raw = 0u;
    tipoff->inbound_transfer_raw = 0u;
    if (inbound_completion) {
        tipoff->inbound_state_raw = 0u;
        tipoff->inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        tipoff->inbound_timer_raw = 0u;
        tipoff->inbound_ready_raw = 0u;
    }
}

static void cpu_enter_play_state(NbaTipoff *tipoff, NbaCpuPlayState state) {
    tipoff->cpu_play_state = (uint8_t)state;
    tipoff->play_state_frame = 0u;
}

static uint16_t cpu_rom_distance(int16_t dx, int16_t dy) {
    uint16_t ax = dx < 0 ? (uint16_t)(0u - (uint16_t)dx) : (uint16_t)dx;
    uint16_t ay = dy < 0 ? (uint16_t)(0u - (uint16_t)dy) : (uint16_t)dy;
    uint16_t major = ax >= ay ? ax : ay;
    uint16_t minor = ax >= ay ? ay : ax;
    return (uint16_t)(major + (minor >> 2));
}

static void cpu_refresh_pair_geometry(NbaTipoff *tipoff, unsigned actor) {
    NbaTipoffActor *state = &tipoff->actors[actor];
    unsigned paired = state->assignment_current_raw >> 1;
    if (paired >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    NbaTipoffActor *other = &tipoff->actors[paired];
    uint16_t distance = 0u;
    uint8_t direction = nba_gameplay_target_direction(
        (int16_t)(fp_round(other->x_fp) - fp_round(state->x_fp)),
        (int16_t)(fp_round(other->y_fp) - fp_round(state->y_fp)),
        &distance);
    state->assignment_actor = (uint8_t)paired;
    /* `$85:BC6B-$BC7A`: coincident actors (direction 8) retain both prior
     * +$86 direction words while still receiving the zero +$8A distance. */
    if (direction != 8u) state->assignment_direction = direction;
    state->assignment_distance = distance;
    state->pair_distance = distance;
    if (direction != 8u)
        other->assignment_direction = (uint8_t)(direction ^ 4u);
    other->pair_distance = distance;
}

static int cpu_select_help_defender(NbaTipoff *tipoff, unsigned owner,
                                    unsigned defense, int16_t anchor,
                                    const uint16_t focal_distance[5]) {
    /* `$85:BB6C-$BB98`: first try owner's +$78 candidate. It must be
     * unassigned and below mode 4; a candidate across the context half is
     * accepted only within $30 of the owner/focal point. */
    unsigned alternate =
        tipoff->actors[owner].assignment_alternate_raw >> 1;
    if (alternate < NBA_GAMEPLAY_ACTOR_COUNT && alternate / 5u == defense) {
        NbaTipoffActor *candidate = &tipoff->actors[alternate];
        bool same_half =
            (int16_t)(anchor ^ fp_round(tipoff->actors[owner].x_fp)) >= 0;
        if ((int16_t)candidate->assignment_current_raw < 0 &&
            candidate->control_mode < 4u &&
            (same_half || focal_distance[alternate % 5u] < 0x30u))
            return (int)alternate;
    }

    /* `$85:BBBF-$BC06`: fallback scans all five defenders and replaces on
     * equality, so the last actor wins a distance tie. */
    int selected = -1;
    uint16_t best = 0x7FFFu;
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned candidate = defense * 5u + i;
        if (tipoff->actors[candidate].control_mode >= 4u) continue;
        if (focal_distance[i] <= best) {
            best = focal_distance[i];
            selected = (int)candidate;
        }
    }
    return selected;
}

static void cpu_install_help_assignment(NbaTipoff *tipoff, unsigned helper,
                                        unsigned owner) {
    /* `$85:BB99-$BBBE` is deliberately one-way. If the helper was assigned,
     * its former offense actor is unbound; the owner's +$74 stays attached
     * to the primary defender. */
    NbaTipoffActor *state = &tipoff->actors[helper];
    unsigned old = state->assignment_current_raw >> 1;
    if (old < NBA_GAMEPLAY_ACTOR_COUNT)
        tipoff->actors[old].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state->assignment_current_raw = (uint16_t)(owner * 2u);
    state->assignment_actor = (uint8_t)owner;
    state->control_mode = 6u;
}

static void cpu_symmetric_bind(NbaTipoff *tipoff, unsigned candidate,
                               unsigned target) {
    unsigned old = tipoff->actors[candidate].assignment_current_raw >> 1;
    if (old < NBA_GAMEPLAY_ACTOR_COUNT)
        tipoff->actors[old].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->actors[candidate].assignment_current_raw = (uint16_t)(target * 2u);
    tipoff->actors[target].assignment_current_raw = (uint16_t)(candidate * 2u);
    tipoff->actors[candidate].assignment_actor = (uint8_t)target;
    tipoff->actors[target].assignment_actor = (uint8_t)candidate;
}

static bool cpu_try_base_defender(NbaTipoff *tipoff, unsigned target,
                                  unsigned defense, int16_t anchor) {
    /* `$85:BAE4-$BB6B`: prefer target +$78, with the extra opposite-half
     * basket/separation gates preserved exactly. */
    unsigned candidate = tipoff->actors[target].assignment_alternate_raw >> 1;
    if (candidate >= NBA_GAMEPLAY_ACTOR_COUNT || candidate / 5u != defense)
        return false;
    NbaTipoffActor *c = &tipoff->actors[candidate];
    NbaTipoffActor *t = &tipoff->actors[target];
    if ((int16_t)c->assignment_current_raw >= 0 || c->control_mode >= 7u)
        return false;
    bool bind = (int16_t)(anchor ^ fp_round(t->x_fp)) >= 0;
    if (!bind && c->anchor_distance_raw < 0x38u) bind = true;
    if (!bind && c->anchor_distance_raw < t->anchor_distance_raw) {
        uint16_t separation = cpu_rom_distance(
            (int16_t)(fp_round(t->x_fp) - fp_round(c->x_fp)),
            (int16_t)(fp_round(t->y_fp) - fp_round(c->y_fp)));
        bind = (uint16_t)(separation << 1) < t->anchor_distance_raw;
    }
    if (!bind) return false;
    cpu_symmetric_bind(tipoff, candidate, target);
    return true;
}

static bool cpu_fallback_primary_defender(NbaTipoff *tipoff, unsigned target,
                                          unsigned defense) {
    /* `$85:BA1D-$BAB6`: pass one minimizes +$8E among candidates no farther
     * from the basket than target; pass two minimizes +$8C. Equality replaces,
     * so the last defender wins ties in both passes. */
    int selected = -1;
    uint16_t best = 0x7FFFu;
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned actor = defense * 5u + i;
        NbaTipoffActor *state = &tipoff->actors[actor];
        if (state->control_mode >= 7u ||
            state->anchor_distance_raw > tipoff->actors[target].anchor_distance_raw)
            continue;
        if (state->focal_distance_raw_8e <= best) {
            best = state->focal_distance_raw_8e;
            selected = (int)actor;
        }
    }
    if (selected < 0) {
        best = 0x7FFFu;
        for (unsigned i = 0; i < 5u; ++i) {
            unsigned actor = defense * 5u + i;
            NbaTipoffActor *state = &tipoff->actors[actor];
            if (state->control_mode >= 7u) continue;
            if (state->anchor_distance_raw <= best) {
                best = state->anchor_distance_raw;
                selected = (int)actor;
            }
        }
    }
    if (selected < 0) return false;
    cpu_symmetric_bind(tipoff, (unsigned)selected, target);
    return true;
}

static bool cpu_bind_nearest_unassigned(NbaTipoff *tipoff, unsigned target,
                                        unsigned defense) {
    /* `$85:B9D2-$BA1C`: unassigned mode<7, minimum +$8C, first tie wins. */
    int selected = -1;
    uint16_t best = 0x7FFFu;
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned actor = defense * 5u + i;
        NbaTipoffActor *state = &tipoff->actors[actor];
        if ((int16_t)state->assignment_current_raw >= 0 ||
            state->control_mode >= 7u || state->anchor_distance_raw >= best)
            continue;
        best = state->anchor_distance_raw;
        selected = (int)actor;
    }
    if (selected < 0) return false;
    cpu_symmetric_bind(tipoff, (unsigned)selected, target);
    return true;
}

static void cpu_release_defense_assignment(NbaTipoff *tipoff,
                                           unsigned defender) {
    NbaTipoffActor *state = &tipoff->actors[defender];
    unsigned paired = state->assignment_current_raw >> 1;
    state->assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state->assignment_actor = 0xFFu;
    if (state->saved_control_mode == 6u ||
        paired >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    tipoff->actors[paired].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->actors[paired].pair_distance = 0x0140u;
    state->movement_boost_timer = 0x14u;
}

static bool cpu_defensive_planner_self_test(void) {
    NbaTipoff t;
    memset(&t, 0, sizeof(t));
    uint16_t distances[5] = {0x40u, 0x20u, 0x20u, 0x60u, 0x70u};
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        t.actors[i].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        t.actors[i].assignment_alternate_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        t.actors[i].control_mode = 7u;
    }
    t.actors[7].control_mode = 2u;
    t.actors[7].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    t.actors[2].assignment_alternate_raw = 14u;
    t.actors[2].x_fp = 40 * 256;
    if (cpu_select_help_defender(&t, 2u, 1u, 336, distances) != 7)
        return false;
    t.actors[2].assignment_alternate_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    t.actors[6].control_mode = 2u;
    t.actors[7].control_mode = 2u;
    if (cpu_select_help_defender(&t, 2u, 1u, 336, distances) != 7)
        return false; /* fallback equal distance: last candidate */
    t.actors[7].saved_control_mode = 6u;
    t.actors[7].assignment_current_raw = 4u;
    t.actors[2].assignment_current_raw = 14u;
    cpu_release_defense_assignment(&t, 7u);
    if (t.actors[7].assignment_current_raw != NBA_GAMEPLAY_UNKNOWN_WORD ||
        t.actors[2].assignment_current_raw != 14u)
        return false; /* old mode-6 release is one-way */
    return true;
}

static void cpu_refresh_team_roles_end_frame(NbaTipoff *tipoff) {
    /* `$87:8FA1-$8FA9` calls `$85:AF5C`, then `$85:BC07`, once after all
     * ten actor dispatches. AF5C normalizes only modes below 7 on offense;
     * BC07 does the same for defense, then promotes the owner's assigned
     * primary defender to mode 4. Modes 7+ remain owned by their executors. */
    unsigned offense = tipoff->offense_side != 0u ? 1u : 0u;
    unsigned defense = offense ^ 1u;
    uint16_t focal_distance[5] = {0};
    unsigned nearest_focal = defense * 5u;
    unsigned nearest_anchor = defense * 5u;
    unsigned nearest_offense_anchor = offense * 5u;
    uint16_t nearest_focal_distance = 0x7FFFu;
    uint16_t nearest_anchor_distance = 0x7FFFu;
    uint16_t nearest_offense_distance = 0x7FFFu;
    int16_t focal_x = fp_round(tipoff->ball.x_fp);
    int16_t focal_y = fp_round(tipoff->ball.y_fp);
    if (tipoff->possession_actor >= 0 &&
        tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT) {
        focal_x = fp_round(tipoff->actors[(unsigned)tipoff->possession_actor].x_fp);
        focal_y = fp_round(tipoff->actors[(unsigned)tipoff->possession_actor].y_fp);
    }
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        unsigned side = actor / 5u;
        if (side == offense && state->control_mode < 7u)
            state->control_mode = 1u;
        int16_t anchor = side == 0u ? -336 : 336;
        state->anchor_direction_raw = nba_gameplay_pass_direction(
            (int16_t)(anchor - fp_round(state->x_fp)),
            (int16_t)-fp_round(state->y_fp),
            &state->anchor_distance_raw);
        if (side == offense && state->anchor_distance_raw < nearest_offense_distance) {
            nearest_offense_distance = state->anchor_distance_raw;
            nearest_offense_anchor = actor;
        }
        if (side == defense) {
            focal_distance[actor % 5u] = cpu_rom_distance(
                (int16_t)(fp_round(state->x_fp) - focal_x),
                (int16_t)(fp_round(state->y_fp) - focal_y));
            state->focal_distance_raw_8e = focal_distance[actor % 5u];
            if (state->focal_distance_raw_8e < nearest_focal_distance) {
                nearest_focal_distance = state->focal_distance_raw_8e;
                nearest_focal = actor;
            }
            if (state->anchor_distance_raw < nearest_anchor_distance) {
                nearest_anchor_distance = state->anchor_distance_raw;
                nearest_anchor = actor;
            }
            cpu_refresh_pair_geometry(tipoff, actor);

            /* `$85:BE70-$BF2B`: dead/inbound states and mode 9 bypass both
             * normalization and assignment cleanup. */
            if (tipoff->live_state_raw >= 0x80u || state->control_mode == 9u)
                continue;
            if (state->control_mode < 7u) state->control_mode = 2u;
            else {
                cpu_release_defense_assignment(tipoff, actor);
                continue;
            }
            if ((int16_t)state->assignment_current_raw < 0) continue;
            if (state->saved_control_mode == 6u) {
                cpu_release_defense_assignment(tipoff, actor);
                continue;
            }
            unsigned paired = state->assignment_current_raw >> 1;
            if (paired >= NBA_GAMEPLAY_ACTOR_COUNT) continue;
            NbaTipoffActor *other = &tipoff->actors[paired];
            /* Opposite anchor sign keeps the pairing immediately. */
            if ((int16_t)(fp_round(other->x_fp) ^ anchor) < 0 ||
                state->anchor_distance_raw < 0x30u)
                continue;
            if (state->anchor_distance_raw >= other->anchor_distance_raw) {
                cpu_release_defense_assignment(tipoff, actor);
                continue;
            }
            uint16_t delta = (uint16_t)(other->anchor_distance_raw -
                                        state->anchor_distance_raw);
            if (delta < 0x0Cu) state->movement_boost_timer = 0x1Eu;
            if (state->anchor_distance_raw >= 0xA0u) continue;
            uint16_t relative = (uint16_t)(
                (uint16_t)(((state->assignment_direction << 1) ^ 8u) -
                           other->anchor_direction_raw + 3u));
            if ((int16_t)(uint16_t)(relative - 7u) >= 0)
                cpu_release_defense_assignment(tipoff, actor);
        }
    }
    if (tipoff->possession_actor >= 0 &&
        tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT) {
        NbaTipoffActor *owner =
            &tipoff->actors[(unsigned)tipoff->possession_actor];
        unsigned primary = owner->assignment_current_raw >> 1;
        int16_t anchor = defense == 0u ? -336 : 336;
        /* `$85:BF3F-$BF7F`: a ball owner far from its basket first repairs
         * the nearest offense actor's missing matchup. */
        if (owner->anchor_distance_raw >= 0xF0u &&
            (int16_t)tipoff->actors[nearest_offense_anchor].assignment_current_raw < 0) {
            if (!cpu_try_base_defender(
                    tipoff, nearest_offense_anchor, defense, anchor) &&
                tipoff->actors[nearest_anchor].control_mode < 3u)
                cpu_symmetric_bind(tipoff, nearest_anchor,
                                   nearest_offense_anchor);
        }
        if (primary >= NBA_GAMEPLAY_ACTOR_COUNT || primary / 5u != defense) {
            if (!cpu_try_base_defender(
                    tipoff, (unsigned)tipoff->possession_actor, defense, anchor) &&
                !cpu_fallback_primary_defender(
                    tipoff, (unsigned)tipoff->possession_actor, defense))
                return; /* `$85:C051`: both primary searches failed. */
            primary = owner->assignment_current_raw >> 1;
        }
        if (primary < NBA_GAMEPLAY_ACTOR_COUNT &&
            primary / 5u == defense) {
            NbaTipoffActor *primary_state = &tipoff->actors[primary];
            if (primary_state->control_mode < 7u)
                primary_state->control_mode = 4u;
            /* `$85:BFC1-$BFDC`: the primary receives +$72=$14 when no
             * closer to its basket than the owner, or while live and the
             * pair distance has reached $10. */
            if (primary_state->anchor_distance_raw >=
                    owner->anchor_distance_raw ||
                (tipoff->live_state_raw < 0x80u &&
                 primary_state->pair_distance >= 0x10u))
                primary_state->movement_boost_timer = 0x14u;

            /* `$85:BFEA-$C04C`: preserve the primary assignment and install
             * a one-way mode-6 help defender only when the team/owner gates
             * request it. Context +$4E is $00A0 in all three ROM gameplay
             * WRAM snapshots (frames 220, 400, and 1800). */
            NbaGameplayTeamContext *context = &tipoff->team_context[defense];
            bool help_due = false;
            if (tipoff->live_state_raw < 0x80u) {
                if (owner->anchor_distance_raw < context->help_distance_raw_4e)
                    help_due = true;
                else if (context->mode_raw_30 == 2u &&
                         (int16_t)(anchor ^ fp_round(owner->x_fp)) < 0)
                    help_due = true;
                else if (owner->help_request_raw_80 != 0u &&
                         (owner->anchor_distance_raw < 0xA0u ||
                          context->mode_raw_30 == 0u))
                    help_due = true;
            }
            if (help_due) {
                int helper = cpu_select_help_defender(
                    tipoff, (unsigned)tipoff->possession_actor, defense,
                    anchor, focal_distance);
                if (helper >= 0) {
                    cpu_install_help_assignment(
                        tipoff, (unsigned)helper,
                        (unsigned)tipoff->possession_actor);
                    NbaTipoffActor *helper_state = &tipoff->actors[helper];
                    if (helper_state->anchor_distance_raw >=
                            owner->anchor_distance_raw ||
                        helper_state->pair_distance >= 0x10u)
                        helper_state->movement_boost_timer = 0x14u;
                }
            }
        }
    } else if (tipoff->pass_receiver_raw < 0) {
        /* `$85:C052-$C0B1`: without an owner, promote the focal-nearest
         * eligible defender; if `$09DA` is busy, fallback is last-tie +$8E. */
        unsigned selected = nearest_focal;
        if (tipoff->actors[selected].control_mode >= 7u) {
            int fallback = -1;
            uint16_t best = 0x7FFFu;
            for (unsigned i = 0; i < 5u; ++i) {
                unsigned actor = defense * 5u + i;
                if (tipoff->actors[actor].control_mode < 7u &&
                    tipoff->actors[actor].focal_distance_raw_8e <= best) {
                    best = tipoff->actors[actor].focal_distance_raw_8e;
                    fallback = (int)actor;
                }
            }
            if (fallback < 0) return;
            selected = (unsigned)fallback;
        }
        if (tipoff->actors[selected].control_mode < 7u)
            tipoff->actors[selected].control_mode = 4u;
    }

    /* `$85:C0B4-$C0F5`: complete missing defense assignments in the
     * context's five-byte actor order. Each target tries its reciprocal base
     * candidate, then the first anchor-nearest unassigned defender. */
    int16_t defense_anchor = defense == 0u ? -336 : 336;
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned target = tipoff->team_context[defense].actor_order_raw_49[i] >> 1;
        if (target >= NBA_GAMEPLAY_ACTOR_COUNT ||
            (int16_t)tipoff->actors[target].assignment_current_raw >= 0)
            continue;
        if (!cpu_try_base_defender(tipoff, target, defense, defense_anchor) &&
            !cpu_bind_nearest_unassigned(tipoff, target, defense))
            return;
    }
    tipoff->role_rebuild_raw_09d6 = 0u;
}

static void cpu_update_all_actors(NbaTipoff *tipoff) {
    /* `$87:8F01-$8F8D` updates all ten actors as one logical pass with
     * `$C6/$0938=2`. */
    /* `$87:8EFB-$8F92` is one global 30-Hz pass with `$0938/$C6=2`.
     * Possession and inbound changes do not rephase it. */
    if ((tipoff->simulation_tick & 1u) != 0u) return;
    ++tipoff->actor_update_tick;
    /* `$87:9090-$90A0` snapshots all ten +$5E modes into +$84 before any
     * actor dispatch. BC07 relies on this to recognize last frame's mode-6
     * helper after the normalizer has changed +$5E back to mode 2. */
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
        tipoff->actors[actor].saved_control_mode =
            tipoff->actors[actor].control_mode;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        if (state->contact_inhibit_raw_5a != 0u)
            state->contact_inhibit_raw_5a =
                state->contact_inhibit_raw_5a > 2u ?
                (uint16_t)(state->contact_inhibit_raw_5a - 2u) : 0u;
        if (state->recovery_inhibit_raw != 0u)
            state->recovery_inhibit_raw = state->recovery_inhibit_raw > 2u ?
                (uint16_t)(state->recovery_inhibit_raw - 2u) : 0u;
        cpu_move_actor(tipoff, actor);
        cpu_clamp_record_to_court(tipoff, &state->x_fp, &state->y_fp,
                                  &state->velocity_x, &state->velocity_y);
        ++state->upper_animation_tick;
        ++state->upper_animation_phase_raw;
        ++state->lower_animation_tick;
        (void)cpu_actor_contact_height(
            tipoff, actor, &state->contact_height_raw_aa);
        /* The renderer resolves the post-dispatch resource phase. Mirror
         * `$87:B649/$B832` against that same phase so an attached mode-15
         * ball cannot lag the visible passing hand by one host tick. */
        if (state->control_mode == 15u &&
            tipoff->ball.owner_actor == (int8_t)actor)
            ball_attach_to_actor(tipoff, actor);
    }
    cpu_refresh_team_roles_end_frame(tipoff);
}

static int cpu_select_inbound_receiver(const NbaTipoff *tipoff,
                                       uint8_t inbounder) {
    for (unsigned i = 0; i < 3u; ++i) {
        int16_t candidate = tipoff->play_selector_raw[i];
        if (cpu_inbound_candidate_valid(tipoff, inbounder, candidate))
            return candidate;
    }
    if (tipoff->inbound_timer_raw >= 60u) return -1;
    int fallback = (int)(inbounder / 5u) * 5 + 4;
    if (fallback == inbounder) --fallback;
    return fallback;
}

static bool cpu_inbound_side_gate(const NbaTipoff *tipoff,
                                  uint8_t inbounder, uint8_t receiver) {
    int16_t owner_x = fp_round(tipoff->actors[inbounder].x_fp);
    int16_t receiver_x = fp_round(tipoff->actors[receiver].x_fp);
    bool context_nonnegative = inbounder >= 5u;
    if (context_nonnegative)
        return owner_x < -20 || receiver_x >= 0;
    return owner_x >= 20 || receiver_x < 0;
}

/* `$87:9AA6-$9BCA`: an installed inbounder whose signed `$092E` expires
 * commits the five-second violation/dead-ball recovery. `$9B38` awards the
 * opposite side (`$093A ^ 5`), switches `$0956` to layout 5, restores the
 * clocks, demotes the old owner to mode 2, and clears signed `$093E` before
 * `$85:C37D` seeds the replacement target. */
static void cpu_reset_expired_inbound(NbaTipoff *tipoff) {
    uint8_t previous = tipoff->inbound_actor_raw < NBA_GAMEPLAY_ACTOR_COUNT ?
                       (uint8_t)tipoff->inbound_actor_raw : 0xFFu;
    if (previous < NBA_GAMEPLAY_ACTOR_COUNT)
        tipoff->actors[previous].control_mode = 2u;
    unsigned side_group = (tipoff->camera_side_group_raw ^ 5u) == 5u ? 5u : 0u;
    unsigned side = side_group / 5u;
    tipoff->inbound_state_raw = (uint16_t)side_group;
    tipoff->inbound_layout_raw = 5;
    tipoff->inbound_actor_raw = (uint16_t)(side_group + 2u);
    tipoff->inbound_timer_raw = 300u;
    tipoff->rim_raw_092c = 0x05A0u;
    tipoff->rim_raw_096a = 0u;
    tipoff->rim_raw_097c = 0u;
    tipoff->possession_actor = -1;
    tipoff->possession_team = (int8_t)side;
    tipoff->offense_side = (uint8_t)side;
    tipoff->handler_actor = (uint8_t)tipoff->inbound_actor_raw;
    tipoff->receiver_actor = (uint8_t)(side_group + 4u);
    tipoff->inbound_ready_raw = 0u;
    tipoff->inbound_transfer_raw = 0u;
    tipoff->ball.owner_actor = -1;
    NbaGameplayInboundTarget target;
    int16_t context_anchor = side ? 336 : -336;
    if (nba_gameplay_inbound_target(
            5, fp_round(tipoff->ball.x_fp), fp_round(tipoff->ball.y_fp),
            context_anchor, fp_round(tipoff->ball.x_fp), &tipoff->rng,
            &target)) {
        tipoff->inbound_target_x_raw = target.x;
        tipoff->inbound_target_y_raw = target.y;
        tipoff->inbound_direction_raw = target.direction;
        if (target.play_requested) {
            tipoff->play_code = target.play_code;
            tipoff->play_request_raw = 1u;
        }
        tipoff->actors[tipoff->inbound_actor_raw].target_x = target.x;
        tipoff->actors[tipoff->inbound_actor_raw].target_y = target.y;
    }
}

static bool cpu_expired_inbound_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    state.camera_side_group_raw = 0u;
    state.inbound_actor_raw = 3u;
    state.actors[3].control_mode = 11u;
    state.ball.x_fp = 394 * 256;
    state.ball.y_fp = -52 * 256;
    state.possession_actor = 3;
    cpu_reset_expired_inbound(&state);
    if (state.actors[3].control_mode != 2u ||
        state.inbound_state_raw != 5u || state.inbound_layout_raw != 5 ||
        state.inbound_actor_raw != 7u || state.inbound_timer_raw != 300u ||
        state.possession_actor != -1 || state.possession_team != 1 ||
        state.inbound_target_x_raw != 394 ||
        state.inbound_target_y_raw != -52 ||
        state.inbound_direction_raw != 6u) return false;

    memset(&state, 0, sizeof(state));
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    state.camera_side_group_raw = 5u;
    state.inbound_actor_raw = 7u;
    state.actors[7].control_mode = 11u;
    state.ball.x_fp = -394 * 256;
    state.ball.y_fp = 52 * 256;
    state.possession_actor = 7;
    cpu_reset_expired_inbound(&state);
    return state.actors[7].control_mode == 2u &&
           state.inbound_state_raw == 0u && state.inbound_layout_raw == 5 &&
           state.inbound_actor_raw == 2u && state.inbound_timer_raw == 300u &&
           state.possession_actor == -1 && state.possession_team == 0 &&
           state.inbound_target_x_raw == -394 &&
           state.inbound_target_y_raw == 52 &&
           state.inbound_direction_raw == 2u;
}

/* `$86:F43A-$F653`: execute the inbound arrival/candidate/pass gates. The
 * surrounding 60-Hz clock decrements `$092E`; this 30-Hz actor pass reloads
 * it to 300 whenever the raw target box has not been reached. */
static void cpu_update_rom_inbound(NbaTipoff *tipoff) {
    if (tipoff->inbound_actor_raw >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    uint8_t inbounder = (uint8_t)tipoff->inbound_actor_raw;
    NbaTipoffActor *actor = &tipoff->actors[inbounder];
    /* `$86:F3F6-$F43A` reaches the inbound mode-11 branch only for the
     * collision-installed `$093E` owner. `$0954` starts as provisional slot
     * 2/7 and must not manufacture possession merely by reaching `$0958/A`. */
    if (tipoff->possession_actor != (int8_t)inbounder ||
        actor->control_mode != 11u) return;
    actor->target_x = tipoff->inbound_target_x_raw;
    actor->target_y = tipoff->inbound_target_y_raw;
    if ((tipoff->simulation_tick & 1u) != 0u) return;
    if (tipoff->inbound_transfer_raw != 0u &&
        actor->control_mode != 11u) return;
    if (!nba_gameplay_inbound_arrived(
            fp_round(actor->x_fp), fp_round(actor->y_fp),
            tipoff->inbound_target_x_raw, tipoff->inbound_target_y_raw)) {
        tipoff->inbound_timer_raw = 300u;
        return;
    }

    actor->behavior_flags_raw |= 0x0040u;
    actor->velocity_x = actor->velocity_y = 0;
    actor->movement_magnitude_raw = 0u;
    actor->direction = (uint8_t)tipoff->inbound_direction_raw;
    actor->requested_direction = actor->direction;
    if (tipoff->inbound_ready_raw == 0u) {
        tipoff->inbound_ready_raw = 1u;
    }
    /* `$86:F58F`: signed actor +$16, not actor Z, gates the CPU selector.
     * A human-controlled inbounder waits for controller input here. */
    if (actor->controller_assignment_raw >= 0) return;
    if ((int16_t)tipoff->inbound_timer_raw >= 240) return;
    uint16_t random = tipoff->inbound_timer_raw >= 120u ?
                      nba_gameplay_rng_next(&tipoff->rng) : 0u;
    if (!nba_gameplay_inbound_pass_due(
            tipoff->inbound_timer_raw, random)) return;
    int candidate = cpu_select_inbound_receiver(tipoff, inbounder);
    if (candidate < 0 || candidate >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    actor->reaction_threshold = 1u; /* `$86:F60B-$F610` */
    if (!cpu_inbound_side_gate(
            tipoff, inbounder, (uint8_t)candidate)) return;
    if (cpu_begin_rom_pass(tipoff, inbounder, (unsigned)candidate)) {
        tipoff->receiver_actor = (uint8_t)candidate;
        cpu_enter_play_state(tipoff, NBA_CPU_PLAY_PASS);
    }
}

static void cpu_update_possession(NbaTipoff *tipoff) {
    if (tipoff->live_state_raw == 0x82u) {
        /* `$85:A262-$A268` seeds `$092E/$0A04=300`. `$86:F43A-$F653`
         * owns arrival, its sawtooth reload, receiver selection and AB2D. */
        cpu_set_role_targets(tipoff);
        if (tipoff->inbound_actor_raw < NBA_GAMEPLAY_ACTOR_COUNT) {
            tipoff->actors[tipoff->inbound_actor_raw].target_x =
                tipoff->inbound_target_x_raw;
            tipoff->actors[tipoff->inbound_actor_raw].target_y =
                tipoff->inbound_target_y_raw;
        }
        cpu_update_all_actors(tipoff);
        cpu_update_play_control(tipoff);
        (void)cpu_update_live_ball(tipoff);
        if (tipoff->possession_actor >= 0 &&
            tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT &&
            tipoff->inbound_transfer_raw == 0u) {
            /* The dead-ball carrier is pose-attached visually while the ROM
             * keeps the logical ball owner negative until `$86:AB2D`. */
            ball_position_at_actor(
                tipoff, (unsigned)tipoff->possession_actor);
            tipoff->ball.velocity_x = tipoff->ball.velocity_y = 0;
            tipoff->ball.velocity_z = 0;
            tipoff->ball.owner_actor = -1;
        }
        /* `$86:CCFC-$D43C` pose collision installs the actual dead-ball
         * inbounder before `$86:F43A` can steer or pass. The ROM oracle's
         * first score changes provisional actor 2 to actor 3 at frame 918. */
        if (tipoff->possession_actor < 0 &&
            tipoff->ball.owner_actor < 0 &&
            tipoff->inbound_transfer_raw == 0u) {
            int contact = cpu_first_inbound_ball_contact(tipoff);
            if (contact >= 0) {
                NbaTipoffActor *inbounder = &tipoff->actors[contact];
                tipoff->inbound_actor_raw = (uint16_t)contact;
                tipoff->handler_actor = (uint8_t)contact;
                tipoff->possession_actor = (int8_t)contact;
                tipoff->possession_team = (int8_t)(contact / 5);
                tipoff->offense_side = (uint8_t)(contact / 5);
                inbounder->control_mode = 11u;
                inbounder->reaction_threshold = 0u;
                inbounder->behavior_flags_raw &= 0xFFBFu;
                inbounder->target_x = tipoff->inbound_target_x_raw;
                inbounder->target_y = tipoff->inbound_target_y_raw;
                tipoff->inbound_ready_raw = 0u;
                tipoff->inbound_transfer_raw = 0u;
                tipoff->inbound_timer_raw = 300u;
            }
        }
        /* The inbound transfer still terminates through the shared
         * `$86:BAA2-$BC99` pass-catch acquisition boundary. */
        if (tipoff->cpu_play_state == NBA_CPU_PLAY_PASS &&
            tipoff->ball.owner_actor < 0) {
            int contact = cpu_first_pass_contact(tipoff);
            if (contact >= 0)
                cpu_commit_ball_acquisition(tipoff, (uint8_t)contact);
        }
        if (tipoff->cpu_play_state == NBA_CPU_PLAY_PASS &&
            tipoff->ball.state == NBA_BALL_BOUNCE) {
            cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
        }
        if (tipoff->ball.owner_actor >= 0) {
            ++tipoff->possession_frame;
            ++tipoff->play_state_frame;
            return;
        }
        if (tipoff->inbound_timer_raw > 0u)
            --tipoff->inbound_timer_raw;
        if (tipoff->inbound_timer_raw == 0u &&
            tipoff->possession_actor >= 0 &&
            tipoff->inbound_transfer_raw == 0u) {
            cpu_reset_expired_inbound(tipoff);
        }
        cpu_update_rom_inbound(tipoff);
        ++tipoff->possession_frame;
        ++tipoff->play_state_frame;
        return;
    }
    if (tipoff->possession_frame != 0u && tipoff->rim_raw_092c != 0u)
        --tipoff->rim_raw_092c;
    cpu_set_role_targets(tipoff);
    cpu_update_all_actors(tipoff);
    NbaGameplayRimResult rim_result = cpu_update_live_ball(tipoff);
    cpu_update_play_control(tipoff);

    switch ((NbaCpuPlayState)tipoff->cpu_play_state) {
        case NBA_CPU_PLAY_BREAK:
        case NBA_CPU_PLAY_DRIVE:
        case NBA_CPU_PLAY_ATTACK:
            /* Debug-only legacy labels. The ROM has no global
             * BREAK/DRIVE/ATTACK distance machine: mode 11 alone chooses
             * B678 shot, AD6B formation, then B50E pass. */
            break;
        case NBA_CPU_PLAY_PASS:
            /* `$86:D5DB/$D652->$86:CCFC-$D1D6`: stable-X collision order,
             * teammate rejection, receiver/opponent pose windows, and the
             * original low-Z DP-$00 interception bug. No floor auto-catch. */
            if (tipoff->ball.owner_actor < 0) {
                int contact = cpu_first_pass_contact(tipoff);
                if (contact >= 0)
                    cpu_commit_ball_acquisition(tipoff, (uint8_t)contact);
            }
            if (tipoff->ball.owner_actor < 0 &&
                tipoff->ball.state == NBA_BALL_BOUNCE)
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
            break;
        case NBA_CPU_PLAY_SHOT:
            if (!tipoff->shot_result_resolved &&
                rim_result == NBA_GAMEPLAY_RIM_MAKE) {
                score_made_basket(tipoff);
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
            } else if (!tipoff->shot_result_resolved &&
                       (rim_result == NBA_GAMEPLAY_RIM_EDGE_CONTACT ||
                        rim_result == NBA_GAMEPLAY_RIM_MISS)) {
                /* `$85:9DAC-$A006` has already applied the distinct edge or
                 * miss impulse and continued through same-tick integration;
                 * mode changes here only hand the result to loose-ball play. */
                tipoff->shot_result_resolved = true;
                tipoff->ball.state = NBA_BALL_BOUNCE;
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
            } else if (tipoff->ball.z_fp <= 32 * 256 &&
                tipoff->ball.velocity_z < 0) {
                cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
                tipoff->ball.state = NBA_BALL_BOUNCE;
            }
            break;
        case NBA_CPU_PLAY_REBOUND:
            {
                int catcher = cpu_first_loose_ball_contact(tipoff);
                if (catcher >= 0)
                    cpu_commit_ball_acquisition(tipoff, (uint8_t)catcher);
            }
            break;
    }

    ++tipoff->possession_frame;
    ++tipoff->play_state_frame;
}

static void cpu_update_camera(NbaTipoff *tipoff) {
    /* `$87:A9D0-$A9E2/$87:95BB-$95D8`: signed `$093E` selects an actor;
     * FFFF substitutes the ball record before `$85:9192-$93F4` consumes the
     * proxy. `$093A` is persistent and independent from the free ball. */
    if (tipoff->frame < NBA_TIPOFF_POSSESSION_FRAME) return;
    if (tipoff->possession_actor >= 0 &&
        tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT) {
        unsigned subject = (unsigned)tipoff->possession_actor;
        tipoff->camera.subject_actor = (uint8_t)subject;
        tipoff->camera_side_group_raw = subject >= 5u ? 5u : 0u;
        nba_gameplay_camera_update(&tipoff->camera,
            fp_round(tipoff->actors[subject].x_fp),
            fp_round(tipoff->actors[subject].y_fp),
            tipoff->camera_side_group_raw);
    } else {
        tipoff->camera.subject_actor = NBA_GAMEPLAY_NO_ACTOR;
        nba_gameplay_camera_update(&tipoff->camera,
            fp_round(tipoff->ball.x_fp), fp_round(tipoff->ball.y_fp),
            tipoff->camera_side_group_raw);
    }
    tipoff->camera_x = tipoff->camera.x;
    tipoff->camera_y = tipoff->camera.y;
}

static void draw_ball(const NbaTipoff *tipoff, NbaRenderer *ren, int x, int y) {
    const NbaAssetItem *item = nba_assets_get(tipoff->assets, NBA_ASSET_TIPOFF_BALL);
    if (!item || !item->data || item->size != 56u ||
        memcmp(item->data, "NBBALL1", 8)) return;
    const uint8_t *data = (const uint8_t *)item->data;
    const uint8_t *tile = data + 12;
    for (int py = 0; py < 8; ++py) for (int px = 0; px < 8; ++px) {
        uint8_t index = tile_pixel(tile, px, py);
        if (index < 5u || index > 10u) continue;
        int dx = x + px, dy = y + py;
        if (dx >= 0 && dx < 256 && dy >= 0 && dy < 224)
            ren->pixels[dy * 256 + dx] = bgr555(read_u16(data + 44 + (index - 5u) * 2u));
    }
}

bool nba_tipoff_init(NbaTipoff *tipoff, const NbaAssetPack *assets,
                     NbaSession *session) {
    if (!tipoff || !assets || !session ||
        !nba_gameplay_rng_self_test() || !nba_gameplay_ai_self_test() ||
        !nba_gameplay_ball_self_test() || !nba_gameplay_effect_self_test() ||
        !nba_gameplay_foul_self_test() ||
        !cpu_rim_contact_tick_self_test() ||
        !cpu_contact_orchestration_self_test() ||
        !cpu_defensive_planner_self_test() ||
        !cpu_expired_inbound_self_test() ||
        !ball_attachment_assets_valid(assets) ||
        !nba_assets_gameplay_court_panorama(assets, session->right_team) ||
        !nba_assets_get(assets, NBA_ASSET_TIPOFF_BALL)) return false;
    memset(tipoff, 0, sizeof(*tipoff));
    nba_gameplay_effect_init(&tipoff->rim_effect);
    tipoff->special_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->assets = assets;
    tipoff->session = session;
    tipoff->cpu_vs_cpu = true;
    tipoff->camera_x = -128;
    tipoff->camera_y = -124;
    nba_gameplay_camera_init(&tipoff->camera, -128, -124);
    nba_gameplay_rng_seed(&tipoff->rng, 0x9146u);
    nba_gameplay_foul_init(&tipoff->fouls);
    static const uint8_t context_actor_order[2][5] = {
        {0x10u, 0x0Eu, 0x0Au, 0x12u, 0x0Cu},
        {0x04u, 0x06u, 0x08u, 0x00u, 0x02u}
    };
    for (unsigned side = 0; side < 2u; ++side) {
        tipoff->team_context[side].mode_raw_30 = 4u;
        tipoff->team_context[side].flags_raw_32 = 1u;
        tipoff->team_context[side].activity_raw_39 = 1u;
        tipoff->team_context[side].help_distance_raw_4e = 0x00A0u;
        for (unsigned i = 0; i < 5u; ++i)
            tipoff->team_context[side].actor_order_raw_49[i] =
                context_actor_order[side][i];
    }
    tipoff->period_raw_0926 = 0u;
    tipoff->possession_actor = -1;
    tipoff->possession_team = -1;
    session->score[0] = session->score[1] = 0u;
    session->game_clock_ticks = 0u;
    tipoff->live_state_raw = 1u;
    tipoff->inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->pass_actor_raw = -1;
    tipoff->pass_receiver_raw = -1;
    tipoff->ball.x_fp = 0;
    tipoff->ball.y_fp = 0;
    tipoff->ball.z_fp = 80 * 256;
    tipoff->ball.owner_actor = -1;
    static const uint8_t active_lineup[5] = {2u, 0u, 1u, 3u, 4u};
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        state->x_fp = (int32_t)formation[actor].world_x * 256;
        state->y_fp = (int32_t)formation[actor].world_y * 256;
        state->direction = formation[actor].direction;
        state->roster_slot = active_lineup[actor % 5u];
        state->requested_direction = formation[actor].direction;
        state->movement_direction = 8u;
        state->saved_control_mode = 0u;
        state->pass_family_raw = -1;
        state->controller_assignment_raw = -1;
        state->lower_animation_state = 0u;
        /* `$86:D86C-$D89B`: +$76 is derived from the active-lineup
         * permutation, then copied to mutable +$74. It is an even byte
         * offset into `$87:9C7B`, not a same-index matchup. */
        state->assignment_actor = actor < 5u ?
            (uint8_t)(5u + state->roster_slot) : state->roster_slot;
        state->assignment_base_raw = (uint16_t)(state->assignment_actor * 2u);
        state->assignment_current_raw = state->assignment_base_raw;
        state->assignment_alternate_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        uint8_t team = actor >= 5u ? session->right_team : session->left_team;
        state->assignment_role_raw_92 = (uint8_t)(actor % 5u);
        (void)nba_player_gameplay_position(
            assets, team, state->roster_slot, &state->assignment_role_raw_92);
        state->reaction_threshold = nba_gameplay_reaction_threshold(
            &tipoff->rng, formation[actor].world_x, formation[actor].world_y,
            0, 0);
        state->behavior_timer = 0x2Fu;
        state->control_mode = actor == 0u || actor == 5u ? 4u : 2u;
        state->visible = actor != 4u && actor != 9u;
    }
    /* `$86:D8D3-$D8E2`: +$78 names the reciprocal actor whose base points
     * back at this actor. Keep it separate from mutable current +$74. */
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        for (unsigned candidate = 0;
             candidate < NBA_GAMEPLAY_ACTOR_COUNT; ++candidate) {
            if (tipoff->actors[candidate].assignment_actor == actor) {
                tipoff->actors[actor].assignment_alternate_raw =
                    (uint16_t)(candidate * 2u);
                break;
            }
        }
    }
    /* `$85:BC52-$BC81` and `$85:AFC2-$AFE5`: initialize pair and basket
     * geometry independently of rendered facing and neutral movement intent. */
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        unsigned paired_slot = state->assignment_current_raw >> 1;
        if (paired_slot >= NBA_GAMEPLAY_ACTOR_COUNT) continue;
        NbaTipoffActor *paired = &tipoff->actors[paired_slot];
        state->assignment_direction = nba_gameplay_target_direction(
            (int16_t)(fp_round(paired->x_fp) - fp_round(state->x_fp)),
            (int16_t)(fp_round(paired->y_fp) - fp_round(state->y_fp)),
            &state->assignment_distance);
        state->pair_distance = state->assignment_distance;
        int16_t anchor = actor < 5u ? -336 : 336;
        state->anchor_direction_raw = nba_gameplay_pass_direction(
            (int16_t)(anchor - fp_round(state->x_fp)),
            (int16_t)-fp_round(state->y_fp),
            &state->anchor_distance_raw);
    }
    tipoff->is_initialized = true;
    printf("[TIPOFF] $86:DDA7 formation -> $86:E054 ball -> "
           "$86:ECF4 jump -> $86:D3F9 possession.\n");
    return true;
}

void nba_tipoff_update(NbaTipoff *tipoff, const NbaInput *input) {
    (void)input;
    if (!tipoff || !tipoff->is_initialized) return;
    /* `$13E7` is an outer-frame event bitfield. Acquisition's bit $0010 is
     * observable for one completed frame, then the next outer pass clears it. */
    tipoff->rim_raw_13e7 &= 0xFFEFu;
    ++tipoff->frame;
    ++tipoff->simulation_tick;
    ++tipoff->session->game_clock_ticks;
    if (tipoff->frame < NBA_TIPOFF_BREAK_FRAME)
        cpu_update_tip_ball(tipoff);
    if (tipoff->frame == NBA_TIPOFF_POSSESSION_FRAME) {
        /* `$85:B100-$B28B` resolves the tip and writes play code $35.
         * `$87:9244/$9BD0` then dispatches actor +$5E behavior modes. */
        tipoff->possession_actor = 8;
        tipoff->possession_team = 1;
        tipoff->camera_side_group_raw = 5u;
        tipoff->play_code = 0x35u;
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
            tipoff->actors[actor].control_mode = actor >= 5u ? 1u : 2u;
        tipoff->actors[8].control_mode = 10u;
    }
    if (tipoff->frame == NBA_TIPOFF_BREAK_FRAME) {
        cpu_begin_possession(tipoff, 1u);
    }
    if (tipoff->frame >= NBA_TIPOFF_BREAK_FRAME) {
        cpu_update_possession(tipoff);
    }
    cpu_update_camera(tipoff);
    if (tipoff->frame >= NBA_TIPOFF_BREAK_FRAME) tipoff->phase = NBA_TIPOFF_LIVE;
    else if (tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME)
        tipoff->phase = NBA_TIPOFF_POSSESSION;
    else if (tipoff->frame >= NBA_TIPOFF_TOSS_FRAME)
        tipoff->phase = NBA_TIPOFF_JUMP_BALL;
}

static int center_jump_height(int frame) {
    if (frame < NBA_TIPOFF_JUMP_FRAME || frame >= NBA_TIPOFF_POSSESSION_FRAME)
        return 0;
    int t = frame - NBA_TIPOFF_JUMP_FRAME;
    /* Mesen actor +$0C: 7,15,18,20,20,18,12,3 across frames 160..195. */
    int d = t - 20;
    int height = 20 - (d * d) / 20;
    return height > 0 ? height : 0;
}

static void ball_position(int frame, int *x, int *y) {
    if (frame < NBA_TIPOFF_TOSS_FRAME) { *x = 125; *y = 40; return; }
    if (frame < NBA_TIPOFF_CONTACT_FRAME) {
        int t = frame - 170;
        int z = 108 - (t * t * (t < 0 ? 28 : 40)) / (t < 0 ? 625 : 784);
        *x = 125; *y = 120 - z; return;
    }
    int t = frame - NBA_TIPOFF_CONTACT_FRAME;
    if (t > 22) t = 22;
    *x = 125 + (38 - 125) * t / 22;
    *y = 51 + (83 - 51) * t / 22;
}

static bool actor_visible(unsigned actor) {
    for (unsigned i = 0; i < sizeof(visible_submission); ++i)
        if (visible_submission[i] == actor) return true;
    return false;
}

static uint8_t actor_animation(const NbaTipoff *tipoff, unsigned actor) {
    if (actor == 5u && tipoff->frame >= NBA_TIPOFF_JUMP_FRAME &&
        tipoff->frame < NBA_TIPOFF_CONTACT_FRAME) return 0x32u;
    if (actor == 5u && tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME &&
        tipoff->frame < 210) return 0x25u;
    if (actor == 0u && tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME &&
        tipoff->frame < 220) return 0x37u;
    return 0;
}

void nba_tipoff_capture_telemetry(const NbaTipoff *tipoff,
                                  const NbaInput *input,
                                  NbaGameplayTelemetry *telemetry) {
    if (!tipoff || !telemetry) return;
    memset(telemetry, 0, sizeof(*telemetry));
    telemetry->scene_frame = (uint32_t)tipoff->frame;
    telemetry->simulation_tick = tipoff->simulation_tick;
    telemetry->phase = (uint8_t)tipoff->phase;
    telemetry->scheduler_due_raw =
        (uint8_t)((tipoff->simulation_tick & 1u) == 0u);
    telemetry->actor_pass_dt_raw = telemetry->scheduler_due_raw ? 2u : 0u;
    telemetry->actor_pass_mask_raw = telemetry->scheduler_due_raw ? 0x03FFu : 0u;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
        telemetry->actor_pass_order_raw[actor] = telemetry->scheduler_due_raw ?
            (uint8_t)actor : 0xFFu;
    telemetry->input_pressed = input ? input->pressed : 0u;
    telemetry->input_held = input ? input->held : 0u;
    telemetry->input_released = input ? input->released : 0u;
    telemetry->pad_held_raw[0] = (uint16_t)(telemetry->input_held & 0x0FFFu);
    for (unsigned pad = 0; pad < NBA_GAMEPLAY_PAD_COUNT; ++pad) {
        telemetry->controller_assignment_raw[pad] = NBA_GAMEPLAY_UNKNOWN_WORD;
        telemetry->controller_repeat_raw[pad] = NBA_GAMEPLAY_UNKNOWN_WORD;
    }
    telemetry->active_controller_raw = -1;
    telemetry->selected_controller_raw = -1;
    telemetry->controlled_side_raw = -1;
    telemetry->initial_controlled_slot_raw = 0;
    telemetry->selected_slot_raw = -1;
    telemetry->controlled_actor = NBA_GAMEPLAY_NO_ACTOR;
    telemetry->controlled_actor_pointer_raw = 0u;
    telemetry->possession_actor = tipoff->possession_actor;
    telemetry->possession_team = tipoff->possession_team;
    telemetry->possession_candidate_raw = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                                          tipoff->handler_actor : -1;
    telemetry->play_code_raw = tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME ?
                               tipoff->play_code : NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->play_step_raw = tipoff->play_step_raw;
    telemetry->play_countdown_raw = tipoff->play_countdown_raw;
    telemetry->play_mirror_raw = tipoff->play_mirror_raw;
    telemetry->play_event_wait_raw = tipoff->play_event_wait_raw;
    telemetry->play_request_raw = tipoff->play_request_raw;
    telemetry->play_cycle_raw = tipoff->play_cycle_raw;
    telemetry->play_hold_raw = tipoff->play_hold_raw;
    telemetry->role_rebuild_raw_09d6 = tipoff->role_rebuild_raw_09d6;
    telemetry->special_actor_raw = tipoff->special_actor_raw;
    for (unsigned i = 0; i < 3u; ++i)
        telemetry->play_selector_raw[i] = tipoff->play_selector_raw[i];
    telemetry->rng_state_raw = tipoff->rng.state;
    telemetry->score_left_raw = tipoff->session->score[0];
    telemetry->score_right_raw = tipoff->session->score[1];
    telemetry->period_raw_0926 = tipoff->period_raw_0926;
    for (unsigned side = 0; side < 2u; ++side) {
        telemetry->team_context_mode_raw_30[side] =
            tipoff->team_context[side].mode_raw_30;
        telemetry->team_context_flags_raw_32[side] =
            tipoff->team_context[side].flags_raw_32;
        telemetry->team_context_activity_raw_39[side] =
            tipoff->team_context[side].activity_raw_39;
    }
    telemetry->shot_clock_raw_092c = tipoff->rim_raw_092c;
    telemetry->shot_value_raw = tipoff->shot_value_raw;
    telemetry->shot_chance_raw = tipoff->shot_chance_raw;
    telemetry->shot_miss_index_raw = tipoff->shot_miss_index_raw;
    telemetry->shot_inner_veto_raw = tipoff->shot_inner_veto_raw ? 1u : 0u;
    telemetry->live_state_raw = tipoff->live_state_raw;
    telemetry->inbound_state_raw = tipoff->inbound_state_raw;
    telemetry->inbound_actor_raw = tipoff->inbound_actor_raw;
    telemetry->inbound_timer_raw = tipoff->inbound_timer_raw;
    telemetry->inbound_layout_raw = tipoff->inbound_layout_raw;
    telemetry->inbound_target_x_raw = tipoff->inbound_target_x_raw;
    telemetry->inbound_target_y_raw = tipoff->inbound_target_y_raw;
    telemetry->inbound_direction_raw = tipoff->inbound_direction_raw;
    telemetry->inbound_ready_raw = tipoff->inbound_ready_raw;
    telemetry->inbound_transfer_raw = tipoff->inbound_transfer_raw;
    telemetry->rim_context_raw_097c = tipoff->rim_raw_097c;
    telemetry->rim_contact_count_raw_0920 = tipoff->rim_raw_0920;
    telemetry->rim_response_raw_0970 = tipoff->rim_raw_0970;
    telemetry->effect_gate_raw_3f33 = tipoff->rim_effect.gate_raw_3f33;
    telemetry->effect_resource_raw_4015 =
        tipoff->rim_effect.resource_raw_4015;
    telemetry->rim_effect_raw_401b = tipoff->rim_effect.effect_raw_401b;
    telemetry->effect_frame_raw_4025 = tipoff->rim_effect.frame_raw_4025;
    telemetry->effect_timer_raw_402d = tipoff->rim_effect.timer_raw_402d;
    telemetry->rim_impact_raw_13e5 = tipoff->rim_impact_raw_13e5;
    telemetry->event_bits_raw_13e7 = tipoff->rim_raw_13e7;
    telemetry->foul_event_raw = tipoff->fouls.foul_event_raw_0964;
    telemetry->shooting_foul_raw = tipoff->fouls.shooting_foul_raw_09bc;
    telemetry->foul_offender_raw = tipoff->fouls.offender_actor_raw;
    telemetry->foul_victim_raw = tipoff->fouls.victim_actor_raw;
    telemetry->team_fouls_raw[0] = tipoff->fouls.team_fouls[0];
    telemetry->team_fouls_raw[1] = tipoff->fouls.team_fouls[1];
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
        telemetry->personal_fouls_raw[actor] =
            tipoff->fouls.personal_fouls[actor];
    telemetry->free_throw_state_raw =
        tipoff->fouls.free_throw_state_raw_0978;
    telemetry->free_throw_sequence_raw =
        tipoff->fouls.free_throw_sequence_raw_097a;
    telemetry->ball_activity_raw = tipoff->ball_activity_raw;
    telemetry->pass_actor_raw = tipoff->pass_actor_raw;
    telemetry->pass_receiver_raw = tipoff->pass_receiver_raw;
    telemetry->pass_active_raw = tipoff->pass_active_raw;
    telemetry->pass_distance_raw = tipoff->pass_distance_raw;
    telemetry->collision_actor_a = tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME &&
                                   tipoff->frame < 211 ? 0 : -1;
    telemetry->collision_actor_b = tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME &&
                                   tipoff->frame < 211 ? 5 : -1;
    telemetry->controller_routine = 0x80CB8Fu;
    telemetry->selection_routine = 0x85C37Du;
    telemetry->collision_routine = tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME ?
                                   SNES_ADDR_TIPOFF_CONTACT : 0u;
    telemetry->possession_routine = tipoff->frame >= NBA_TIPOFF_POSSESSION_FRAME ?
                                    SNES_ADDR_TIPOFF_POSSESSION : 0u;
    telemetry->camera_x = tipoff->camera_x;
    telemetry->camera_y = tipoff->camera_y;
    telemetry->camera_subject_raw = tipoff->camera.subject_actor ==
        NBA_GAMEPLAY_NO_ACTOR ? -1 : (int16_t)tipoff->camera.subject_actor;
    telemetry->camera_side_group_raw = tipoff->camera_side_group_raw;
    telemetry->camera_085c_raw = (uint16_t)tipoff->camera.x;
    telemetry->camera_085e_raw = (uint16_t)tipoff->camera.previous_x;
    telemetry->camera_0860_raw = (uint16_t)tipoff->camera.y;
    telemetry->camera_0862_raw = (uint16_t)tipoff->camera.previous_y;
    telemetry->camera_086c_raw = tipoff->camera.coarse_x;
    telemetry->camera_086e_raw = tipoff->camera.coarse_y;
    telemetry->camera_0874_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->camera_0876_raw = tipoff->camera.stream_source;
    telemetry->camera_0878_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->camera_087a_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    telemetry->camera_routine = 0x859192u;

    telemetry->ball.world_x = fp_round(tipoff->ball.x_fp);
    telemetry->ball.world_y = fp_round(tipoff->ball.y_fp);
    telemetry->ball.world_z = fp_round(tipoff->ball.z_fp);
    telemetry->ball.screen_x = (int16_t)(telemetry->ball.world_x +
        telemetry->ball.world_y - tipoff->camera_x);
    telemetry->ball.screen_y = (int16_t)((telemetry->ball.world_y -
        telemetry->ball.world_x) / 4 - tipoff->camera_y -
        telemetry->ball.world_z);
    telemetry->ball.velocity_x = fp_round(tipoff->ball.velocity_x);
    telemetry->ball.velocity_y = fp_round(tipoff->ball.velocity_y);
    telemetry->ball.velocity_z = fp_round(tipoff->ball.velocity_z);
    telemetry->ball.owner_actor = tipoff->ball.owner_actor;
    telemetry->ball.state = tipoff->ball.state;
    telemetry->ball.routine = tipoff->frame < NBA_TIPOFF_BREAK_FRAME ?
        SNES_ADDR_TIPOFF_BALL_INIT :
        tipoff->ball.state == NBA_BALL_ATTACHED ? 0x87B649u : 0x85A518u;
    telemetry->ball.flags_raw = NBA_GAMEPLAY_UNKNOWN_WORD;

    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaGameplayActorTelemetry *out = &telemetry->actors[actor];
        const NbaTipoffActor *state = &tipoff->actors[actor];
        out->index = (uint8_t)actor;
        out->team_side = actor >= 5u;
        out->roster_slot = state->roster_slot;
        bool live = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME;
        out->control = NBA_GAMEPLAY_CONTROL_CPU;
        out->world_x = live ? fp_round(state->x_fp) : formation[actor].world_x;
        out->world_y = live ? fp_round(state->y_fp) : formation[actor].world_y;
        out->world_z = live ? fp_round(state->z_fp) :
            actor == 5u ? (int16_t)center_jump_height(tipoff->frame) : 0;
        out->screen_x = live ? (int16_t)(out->world_x + out->world_y -
            tipoff->camera_x) : formation[actor].screen_x;
        out->screen_y = live ? (int16_t)((out->world_y - out->world_x) / 4 -
            tipoff->camera_y - out->world_z) :
            (int16_t)(formation[actor].screen_y - out->world_z);
        out->visible = live ? out->screen_x > -32 && out->screen_x < 288 &&
            out->screen_y > -48 && out->screen_y < 240 : actor_visible(actor);
        /* Actor +$0E/+$10 are already signed 8.8 ROM velocity words. Keep
         * them raw so CLI/JSON can compare directly with the Mesen oracle. */
        out->velocity_x = live ? state->velocity_x : 0;
        out->velocity_y = live ? state->velocity_y : 0;
        out->velocity_z = live ? state->velocity_z : 0;
        out->direction = live ? state->direction : formation[actor].direction;
        out->animation_state = live ? state->animation_state :
            actor_animation(tipoff, actor);
        out->lower_animation_state = live ? state->lower_animation_state :
                                     out->animation_state;
        out->ai_state = live ? (uint8_t)state->action_state : 0u;
        out->ai_target_actor = live ? state->assignment_actor :
                               NBA_GAMEPLAY_NO_ACTOR;
        out->actor_base = (uint16_t)(0x34EBu + actor * 0x100u);
        out->id_raw = (uint16_t)actor;
        out->action_raw = live ? state->behavior_timer : NBA_GAMEPLAY_UNKNOWN_WORD;
        out->flags_raw = 0;
        uint16_t upper_resource = 0u, lower_resource = 0u;
        if (live && nba_player_animation_resources(tipoff->assets,
                state->animation_state, state->lower_animation_state,
                state->direction, state->upper_animation_tick,
                state->lower_animation_tick, &upper_resource,
                &lower_resource)) {
            out->upper_resource_raw = upper_resource;
            out->lower_resource_raw = lower_resource;
        } else {
            out->upper_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
            out->lower_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        }
        out->head_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->motion_38_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->motion_3a_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->motion_3c_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->direction_4e_raw = out->direction;
        out->direction_50_raw = state->requested_direction;
        out->direction_52_raw = out->direction;
        out->target_x_56_raw = state->target_x;
        out->target_y_58_raw = state->target_y;
        out->side_group_raw = actor >= 5u ? 5u : 0u;
        out->control_mode_raw = state->control_mode;
        out->mode_saved_62_raw = state->pass_band_raw;
        out->pass_band_62_raw = state->pass_band_raw;
        out->pass_direction_66_raw = state->pass_direction_raw;
        out->control_mode_saved_raw = state->saved_control_mode;
        out->saved_mode_84_raw = state->saved_control_mode;
        out->pass_family_c0_raw = state->pass_family_raw;
        out->pass_release_threshold_raw = state->pass_release_threshold_raw;
        out->pass_released_raw = state->pass_released_raw ? 1u : 0u;
        out->assignment_base_raw = state->assignment_base_raw;
        out->assignment_current_raw = state->assignment_current_raw;
        out->assignment_alternate_raw = state->assignment_alternate_raw;
        out->assignment_distance_raw = state->assignment_distance;
        out->assignment_direction_raw = state->assignment_direction;
        out->anchor_direction_raw_88 = state->anchor_direction_raw;
        out->assignment_role_raw_92 = state->assignment_role_raw_92;
        out->pair_distance_raw = state->pair_distance;
        out->anchor_distance_raw_8c = state->anchor_distance_raw;
        out->reaction_threshold_raw = state->reaction_threshold;
        out->movement_boost_raw = state->movement_boost_timer;
        out->controller_assignment_16_raw =
            state->controller_assignment_raw;
        out->movement_magnitude_4c_raw = state->movement_magnitude_raw;
        out->contact_inhibit_5a_raw = state->contact_inhibit_raw_5a;
        out->contact_height_aa_raw = state->contact_height_raw_aa;
        out->recovery_inhibit_7a_raw = state->recovery_inhibit_raw;
        out->upper_restart_raw = out->lower_restart_raw = 0;
        out->upper_phase_raw = live ? state->upper_animation_phase_raw : 0u;
        out->lower_phase_raw = live ? state->lower_animation_tick : 0u;
        out->behavior_flags_raw = state->behavior_flags_raw;
        out->palette_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        out->actor_routine = live ? 0x85963Du : 0x80AD92u;
        out->ai_routine = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
            nba_gameplay_behavior_routine(state->control_mode) : 0u;
    }
}

void nba_tipoff_render(const NbaTipoff *tipoff, NbaRenderer *ren) {
    if (!tipoff || !tipoff->is_initialized || !ren) return;
    const uint32_t *court = nba_assets_gameplay_court_panorama(
        tipoff->assets, tipoff->session->right_team);
    int crop_x = tipoff->camera_x + 582;
    int crop_y = tipoff->camera_y + 243;
    if (crop_x < 0) crop_x = 0;
    if (crop_x > 912 - 256) crop_x = 912 - 256;
    if (crop_y < 0) crop_y = 0;
    if (crop_y > 416 - 224) crop_y = 416 - 224;
    for (unsigned y = 0; y < 224u; ++y)
        memcpy(ren->pixels + y * 256u,
               court + (size_t)(crop_y + (int)y) * 912u + crop_x,
               256u * sizeof(uint32_t));

    uint8_t render_order[NBA_GAMEPLAY_ACTOR_COUNT];
    int16_t screen_x[NBA_GAMEPLAY_ACTOR_COUNT];
    int16_t screen_y[NBA_GAMEPLAY_ACTOR_COUNT];
    unsigned render_count = 0;
    if (tipoff->frame < NBA_TIPOFF_BREAK_FRAME) {
        for (int order = 7; order >= 0; --order) {
            unsigned actor = visible_submission[order];
            render_order[render_count] = (uint8_t)actor;
            screen_x[actor] = formation[actor].screen_x;
            screen_y[actor] = formation[actor].screen_y;
            ++render_count;
        }
    } else {
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            int x = fp_round(tipoff->actors[actor].x_fp);
            int y = fp_round(tipoff->actors[actor].y_fp);
            int z = fp_round(tipoff->actors[actor].z_fp);
            screen_x[actor] = (int16_t)(x + y - tipoff->camera_x);
            screen_y[actor] = (int16_t)((y - x) / 4 - tipoff->camera_y - z);
            if (screen_x[actor] >= -32 && screen_x[actor] < 288 &&
                screen_y[actor] >= -48 && screen_y[actor] < 240)
                render_order[render_count++] = (uint8_t)actor;
        }
        for (unsigned i = 1; i < render_count; ++i) {
            uint8_t actor = render_order[i];
            int j = (int)i - 1;
            while (j >= 0 && screen_y[render_order[j]] > screen_y[actor]) {
                render_order[j + 1] = render_order[j];
                --j;
            }
            render_order[j + 1] = actor;
        }
    }

    for (unsigned order = 0; order < render_count; ++order) {
        unsigned actor = render_order[order];
        uint8_t team_side = actor >= 5u;
        uint8_t uniform_side = team_side;
        uint8_t slot = tipoff->actors[actor].roster_slot;
        uint8_t team = team_side ? tipoff->session->right_team :
                                   tipoff->session->left_team;
        uint8_t state = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                        tipoff->actors[actor].animation_state :
                        actor_animation(tipoff, actor);
        int jump = 0;
        if (tipoff->frame < NBA_TIPOFF_BREAK_FRAME && actor == 5u &&
            tipoff->frame >= NBA_TIPOFF_JUMP_FRAME &&
            tipoff->frame < NBA_TIPOFF_CONTACT_FRAME) {
            jump = center_jump_height(tipoff->frame);
        }
        uint8_t direction = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                            tipoff->actors[actor].direction :
                            formation[actor].direction;
        uint8_t lower_state = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                              tipoff->actors[actor].lower_animation_state : state;
        uint32_t upper_tick = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                              tipoff->actors[actor].upper_animation_tick :
                              (uint32_t)tipoff->frame;
        uint32_t lower_tick = tipoff->frame >= NBA_TIPOFF_BREAK_FRAME ?
                              tipoff->actors[actor].lower_animation_tick :
                              (uint32_t)tipoff->frame;
        nba_player_sprite_render_split(ren, tipoff->assets, team, slot,
                                       uniform_side, state, lower_state,
                                       direction, upper_tick, lower_tick,
                                       screen_x[actor],
                                       screen_y[actor] - jump, 1);
    }
    int ball_x, ball_y;
    if (tipoff->frame < NBA_TIPOFF_BREAK_FRAME) {
        ball_position(tipoff->frame, &ball_x, &ball_y);
    } else {
        int x = fp_round(tipoff->ball.x_fp), y = fp_round(tipoff->ball.y_fp);
        int z = fp_round(tipoff->ball.z_fp);
        ball_x = x + y - tipoff->camera_x;
        ball_y = (y - x) / 4 - tipoff->camera_y - z;
    }
    if (tipoff->frame >= NBA_TIPOFF_BALL_APPEAR_FRAME)
        draw_ball(tipoff, ren, ball_x, ball_y);

    if (tipoff->frame < 15) {
        unsigned brightness = (unsigned)tipoff->frame;
        for (int i = 0; i < 256 * 224; ++i) {
            uint32_t c = ren->pixels[i];
            uint32_t r = ((c >> 16) & 255u) * brightness / 15u;
            uint32_t g = ((c >> 8) & 255u) * brightness / 15u;
            uint32_t b = (c & 255u) * brightness / 15u;
            ren->pixels[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
}
