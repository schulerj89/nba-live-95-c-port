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

typedef enum {
    CPU_MODE11_NORMAL_RETURN = 0,
    CPU_MODE11_CONSUMED_ACTION,
    CPU_MODE11_SHOT_STARTED
} CpuMode11Outcome;

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
bool nba_tipoff_update_rom_passer(NbaTipoff *tipoff, unsigned slot);
static void cpu_enter_play_state(NbaTipoff *tipoff, NbaCpuPlayState state);
static CpuMode11Outcome cpu_dispatch_rom_mode11(NbaTipoff *tipoff,
                                                 unsigned slot,
                                                 uint8_t *direction);
static bool cpu_update_rom_shooter(NbaTipoff *tipoff, unsigned slot);
static bool cpu_update_rom_layup(NbaTipoff *tipoff, unsigned slot);
static bool cpu_update_rom_special_receiver(NbaTipoff *tipoff, unsigned slot);
static void cpu_commit_ball_acquisition(NbaTipoff *tipoff, uint8_t catcher);
static bool cpu_try_owned_ball_contact(NbaTipoff *tipoff);
static bool cpu_try_detached_shot_contact(NbaTipoff *tipoff);
static int cpu_first_pass_contact(NbaTipoff *tipoff);
static int cpu_first_loose_ball_contact(const NbaTipoff *tipoff);
static int cpu_first_inbound_ball_contact(const NbaTipoff *tipoff);
static void cpu_dispatch_pending_event(NbaTipoff *tipoff);
static bool cpu_update_free_throw_scene(NbaTipoff *tipoff);

/* `$86:A613-$A628`, reached by the shared `$85:A656-$A726` rectangular
 * actor/ball clamp. The port represents the proven globals it mutates; the
 * unrepresented `$0944/$094A/$09B8` words remain documented in the dump. */
static void cpu_cancel_boundary_activity(NbaTipoff *tipoff) {
    tipoff->pass_actor_raw = -1;
    tipoff->pass_receiver_raw = -1;
    tipoff->ball_activity_raw = 0u;
}

/* Complete `$86:A613-$A628` mutation. The helper deliberately leaves
 * `$093E`, `$09C4`, the ball record and its velocities intact: mode 15 and
 * `$86:CCFC-$D3C5` decide whether a canceled pass aborts before release or
 * becomes a generic loose-ball acquisition after release. */
static void cpu_cancel_rom_pass_activity(NbaTipoff *tipoff) {
    cpu_cancel_boundary_activity(tipoff);
    tipoff->pass_aux_raw = -1;
    tipoff->rim_raw_094a = 0u;
    tipoff->inbound_transfer_raw = 0u;
}

/* `$86:9846-$986C`: restore an actor from a transient receiver/action mode.
 * Native +$60 is represented by reaction_threshold for modes 10/14. */
static void cpu_restore_normal_mode(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    uint8_t side_group = slot >= 5u ? 5u : 0u;
    actor->control_mode = side_group == tipoff->camera_side_group_raw ? 1u : 2u;
    actor->behavior_timer = 0x2Fu;
    actor->reaction_threshold = 0u;
    actor->behavior_flags_raw = 0u;
    actor->actor_status_raw_28 = 0u;
}

/* Player movement has its own court restriction contract. Reusing the ball
 * integrator's `$86:A613` side effect here made a baseline player cancel an
 * unrelated pass every 30-Hz sweep. */
static void cpu_clamp_actor_to_court(int32_t *x_fp, int32_t *y_fp,
                                     int16_t *velocity_x,
                                     int16_t *velocity_y) {
    (void)nba_gameplay_court_clamp(
        x_fp, y_fp, velocity_x, velocity_y);
}

/* `$85:A3B7-$A813` is the ownerless-ball physics chain. Its four rectangular
 * edge branches call `$86:A613` before clamping the ball axis; the later
 * diagonal/isometric correction does not. */
static void cpu_clamp_ball_to_court(NbaTipoff *tipoff,
                                    int32_t *x_fp, int32_t *y_fp,
                                    int16_t *velocity_x,
                                    int16_t *velocity_y) {
    if (nba_gameplay_court_clamp(
            x_fp, y_fp, velocity_x, velocity_y))
        cpu_cancel_rom_pass_activity(tipoff);
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

/* `$85:F5E4-$F727`: an opponent blocks the cutter when its center is in the
 * ROM's lower-inclusive/upper-exclusive actor-to-basket rectangle. It
 * traverses linked actor neighbors; the fixed ten-record C array yields the
 * same predicate. */
static bool cpu_lane_to_basket_is_clear(const NbaTipoff *tipoff,
                                        unsigned slot) {
    NbaGameplayLaneActor actors[NBA_GAMEPLAY_ACTOR_COUNT];
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        actors[i].x = fp_round(tipoff->actors[i].x_fp);
        actors[i].y = fp_round(tipoff->actors[i].y_fp);
        actors[i].team_group = (uint8_t)((i / 5u) * 5u);
    }
    return nba_gameplay_lane_to_basket_clear(
        (uint8_t)slot,
        tipoff->team_context[slot / 5u].anchor_x_raw_0a,
        actors, NBA_GAMEPLAY_ACTOR_COUNT);
}

/* `$85:B4B9-$B50D`: actor +$64 cadence selects a clear-lane cutter while
 * `$09A4` is active. This runs before the mode's formation/arrival work. */
static void cpu_update_special_actor(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    bool has_owner = tipoff->possession_actor >= 0;
    uint16_t owner_distance = UINT16_MAX;
    if (has_owner) {
        const NbaTipoffActor *owner =
            &tipoff->actors[tipoff->possession_actor];
        int dx = fp_round(owner->x_fp) - fp_round(actor->x_fp);
        int dy = fp_round(owner->y_fp) - fp_round(actor->y_fp);
        owner_distance = actor_distance(dx, dy);
    }
    nba_gameplay_special_actor_step(
        &actor->behavior_timer, actor->control_mode,
        tipoff->play_cycle_raw, has_owner,
        cpu_lane_to_basket_is_clear(tipoff, slot), owner_distance,
        (uint8_t)slot, &tipoff->special_actor_raw);
}

static NbaPlayerAnimationChannels actor_animation_channels(const NbaTipoffActor *a) {
    NbaPlayerAnimationChannels c = {
        a->animation_upper_queue_cursor_raw_18, a->animation_lower_queue_cursor_raw_1a,
        a->animation_state, a->lower_animation_state, a->base_animation_state_raw_38,
        a->rom_upper_animation_phase_raw_3a, a->rom_lower_animation_phase_raw_3c,
        a->upper_animation_accumulator_raw_42, a->lower_animation_accumulator_raw_44,
        a->upper_animation_lock_raw_46, a->lower_animation_lock_raw_48, {0}, {0}
    };
    memcpy(c.upper_queue, a->animation_upper_queue_raw_1c, sizeof(c.upper_queue));
    memcpy(c.lower_queue, a->animation_lower_queue_raw_22, sizeof(c.lower_queue));
    return c;
}

static void actor_store_animation_channels(NbaTipoffActor *a,
                                           const NbaPlayerAnimationChannels *c) {
    if (a->animation_state != c->upper_state) {
        a->upper_animation_tick = 0;
        a->upper_animation_phase_raw = 0;
    }
    if (a->lower_animation_state != c->lower_state) {
        a->lower_animation_tick = 0;
        a->lower_animation_phase_raw = 0;
    }
    a->animation_state = (uint8_t)c->upper_state;
    a->lower_animation_state = (uint8_t)c->lower_state;
    a->base_animation_state_raw_38 = (uint8_t)c->base_state;
    a->rom_upper_animation_phase_raw_3a = c->upper_phase;
    a->rom_lower_animation_phase_raw_3c = c->lower_phase;
    a->upper_animation_accumulator_raw_42 = c->upper_accumulator;
    a->lower_animation_accumulator_raw_44 = c->lower_accumulator;
    a->upper_animation_lock_raw_46 = c->upper_lock;
    a->lower_animation_lock_raw_48 = c->lower_lock;
    a->animation_upper_queue_cursor_raw_18 = c->upper_queue_cursor;
    a->animation_lower_queue_cursor_raw_1a = c->lower_queue_cursor;
}

static void actor_animation_command(NbaTipoff *tipoff, NbaTipoffActor *actor,
    NbaPlayerAnimationCommand command, uint16_t state) {
    NbaPlayerAnimationChannels channels = actor_animation_channels(actor);
    if (!nba_player_animation_command(tipoff->assets, &channels, command,
            &state, actor->movement_boost_timer != 0,
            actor->free_throw_launch_half_raw_a8 != 0)) return;
    actor_store_animation_channels(actor, &channels);
    actor->animation_resources_valid = false;
    /* `$87:AEC3` refreshes the new pose without consuming a cadence tick.
     * Adopt this only for the already-integrated live-pass action path. */
    if (actor->exact_pass_animation) {
        NbaPlayerResolvedPose pose = {0};
        pose.direction = actor->direction;
        actor->animation_resources_valid = nba_player_resolve_pose(
            tipoff->assets, &channels, actor->direction,
            actor->free_throw_launch_half_raw_a8 != 0u,
            actor->animation_variant_raw_6c, &pose);
        if (actor->animation_resources_valid) {
            actor->upper_animation_resource_raw_2a = pose.upper_resource;
            actor->lower_animation_resource_raw_2c = pose.lower_resource;
        }
    }
}

static void actor_set_upper_animation(NbaTipoffActor *actor, uint8_t upper) {
    if (actor->animation_state == upper) return;
    actor->animation_state = upper;
    actor->upper_animation_tick = 0u;
    actor->upper_animation_phase_raw = 0u;
    actor->rom_upper_animation_phase_raw_3a = 0u;
    actor->upper_animation_accumulator_raw_42 = 0u;
    actor->animation_resources_valid = false;
}

static void actor_set_animation(NbaTipoffActor *actor, uint8_t upper,
                                uint8_t lower) {
    /* Non-pass callers still have provisional action scheduling. Migrate
     * those with their own release/cancel boundaries, not all at once. */
    actor_set_upper_animation(actor, upper);
    if (actor->lower_animation_state != lower) {
        actor->lower_animation_state = lower;
        actor->lower_animation_tick = 0u;
        actor->lower_animation_phase_raw = 0u;
        actor->rom_lower_animation_phase_raw_3c = 0u;
        actor->lower_animation_accumulator_raw_44 = 0u;
        actor->animation_resources_valid = false;
    }
}

void nba_tipoff_ease_display_direction(uint8_t desired,
                                       uint16_t upper_animation_lock,
                                       uint8_t *shown_raw, uint8_t *timer) {
    /* `$87:8F13-$8F5E`: +$52 eases toward desired +$4E. The final adjacent
     * octant is held for three logical actor passes; locked upper actions
     * snap immediately so their pose-point geometry remains coherent. */
    if (!shown_raw || !timer || desired >= 8u) return;
    if (upper_animation_lock != 0u) {
        *shown_raw = desired;
        *timer = 0u;
        return;
    }
    uint8_t shown = *shown_raw & 7u;
    uint8_t delta = (uint8_t)((desired - shown) & 7u);
    if (delta == 0u) return;
    uint8_t candidate = delta < 4u ? (uint8_t)((shown + 1u) & 7u) :
                                     (uint8_t)((shown - 1u) & 7u);
    if (candidate != desired) {
        *shown_raw = candidate;
        *timer = 0u;
        return;
    }
    if (++*timer >= 3u) {
        *shown_raw = desired;
        *timer = 0u;
    }
}

static void cpu_ease_display_direction(NbaTipoffActor *actor) {
    nba_tipoff_ease_display_direction(
        actor->movement_direction, actor->upper_animation_lock_raw_46,
        &actor->direction, &actor->facing_ease_timer_raw_be);
}

static void cpu_resolve_locomotion_animation(NbaTipoff *tipoff,
                                             unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    uint8_t state = actor->base_animation_state_raw_38;
    if (state >= 19u) return;
    uint16_t velocity_or = (uint16_t)actor->velocity_x |
                           (uint16_t)actor->velocity_y;
    bool is_stationary = velocity_or < 5u;
    if (!is_stationary) actor->catcher_latch_raw_ae = 0u;
    state = nba_player_locomotion_state(
        state, is_stationary, actor->movement_boost_timer != 0u,
        tipoff->possession_actor == (int8_t)slot,
        fp_integer_word(actor->z_fp) != 0);
    bool alternate_lower = actor->free_throw_launch_half_raw_a8 != 0u;
    /* `$87:B5FC/$B62F`: changing locomotion state clears only the channel
     * accumulator. The existing phase survives when the new descriptor has
     * that frame; resetting it unconditionally caused the visible wiggle. */
    if (actor->upper_animation_lock_raw_46 == 0u &&
        actor->animation_state != state) {
        uint16_t count = 0u;
        actor->animation_state = state;
        actor->upper_animation_tick = 0u; /* legacy collision phase */
        actor->upper_animation_accumulator_raw_42 = 0u;
        if (!nba_player_animation_frame_count(tipoff->assets, true, state,
                                               alternate_lower, &count) ||
            actor->rom_upper_animation_phase_raw_3a >= count)
            actor->rom_upper_animation_phase_raw_3a = 0u;
        actor->animation_resources_valid = false;
    }
    if (actor->lower_animation_lock_raw_48 == 0u &&
        actor->lower_animation_state != state) {
        uint16_t count = 0u;
        actor->lower_animation_state = state;
        actor->lower_animation_tick = 0u; /* legacy collision phase */
        actor->lower_animation_accumulator_raw_44 = 0u;
        if (!nba_player_animation_frame_count(tipoff->assets, false, state,
                                               alternate_lower, &count) ||
            actor->rom_lower_animation_phase_raw_3c >= count)
            actor->rom_lower_animation_phase_raw_3c = 0u;
        actor->animation_resources_valid = false;
    }
}

static void cpu_advance_actor_animation(NbaTipoff *tipoff,
                                        NbaTipoffActor *actor) {
    ++actor->upper_animation_tick;
    ++actor->lower_animation_tick;
    if (actor->upper_animation_lock_raw_46 != 0u ||
        actor->lower_animation_lock_raw_48 != 0u ||
        (actor->control_mode == 15u && actor->exact_pass_animation)) {
        NbaPlayerAnimationChannels channels = actor_animation_channels(actor);
        actor->animation_resources_valid = nba_player_animation_step_channels(
            tipoff->assets, &channels, actor->direction,
            (uint16_t)(actor->movement_magnitude_raw << 1), 0x200u,
            actor->free_throw_launch_half_raw_a8 != 0u, actor->animation_variant_raw_6c,
            &tipoff->rng.state,
            &actor->upper_animation_resource_raw_2a,
            &actor->lower_animation_resource_raw_2c);
        if (actor->animation_resources_valid)
            actor_store_animation_channels(actor, &channels);
    } else {
        /* Keep ordinary locomotion on the already-verified common cadence.
         * Integrating mode-2 idle RNG here would move the AI RNG stream and
         * is intentionally outside this action-only checkpoint. */
        actor->animation_resources_valid = nba_player_animation_rom_step(
            tipoff->assets, actor->animation_state,
            actor->lower_animation_state, actor->direction,
            (uint16_t)(actor->movement_magnitude_raw << 1),
            actor->free_throw_launch_half_raw_a8 != 0u,
            actor->animation_variant_raw_6c,
            &actor->upper_animation_accumulator_raw_42,
            &actor->lower_animation_accumulator_raw_44,
            &actor->rom_upper_animation_phase_raw_3a,
            &actor->rom_lower_animation_phase_raw_3c,
            &actor->upper_animation_resource_raw_2a,
            &actor->lower_animation_resource_raw_2c);
    }
    /* Other action branches ported before the descriptor accumulator consume
     * their verified logical-tick phase. Keep that integration boundary
     * stable while the exact ROM phases above drive the visible resources. */
    uint16_t lower_phase = 0u;
    if (nba_player_animation_phases(
            tipoff->assets, actor->animation_state,
            actor->lower_animation_state, actor->upper_animation_tick,
            actor->lower_animation_tick,
            &actor->upper_animation_phase_raw, &lower_phase))
        actor->lower_animation_phase_raw = lower_phase;
    /* Action release gates consume the same phase that the locked resource
     * advances. Keeping the old half-rate tick here can finish/unlock a pass
     * before its release threshold is ever reached. Ordinary locomotion's
     * legacy contact/ball phase remains the next integration boundary. */
    if (actor->animation_resources_valid &&
        (actor->upper_animation_lock_raw_46 != 0u ||
         (actor->control_mode == 15u && actor->exact_pass_animation)))
        actor->upper_animation_phase_raw = actor->rom_upper_animation_phase_raw_3a;
    if (actor->animation_resources_valid && actor->lower_animation_lock_raw_48 != 0u)
        actor->lower_animation_phase_raw = actor->rom_lower_animation_phase_raw_3c;
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

/* `$86:AB2D-$AF65`: grounded mode-15 setup. This includes the aligned
 * `$2A-$2C` selector at `$86:AE52-$AED7`; airborne `$AFC4` and catch-preinit
 * `$AF66` still depend on raw writers not represented by the port. */
bool nba_tipoff_begin_rom_pass(NbaTipoff *tipoff, unsigned passer_slot,
                               unsigned receiver_slot) {
    NbaTipoffActor *passer = &tipoff->actors[passer_slot];
    NbaTipoffActor *receiver = &tipoff->actors[receiver_slot];
    /* `$86:AB83-$ABE5` reads the signed integer position words directly;
     * subpixel bytes do not round either endpoint before the velocity lead. */
    int16_t passer_x = (int16_t)(fp_integer_word(passer->x_fp) +
        pass_predict_component(passer->velocity_x, 4u));
    int16_t passer_y = (int16_t)(fp_integer_word(passer->y_fp) +
        pass_predict_component(passer->velocity_y, 4u));
    int16_t receiver_x = (int16_t)(fp_integer_word(receiver->x_fp) +
        pass_predict_component(receiver->velocity_x, 3u));
    int16_t receiver_y = (int16_t)(fp_integer_word(receiver->y_fp) +
        pass_predict_component(receiver->velocity_y, 3u));
    uint16_t distance = 0u;
    uint8_t fine = nba_gameplay_pass_direction(
        (int16_t)(receiver_x - passer_x),
        (int16_t)(receiver_y - passer_y), &distance);
    uint8_t pass_direction = fine >> 1;
    if (pass_direction >= 8u) return false;

    uint8_t band = pass_band_from_distance(distance);
    uint8_t relative = (uint8_t)(
        (pass_direction - passer->movement_direction) & 7u);
    if (fp_integer_word(passer->z_fp) != 0) return false;
    /* `$86:AB3D` cancels the prior upper action before installing a pass. */
    actor_animation_command(tipoff, passer, NBA_ANIMATION_CANCEL_UPPER, 0u);
    passer->exact_pass_animation = tipoff->live_state_raw < 0x80u;
    uint8_t upper = 0u;
    int16_t family = -1;
    bool airborne_family = false;
    /* `$86:AB7D-$AB80` seeds every receiver with mode 10/timer $28. The
     * side/back path alone overwrites it with $3C at `$86:ACA9-$ACAE`. */
    uint16_t receiver_timer = 0x28u;
    if (relative >= 3u && relative < 6u) {
        receiver_timer = 0x3Cu;
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
            passer->movement_boost_timer != 0u) {
            /* `$86:AC87-$AC96 -> $86:AFC4-$B00A`: long boosted pass.
             * Receiver +$60 becomes $46; passer +$C0 selects table 2,
             * planar velocity is halved, and the independent upper/lower
             * resources are `$18/$1F`. `$86:A629` supplies the jump launch
             * which later changes family 2 to release family 4 at the apex. */
            receiver_timer = 0x46u;
            family = 2;
            upper = 0x18u;
            airborne_family = true;
            passer->velocity_x = nba_gameplay_arithmetic_shift_right(
                passer->velocity_x, 1u);
            passer->velocity_y = nba_gameplay_arithmetic_shift_right(
                passer->velocity_y, 1u);
            passer->velocity_z = 0x0258;
        }
        bool grounded_special = forced_special || stationary;
        if (airborne_family) {
            /* Selection is complete; do not fall through into `$ACB1`. */
        } else if (grounded_special) {
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
                    (fine - passer->movement_direction -
                     passer->movement_direction) & 15u);
                if (fine_relative == 8u) {
                    passer->movement_direction = (uint8_t)(
                        (passer->movement_direction + 1u) & 7u);
                    relative = (uint8_t)(
                        (pass_direction - passer->movement_direction) & 7u);
                    selector = relative;
                } else selector = fine_relative == 9u ? 3 : 5;
            }
            int16_t sign_test = (int16_t)(selector * 2 - 7);
            if (passer->movement_direction < 3u)
                sign_test = (int16_t)~sign_test;
            upper = sign_test < 0 ? 0x2Eu : 0x2Du;
        }
    } else {
        /* `$86:AE10-$AE4F`: off-axis route. */
        int selector = relative;
        if (relative == 0u) {
            selector = (fine - passer->movement_direction -
                        passer->movement_direction) & 15u;
            if (selector == 0) {
                /* `$86:AE52-$AED7`: captured inbound layout 5 forces the
                 * straight-ahead `$2B` family. This is the route reached by
                 * `$86:F43A-$F653` after a made basket. It deliberately
                 * retains the common `$28` receiver timer seeded above. */
                if (tipoff->live_state_raw == 0x82u &&
                    tipoff->inbound_layout_raw == 5) {
                    upper = 0x2Bu;
                    family = -1;
                } else {
                    /* `$86:AEC3-$AECD` is the native conservative aligned
                     * fallback. For distances below `$F1`, `$86:AEF2-AEFE`
                     * promotes `$2C` to the shared `$2F` visible pose while
                     * preserving family 1. */
                    upper = distance < 0xF1u ? 0x2Fu : 0x2Cu;
                    family = 1;
                }
            }
        }
        if (upper == 0u) {
            bool choose_30 = selector < 3;
            if (passer->movement_direction < 3u) choose_30 = !choose_30;
            upper = choose_30 ? 0x30u : 0x31u;
        }
    }

    uint8_t threshold = 0u;
    if (!airborne_family && !nba_assets_gameplay_pass_release_threshold(
            tipoff->assets, upper, &threshold)) return false;
    passer->pass_band_raw = (uint16_t)(band * 6u);
    /* +$66 is animation-resource state, not the coarse pass direction.
     * `$87:B47A/$B649` may rewrite it while resolving the selected pose. */
    passer->pass_family_raw = family;
    passer->pass_release_threshold_raw = threshold;
    passer->pass_released_raw = false;
    passer->control_mode = 15u;
    passer->behavior_flags_raw |= 0x0006u;
    receiver->control_mode = 10u;
    receiver->reaction_threshold = receiver_timer;
    if (passer->exact_pass_animation) {
        actor_animation_command(tipoff, passer, NBA_ANIMATION_INSTALL_UPPER, upper);
        if (airborne_family)
            actor_animation_command(tipoff, passer, NBA_ANIMATION_INSTALL_LOWER, 0x1Fu);
    } else if (airborne_family) {
        actor_set_animation(passer, upper, 0x1Fu);
    } else {
        actor_set_upper_animation(passer, upper);
    }
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

static void cpu_predicted_ball_xy(const NbaTipoff *tipoff,
                                  int16_t *x, int16_t *y) {
    /* `$85:A76D-$A79E` writes `$0918/$091A` after ball integration. Four
     * signed RORs on 8.8 velocity are a 16-simulation-tick lookahead. */
    int16_t predicted_x = (int16_t)(
        (uint16_t)fp_integer_word(tipoff->ball.x_fp) +
        (uint16_t)nba_gameplay_arithmetic_shift_right(
            tipoff->ball.velocity_x, 4u));
    int16_t predicted_y = (int16_t)(
        (uint16_t)fp_integer_word(tipoff->ball.y_fp) +
        (uint16_t)nba_gameplay_arithmetic_shift_right(
            tipoff->ball.velocity_y, 4u));
    if (x) *x = predicted_x;
    if (y) *y = predicted_y;
}

static void cpu_cache_predicted_ball_xy(NbaTipoff *tipoff) {
    cpu_predicted_ball_xy(tipoff, &tipoff->role_focal_x_raw_0918,
                          &tipoff->role_focal_y_raw_091a);
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
    paired->assignment_distance = pair_distance;
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
        /* `$86:F0B7-$F0FC`: +$7A pauses the override clock. Once active,
         * values >=10 keep steering; the final 0..9 window holds position.
         * Expiry restores the mode saved in overloaded +$62, not the
         * per-frame +$84 snapshot. */
        if (actor->recovery_inhibit_raw != 0u) {
            actor->velocity_x = actor->velocity_y = 0;
            return true;
        }
        uint16_t remaining = (uint16_t)(actor->reaction_threshold - 2u);
        actor->reaction_threshold = remaining;
        if ((remaining & 0x8000u) != 0u) {
            actor->reaction_threshold = 0u;
            actor->movement_boost_timer = 0u;
            actor->control_mode = (uint8_t)actor->pass_band_raw;
            actor->action_state = 3u;
            actor_set_animation(actor, actor->animation_state, 3u);
            return true;
        }
        if (remaining < 10u) {
            actor->velocity_x = actor->velocity_y = 0;
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
    cpu_restore_normal_mode(tipoff, slot);
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
        tipoff->possession_actor != (int8_t)slot ||
        tipoff->actors[slot].control_mode != 11u ||
        (tipoff->inbound_transfer_raw != 0u &&
         tipoff->actors[slot].control_mode != 11u)) return false;
    NbaTipoffActor *actor = &tipoff->actors[slot];
    cpu_update_special_actor(tipoff, slot);
    /* `$86:F4F2-$F520` tests the current actor against the compensated
     * target on every dispatch. `$09BA` is written after arrival and is not
     * a substitute for this geometry test; a recovery carrier may inherit
     * the prior carrier's nonzero latch while still outside the box. */
    if (nba_gameplay_inbound_arrived(
            fp_round(actor->x_fp), fp_round(actor->y_fp),
            tipoff->inbound_target_x_raw, tipoff->inbound_target_y_raw)) {
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

/* `$87:9C67 -> $86:C6AD-$C74D`: mode 8 is timer-authoritative knockdown
 * recovery. The global scheduler saturates `+$5A`; this executor wraps
 * `+$60` through negative. Action $36's nonnegative `+$56` enables its
 * landing hop/settle. */
static bool cpu_update_knockdown_actor(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (actor->control_mode != 8u) return false;
    /* Native `$85:963D` commits the velocity from the preceding behavior
     * pass before `$87:9244` reaches C6AD. Landing writes below therefore
     * become motion input on the next 30-Hz pass. */
    actor->x_fp += (int32_t)actor->velocity_x * 2;
    actor->y_fp += (int32_t)actor->velocity_y * 2;
    cpu_integrate_actor_vertical(actor);
    actor->behavior_flags_raw |= 6u;
    if (actor->special_contact_raw_56 >= 0 &&
        actor->velocity_z == 0 && actor->z_fp == 0) {
        if ((int16_t)actor->pass_direction_raw >= 0) {
            tipoff->rim_raw_13e7 |= 0x0100u;
            actor->velocity_z = 0x00F0;
            actor->velocity_x = nba_gameplay_arithmetic_shift_right(
                actor->velocity_x, 1u);
            actor->velocity_y = nba_gameplay_arithmetic_shift_right(
                actor->velocity_y, 1u);
        } else {
            actor->velocity_x = 0;
            actor->velocity_y = 0;
        }
        actor->pass_direction_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    }
    actor->contact_action_timer_raw_60 = (uint16_t)(
        actor->contact_action_timer_raw_60 - 2u);
    int16_t presentation_phase = (int16_t)(uint16_t)(
        actor->contact_action_timer_raw_60 - 0x36u);
    actor->actor_status_raw_28 &= 0xFFE7u;
    if (presentation_phase >= 10 && presentation_phase < 20)
        actor->actor_status_raw_28 |= 0x0010u;
    else if (presentation_phase >= 20 && presentation_phase < 30)
        actor->actor_status_raw_28 |= 0x0008u;
    else if (presentation_phase >= 30 && presentation_phase < 40)
        actor->actor_status_raw_28 |= 0x0010u;

    if ((int16_t)actor->contact_action_timer_raw_60 >= 0) return true;

    uint16_t actor_group = slot >= 5u ? 5u : 0u;
    actor->control_mode = actor_group == tipoff->camera_side_group_raw ? 1u : 2u;
    actor->behavior_timer = 0x2Fu;
    actor->contact_action_timer_raw_60 = 0u;
    actor->behavior_flags_raw = 0u;
    actor->actor_status_raw_28 = 0u;
    actor->contact_inhibit_raw_5a = 0u;
    if (tipoff->possession_actor == (int8_t)slot)
        actor->control_mode = 11u;
    return true;
}

/* `$87:9244/$9BD0` is a second ten-actor scheduler pass. It runs after the
 * `$87:8EFB-$8F92 -> $85:963D` locomotion/physics loop and the intervening
 * ball/contact work. Keeping the dispatcher separate means a new owner can
 * receive its mode-11 target and velocity without retroactively moving in
 * the already-completed physics pass. */
static void cpu_dispatch_normal_actor_behavior(NbaTipoff *tipoff,
                                               unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (!((actor->control_mode >= 1u && actor->control_mode <= 6u) ||
          actor->control_mode == 11u)) return;

    cpu_update_special_actor(tipoff, slot);
    if (actor->control_mode == 11u &&
        slot != (unsigned)tipoff->possession_actor) {
        /* `$86:F3F6-$F40A`: stale mode-11 actors normalize in the behavior
         * pass, after their previous motion has already been committed. */
        actor->control_mode = 1u;
        actor->behavior_timer = 47u;
        actor->reaction_threshold = 0u;
        actor->behavior_flags_raw = 0u;
    }
    if (actor->control_mode == 5u &&
        slot != (unsigned)tipoff->possession_actor) {
        actor->control_mode = 1u;
        actor->behavior_timer = 47u;
        actor->reaction_threshold = 0u;
        actor->behavior_flags_raw = 0u;
    }

    int x = fp_round(actor->x_fp), y = fp_round(actor->y_fp);
    /* `$86:E593-$E5AA`, the verified terminal fallback of the larger
     * `$86:E4A7-$E5AA` selector: the catcher/dead-ball latch selects base 12;
     * an ordinary owner selects base 5. The earlier proximity/facing branches
     * remain a separate porting increment. `$86:E3CB-$E3DD` repairs special
     * mode 1-6 bases. Writes affect the next logical physics pass. */
    if (actor->control_mode == 11u) {
        NbaGameplayOwnerDribbleGate gate = nba_gameplay_owner_dribble_gate(
            (int16_t)fp_round(actor->z_fp),
            tipoff->fouls.free_throw_state_raw_0978,
            tipoff->live_state_raw, actor->movement_magnitude_raw);
        if (gate != NBA_GAMEPLAY_OWNER_DRIBBLE_SKIP) {
            NbaGameplayOwnerProximityResult proximity =
                NBA_GAMEPLAY_OWNER_PROXIMITY_FALLBACK;
            if (gate == NBA_GAMEPLAY_OWNER_DRIBBLE_CONTINUE) {
                unsigned paired = actor->assignment_actor;
                if (paired < NBA_GAMEPLAY_ACTOR_COUNT) {
                    NbaTipoffActor *paired_actor = &tipoff->actors[paired];
                    proximity = nba_gameplay_owner_dribble_proximity(
                        tipoff->team_context[slot / 5u].anchor_x_raw_0a,
                        (int16_t)x, paired_actor->movement_magnitude_raw,
                        actor->assignment_distance,
                        paired_actor->assignment_direction,
                        tipoff->dead_ball_raw_0968,
                        actor->catcher_latch_raw_ae,
                        &actor->requested_direction);
                }
            }
            if (proximity == NBA_GAMEPLAY_OWNER_PROXIMITY_UNLATCHED) {
                actor->base_animation_state_raw_38 =
                    nba_gameplay_owner_unlatched_pose(
                        actor->velocity_x, actor->velocity_y,
                        actor->requested_direction, &actor->direction);
            } else {
                actor->base_animation_state_raw_38 =
                    nba_gameplay_owner_dribble_fallback_pose(
                        tipoff->dead_ball_raw_0968,
                        actor->catcher_latch_raw_ae);
            }
        }
    } else if (actor->control_mode >= 1u &&
               actor->control_mode <= 6u &&
               (actor->animation_state == 8u ||
                actor->animation_state == 10u)) {
        actor->base_animation_state_raw_38 = 3u;
    }

    bool decision_due = cpu_active_decision_due(tipoff, slot);
    uint8_t direction = actor->movement_direction;
    bool stop_velocity = false;
    bool apply_velocity_step = false;
    NbaGameplayLoosePursuitGateInput pursuit = {
        .live_state_raw_0936 = tipoff->live_state_raw,
        .ball_activity_raw_0948 = tipoff->ball_activity_raw,
        .bounce_age_raw_094a = tipoff->rim_raw_094a,
        .free_throw_state_raw_0978 =
            tipoff->fouls.free_throw_state_raw_0978,
        .play_code_raw_0996 = tipoff->play_code,
        .foul_actor_raw_7e492f = tipoff->collision_actor_b_raw,
        .actor_id = (uint8_t)slot,
        .actor_control_mode = actor->control_mode,
        .actor_team_group_raw_6e = slot < 5u ? 0u : 5u,
        .offense_group_raw_093a = tipoff->camera_side_group_raw,
        .inbound_group_raw_0952 = (uint8_t)tipoff->inbound_state_raw
    };
    bool loose_pursuit = tipoff->ball.owner_actor < 0 &&
        nba_gameplay_loose_ball_pursuit_allowed(&pursuit);
    if (loose_pursuit && decision_due &&
        actor->recovery_inhibit_raw == 0u) {
        cpu_predicted_ball_xy(tipoff, &actor->target_x, &actor->target_y);
        direction = nba_gameplay_target_direction(
            (int16_t)(actor->target_x - x),
            (int16_t)(actor->target_y - y), NULL);
        if (direction < 8u) actor->requested_direction = direction;
        apply_velocity_step = true;
    } else if (decision_due) {
        uint8_t mode = actor->control_mode;
        if (mode == 11u && tipoff->live_state_raw != 0x82u &&
                slot == (unsigned)tipoff->possession_actor &&
                actor->controller_assignment_raw < 0) {
            CpuMode11Outcome outcome = cpu_dispatch_rom_mode11(
                tipoff, slot, &direction);
            if (outcome != CPU_MODE11_NORMAL_RETURN) {
                if (actor->control_mode == 13u) {
                    ball_attach_to_actor(tipoff, slot);
                    actor->action_state = tipoff->cpu_play_state;
                    return;
                }
                apply_velocity_step =
                    outcome == CPU_MODE11_CONSUMED_ACTION && direction < 8u;
            } else {
                if (actor->recovery_inhibit_raw == 0u) {
                    direction = cpu_formation_target_direction(tipoff, slot);
                    apply_velocity_step = true;
                }
                int selected = cpu_select_rom_receiver(tipoff, (uint8_t)slot);
                bool special_receiver = selected >= 0 &&
                    (uint16_t)selected == tipoff->special_actor_raw;
                if (selected >= 0) {
                    if (!special_receiver) actor->reaction_threshold = 1u;
                    if (nba_tipoff_begin_rom_pass(
                            tipoff, slot, (unsigned)selected)) {
                        tipoff->receiver_actor = (uint8_t)selected;
                        tipoff->actors[selected].control_mode =
                            special_receiver ? 14u : 10u;
                        if (special_receiver) {
                            NbaTipoffActor *receiver = &tipoff->actors[selected];
                            receiver->mode13_baseline_velocity_x =
                                receiver->velocity_x;
                            receiver->mode13_baseline_velocity_y =
                                receiver->velocity_y;
                        }
                        cpu_enter_play_state(tipoff, NBA_CPU_PLAY_PASS);
                    }
                }
            }
        } else if ((mode == 1u || mode == 3u || mode == 5u) &&
                actor->controller_assignment_raw < 0 &&
                actor->recovery_inhibit_raw == 0u) {
            direction = cpu_formation_target_direction(tipoff, slot);
            apply_velocity_step = true;
        }
        else if (tipoff->cpu_play_state != NBA_CPU_PLAY_REBOUND &&
                 (mode == 2u || mode == 4u || mode == 6u) &&
                 actor->recovery_inhibit_raw == 0u) {
            (void)cpu_refresh_defense_target(tipoff, slot, &stop_velocity);
            direction = stop_velocity ? 8u : nba_gameplay_target_direction(
                (int16_t)(actor->target_x - x),
                (int16_t)(actor->target_y - y), NULL);
            apply_velocity_step = !stop_velocity;
        } else
            direction = actor->movement_direction;
        if (direction < 8u) actor->requested_direction = direction;
    }
    if (stop_velocity) actor->velocity_x = actor->velocity_y = 0;
    uint8_t team = slot >= 5u ? tipoff->session->right_team :
                               tipoff->session->left_team;
    uint8_t profile_42 = 0x58u;
    (void)nba_player_gameplay_movement_profile(
        tipoff->assets, team, actor->roster_slot, &profile_42);
    if (apply_velocity_step)
        nba_gameplay_velocity_step(
            &actor->velocity_x, &actor->velocity_y,
            &actor->movement_boost_timer, direction, profile_42, 2u,
            tipoff->live_state_raw == 0x81u || fp_round(actor->z_fp) != 0,
            (int16_t)tipoff->possession_actor);
    actor->movement_magnitude_raw = actor_distance(
        actor->velocity_x, actor->velocity_y);
    uint8_t velocity_direction = nba_gameplay_target_direction(
        actor->velocity_x, actor->velocity_y, NULL);
    actor->velocity_direction_raw_a2 = velocity_direction;
    if (velocity_direction < 8u) {
        actor->movement_direction = velocity_direction;
        actor->requested_direction = velocity_direction;
    }
    actor->action_state = tipoff->cpu_play_state;
}

static void cpu_move_actor(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (actor->control_mode == 15u &&
        nba_tipoff_update_rom_passer(tipoff, slot))
        return;
    if (actor->control_mode == 12u && cpu_update_rom_shooter(tipoff, slot))
        return;
    if (actor->control_mode == 13u && cpu_update_rom_layup(tipoff, slot))
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
    if (actor->control_mode == 14u &&
        cpu_update_rom_special_receiver(tipoff, slot)) return;
    if (cpu_update_knockdown_actor(tipoff, slot)) return;
    if (cpu_apply_passive_mode(actor)) {
        cpu_integrate_actor_vertical(actor);
        return;
    }
    /* `$87:B572` resolves the pose from the velocity installed by the prior
     * behavior pass. `$85:963D-$985F` then commits that velocity before the
     * mode dispatcher can replace it for the next 30-Hz pass. Keeping this
     * order is essential: calling A82C before every coordinate commit makes
     * CPU actors skate and prevents the stationary owner table from choosing
     * idle-dribble state 12. */
    cpu_resolve_locomotion_animation(tipoff, slot);
    NbaGameplayActorCommit commit = {
        .x_fraction = (uint16_t)((uint16_t)actor->x_fp << 8),
        .y_fraction = (uint16_t)((uint16_t)actor->y_fp << 8),
        .z_fraction = (uint16_t)((uint16_t)actor->z_fp << 8),
        .x = (int16_t)(actor->x_fp >> 8),
        .y = (int16_t)(actor->y_fp >> 8),
        .z = (int16_t)(actor->z_fp >> 8),
        .velocity_x = actor->velocity_x,
        .velocity_y = actor->velocity_y,
        .velocity_z = actor->velocity_z,
        .behavior_flags_raw_7e = actor->behavior_flags_raw,
        .speed_raw_4a = (uint16_t)(actor->movement_magnitude_raw << 1),
        .movement_distance_raw_4c = actor->movement_magnitude_raw,
        .facing_raw_4e = actor->movement_direction,
        .velocity_direction_raw_a2 = actor->velocity_direction_raw_a2
    };
    nba_gameplay_actor_commit(&commit, 2u, true);
    actor->x_fp = (int32_t)commit.x * 256 + (commit.x_fraction >> 8);
    actor->y_fp = (int32_t)commit.y * 256 + (commit.y_fraction >> 8);
    actor->z_fp = (int32_t)commit.z * 256 + (commit.z_fraction >> 8);
    actor->velocity_x = commit.velocity_x;
    actor->velocity_y = commit.velocity_y;
    actor->velocity_z = commit.velocity_z;
    actor->movement_magnitude_raw = commit.movement_distance_raw_4c;
    actor->velocity_direction_raw_a2 = commit.velocity_direction_raw_a2;
    actor->movement_direction = commit.facing_raw_4e;
}

static bool actor_animation_resources(const NbaTipoff *tipoff,
                                      const NbaTipoffActor *actor,
                                      uint8_t direction,
                                      uint16_t *upper_resource,
                                      uint16_t *lower_resource) {
    /* Adopted live-play passes share the rendered ROM phase with their hand
     * point and release gate. Inbound remains a separate integration boundary.
     * `$87:AEC3` also resolves a just-installed action before its first tick. */
    if (actor->control_mode == 15u && actor->exact_pass_animation &&
        actor->animation_resources_valid &&
        direction == actor->direction) {
        *upper_resource = actor->upper_animation_resource_raw_2a;
        *lower_resource = actor->lower_animation_resource_raw_2c;
        return true;
    }
    /* Other collision/contact routines still use their separately verified logical
     * tick representation. The exact +$2A/+$2C resources drive rendering;
     * switching physics here belongs with the remaining `$87:B649-$B952`
     * integration branch, not this animation-only increment. */
    return nba_player_animation_resources(
        tipoff->assets, actor->animation_state, actor->lower_animation_state,
        direction, actor->upper_animation_tick, actor->lower_animation_tick,
        upper_resource, lower_resource);
}

static void ball_position_at_actor(NbaTipoff *tipoff, unsigned owner) {
    /* `$87:B649`, `$87:B66A`, `$87:B832`, `$87:B953`: resolve the current independent upper
     * and lower resources, then compose their ROM attachment tables. */
    NbaTipoffActor *actor = &tipoff->actors[owner];
    uint8_t direction = actor->direction < 8u ? actor->direction : 0u;
    uint16_t upper_resource = 0u, lower_resource = 0u;
    int16_t offset_x = 0, offset_y = 0, offset_z = 0;
    uint16_t mirror_flags = direction < 3u ? 0x8000u : 0u;
    bool resolved = actor_animation_resources(
        tipoff, actor, direction, &upper_resource, &lower_resource) &&
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
    if (!actor_animation_resources(tipoff, actor, direction,
                                   &upper_resource, &lower_resource))
        return false;
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
    return actor_animation_resources(tipoff, actor, direction,
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

static bool cpu_actor_ball_contact_allowed(const NbaTipoffActor *actor) {
    /* `$86:CCCD-$CCF3` rejects behavior mode `+$5E` 7/8; `$86:CCFC-$CD08`
     * separately rejects nonzero contact inhibit `+$5A`. Neither gate tests
     * the selected upper-animation resource at `+$30`. */
    return actor && actor->contact_inhibit_raw_5a == 0u &&
           actor->control_mode != 7u && actor->control_mode != 8u;
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
    if (!cpu_actor_ball_contact_allowed(actor) ||
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
    if (!cpu_actor_ball_contact_allowed(actor) ||
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

static int16_t cpu_contact_mul_low(int16_t a, int16_t b) {
    return (int16_t)(uint16_t)((int32_t)a * (int32_t)b);
}

static int16_t cpu_contact_add(int16_t a, int16_t b) {
    return (int16_t)(uint16_t)((uint16_t)a + (uint16_t)b);
}

static int16_t cpu_contact_sub(int16_t a, int16_t b) {
    return (int16_t)(uint16_t)((uint16_t)a - (uint16_t)b);
}

static bool cpu_actor_in_contact_window(const NbaTipoffActor *actor) {
    int x = fp_round(actor->x_fp), y = fp_round(actor->y_fp);
    return x >= -378 && x < 379 && y >= -208 && y < 209;
}

/* `$86:C302-$C34B`: run after any defender velocity restoration performed
 * by `$86:C261-$C275`; its gate observes those restored values. */
static void cpu_nudge_contact_vertical(NbaTipoffActor *x,
                                       const NbaTipoffActor *y) {
    int16_t vx_delta = cpu_contact_sub(x->velocity_x, y->velocity_x);
    unsigned abs_vx = (unsigned)(vx_delta < 0 ? -vx_delta : vx_delta);
    if (abs_vx >= 0x80u) return;
    int16_t vy = x->velocity_y;
    unsigned abs_vy = (unsigned)(vy < 0 ? -vy : vy);
    if (abs_vy >= 0x100u) return;
    if (fp_round(x->y_fp) >= fp_round(y->y_fp))
        x->velocity_y = (int16_t)(abs_vy + 0x10u);
    else
        x->velocity_y = (int16_t)-(int16_t)(abs_vy + 0x20u);
}

/* `$86:BD41-$BF08` and `$86:BF0B-$C475` share the same signed fixed-point
 * collision projection. Teammates use three arithmetic shifts; opponents
 * use four through `$86:C34C`. `$85:F78B` contributes the wrapped low word
 * of each signed product to these formulas. */
static bool cpu_apply_player_contact_response(NbaTipoff *tipoff,
                                              unsigned x_slot,
                                              unsigned y_slot,
                                              unsigned shift,
                                              bool nudge_x_vertical) {
    NbaTipoffActor *x = &tipoff->actors[x_slot];
    NbaTipoffActor *y = &tipoff->actors[y_slot];
    if (!cpu_actor_in_contact_window(x) || !cpu_actor_in_contact_window(y))
        return false;
    int16_t rel_x = cpu_contact_sub(y->velocity_x, x->velocity_x);
    int16_t rel_y = cpu_contact_sub(y->velocity_y, x->velocity_y);
    if ((uint16_t)rel_x == 0u && (uint16_t)rel_y == 0u) return false;
    int16_t dx = cpu_contact_sub(fp_round(x->x_fp), fp_round(y->x_fp));
    int16_t dy = cpu_contact_sub(fp_round(x->y_fp), fp_round(y->y_fp));
    int16_t dot = cpu_contact_add(cpu_contact_mul_low(dx, rel_x),
                                  cpu_contact_mul_low(dy, rel_y));
    if (dot < 0) return false;

    int16_t radial = nba_gameplay_arithmetic_shift_right(dot, shift);
    int16_t cross = nba_gameplay_arithmetic_shift_right(
        cpu_contact_sub(cpu_contact_mul_low(dy, rel_x),
                        cpu_contact_mul_low(dx, rel_y)), shift);
    int16_t half = nba_gameplay_arithmetic_shift_right(radial, 1u);
    int16_t old_x_vx = x->velocity_x, old_x_vy = x->velocity_y;
    int16_t y_add_x = nba_gameplay_arithmetic_shift_right(
        cpu_contact_add(cpu_contact_mul_low(cross, dy),
                        cpu_contact_mul_low(half, dx)), shift);
    int16_t y_add_y = nba_gameplay_arithmetic_shift_right(
        cpu_contact_sub(cpu_contact_mul_low(half, dy),
                        cpu_contact_mul_low(cross, dx)), shift);
    y->velocity_x = cpu_contact_add(old_x_vx, y_add_x);
    y->velocity_y = cpu_contact_add(old_x_vy, y_add_y);
    x->velocity_x = cpu_contact_add(old_x_vx,
        nba_gameplay_arithmetic_shift_right(
            cpu_contact_mul_low(half, dx), shift));
    x->velocity_y = cpu_contact_add(old_x_vy,
        nba_gameplay_arithmetic_shift_right(
            cpu_contact_mul_low(half, dy), shift));

    /* `$86:C302-$C34B` prevents a nearly parallel opponent response from
     * leaving X sliding through Y on the vertical axis. */
    if (nudge_x_vertical) cpu_nudge_contact_vertical(x, y);
    return true;
}

static bool cpu_player_pose_special_contact(const NbaTipoff *tipoff,
                                            unsigned attacker,
                                            unsigned target) {
    const NbaTipoffActor *a = &tipoff->actors[attacker];
    const NbaTipoffActor *t = &tipoff->actors[target];
    if (a->animation_state != 0x38u ||
        a->upper_animation_phase_raw >= 3u || attacker / 5u == target / 5u ||
        tipoff->live_state_raw >= 0x80u) return false;
    int dy = fp_round(t->y_fp) - fp_round(a->y_fp);
    if (dy < -32 || dy > 32) return false;
    if (t->control_mode == 8u && t->special_contact_raw_56 != -1)
        return false;
    NbaGameplayPosePoint points[2];
    if (!cpu_actor_pose_points(tipoff, attacker, points)) return false;
    int px = points[0].x - fp_round(t->x_fp);
    int py = points[0].y - fp_round(t->y_fp);
    return px >= -7 && px <= 7 && py >= -7 && py <= 7;
}

static void cpu_record_player_contact(NbaTipoff *tipoff,
                                      unsigned actor_a,
                                      unsigned actor_b,
                                      uint32_t routine) {
    ++tipoff->player_contact_count_raw;
    tipoff->player_contact_actor_a_raw = (int8_t)actor_a;
    tipoff->player_contact_actor_b_raw = (int8_t)actor_b;
    tipoff->player_contact_routine_raw = routine;
}

static int16_t cpu_knockdown_velocity(int16_t source, uint8_t jitter) {
    int16_t projected = cpu_contact_add(
        nba_gameplay_arithmetic_shift_right(source, 1u),
        nba_gameplay_arithmetic_shift_right(source, 3u));
    return cpu_contact_add(projected, (int16_t)((int)jitter - 128));
}

/* `$86:C4FE-$C6AC`: normalize the native X=victim/Y=offender register
 * contract into the portable classifier. Its return value never gates the
 * caller's collision physics, but its direct `$07F6` mutation must occur at
 * the original call position. */
static void cpu_classify_player_contact(NbaTipoff *tipoff,
                                        unsigned victim_slot,
                                        unsigned offender_slot,
                                        uint16_t context_tag) {
    if (!tipoff || !tipoff->session || victim_slot >= 10u ||
        offender_slot >= 10u) return;
    const NbaTipoffActor *offender = &tipoff->actors[offender_slot];
    NbaGameplayContactFoulInput input = {
        (uint8_t)offender_slot,
        (uint8_t)victim_slot,
        (uint8_t)(offender_slot / 5u),
        (uint8_t)(offender_slot >= 5u ? 5u : 0u),
        offender->control_mode,
        offender->movement_magnitude_raw,
        offender->movement_boost_timer,
        tipoff->possession_actor,
        (int8_t)tipoff->shot_actor_raw_09c8,
        tipoff->camera_side_group_raw,
        tipoff->live_state_raw,
        tipoff->ball_activity_raw,
        tipoff->period_raw_0926,
        context_tag,
        tipoff->session->config.rules[0],
        tipoff->session->config.rules[1]
    };
    (void)nba_gameplay_foul_classify_contact(
        &tipoff->fouls, &tipoff->rng, &input, &tipoff->rim_raw_13e7);
}

/* `$86:BFBA-$C238` is a separate high-speed knockdown outcome, not part of
 * `$86:C239`'s ordinary elastic response. Register identities are normalized
 * so victim is stationary and hitter is moving. `$86:BFF2` calls the
 * fire-and-forget `$86:C4FE` classifier before any response writes. */
static bool cpu_try_player_knockdown_contact(NbaTipoff *tipoff,
                                             unsigned first_slot,
                                             unsigned second_slot) {
    unsigned victim_slot = first_slot, hitter_slot = second_slot;
    NbaTipoffActor *victim = &tipoff->actors[victim_slot];
    NbaTipoffActor *hitter = &tipoff->actors[hitter_slot];
    if (tipoff->fouls.free_throw_state_raw_0978 != 0u ||
        (victim->behavior_flags_raw & 1u) != 0u) return false;
    if (victim->movement_magnitude_raw != 0u) {
        victim_slot = second_slot;
        hitter_slot = first_slot;
        victim = &tipoff->actors[victim_slot];
        hitter = &tipoff->actors[hitter_slot];
    }
    if ((victim->behavior_flags_raw & 1u) != 0u ||
        victim->movement_magnitude_raw != 0u ||
        (hitter->behavior_flags_raw & 1u) != 0u ||
        hitter->movement_magnitude_raw < 0x0250u ||
        (nba_gameplay_rng_next(&tipoff->rng) & 7u) != 0u)
        return false;

    cpu_classify_player_contact(tipoff, victim_slot, hitter_slot, 0u);

    /* `$86:C476`: cancel receiver/pass state independently of the later
     * knockdown presentation. Mode 16 and active mode-8 records fall back to
     * the ordinary response. */
    if (victim->control_mode == 14u || victim->control_mode == 10u) {
        victim->contact_action_timer_raw_60 = 0u;
        victim->behavior_flags_raw = 0u;
        victim->actor_status_raw_28 = 0u;
        tipoff->pass_receiver_raw = -1;
    }
    if (victim->control_mode == 16u ||
        (victim->control_mode == 8u &&
         victim->special_contact_raw_56 == 0)) return false;

    victim->velocity_x = cpu_knockdown_velocity(
        hitter->velocity_x,
        (uint8_t)nba_gameplay_rng_next(&tipoff->rng));
    victim->velocity_y = cpu_knockdown_velocity(
        hitter->velocity_y,
        (uint8_t)nba_gameplay_rng_next(&tipoff->rng));
    victim->movement_magnitude_raw = actor_distance(
        victim->velocity_x, victim->velocity_y);
    hitter->velocity_x = 0;
    hitter->velocity_y = 0;
    hitter->movement_magnitude_raw = 0u;
    victim->recovery_inhibit_raw = 8u;
    hitter->recovery_inhibit_raw = 20u;

    uint16_t action = 0x35u;
    if (victim->movement_boost_timer != 0u &&
        victim->movement_magnitude_raw >= 0x03F0u &&
        (nba_gameplay_rng_next(&tipoff->rng) & 0x0Fu) == 0u)
        action = 0x36u;
    /* `$86:C0BF/$86:C10C`: both knockdown presentations make the second
     * context-$87 call after jitter/action RNG but before installing state. */
    cpu_classify_player_contact(tipoff, victim_slot, hitter_slot, 0x87u);
    tipoff->rim_raw_13e7 |= 0x0080u;
    victim->action_state = action;
    victim->animation_state = (uint8_t)action;
    victim->upper_animation_tick = 0u;
    victim->upper_animation_phase_raw = 0u;
    victim->contact_action_timer_raw_60 = action == 0x36u ? 174u : 30u;
    victim->contact_inhibit_raw_5a = victim->contact_action_timer_raw_60;
    victim->special_contact_raw_56 = action == 0x36u ? 0 : -1;
    victim->recovery_inhibit_raw = 0u;
    if (tipoff->live_state_raw != 1u) {
        if (tipoff->live_state_raw < 0x80u) tipoff->live_state_raw = 0u;
        tipoff->ball_activity_raw = 0u;
    }
    if (victim_slot == (unsigned)tipoff->possession_actor) {
        bool inhibit_hitter = action == 0x36u ||
            (nba_gameplay_rng_next(&tipoff->rng) & 3u) == 0u;
        if (inhibit_hitter) hitter->contact_inhibit_raw_5a = 10u;
        if (action == 0x36u) {
            tipoff->ball.velocity_z = 480;
            victim->velocity_z = 600;
        }
        tipoff->possession_actor = -1;
        tipoff->possession_team = -1;
        tipoff->deferred_shot_foul_phase_raw_0a02 = 1u;
        tipoff->ball.owner_actor = -1;
        tipoff->ball.state = NBA_BALL_LOOSE;
        tipoff->ball.velocity_x = nba_gameplay_arithmetic_shift_right(
            victim->velocity_x, 1u);
        tipoff->ball.velocity_y = nba_gameplay_arithmetic_shift_right(
            victim->velocity_y, 1u);
        tipoff->live_state_raw = 0u;
        /* `$0936=0/$093E=FFFF` returns native dispatch to ownerless-ball
         * pursuit. The host play label must follow that same boundary. */
        cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
    }
    victim->behavior_flags_raw |= 6u;
    victim->control_mode = 8u;
    victim->pass_direction_raw = 0u;
    uint8_t impact_direction = nba_gameplay_pass_direction(
        victim->velocity_x, victim->velocity_y, NULL);
    victim->movement_direction = impact_direction ^ 4u;
    victim->direction = victim->movement_direction;
    cpu_record_player_contact(
        tipoff, victim_slot, hitter_slot, 0x86BFBAu);
    return true;
}

/* `$86:C91E-$CB83`: animation-$38 pose contact. The normal CPU path calls
 * C4FE tag 0, selects CB84/CBA4 impulse physics, and may install action 35/36
 * plus the ROM's probabilistic owner drop. The human-attacker `$17C5` fork
 * skips the first classifier and contact counter; it is represented through
 * the existing controller assignments when CPU-vs-CPU is disabled. */
static void cpu_apply_pose_special_impulse(NbaTipoff *tipoff,
                                           unsigned attacker,
                                           unsigned target) {
    static const int16_t standard_impulse[8][2] = {
        {0, 592}, {418, 418}, {592, 0}, {418, -418},
        {0, -592}, {-418, -418}, {-592, 0}, {-418, 418}
    };
    static const int16_t alternate_impulse[8][2] = {
        {0, 504}, {356, 356}, {504, 0}, {356, -356},
        {0, -504}, {-356, -356}, {-504, 0}, {-356, 356}
    };
    NbaTipoffActor *a = &tipoff->actors[attacker];
    NbaTipoffActor *t = &tipoff->actors[target];
    uint8_t old_mode = t->control_mode;
    bool controls_active = !tipoff->cpu_vs_cpu;
    bool human_attacker = controls_active &&
                          a->controller_assignment_raw >= 0;
    bool alternate = human_attacker;
    if (!human_attacker) {
        cpu_classify_player_contact(tipoff, target, attacker, 0u);
        ++tipoff->team_pose_contact_count_raw[target / 5u];
        if (a->movement_boost_timer != 0u &&
            (nba_gameplay_rng_next(&tipoff->rng) & 0x0Fu) == 0u)
            alternate = true;
    }

    unsigned direction = a->direction & 7u; /* actor facing `+$4E` */
    const int16_t (*table)[2] = alternate ? alternate_impulse :
                                               standard_impulse;
    int16_t base_x = table[direction][0];
    int16_t base_y = table[direction][1];
    t->velocity_x = base_x;
    t->velocity_y = base_y;
    t->movement_magnitude_raw = actor_distance(t->velocity_x, t->velocity_y);
    tipoff->rim_raw_13e7 |= 0x0080u;

    uint16_t action = alternate ? 0x36u : 0x35u;
    bool human_target = controls_active &&
                        t->controller_assignment_raw >= 0;
    if (alternate && human_target) {
        t->velocity_x = cpu_contact_add(t->velocity_x, t->velocity_x);
        t->velocity_y = cpu_contact_add(t->velocity_y, t->velocity_y);
        t->movement_magnitude_raw = actor_distance(
            t->velocity_x, t->velocity_y);
    } else {
        if ((nba_gameplay_rng_next(&tipoff->rng) & 3u) != 0u) return;
        if (alternate) {
            cpu_classify_player_contact(tipoff, target, attacker, 0x87u);
        } else if ((nba_gameplay_rng_next(&tipoff->rng) & 0x0Fu) >= 6u) {
            cpu_classify_player_contact(tipoff, target, attacker, 0x87u);
        }
    }

    t->action_state = action;
    t->animation_state = (uint8_t)action;
    t->upper_animation_tick = 0u;
    t->upper_animation_phase_raw = 0u;
    t->special_contact_raw_56 = alternate ? 0 : -1;
    t->contact_action_timer_raw_60 = alternate ? 174u : 30u;
    t->contact_inhibit_raw_5a = t->contact_action_timer_raw_60;
    t->recovery_inhibit_raw = 0u;
    if (alternate) t->velocity_z = 600;
    if (alternate && target == (unsigned)tipoff->possession_actor) {
        a->contact_inhibit_raw_5a = 10u;
        tipoff->ball.velocity_z = 480;
    } else if (!alternate && attacker == (unsigned)tipoff->possession_actor) {
        a->contact_inhibit_raw_5a = 10u;
    }
    if (tipoff->live_state_raw != 1u) {
        if (tipoff->live_state_raw < 0x80u) tipoff->live_state_raw = 0u;
        tipoff->ball_activity_raw = 0u;
    }

    bool owner_drop =
        (nba_gameplay_rng_next(&tipoff->rng) & 3u) == 0u &&
        target == (unsigned)tipoff->possession_actor;
    if (owner_drop) {
        tipoff->possession_actor = -1;
        tipoff->possession_team = -1;
        tipoff->ball.owner_actor = -1;
        tipoff->ball.state = NBA_BALL_LOOSE;
        tipoff->ball.velocity_x = nba_gameplay_arithmetic_shift_right(
            base_x, 1u);
        tipoff->ball.velocity_y = nba_gameplay_arithmetic_shift_right(
            base_y, 1u);
        tipoff->deferred_shot_foul_phase_raw_0a02 = 1u;
        tipoff->live_state_raw = 0u;
        if (human_attacker) t->velocity_z = cpu_contact_add(
            t->velocity_z, t->velocity_z);
        cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
    }
    t->behavior_flags_raw |= 6u;
    if (old_mode == 14u || old_mode == 10u)
        tipoff->pass_receiver_raw = -1;
    t->control_mode = 8u;
    t->pass_direction_raw = 0u;
    uint8_t impact_direction = nba_gameplay_pass_direction(
        base_x, base_y, NULL);
    t->movement_direction = impact_direction ^ 4u;
    t->direction = t->movement_direction;
}

/* `$86:C88F-$C91D/$86:CBC4-$CCCC`: return false only when D652 must stop
 * scanning later sorted-X partners. A geometrically visited pair returns
 * true even when its Y/metric gates reject the actual response. */
static bool cpu_process_player_pair(NbaTipoff *tipoff,
                                    unsigned x_slot, unsigned y_slot) {
    NbaTipoffActor *x = &tipoff->actors[x_slot];
    NbaTipoffActor *y = &tipoff->actors[y_slot];
    if (tipoff->live_state_raw == 0x82u &&
        (x_slot == tipoff->inbound_actor_raw ||
         y_slot == tipoff->inbound_actor_raw ||
         x->control_mode == 3u || y->control_mode == 3u)) return false;
    int dx = fp_round(y->x_fp) - fp_round(x->x_fp);
    if (dx < 0) dx = -dx;
    if (dx < 33) {
        if (cpu_player_pose_special_contact(tipoff, x_slot, y_slot)) {
            cpu_apply_pose_special_impulse(tipoff, x_slot, y_slot);
            cpu_record_player_contact(tipoff, x_slot, y_slot, 0x86C91Eu);
            return true;
        }
        if (cpu_player_pose_special_contact(tipoff, y_slot, x_slot)) {
            cpu_apply_pose_special_impulse(tipoff, y_slot, x_slot);
            cpu_record_player_contact(tipoff, y_slot, x_slot, 0x86C91Eu);
            return true;
        }
    }
    int signed_dx = fp_round(y->x_fp) - fp_round(x->x_fp);
    if (signed_dx < -16 || signed_dx > 16) return false;
    int dy = fp_round(y->y_fp) - fp_round(x->y_fp);
    bool opponents = x_slot / 5u != y_slot / 5u;
    if (opponents) {
        if (dy < -16 || dy >= 16 || actor_distance(signed_dx, dy) >= 17u)
            return true;
        /* `$86:BFBA-$BFDB` can swap X/Y while normalizing a moving first
         * record for the knockdown probe. The symmetric ordinary impulse is
         * represented in source order, except that `$86:C23B` must see the
         * swapped defender when it is assigned to the current owner. */
        unsigned response_x_slot = x_slot;
        unsigned response_y_slot = y_slot;
        if ((x->behavior_flags_raw & 1u) == 0u &&
            x->movement_magnitude_raw != 0u &&
            tipoff->possession_actor >= 0 &&
            (unsigned)(y->assignment_current_raw >> 1) ==
                (unsigned)tipoff->possession_actor &&
            y->anchor_distance_raw < 0x90u &&
            (y->control_mode == 2u || y->control_mode == 4u ||
             y->control_mode == 6u)) {
            response_x_slot = y_slot;
            response_y_slot = x_slot;
        }
        if (cpu_try_player_knockdown_contact(tipoff, x_slot, y_slot))
            return true;
        int16_t saved_x[3] = {x->velocity_x, x->velocity_y, x->velocity_z};
        int16_t saved_y[3] = {y->velocity_x, y->velocity_y, y->velocity_z};
        NbaTipoffActor *response_x = &tipoff->actors[response_x_slot];
        NbaTipoffActor *response_y = &tipoff->actors[response_y_slot];
        bool x_defender = response_x->anchor_distance_raw < 0x90u &&
            (response_x->control_mode == 2u ||
             response_x->control_mode == 4u ||
             response_x->control_mode == 6u);
        bool y_defender = response_y->anchor_distance_raw < 0x90u &&
            (response_y->control_mode == 2u ||
             response_y->control_mode == 4u ||
             response_y->control_mode == 6u);
        bool assignment_case = tipoff->possession_actor >= 0 &&
            (unsigned)(response_x->assignment_current_raw >> 1) ==
                (unsigned)tipoff->possession_actor;
        bool responded = false;
        if (assignment_case && x_defender) {
            int16_t preserve[2] = {
                response_x->velocity_x, response_x->velocity_y};
            responded = cpu_apply_player_contact_response(
                tipoff, response_y_slot, response_x_slot, 4u, false);
            response_x->velocity_x = preserve[0];
            response_x->velocity_y = preserve[1];
            if (responded) {
                cpu_nudge_contact_vertical(response_y, response_x);
                response_y->recovery_inhibit_raw = 8u;
                response_x->recovery_inhibit_raw = 2u;
            }
        } else if (assignment_case && y_defender) {
            int16_t preserve[2] = {
                response_y->velocity_x, response_y->velocity_y};
            responded = cpu_apply_player_contact_response(
                tipoff, response_x_slot, response_y_slot, 4u, false);
            response_y->velocity_x = preserve[0];
            response_y->velocity_y = preserve[1];
            if (responded) {
                cpu_nudge_contact_vertical(response_x, response_y);
                response_x->recovery_inhibit_raw = 8u;
                response_y->recovery_inhibit_raw = 2u;
            }
        } else {
            responded = cpu_apply_player_contact_response(
                tipoff, response_x_slot, response_y_slot, 4u, true);
            if (responded) {
                /* `$86:C2C1-$C300` always installs the pair cooldowns.
                 * Even modes below seven use X=8/Y=2; odd or high modes
                 * reverse them to X=2/Y=8. */
                if (response_x->control_mode < 7u &&
                    (response_x->control_mode & 1u) == 0u) {
                    response_x->recovery_inhibit_raw = 8u;
                    response_y->recovery_inhibit_raw = 2u;
                } else {
                    response_x->recovery_inhibit_raw = 2u;
                    response_y->recovery_inhibit_raw = 8u;
                }
            }
        }
        if (responded) {
            cpu_record_player_contact(
                tipoff, response_x_slot, response_y_slot, 0x86BF0Bu);
            if (response_x->controller_assignment_raw >= 0)
                response_x->recovery_inhibit_raw = 0u;
            if (response_y->controller_assignment_raw >= 0)
                response_y->recovery_inhibit_raw = 0u;
        }
        if ((y->behavior_flags_raw & 1u) != 0u) {
            y->velocity_x = saved_y[0]; y->velocity_y = saved_y[1];
            y->velocity_z = saved_y[2];
        }
        if ((x->behavior_flags_raw & 1u) != 0u) {
            x->velocity_x = saved_x[0]; x->velocity_y = saved_x[1];
            x->velocity_z = saved_x[2];
        }
    } else {
        if (dy < -8 || dy >= 8 || actor_distance(signed_dx, dy) >= 8u)
            return true;
        if (cpu_apply_player_contact_response(
                tipoff, x_slot, y_slot, 3u, false)) {
            cpu_record_player_contact(tipoff, x_slot, y_slot, 0x86BD41u);
            x->recovery_inhibit_raw = 8u;
            y->recovery_inhibit_raw = 2u;
        }
    }
    return true;
}

static void cpu_update_player_contacts(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u) return;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        for (unsigned j = i + 1u; j < NBA_GAMEPLAY_ACTOR_COUNT; ++j) {
            if (!cpu_process_player_pair(tipoff, order[i], order[j])) break;
        }
    }
}

void nba_tipoff_replay_player_contact_order(NbaTipoff *tipoff,
                                            const uint8_t *order,
                                            unsigned count) {
    if (!tipoff || !order || count > NBA_GAMEPLAY_ACTOR_COUNT) return;
    for (unsigned i = 0; i < count; ++i) {
        if (order[i] >= NBA_GAMEPLAY_ACTOR_COUNT) return;
        for (unsigned j = i + 1u; j < count; ++j) {
            if (order[j] >= NBA_GAMEPLAY_ACTOR_COUNT ||
                !cpu_process_player_pair(tipoff, order[i], order[j])) break;
        }
    }
}

void nba_tipoff_replay_collision_order(NbaTipoff *tipoff,
                                       const uint8_t *order,
                                       unsigned count) {
    if (!tipoff || !order || count > NBA_GAMEPLAY_ACTOR_COUNT) return;
    tipoff->simulation_tick &= ~1u;
    nba_tipoff_replay_player_contact_order(tipoff, order, count);
    bool handled = cpu_try_detached_shot_contact(tipoff);
    if (!handled) handled = cpu_try_owned_ball_contact(tipoff);
    if (handled || tipoff->ball.owner_actor >= 0) return;
    int catcher = -1;
    if (tipoff->ball.state == NBA_BALL_PASS)
        catcher = cpu_first_pass_contact(tipoff);
    else if (tipoff->live_state_raw == 0x82u)
        catcher = cpu_first_inbound_ball_contact(tipoff);
    else
        catcher = cpu_first_loose_ball_contact(tipoff);
    if (catcher >= 0)
        cpu_commit_ball_acquisition(tipoff, (uint8_t)catcher);
}

void nba_tipoff_replay_player_contact_sweep(NbaTipoff *tipoff) {
    if (!tipoff) return;
    tipoff->simulation_tick &= ~1u;
    cpu_update_player_contacts(tipoff);
}

static bool cpu_player_contact_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        state.actors[i].x_fp = (int32_t)(100 + (int)i * 40) * 256;
        state.actors[i].y_fp = 0;
        state.actors[i].controller_assignment_raw = -1;
        state.actors[i].control_mode = 2u;
    }
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.actors[0].x_fp = 0; state.actors[5].x_fp = 8 * 256;
    state.actors[0].velocity_x = 100; state.actors[5].velocity_x = -100;
    if (!cpu_process_player_pair(&state, 0u, 5u) ||
        state.actors[0].velocity_x != 75 ||
        state.actors[0].velocity_y != 16 ||
        state.actors[5].velocity_x != 75 ||
        state.actors[5].velocity_y != 0 ||
        state.actors[0].recovery_inhibit_raw != 8u ||
        state.actors[5].recovery_inhibit_raw != 2u) return false;

    memset(&state, 0, sizeof(state));
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[5].controller_assignment_raw = -1;
    state.actors[0].control_mode = state.actors[5].control_mode = 2u;
    state.actors[5].y_fp = 8 * 256;
    state.actors[0].velocity_y = 100;
    state.actors[5].velocity_y = -100;
    if (!cpu_process_player_pair(&state, 0u, 5u) ||
        state.actors[0].velocity_x != 0 ||
        state.actors[0].velocity_y != -107 ||
        state.actors[5].velocity_x != 0 ||
        state.actors[5].velocity_y != 75) return false;

    memset(&state, 0, sizeof(state));
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[1].controller_assignment_raw = -1;
    state.actors[0].control_mode = state.actors[1].control_mode = 2u;
    state.actors[1].x_fp = 7 * 256;
    state.actors[0].velocity_x = 100;
    state.actors[1].velocity_x = -100;
    if (!cpu_process_player_pair(&state, 0u, 1u) ||
        state.actors[0].velocity_x != 23 ||
        state.actors[1].velocity_x != 23) return false;

    memset(&state, 0, sizeof(state));
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[5].controller_assignment_raw = -1;
    state.actors[0].control_mode = state.actors[5].control_mode = 2u;
    state.actors[5].x_fp = 8 * 256;
    state.actors[0].velocity_x = 100;
    state.actors[5].velocity_x = -100;
    state.actors[0].behavior_flags_raw = 1u;
    if (!cpu_process_player_pair(&state, 0u, 5u) ||
        state.actors[0].velocity_x != 100 ||
        state.actors[0].velocity_y != 0 ||
        state.actors[5].velocity_x != 75) return false;

    memset(&state, 0, sizeof(state));
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.actors[0].x_fp = 0; state.actors[5].x_fp = 17 * 256;
    if (cpu_process_player_pair(&state, 0u, 5u)) return false;
    state.actors[5].x_fp = 16 * 256; state.actors[5].y_fp = 4 * 256;
    if (!cpu_process_player_pair(&state, 0u, 5u)) return false;
    if (state.actors[0].velocity_x != 0 || state.actors[5].velocity_x != 0)
        return false; /* metric 17 visits but rejects response. */
    state.actors[1].x_fp = 7 * 256; state.actors[1].y_fp = 0;
    state.actors[0].velocity_x = 100; state.actors[1].velocity_x = -100;
    if (!cpu_process_player_pair(&state, 0u, 1u) ||
        state.actors[0].velocity_x == 100) return false;
    state.actors[0].velocity_x = 100; state.actors[0].velocity_y = 0;
    state.actors[1].velocity_x = -100; state.actors[1].velocity_y = 0;
    state.actors[1].x_fp = 8 * 256;
    if (!cpu_process_player_pair(&state, 0u, 1u) ||
        state.actors[0].velocity_x != 100) return false;
    state.live_state_raw = 0x82u; state.inbound_actor_raw = 0u;
    if (cpu_process_player_pair(&state, 0u, 1u)) return false;
    if (cpu_knockdown_velocity(640, 0x80u) != 400 ||
        cpu_knockdown_velocity(-320, 0x80u) != -200 ||
        actor_distance(400, -200) != 450u) return false;
    memset(&state, 0, sizeof(state));
    state.camera_side_group_raw = 0u;
    state.possession_actor = -1;
    state.actors[0].control_mode = 8u;
    state.actors[0].special_contact_raw_56 = -1;
    state.actors[0].contact_action_timer_raw_60 = 2u;
    state.actors[0].contact_inhibit_raw_5a = 30u;
    state.actors[0].behavior_flags_raw = 6u;
    state.actors[0].velocity_x = 400;
    state.actors[0].velocity_y = -200;
    if (!cpu_update_knockdown_actor(&state, 0u) ||
        state.actors[0].control_mode != 8u ||
        state.actors[0].contact_action_timer_raw_60 != 0u ||
        state.actors[0].contact_inhibit_raw_5a != 30u) return false;
    if (!cpu_update_knockdown_actor(&state, 0u) ||
        state.actors[0].control_mode != 1u ||
        state.actors[0].behavior_timer != 0x2Fu ||
        state.actors[0].contact_action_timer_raw_60 != 0u ||
        state.actors[0].contact_inhibit_raw_5a != 0u ||
        state.actors[0].behavior_flags_raw != 0u) return false;
    memset(&state, 0, sizeof(state));
    state.possession_actor = -1;
    state.actors[0].control_mode = 8u;
    state.actors[0].special_contact_raw_56 = 0;
    state.actors[0].contact_action_timer_raw_60 = 174u;
    state.actors[0].contact_inhibit_raw_5a = 174u;
    state.actors[0].pass_direction_raw = 0u;
    state.actors[0].velocity_x = 401;
    state.actors[0].velocity_y = -401;
    if (!cpu_update_knockdown_actor(&state, 0u) ||
        state.actors[0].velocity_x != 200 ||
        state.actors[0].velocity_y != -201 ||
        state.actors[0].velocity_z != 0x00F0 ||
        state.actors[0].pass_direction_raw != NBA_GAMEPLAY_UNKNOWN_WORD ||
        state.actors[0].contact_action_timer_raw_60 != 172u ||
        state.actors[0].contact_inhibit_raw_5a != 174u ||
        (state.rim_raw_13e7 & 0x0100u) == 0u) return false;
    state.actors[0].z_fp = 0;
    state.actors[0].velocity_z = 0;
    bool recovery_done = cpu_update_knockdown_actor(&state, 0u) &&
        state.actors[0].velocity_x == 0 &&
        state.actors[0].velocity_y == 0 &&
        state.actors[0].contact_action_timer_raw_60 == 170u;
    if (!recovery_done) return false;

    /* End-to-end `$86:BFBA-$C10C` RNG-order vector. Gate $20 passes,
     * context 0 rejects the $0250 speed, jitters consume $40/$80, and the
     * context-$87 classifier shifts live `$07F6` to $40 and awards code 1. */
    NbaSession session;
    memset(&state, 0, sizeof(state));
    memset(&session, 0, sizeof(session));
    state.session = &session;
    session.config.rules[0] = 45u;
    session.config.rules[1] = 25u;
    state.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.possession_actor = -1;
    state.shot_actor_raw_09c8 = -1;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[5].controller_assignment_raw = -1;
    state.actors[0].control_mode = 2u;
    state.actors[5].control_mode = 1u;
    state.actors[5].velocity_x = 640;
    state.actors[5].movement_magnitude_raw = 0x0250u;
    nba_gameplay_rng_seed(&state.rng, 0x0010u);
    nba_gameplay_foul_init(&state.fouls);
    bool high_speed_foul = cpu_try_player_knockdown_contact(&state, 0u, 5u) &&
        state.rng.state == 0x0040u &&
        state.fouls.foul_event_raw_0964 == NBA_GAMEPLAY_FOUL_DEFENSIVE &&
        state.fouls.team_fouls[1] == 1u &&
        state.fouls.personal_fouls[5] == 1u &&
        state.actors[0].control_mode == 8u;
    if (!high_speed_foul) return false;

    /* `$86:C99D-$CA49` standard impulse-only vector: tag 0 masks the
     * cached RNG to 1, B advances to 2 and skips all action writes. */
    memset(&state, 0, sizeof(state));
    memset(&session, 0, sizeof(session));
    state.session = &session;
    state.cpu_vs_cpu = true;
    state.possession_actor = -1;
    state.shot_actor_raw_09c8 = -1;
    state.actors[0].direction = 0u;
    state.actors[0].movement_magnitude_raw = 0x0300u;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[5].controller_assignment_raw = -1;
    nba_gameplay_rng_seed(&state.rng, 1u);
    nba_gameplay_foul_init(&state.fouls);
    cpu_apply_pose_special_impulse(&state, 0u, 5u);
    if (state.rng.state != 2u || state.actors[5].velocity_x != 0 ||
        state.actors[5].velocity_y != 592 ||
        state.actors[5].control_mode != 0u ||
        state.actors[5].contact_action_timer_raw_60 != 0u ||
        state.team_pose_contact_count_raw[1] != 1u ||
        (state.rim_raw_13e7 & 0x0080u) == 0u) return false;

    /* Standard action-35/owner-drop vector: B=8, C=16, D=32. The ball
     * receives half the undoubled CB84 base vector. */
    memset(&state, 0, sizeof(state));
    state.session = &session;
    state.cpu_vs_cpu = true;
    state.possession_actor = 5;
    state.possession_team = 1;
    state.ball.owner_actor = 5;
    state.ball.state = NBA_BALL_ATTACHED;
    state.actors[0].direction = 0u;
    state.actors[0].movement_magnitude_raw = 0x0300u;
    state.actors[0].controller_assignment_raw = -1;
    state.actors[5].controller_assignment_raw = -1;
    nba_gameplay_rng_seed(&state.rng, 4u);
    nba_gameplay_foul_init(&state.fouls);
    cpu_apply_pose_special_impulse(&state, 0u, 5u);
    return state.rng.state == 32u &&
        state.actors[5].velocity_x == 0 &&
        state.actors[5].velocity_y == 592 &&
        state.actors[5].action_state == 0x35u &&
        state.actors[5].control_mode == 8u &&
        state.actors[5].special_contact_raw_56 == -1 &&
        state.actors[5].contact_action_timer_raw_60 == 30u &&
        state.possession_actor == -1 && state.ball.owner_actor == -1 &&
        state.ball.state == NBA_BALL_LOOSE &&
        state.ball.velocity_x == 0 && state.ball.velocity_y == 296 &&
        state.deferred_shot_foul_phase_raw_0a02 == 1u;
}

/* `$86:CCFC-$D548`: contact against an attached ball is opponent-only and
 * has no body-box fallback. The current animation selects a strict 4- or
 * 12-unit pose-point cube; `$86:D035` then consumes the ROM's random gates.
 * A successful `+$3A` roll either installs the candidate through BAA2 or,
 * for animations $32/$33, carries the old owner's velocity into D43E's
 * loose-ball deflection response. */
static bool cpu_try_owned_ball_contact(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u ||
        tipoff->live_state_raw >= 0x80u ||
        tipoff->ball.state != NBA_BALL_ATTACHED ||
        tipoff->ball.owner_actor < 0 ||
        tipoff->ball.owner_actor >= NBA_GAMEPLAY_ACTOR_COUNT)
        return false;
    uint8_t owner = (uint8_t)tipoff->ball.owner_actor;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        uint8_t candidate = order[i];
        NbaTipoffActor *candidate_state = &tipoff->actors[candidate];
        if (candidate / 5u == owner / 5u) continue;
        uint8_t threshold = candidate_state->animation_state == 0x13u ?
                            12u : 4u;
        int point = cpu_actor_ball_contact_index(
            tipoff, candidate, false, threshold);
        if (point < 0) continue;
        uint8_t team = candidate >= 5u ? tipoff->session->right_team :
                                        tipoff->session->left_team;
        uint8_t contact_rating = 0u;
        if (!nba_player_gameplay_contact_rating(
                tipoff->assets, team, candidate_state->roster_slot,
                &contact_rating)) continue;
        bool foul_state_clear =
            tipoff->fouls.free_throw_state_raw_0978 == 0u &&
            tipoff->fouls.shooting_foul_raw_09bc == 0u &&
            tipoff->fouls.foul_event_raw_0964 == 0u &&
            tipoff->fouls.whistle_active_raw_09b6 == 0u;
        NbaGameplayOwnedContactResult result =
            nba_gameplay_owned_contact_attempt(
                &tipoff->rng, candidate_state->animation_state,
                (uint8_t)point, contact_rating,
                tipoff->session->config.main_values[2],
                tipoff->session->config.rules[0], foul_state_clear);
        if (result == NBA_GAMEPLAY_OWNED_CONTACT_NONE) continue;
        tipoff->collision_actor_a_raw = (int8_t)candidate;
        tipoff->collision_actor_b_raw = (int8_t)owner;
        if (result == NBA_GAMEPLAY_OWNED_CONTACT_FOUL) {
            tipoff->collision_routine_raw = 0x86D12Du;
            (void)nba_gameplay_foul_record_contact(
                &tipoff->fouls, NBA_GAMEPLAY_FOUL_DEFENSIVE,
                candidate, owner, candidate / 5u,
                tipoff->ball_activity_raw != 0u,
                tipoff->period_raw_0926);
            if (tipoff->ball_activity_raw != 0u)
                tipoff->rim_raw_13e7 |= 0x2000u;
            return true;
        }

        tipoff->collision_routine_raw = 0x86D1D9u;
        NbaTipoffActor *owner_state = &tipoff->actors[owner];
        owner_state->contact_inhibit_raw_5a = 15u;
        tipoff->ball.velocity_x = owner_state->velocity_x;
        tipoff->ball.velocity_y = owner_state->velocity_y;
        tipoff->ball.velocity_z = owner_state->velocity_z;
        uint8_t owner_group = owner >= 5u ? 5u : 0u;
        owner_state->control_mode =
            owner_group == tipoff->camera_side_group_raw ? 1u : 2u;
        owner_state->reaction_threshold = 0u;
        owner_state->behavior_flags_raw = 0u;
        tipoff->ball.owner_actor = -1;
        tipoff->possession_actor = -1;
        tipoff->pass_actor_raw = -1;
        tipoff->pass_receiver_raw = -1;
        tipoff->pass_active_raw = 0u;
        tipoff->pass_distance_raw = 0u;
        tipoff->ball_activity_raw = 0u;
        if (candidate_state->animation_state == 0x32u ||
            candidate_state->animation_state == 0x33u) {
            candidate_state->contact_inhibit_raw_5a = 15u;
            NbaGameplayRimState deflect = {
                fp_integer_word(tipoff->ball.x_fp),
                fp_integer_word(tipoff->ball.y_fp),
                fp_integer_word(tipoff->ball.z_fp),
                tipoff->ball.velocity_x, tipoff->ball.velocity_y,
                tipoff->ball.velocity_z
            };
            nba_gameplay_ball_apply_deflection(&deflect, &tipoff->rng);
            tipoff->ball.velocity_x = deflect.velocity_x;
            tipoff->ball.velocity_y = deflect.velocity_y;
            tipoff->ball.velocity_z = deflect.velocity_z;
            tipoff->ball.state = NBA_BALL_BOUNCE;
            tipoff->collision_routine_raw = 0x86D43Eu;
            cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
        } else {
            cpu_commit_ball_acquisition(tipoff, candidate);
        }
        return true;
    }
    return false;
}

static void cpu_record_interference_score(NbaTipoff *tipoff,
                                          uint8_t candidate,
                                          uint8_t shooter) {
    unsigned scoring_side = shooter / 5u;
    tipoff->rim_raw_096a = 0u;
    if (!nba_gameplay_foul_record_violation(
            &tipoff->fouls, NBA_GAMEPLAY_VIOLATION_INTERFERENCE,
            candidate, shooter)) return;
    tipoff->session->score[scoring_side] = (uint16_t)(
        tipoff->session->score[scoring_side] + tipoff->shot_value_raw);
    /* `$86:CE51-$CE65` updates the side-leading latch and the corresponding
     * lead-change counter only when the new award crosses the other score. */
    if (tipoff->leading_side_raw_1403 == 0u) {
        if (tipoff->session->score[1] > tipoff->session->score[0]) {
            tipoff->leading_side_raw_1403 = 1u;
            ++tipoff->right_lead_change_count_raw_1407;
        }
    } else if (tipoff->session->score[0] > tipoff->session->score[1]) {
        tipoff->leading_side_raw_1403 = 0u;
        ++tipoff->left_lead_change_count_raw_1405;
    }
    tipoff->shot_result_resolved = true;
    tipoff->collision_actor_a_raw = (int8_t)candidate;
    tipoff->collision_actor_b_raw = (int8_t)shooter;
    tipoff->collision_routine_raw = 0x86CE1Eu;
}

/* `$86:CD97-$D1D6`: an active detached shot on its downward path uses only
 * the strict eight-unit pose points. An opposing actor may trigger code-6
 * basket interference while the ball is above $50, then the same contact
 * continues into `$D078`'s ROM catch test. There is no body-box fallback. */
static bool cpu_try_detached_shot_contact(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u ||
        tipoff->live_state_raw >= 0x80u ||
        tipoff->ball.state != NBA_BALL_SHOT ||
        tipoff->ball.owner_actor >= 0 ||
        tipoff->ball_activity_raw == 0u ||
        tipoff->ball.velocity_z >= 0 ||
        tipoff->match_clock_raw_0928 < 5u ||
        tipoff->match_clock_raw_0928 >= 0xFF00u ||
        tipoff->shot_actor_raw_09c8 < 0 ||
        tipoff->shot_actor_raw_09c8 >= NBA_GAMEPLAY_ACTOR_COUNT)
        return false;

    uint8_t shooter = (uint8_t)tipoff->shot_actor_raw_09c8;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        uint8_t candidate = order[i];
        if (candidate / 5u == shooter / 5u) continue;
        int point = cpu_actor_ball_contact_index(
            tipoff, candidate, false, 8u);
        if (point < 0) continue;

        bool event_state_clear =
            tipoff->fouls.free_throw_state_raw_0978 == 0u &&
            tipoff->fouls.shooting_foul_raw_09bc == 0u &&
            tipoff->fouls.foul_event_raw_0964 == 0u &&
            tipoff->fouls.whistle_active_raw_09b6 == 0u;
        if (event_state_clear &&
            tipoff->session->config.rules[5] != 0u &&
            tipoff->rim_raw_096a != 0u &&
            fp_round(tipoff->ball.z_fp) >= 0x50) {
            cpu_record_interference_score(tipoff, candidate, shooter);
        }

        if (!nba_gameplay_detached_shot_contact_attempt(
                &tipoff->rng, (uint8_t)point,
                tipoff->rim_raw_097c != 0u)) continue;
        tipoff->collision_actor_a_raw = (int8_t)candidate;
        tipoff->collision_actor_b_raw = -1;
        tipoff->collision_routine_raw = 0x86D25Au;
        cpu_commit_ball_acquisition(tipoff, candidate);
        return true;
    }
    return false;
}

static bool cpu_generic_loose_contact_due(const NbaTipoff *tipoff) {
    if (!tipoff || (tipoff->simulation_tick & 1u) != 0u) return false;
    /* `$86:CD97-$D1D6` owns an active descending shot. It must finish that
     * classifier before the generic loose-ball `$D25A` acquisition route
     * becomes eligible, even if the host play label has become REBOUND. */
    return !(tipoff->ball.state == NBA_BALL_SHOT &&
             tipoff->ball_activity_raw != 0u &&
             tipoff->ball.velocity_z < 0);
}

static int cpu_first_loose_ball_contact(const NbaTipoff *tipoff) {
    if (!cpu_generic_loose_contact_due(tipoff)) return -1;
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        unsigned actor = order[i];
        /* `$86:CF01` doubles the generic loose-ball window from 8 to 16;
         * animation `$13` changes only the owned-ball strip window. */
        if (cpu_actor_contacts_ball(tipoff, actor, false, 16u))
            return (int)actor;
    }
    return -1;
}

static int cpu_first_receiverless_pass_contact(const NbaTipoff *tipoff) {
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    unsigned inbound_base =
        tipoff->inbound_state_raw == 5u ? 5u : 0u;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        unsigned actor = order[i];
        /* `$82` remains a dead-ball context after A613 erases `$0946`.
         * Recovery is still restricted to the inbound context; allowing an
         * opponent to win this now-receiverless pass installs the wrong
         * mode-11 carrier and can repeat five-second violations forever. */
        if (tipoff->live_state_raw == 0x82u &&
            (actor < inbound_base || actor >= inbound_base + 5u))
            continue;
        if (cpu_actor_contacts_ball(tipoff, actor, false, 16u))
            return (int)actor;
    }
    return -1;
}

/* During live state `$82`, only the inbound side may recover the dead ball.
 * The collision winner replaces the provisional team-slot-2 `$0954`, as the
 * CPU oracle does when side-0 actor 3 becomes the actual inbounder. */
static int cpu_first_inbound_ball_contact(const NbaTipoff *tipoff) {
    /* `$86:D5DB/D652` is the post-actor-pass 30-Hz collision sweep. */
    if ((tipoff->simulation_tick & 1u) != 0u) return -1;
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

static bool cpu_is_raw_pass_receiver(const NbaTipoff *tipoff,
                                     unsigned actor) {
    return tipoff && tipoff->pass_receiver_raw >= 0 &&
           actor == (unsigned)tipoff->pass_receiver_raw;
}

/* `$86:CE88-$D1D6`: a detached pass rejects same-side nonreceivers. The
 * intended receiver uses radius 16; an opposing player uses the radius-12
 * branch at `$86:CEE2-$CEFF`. At Z<24 the ROM computes a full rating into
 * DP $AA but accidentally compares RNG with DP $00, the hit-point index:
 * point 0 can never win and point 1 wins only for RNG byte zero. */
static int cpu_first_pass_contact(NbaTipoff *tipoff) {
    if ((tipoff->simulation_tick & 1u) != 0u ||
        tipoff->ball.state != NBA_BALL_PASS) return -1;
    /* `$86:CEE2-$CF01`: once `$86:A613` has cleared `$0946`, the detached
     * pass is no longer receiver/opponent classified. Every actor instead
     * uses the deterministic generic 16-unit pose/body contact branch. */
    if (tipoff->pass_receiver_raw < 0)
        return cpu_first_receiverless_pass_contact(tipoff);
    uint8_t order[NBA_GAMEPLAY_ACTOR_COUNT];
    cpu_actor_contact_order(tipoff, order);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        unsigned actor = order[i];
        /* `$0946` is authoritative. `receiver_actor` is a host convenience
         * cache and can intentionally lag while the raw pass state clears. */
        bool receiver = cpu_is_raw_pass_receiver(tipoff, actor);
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

    NbaTipoffActor actor = {0};
    actor.animation_state = 7u;
    actor.control_mode = 2u;
    if (!cpu_actor_ball_contact_allowed(&actor)) return false;
    actor.animation_state = 0u;
    actor.control_mode = 7u;
    if (cpu_actor_ball_contact_allowed(&actor)) return false;
    actor.control_mode = 8u;
    if (cpu_actor_ball_contact_allowed(&actor)) return false;
    actor.control_mode = 2u;
    actor.contact_inhibit_raw_5a = 1u;
    if (cpu_actor_ball_contact_allowed(&actor)) return false;

    memset(&state, 0, sizeof(state));
    state.simulation_tick = 1u;
    if (cpu_generic_loose_contact_due(&state)) return false;
    state.simulation_tick = 2u;
    if (!cpu_generic_loose_contact_due(&state)) return false;
    state.ball.state = NBA_BALL_SHOT;
    state.ball_activity_raw = 1u;
    state.ball.velocity_z = -1;
    if (cpu_generic_loose_contact_due(&state)) return false;
    state.ball.velocity_z = 0;
    if (!cpu_generic_loose_contact_due(&state)) return false;
    state.ball.velocity_z = -1;
    state.ball_activity_raw = 0u;
    if (!cpu_generic_loose_contact_due(&state)) return false;

    /* `$0946`, rather than the host receiver cache, classifies a pass. */
    state.pass_receiver_raw = 3;
    state.receiver_actor = 7u;
    if (!cpu_is_raw_pass_receiver(&state, 3u) ||
        cpu_is_raw_pass_receiver(&state, 7u)) return false;
    state.pass_receiver_raw = -1;
    if (cpu_is_raw_pass_receiver(&state, 3u)) return false;
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
    tipoff->shot_actor_raw_09c8 = -1;
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

static int16_t mode13_velocity_component(int32_t delta_fp) {
    int32_t velocity = delta_fp / 40;
    if (velocity > INT16_MAX) velocity = INT16_MAX;
    if (velocity < INT16_MIN) velocity = INT16_MIN;
    return (int16_t)velocity;
}

/* `$86:B34F-$B624`: install the distinct carried-ball close-finish state.
 * The selector/resource tables are ROM data at `$86:B430-$B467`. Pose
 * attachment comes from the asset pack before the 40-tick trajectory is
 * solved, so this does not depend on captured emulator art. */
static bool cpu_start_rom_layup(NbaTipoff *tipoff, unsigned slot) {
    static const uint8_t upper_table[8] = {
        0x18u, 0x1Cu, 0x18u, 0x1Au, 0x19u, 0x1Bu, 0x1Du, 0x1Eu
    };
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (fp_integer_word(actor->z_fp) != 0 || actor->velocity_z != 0 ||
        actor->anchor_distance_raw >= 0x69u ||
        actor->movement_magnitude_raw < 0x0100u) return false;

    uint8_t team = slot >= 5u ? tipoff->session->right_team :
                               tipoff->session->left_team;
    uint8_t profile_39 = 0u, unused_3e = 0u;
    if (!nba_player_gameplay_pass_profiles(
            tipoff->assets, team, actor->roster_slot,
            &profile_39, &unused_3e)) return false;
    uint8_t anchor_direction = (uint8_t)((actor->anchor_direction_raw >> 1) & 7u);
    bool force_selector_seven = false;
    if (profile_39 < 0x4Cu) {
        if (anchor_direction == 0u || anchor_direction == 4u) return false;
        force_selector_seven = true;
    } else if (profile_39 < 0x54u) {
        if ((nba_gameplay_rng_next(&tipoff->rng) & 1u) == 0u) {
            if (anchor_direction == 0u || anchor_direction == 4u) return false;
            force_selector_seven = true;
        }
    } else if (profile_39 < 0x5Cu) {
        if ((nba_gameplay_rng_next(&tipoff->rng) & 3u) == 0u) {
            if (anchor_direction == 0u || anchor_direction == 4u) return false;
            force_selector_seven = true;
        }
    }
    uint8_t relative = (uint8_t)((anchor_direction - actor->direction) & 7u);
    if (relative != 0u && relative != 1u && relative != 7u) return false;

    uint8_t variant = (uint8_t)((nba_gameplay_rng_next(&tipoff->rng) & 3u) << 1);
    uint8_t selector;
    for (;;) {
        selector = force_selector_seven ? 7u :
            (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 7u);
        force_selector_seven = false;
        /* In the B678 caller DP B2 is team group 0/5, so selector one and
         * cardinal selector seven both execute B47E and reroll. */
        if (selector == 1u ||
            (selector == 7u &&
             (anchor_direction == 0u || anchor_direction == 4u)))
            continue;
        break;
    }

    actor->contact_action_timer_raw_60 = 0x28u;
    actor->control_mode = 13u;
    actor->special_contact_raw_56 = (int16_t)selector - 1;
    actor->mode13_variant_raw_58 = variant;
    actor->pass_direction_raw = upper_table[selector];
    actor->direction = anchor_direction;
    actor->requested_direction = anchor_direction;
    actor->movement_direction = anchor_direction;
    actor->z_fp = 1;
    actor->velocity_z = 0;
    actor_set_animation(actor, upper_table[selector], 0x1Fu);
    ball_attach_to_actor(tipoff, slot);
    actor->velocity_x = mode13_velocity_component(
        (int32_t)basket_x_for_side(tipoff->offense_side) * 256 -
        tipoff->ball.x_fp);
    actor->velocity_y = mode13_velocity_component(-tipoff->ball.y_fp);
    actor->mode13_baseline_velocity_x = actor->velocity_x;
    actor->mode13_baseline_velocity_y = actor->velocity_y;
    actor->movement_magnitude_raw = actor_distance(
        actor->velocity_x, actor->velocity_y);
    actor->behavior_flags_raw |= 0x0006u;
    if ((nba_gameplay_rng_next(&tipoff->rng) & 0x0030u) != 0u)
        actor->behavior_flags_raw |= 0x0001u;
    tipoff->live_state_raw = 2u;
    cpu_enter_play_state(tipoff, NBA_CPU_PLAY_SHOT);
    return true;
}

/* `$85:B678-$B8CA`: preserve the ROM's three-way return contract, both
 * clock urgency gates, blocked-lane rating tail, and clear-lane mode-13
 * route. */
static CpuMode11Outcome cpu_dispatch_rom_mode11(
        NbaTipoff *tipoff, unsigned slot, uint8_t *direction) {
    if (!tipoff || slot >= NBA_GAMEPLAY_ACTOR_COUNT)
        return CPU_MODE11_NORMAL_RETURN;
    NbaTipoffActor *actor = &tipoff->actors[slot];
    int16_t rom_x = fp_round(actor->x_fp);
    int16_t y = fp_round(actor->y_fp);
    int16_t z = fp_round(actor->z_fp);
    if (rom_x < -338 || rom_x >= 338) return CPU_MODE11_NORMAL_RETURN;

    if (tipoff->rim_raw_092c < 120u ||
        tipoff->match_clock_raw_0928 < 120u) {
        uint16_t random = nba_gameplay_rng_next(&tipoff->rng) & 0x7FFFu;
        if ((random & 0x0008u) == 0u) {
            if (z != 0) return CPU_MODE11_NORMAL_RETURN;
            return cpu_start_rom_shot(tipoff, slot) ?
                CPU_MODE11_SHOT_STARTED : CPU_MODE11_NORMAL_RETURN;
        }
        if (direction) *direction = cpu_formation_target_direction(tipoff, slot);
        return CPU_MODE11_CONSUMED_ACTION;
    }

    int16_t side_anchor = slot < 5u ? -336 : 336;
    bool same_attack_half = (int16_t)(rom_x ^ side_anchor) >= 0;
    if (cpu_lane_to_basket_is_clear(tipoff, slot)) {
        if (actor->anchor_distance_raw >= 0x70u) {
            if (direction) *direction =
                cpu_formation_target_direction(tipoff, slot);
            return CPU_MODE11_CONSUMED_ACTION;
        }
        if (cpu_start_rom_layup(tipoff, slot))
            return CPU_MODE11_CONSUMED_ACTION;
        uint16_t random = nba_gameplay_rng_next(&tipoff->rng) & 0x7FFFu;
        if ((random & 0x00F8u) == 0u) {
            if (z != 0) return CPU_MODE11_NORMAL_RETURN;
            return cpu_start_rom_shot(tipoff, slot) ?
                CPU_MODE11_SHOT_STARTED : CPU_MODE11_NORMAL_RETURN;
        }
        if (direction) *direction = cpu_formation_target_direction(tipoff, slot);
        return CPU_MODE11_CONSUMED_ACTION;
    }

    if (!same_attack_half) return CPU_MODE11_NORMAL_RETURN;
    if (same_attack_half &&
        nba_gameplay_mode11_shot_rectangle(rom_x, y, z))
        return cpu_start_rom_shot(tipoff, slot) ?
            CPU_MODE11_SHOT_STARTED : CPU_MODE11_NORMAL_RETURN;

    uint8_t team = slot >= 5u ? tipoff->session->right_team :
                               tipoff->session->left_team;
    uint8_t rating_36 = 0u, rating_37 = 0u, range_49 = 0u;
    if (!nba_player_gameplay_shot_ratings(
            tipoff->assets, team, actor->roster_slot,
            &rating_36, &rating_37) ||
        !nba_player_gameplay_shot_range(
            tipoff->assets, team, actor->roster_slot, &range_49))
        return CPU_MODE11_NORMAL_RETURN;
    NbaGameplayMode11ShotInput decision = {
        .play_step_raw_0998 = tipoff->play_step_raw,
        .play_cycle_raw_09a4 = tipoff->play_cycle_raw,
        .play_hold_raw_09d0 = tipoff->play_hold_raw,
        .dead_ball_raw_0968 = tipoff->dead_ball_raw_0968,
        .shot_clock_rule_raw_17e1 = tipoff->session->config.rules[8],
        .difficulty_raw_17af = tipoff->session->config.main_values[2],
        .assignment_distance_raw_8a = actor->assignment_distance,
        .anchor_distance_raw_8c = actor->anchor_distance_raw,
        .two_point_rating_raw_36 = rating_36,
        .three_point_rating_raw_37 = rating_37,
        .shot_range_raw_49 = range_49,
        .actor_z = z,
        .same_attack_half = same_attack_half
    };
    if (nba_gameplay_mode11_shot_decision(&decision, &tipoff->rng))
        return cpu_start_rom_shot(tipoff, slot) ?
            CPU_MODE11_SHOT_STARTED : CPU_MODE11_NORMAL_RETURN;
    return CPU_MODE11_NORMAL_RETURN;
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
    /* `$86:9DBF/$9DFF` preserve the shooter and shot value for the later
     * detached-contact/interference dispatcher. */
    tipoff->shot_actor_raw_09c8 = (int16_t)slot;
    tipoff->rim_raw_096a = tipoff->shot_value_raw;
    nba_gameplay_shot_launch(tipoff->ball.x_fp,
        tipoff->ball.y_fp, tipoff->ball.z_fp,
        (int16_t)basket_x, (int16_t)basket_y,
        &tipoff->ball.velocity_x, &tipoff->ball.velocity_y,
        &tipoff->ball.velocity_z);
    tipoff->ball.owner_actor = -1;
    tipoff->ball.state = NBA_BALL_SHOT;
    tipoff->possession_actor = -1;
    if (tipoff->fouls.shooting_foul_raw_09bc != 0u)
        tipoff->deferred_shot_foul_phase_raw_0a02 = 1u;
    shooter->control_mode = 11u;
    shooter->reaction_threshold = 0u;
    shooter->behavior_flags_raw = 0u;
    actor_set_upper_animation(shooter, 0x17u);
}

/* `$86:A9D0-$AA69`: shared terminal close-finish release used by behavior
 * modes 13 and 14. It seeds a two-point detached ball; the ordinary rim
 * engine remains solely responsible for awarding the basket. */
static void cpu_finish_rom_close_shot(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *actor = &tipoff->actors[slot];
    tipoff->shot_actor_raw_09c8 = (int16_t)slot;
    tipoff->shot_origin_x = fp_round(actor->x_fp);
    tipoff->shot_origin_y = fp_round(actor->y_fp);
    tipoff->shot_value_raw = 2u;
    tipoff->rim_raw_096a = 2u;
    tipoff->shot_inner_veto_raw = false;
    tipoff->shot_miss_index_raw = 0xFFu;
    tipoff->shot_result_resolved = false;
    tipoff->live_state_raw = 1u;
    tipoff->ball_activity_raw = 1u;
    tipoff->ball.owner_actor = -1;
    tipoff->ball.state = NBA_BALL_SHOT;
    tipoff->possession_actor = -1;
    cpu_enter_play_state(tipoff, NBA_CPU_PLAY_SHOT);
    actor->control_mode = 1u;
    actor->contact_inhibit_raw_5a = 0x14u;
    actor->behavior_flags_raw = 0u;
    if (actor->special_contact_raw_56 == 6) {
        tipoff->ball.velocity_x = tipoff->ball.x_fp < 0 ? -0x01A0 : 0x01A0;
        tipoff->ball.velocity_y = tipoff->ball.y_fp < 0 ? 0x0080 : -0x0080;
        tipoff->ball.velocity_z = 0x0048;
    } else {
        tipoff->ball.velocity_x = 0;
        tipoff->ball.velocity_y = 0;
        tipoff->ball.velocity_z = (int16_t)0xFE98u;
    }
}

/* `$87:9BD3[13] -> $87:9C49 -> $86:A7DA-$AA69`: carried-ball close
 * finish. It owns movement, vertical physics, pose attachment and both
 * release exits, so generic mode-11 steering must never run over it. */
static bool cpu_update_rom_layup(NbaTipoff *tipoff, unsigned slot) {
    static const int8_t turn_negative[13] = {
        1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 7, 7, 0
    };
    static const int8_t turn_zero[13] = {
        4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0
    };
    NbaTipoffActor *actor = &tipoff->actors[slot];
    if (tipoff->possession_actor != (int8_t)slot) return true;

    tipoff->ball_activity_raw = 1u;
    bool airborne = fp_integer_word(actor->z_fp) != 0;
    int dx = actor->velocity_x - actor->mode13_baseline_velocity_x;
    int dy = actor->velocity_y - actor->mode13_baseline_velocity_y;
    bool disturbed = dx < -0x50 || dx >= 0x51 ||
                     dy < -0x50 || dy >= 0x51;
    bool wrong_upper = actor->animation_state < 0x18u ||
                       actor->animation_state >= 0x1Fu;
    if (airborne && (wrong_upper ||
        (actor->contact_action_timer_raw_60 >= 0x12u && disturbed))) {
        actor->direction = (uint8_t)((actor->anchor_direction_raw >> 1) & 7u);
        actor_set_animation(actor, 0x17u, 0u);
        ball_attach_to_actor(tipoff, slot);
        tipoff->shot_origin_x = fp_round(actor->x_fp);
        tipoff->shot_origin_y = fp_round(actor->y_fp);
        cpu_release_rom_shot(tipoff, slot);
        cpu_integrate_actor_vertical(actor);
        actor->x_fp += (int32_t)actor->velocity_x * 2;
        actor->y_fp += (int32_t)actor->velocity_y * 2;
        return true;
    }

    ball_attach_to_actor(tipoff, slot);
    uint16_t timer = actor->contact_action_timer_raw_60;
    if ((timer == 0x24u || (timer < 0x24u && !airborne)) &&
        actor->velocity_z == 0) {
        actor->velocity_z =
            (actor->special_contact_raw_56 == 0 ||
             (actor->special_contact_raw_56 == 6 && actor->direction == 3u)) ?
            0x0270 : 0x0264;
    }
    timer = timer >= 2u ? (uint16_t)(timer - 2u) : 0u;
    actor->contact_action_timer_raw_60 = timer;

    if (timer < 0x24u && actor->special_contact_raw_56 <= 0) {
        if (timer <= 8u) {
            actor->direction = actor->special_contact_raw_56 < 0 ?
                actor->requested_direction :
                (uint8_t)(actor->requested_direction ^ 4u);
        } else {
            unsigned index = (unsigned)((timer - 10u) / 2u);
            if (index > 12u) index = 12u;
            int8_t turn = actor->special_contact_raw_56 < 0 ?
                turn_negative[index] : turn_zero[index];
            actor->direction = (uint8_t)((actor->requested_direction + turn) & 7u);
        }
    }

    actor->x_fp += (int32_t)actor->velocity_x * 2;
    actor->y_fp += (int32_t)actor->velocity_y * 2;
    cpu_integrate_actor_vertical(actor);
    ball_attach_to_actor(tipoff, slot);
    tipoff->ball.velocity_z = (int16_t)0xFE98u;
    if (timer != 0u) return true;

    cpu_finish_rom_close_shot(tipoff, slot);
    return true;
}

/* `$87:9BD3[14] -> $87:9C4E -> $86:B154-$B334`: special receiver close
 * finish. Unlike mode 10, this receiver preserves its pass relationship,
 * jumps before the catch, and can finish directly after ownership changes.
 * The executor itself consumes no RNG. */
static bool cpu_update_rom_special_receiver(NbaTipoff *tipoff, unsigned slot) {
    static const uint8_t upper_table[8] = {
        0x18u, 0x1Cu, 0x18u, 0x1Au, 0x19u, 0x1Bu, 0x1Du, 0x1Eu
    };
    static const int8_t facing_table[13] = {
        4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0
    };
    NbaTipoffActor *actor = &tipoff->actors[slot];

    /* `$85:963D` commits common planar/Z physics before the behavior jump. */
    actor->x_fp += (int32_t)actor->velocity_x * 2;
    actor->y_fp += (int32_t)actor->velocity_y * 2;
    cpu_integrate_actor_vertical(actor);
    actor->movement_magnitude_raw = actor_distance(
        actor->velocity_x, actor->velocity_y);

    uint16_t timer = (uint16_t)(actor->reaction_threshold - 2u);
    actor->reaction_threshold = timer;
    if (timer == 0u || (timer & 0x8000u) != 0u) {
        if (tipoff->possession_actor == (int8_t)slot) {
            tipoff->ball_activity_raw = 1u;
            ball_position_at_actor(tipoff, slot);
            cpu_finish_rom_close_shot(tipoff, slot);
        } else {
            tipoff->rim_force_raw_1866 = 0u;
            cpu_restore_normal_mode(tipoff, slot);
            cpu_cancel_rom_pass_activity(tipoff);
        }
        return true;
    }

    bool airborne = fp_integer_word(actor->z_fp) != 0;
    if (airborne && tipoff->possession_actor == (int8_t)slot) {
        bool wrong_upper = actor->animation_state < 0x18u ||
                           actor->animation_state >= 0x1Fu;
        int dx = actor->velocity_x - actor->mode13_baseline_velocity_x;
        int dy = actor->velocity_y - actor->mode13_baseline_velocity_y;
        bool disturbed = timer >= 0x12u &&
            (dx < -0x50 || dx > 0x50 || dy < -0x50 || dy > 0x50);
        if (wrong_upper || disturbed) {
            cpu_restore_normal_mode(tipoff, slot);
            actor->direction = (uint8_t)((actor->anchor_direction_raw >> 1) & 7u);
            actor_set_upper_animation(actor, 0x17u);
            ball_attach_to_actor(tipoff, slot);
            tipoff->ball_activity_raw = 0xFFFFu;
            tipoff->shot_origin_x = fp_round(actor->x_fp);
            tipoff->shot_origin_y = fp_round(actor->y_fp);
            cpu_enter_play_state(tipoff, NBA_CPU_PLAY_SHOT);
            cpu_release_rom_shot(tipoff, slot);
            return true;
        }
    }

    if (!airborne) {
        if (timer >= 0x28u) return true;
        if (actor->animation_state < 0x18u || actor->animation_state >= 0x1Fu) {
            int selector = actor->special_contact_raw_56 + 1;
            if (selector < 0 || selector > 7) selector = 0;
            actor->pass_direction_raw = upper_table[selector];
            actor_set_animation(actor, (uint8_t)actor->pass_direction_raw,
                                0x1Fu);
            return true;
        }
        if (actor->animation_state < 0x24u) {
            actor->velocity_z =
                (actor->special_contact_raw_56 == 0 ||
                 (actor->special_contact_raw_56 == 6 &&
                  actor->direction == 3u)) ? 0x0270 : 0x0264;
        }
        return true;
    }

    if (actor->special_contact_raw_56 != 0) {
        actor->direction = actor->requested_direction;
    } else if (timer <= 8u) {
        actor->direction = (uint8_t)(actor->requested_direction ^ 4u);
    } else {
        unsigned index = (unsigned)((timer - 10u) / 2u);
        if (index > 12u) index = 12u;
        actor->direction = (uint8_t)(
            (actor->requested_direction + facing_table[index]) & 7u);
    }

    bool relationship_valid;
    if (tipoff->possession_actor < 0) {
        relationship_valid = tipoff->pass_receiver_raw == (int16_t)slot;
    } else if (tipoff->possession_actor == (int8_t)slot) {
        relationship_valid = true;
    } else {
        unsigned owner = (unsigned)tipoff->possession_actor;
        relationship_valid = owner < NBA_GAMEPLAY_ACTOR_COUNT &&
            tipoff->actors[owner].control_mode == 15u &&
            tipoff->pass_receiver_raw == (int16_t)slot;
    }
    if (!relationship_valid) {
        tipoff->rim_force_raw_1866 = 0u;
        cpu_restore_normal_mode(tipoff, slot);
        cpu_cancel_rom_pass_activity(tipoff);
        return true;
    }
    if (tipoff->possession_actor == (int8_t)slot) {
        tipoff->live_state_raw = 2u;
        if (actor->special_contact_raw_56 != 6)
            tipoff->rim_force_raw_1866 =
                (uint16_t)(0x34EBu + slot * 0x100u);
        ball_attach_to_actor(tipoff, slot);
    }
    return true;
}

static bool cpu_special_receiver_self_test(void) {
    NbaTipoff state;
    NbaSession session;
    uint16_t seed;

    memset(&state, 0, sizeof(state));
    memset(&session, 0, sizeof(session));
    state.session = &session;
    state.possession_actor = -1;
    state.pass_actor_raw = 3;
    state.pass_aux_raw = 4;
    state.pass_receiver_raw = 1;
    state.ball_activity_raw = 1u;
    state.rim_raw_094a = 1u;
    state.inbound_transfer_raw = 1u;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 2u;
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    seed = state.rng.state;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 1u ||
        state.actors[0].behavior_timer != 0x2Fu ||
        state.pass_actor_raw != -1 || state.pass_aux_raw != -1 ||
        state.pass_receiver_raw != -1 || state.ball_activity_raw != 0u ||
        state.rim_raw_094a != 0u || state.inbound_transfer_raw != 0u ||
        state.rng.state != seed) return false;

    memset(&state, 0, sizeof(state));
    state.session = &session;
    state.possession_actor = 0;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 2u;
    state.actors[0].special_contact_raw_56 = 1;
    state.ball.owner_actor = 0;
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    seed = state.rng.state;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 1u ||
        state.possession_actor != -1 || state.ball.owner_actor != -1 ||
        state.ball.state != NBA_BALL_SHOT || state.shot_value_raw != 2u ||
        state.ball.velocity_x != 0 || state.ball.velocity_y != 0 ||
        state.ball.velocity_z != (int16_t)0xFE98u ||
        state.rng.state != seed) return false;

    memset(&state, 0, sizeof(state));
    state.session = &session;
    state.possession_actor = 0;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 2u;
    state.actors[0].special_contact_raw_56 = 6;
    state.actors[0].x_fp = -256;
    state.ball.owner_actor = 0;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.ball.velocity_x != -0x01A0 ||
        state.ball.velocity_y != -0x0080 ||
        state.ball.velocity_z != 0x0048) return false;

    memset(&state, 0, sizeof(state));
    state.session = &session;
    state.possession_actor = -1;
    state.pass_receiver_raw = 0;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 40u;
    state.actors[0].animation_state = 0x18u;
    state.actors[0].special_contact_raw_56 = 0;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].reaction_threshold != 38u ||
        state.actors[0].velocity_z != 0x0270) return false;
    state.actors[0].velocity_z = 0;
    state.actors[0].z_fp = 0;
    state.actors[0].reaction_threshold = 40u;
    state.actors[0].special_contact_raw_56 = 1;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].velocity_z != 0x0264) return false;

    /* `$86:B1E2-$B202`: inclusive +/-80 retains; 81 converts to 9D6E. */
    memset(&state, 0, sizeof(state));
    state.session = &session;
    state.possession_actor = 0;
    state.pass_receiver_raw = 0;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 22u;
    state.actors[0].animation_state = 0x18u;
    state.actors[0].z_fp = 10 * 256;
    state.actors[0].velocity_x = 80;
    state.actors[0].mode13_baseline_velocity_x = 0;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 14u ||
        state.ball.owner_actor != 0) return false;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 22u;
    state.actors[0].animation_state = 0x18u;
    state.actors[0].z_fp = 10 * 256;
    state.actors[0].velocity_z = 0;
    state.actors[0].velocity_x = 81;
    state.possession_actor = 0;
    state.ball.owner_actor = 0;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 11u ||
        state.ball.owner_actor != -1 || state.ball.state != NBA_BALL_SHOT)
        return false;

    memset(&state, 0, sizeof(state));
    state.session = &session;
    state.possession_actor = 1;
    state.pass_receiver_raw = 0;
    state.actors[1].control_mode = 15u;
    state.actors[0].control_mode = 14u;
    state.actors[0].reaction_threshold = 22u;
    state.actors[0].animation_state = 0x18u;
    state.actors[0].z_fp = 10 * 256;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 14u) return false;
    state.actors[0].reaction_threshold = 22u;
    state.actors[0].z_fp = 10 * 256;
    state.pass_receiver_raw = 2;
    if (!cpu_update_rom_special_receiver(&state, 0u) ||
        state.actors[0].control_mode != 1u ||
        state.pass_receiver_raw != -1) return false;
    return true;
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
bool nba_tipoff_update_rom_passer(NbaTipoff *tipoff, unsigned slot) {
    NbaTipoffActor *passer = &tipoff->actors[slot];
    /* `$86:A6B3-$A6C7`: a mode-15 actor that no longer matches ownership
     * `$093E` counts +$60 down at the logical-pass delta. On underflow,
     * A772 dispatches `$86:9846/$9861`: mode 1, +$64=$2F, and complete
     * clearing of +$60/+$7E/+$28. Leaving this actor in mode 15 forever
     * prevents `$85:B271`'s five-player readiness barrier from completing. */
    if (tipoff->possession_actor != (int8_t)slot) {
        passer->reaction_threshold = (uint16_t)(
            passer->reaction_threshold - 2u);
        if ((int16_t)passer->reaction_threshold < 0) {
            passer->control_mode = 1u;
            passer->behavior_timer = 0x2Fu;
            passer->reaction_threshold = 0u;
            passer->behavior_flags_raw = 0u;
            passer->actor_status_raw_28 = 0u;
        }
        return true;
    }
    /* `$86:A6B3` keys ownership from `$093E`; the release gate at `$A749`
     * rereads raw `$0946`. Cached host identities may drive attachment while
     * waiting, but cannot authorize a launch after `$86:A613` canceled it. */
    if (tipoff->handler_actor != slot ||
        tipoff->receiver_actor >= NBA_GAMEPLAY_ACTOR_COUNT)
        return true;
    if (passer->pass_released_raw) return true;
    if (passer->pass_family_raw == 2) {
        /* `$86:A6EC -> $86:A629`: the boosted `$AFC4` family stays attached
         * through its jump. Common actor physics runs before this behavior
         * dispatch; the first descending pass changes +$C0 to 4 and installs
         * `$2C/$24`, then the following family-4 dispatch releases directly
         * through `$A749` without the `$2A-$31` phase table. */
        passer->x_fp += (int32_t)passer->velocity_x * 2;
        passer->y_fp += (int32_t)passer->velocity_y * 2;
        cpu_integrate_actor_vertical(passer);
        ball_attach_to_actor(tipoff, slot);
        if (passer->velocity_z < 0) {
            passer->pass_family_raw = 4;
            if (passer->exact_pass_animation) {
                actor_animation_command(tipoff, passer, NBA_ANIMATION_INSTALL_UPPER, 0x2Cu);
                actor_animation_command(tipoff, passer, NBA_ANIMATION_INSTALL_LOWER, 0x24u);
            } else {
                actor_set_animation(passer, 0x2Cu, 0x24u);
            }
            passer->upper_animation_phase_raw = 3u;
        }
        return true;
    }
    bool immediate_airborne_release = passer->pass_family_raw == 4;
    if (!immediate_airborne_release && passer->upper_animation_phase_raw <=
            passer->pass_release_threshold_raw) {
        ball_attach_to_actor(tipoff, slot);
        return true;
    }
    if (tipoff->pass_receiver_raw < 0) {
        /* `$86:A777-$A78F`: normalize the installed pass executor back to
         * mode 11 and retain `$09C4`; no ball launch occurs. A later mode-11
         * decision may install a fresh receiver and overwrite the metadata. */
        passer->control_mode = 11u;
        passer->reaction_threshold = 0u;
        passer->behavior_flags_raw = 0u;
        passer->actor_status_raw_28 = 0u;
        tipoff->pass_actor_raw = -1;
        tipoff->pass_receiver_raw = -1;
        tipoff->pass_aux_raw = -1;
        if (tipoff->live_state_raw < 0x80u)
            cpu_enter_play_state(tipoff, NBA_CPU_PLAY_ATTACK);
        return true;
    }
    /* `$86:9A07-$9A1D`: signed +$C0 selects all three packed tables:
     * negative `$9C6F`, positive `$9C93`, and zero `$9CB7`. */
    unsigned family = passer->pass_family_raw < 0 ? 0u :
                      passer->pass_family_raw > 0 ? 1u : 2u;
    unsigned band = passer->pass_band_raw / 6u;
    int16_t duration = 0, vertical = 0, opaque = 0;
    if (!nba_assets_gameplay_pass_launch(
            tipoff->assets, (uint8_t)family, (uint8_t)band,
            &duration, &vertical, &opaque) || duration <= 0)
        return true;
    (void)opaque; /* `$86:99C4` does not consume the third record word. */
    uint8_t receiver_slot = (uint8_t)tipoff->pass_receiver_raw;
    const NbaTipoffActor *receiver = &tipoff->actors[receiver_slot];
    tipoff->receiver_actor = receiver_slot;
    int target_x = fp_round(receiver->x_fp) +
        (int)pass_lead_component(receiver->velocity_x, (uint16_t)duration);
    int target_y = fp_round(receiver->y_fp) +
        (int)pass_lead_component(receiver->velocity_y, (uint16_t)duration);
    /* `$86:9A8B-$9AAF` clamps the predicted endpoint before subtracting the
     * current ball position and dividing by the launch scalar. This matters
     * most on baseline inbounds: the hand attachment may sit outside these
     * limits, so omitting the clamp can manufacture a zero-velocity pass
     * which the court-edge routine immediately cancels. */
    if (target_x > 362) target_x = 362;
    if (target_x < -362) target_x = -362;
    if (target_y > 192) target_y = 192;
    if (target_y < -192) target_y = -192;
    /* `$86:A74E` clears `$09C4` immediately before `$86:99C4` detaches the
     * ball. `$09B8` remains the independent inbound-transfer witness. */
    tipoff->pass_active_raw = 0u;
    ball_launch(tipoff, target_x, target_y, (unsigned)duration,
                vertical, NBA_BALL_PASS);
    /* `$86:A761-$A76C`: ordinary families retain a ten-tick post-release
     * actor timer. Airborne families 4/5 bypass this write. */
    if (passer->pass_family_raw < 4)
        passer->reaction_threshold = 0x0Au;
    /* `$86:9B84-$9B8F`: `$99C4` closes the temporary pass-init state 2
     * before normal live ball processing resumes. Leaving it set makes
     * `$85:9D40` reject an otherwise centered basket. */
    if (tipoff->live_state_raw < 0x81u) tipoff->live_state_raw = 0u;
    tipoff->possession_actor = -1;
    passer->pass_released_raw = true;
    return true;
}

static bool cpu_boundary_pass_recovery_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.pass_actor_raw = 3;
    state.pass_aux_raw = 5;
    state.pass_receiver_raw = 4;
    state.rim_raw_094a = 7u;
    state.ball_activity_raw = 9u;
    state.inbound_transfer_raw = 1u;
    state.pass_active_raw = 1u;
    state.possession_actor = 3;
    state.ball.owner_actor = 3;
    state.ball.state = NBA_BALL_PASS;
    state.ball.velocity_x = 71;
    state.ball.velocity_y = -37;
    int32_t x_fp = 395 * 256;
    int32_t y_fp = 12 * 256;
    int16_t velocity_x = 64;
    int16_t velocity_y = -11;
    cpu_clamp_ball_to_court(
        &state, &x_fp, &y_fp, &velocity_x, &velocity_y);
    if (x_fp != 394 * 256 || velocity_x != 0 || velocity_y != -11 ||
        state.pass_actor_raw != -1 || state.pass_aux_raw != -1 ||
        state.pass_receiver_raw != -1 || state.rim_raw_094a != 0u ||
        state.ball_activity_raw != 0u || state.inbound_transfer_raw != 0u ||
        state.pass_active_raw != 1u || state.possession_actor != 3 ||
        state.ball.owner_actor != 3 || state.ball.state != NBA_BALL_PASS ||
        state.ball.velocity_x != 71 || state.ball.velocity_y != -37)
        return false;

    /* A player at the same rectangle must not execute the ball-global clear. */
    state.pass_actor_raw = 3;
    state.pass_aux_raw = 5;
    state.pass_receiver_raw = 4;
    state.inbound_transfer_raw = 1u;
    y_fp = -225 * 256;
    velocity_y = -64;
    cpu_clamp_actor_to_court(&x_fp, &y_fp, &velocity_x, &velocity_y);
    if (y_fp != -224 * 256 || velocity_y != 0 ||
        state.pass_actor_raw != 3 || state.pass_aux_raw != 5 ||
        state.pass_receiver_raw != 4 || state.inbound_transfer_raw != 1u)
        return false;

    /* `$86:A749-$A78F`: a prior ball-edge clear makes the release abort
     * without RNG, detachment, or a host-cached receiver launch. */
    memset(&state, 0, sizeof(state));
    state.handler_actor = 3u;
    state.receiver_actor = 4u;
    state.possession_actor = 3;
    state.pass_actor_raw = -1;
    state.pass_aux_raw = -1;
    state.pass_receiver_raw = -1;
    state.pass_active_raw = 1u;
    state.ball.owner_actor = 3;
    state.ball.state = NBA_BALL_ATTACHED;
    state.actors[3].control_mode = 15u;
    state.actors[3].reaction_threshold = 9u;
    state.actors[3].behavior_flags_raw = 0xFFFFu;
    state.actors[3].actor_status_raw_28 = 0xFFFFu;
    state.actors[3].upper_animation_phase_raw = 2u;
    state.actors[3].pass_release_threshold_raw = 1u;
    if (!nba_tipoff_update_rom_passer(&state, 3u) ||
        state.actors[3].control_mode != 11u ||
        state.actors[3].reaction_threshold != 0u ||
        state.actors[3].behavior_flags_raw != 0u ||
        state.actors[3].actor_status_raw_28 != 0u ||
        state.pass_active_raw != 1u || state.ball.owner_actor != 3 ||
        state.ball.state != NBA_BALL_ATTACHED)
        return false;

    /* `$86:A6B3-$A6C7 -> $86:9846`: stale non-owner passers expire. */
    state.possession_actor = 3;
    state.actors[2].control_mode = 15u;
    state.actors[2].reaction_threshold = 2u;
    state.actors[2].behavior_flags_raw = 6u;
    state.actors[2].actor_status_raw_28 = 0xFFFFu;
    if (!nba_tipoff_update_rom_passer(&state, 2u) ||
        state.actors[2].control_mode != 15u ||
        state.actors[2].reaction_threshold != 0u) return false;
    if (!nba_tipoff_update_rom_passer(&state, 2u) ||
        state.actors[2].control_mode != 1u ||
        state.actors[2].behavior_timer != 0x2Fu ||
        state.actors[2].reaction_threshold != 0u ||
        state.actors[2].behavior_flags_raw != 0u ||
        state.actors[2].actor_status_raw_28 != 0u) return false;
    return true;
}

static bool cpu_boosted_pass_self_test(const NbaAssetPack *assets,
                                       NbaSession *session) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.assets = assets;
    state.session = session;
    state.possession_actor = 0;
    state.handler_actor = 0u;
    state.receiver_actor = 1u;
    state.pass_actor_raw = -1;
    state.pass_receiver_raw = -1;
    state.actors[0].x_fp = -300 * 256;
    state.actors[0].y_fp = 40 * 256;
    state.actors[1].x_fp = 80 * 256;
    state.actors[1].y_fp = 40 * 256;
    state.actors[0].movement_boost_timer = 20u;
    state.actors[0].lower_animation_state = 0x0Cu;
    state.actors[0].roster_slot = 0u;
    uint8_t profile_39 = 0u, profile_3e = 0u;
    for (uint8_t roster = 0u; roster < 12u; ++roster) {
        if (nba_player_gameplay_pass_profiles(
                assets, session->left_team, roster,
                &profile_39, &profile_3e) && profile_3e >= 0x55u) {
            state.actors[0].roster_slot = roster;
            break;
        }
    }
    if (profile_3e < 0x55u) return false;
    uint16_t distance = 0u;
    uint8_t fine = nba_gameplay_pass_direction(380, 0, &distance);
    if (fine >= 16u || distance < 0x119u) return false;
    state.actors[0].movement_direction =
        (uint8_t)(((fine >> 1) + 4u) & 7u);
    if (!nba_tipoff_begin_rom_pass(&state, 0u, 1u) ||
        state.actors[0].control_mode != 15u ||
        state.actors[1].control_mode != 10u ||
        state.actors[1].reaction_threshold != 0x46u ||
        state.actors[0].pass_family_raw != 2 ||
        state.actors[0].animation_state != 0x18u ||
        state.actors[0].lower_animation_state != 0x1Fu ||
        state.actors[0].velocity_z != 0x0258 ||
        state.pass_active_raw != 1u) return false;
    for (unsigned pass = 0u; pass < 32u &&
            state.actors[0].pass_family_raw == 2; ++pass)
        (void)nba_tipoff_update_rom_passer(&state, 0u);
    if (state.actors[0].pass_family_raw != 4 ||
        state.actors[0].animation_state != 0x2Cu ||
        state.actors[0].lower_animation_state != 0x24u) return false;
    (void)nba_tipoff_update_rom_passer(&state, 0u);
    return state.actors[0].pass_released_raw &&
           state.ball.state == NBA_BALL_PASS &&
           state.ball.owner_actor < 0 && state.pass_active_raw == 0u;
}

static void score_made_basket(NbaTipoff *tipoff) {
    /* `$85:A079-$A345` is the made-basket branch. `$094C` is added to
     * team-record +$26 (`$4711/$4791`), `$0936` becomes `$82`, and
     * `$0952/$0954` seed the dead-ball/inbound path before `$094C` clears. */
    unsigned scoring_side = tipoff->offense_side & 1u;
    unsigned inbound_side = scoring_side ^ 1u;
    uint16_t shot_value = tipoff->shot_value_raw;

    /* `$85:A0EB-$A115`: the make effect is selected before foul/stat/score
     * bookkeeping. Effect four suppresses the selector entirely; otherwise
     * zero is rejected, so the gameplay LFSR may advance more than once. */
    tipoff->rim_raw_092c = 0x05A0u;
    tipoff->rim_raw_096a = 0u;
    tipoff->inbound_transfer_raw = 0u;
    if (tipoff->rim_effect.effect_raw_401b != 4u) {
        uint16_t selector;
        do {
            selector = (uint16_t)(nba_gameplay_rng_next(&tipoff->rng) & 3u);
        } while (selector == 0u);
        (void)nba_gameplay_effect_start(&tipoff->rim_effect,
                                        (uint16_t)(selector - 1u));
    }
    (void)nba_gameplay_foul_record_made_basket(&tipoff->fouls);
    tipoff->ball_activity_raw = 0u;
    tipoff->rim_raw_094a = 0u;
    tipoff->rim_raw_0962 = 0u;
    tipoff->session->score[scoring_side] = (uint16_t)(
        tipoff->session->score[scoring_side] + shot_value);
    tipoff->last_scoring_side = (uint8_t)scoring_side;
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
        /* `$85:C5AD-$C5BD` installs the boundary target before the later
         * `$85:AD86-$AD95` formation pass deliberately skips this actor. */
        tipoff->actors[tipoff->inbound_actor_raw].target_x = target.x;
        tipoff->actors[tipoff->inbound_actor_raw].target_y = target.y;
    }
    tipoff->inbound_ready_raw = 0u;
    tipoff->shot_value_raw = 0u;

    /* `$85:A26F-$A292`: ties retain the previous leader. */
    if (tipoff->leading_side_raw_1403 == 0u) {
        if (tipoff->session->score[1] > tipoff->session->score[0]) {
            tipoff->leading_side_raw_1403 = 1u;
            ++tipoff->right_lead_change_count_raw_1407;
        }
    } else if (tipoff->session->score[0] > tipoff->session->score[1]) {
        tipoff->leading_side_raw_1403 = 0u;
        ++tipoff->left_lead_change_count_raw_1405;
    }
    /* `$85:A292-$A336`: these event bits are gameplay-visible presentation
     * state. The side-zero three-point branch also consumes one result whose
     * value is discarded, preserving subsequent RNG cadence. */
    if (shot_value == 3u) {
        tipoff->rim_raw_13e7 |= 0x1000u;
        if (tipoff->camera_side_group_raw == 0u)
            (void)nba_gameplay_rng_next(&tipoff->rng);
    }
    tipoff->rim_raw_13e7 |= 0x0004u;
    tipoff->shot_result_resolved = true;
    tipoff->ball.owner_actor = -1;
    tipoff->possession_actor = -1;
}

/* `$87:92AD-$949E -> $87:9B38/$9B41-$9BC8`: violation and ordinary foul
 * branches select an inbound side/layout, then share one `$82` dead-ball
 * initializer. The ROM owner is cleared after its current actor is put back
 * in mode 2; BOUNCE is the port's ownerless equivalent of that record state. */
static void cpu_begin_dead_ball(NbaTipoff *tipoff, uint8_t selected_actor,
                                uint16_t side_group, int16_t layout,
                                bool interference) {
    if (!tipoff || selected_actor >= NBA_GAMEPLAY_ACTOR_COUNT ||
        (side_group != 0u && side_group != 5u)) return;
    unsigned side = side_group / 5u;
    /* `$87:92C5-$92CB` makes code 6 the sole path here that updates both
     * persistent camera side `$093A` and inbound side `$0952`. Ordinary
     * foul branches only write `$0952`, so keep those two words distinct. */
    if (interference) tipoff->camera_side_group_raw = (uint8_t)side_group;
    tipoff->inbound_state_raw = side_group;
    if (interference)
        tipoff->team_context[side].dead_ball_actor_raw_3f = selected_actor;
    /* `$85:C37D-$C388`: the provisional inbound actor is always slot 2 or
     * 7, derived from `$0952`. `$87:9B41`'s selected actor is only the old
     * owner/context record that gets demoted before this initializer. */
    tipoff->inbound_actor_raw = (uint16_t)(side_group + 2u);
    tipoff->inbound_layout_raw = layout;
    if (interference) {
        tipoff->dead_ball_raw_096c = 0u;
        tipoff->dead_ball_raw_0966 = NBA_GAMEPLAY_UNKNOWN_WORD;
    }

    tipoff->live_state_raw = 0x82u;
    tipoff->inbound_timer_raw = 300u;
    tipoff->role_rebuild_raw_09d6 = 300u;
    tipoff->rim_raw_092c = 0x05A0u;
    tipoff->shot_clock_mirror_raw_09c6 = 0x05A0u;
    tipoff->dead_ball_raw_0968 = 0u;
    tipoff->rim_raw_096a = 0u;
    tipoff->dead_ball_x_raw_09b0 = fp_integer_word(tipoff->ball.x_fp);
    tipoff->dead_ball_y_raw_09b2 = fp_integer_word(tipoff->ball.y_fp);

    bool had_owner = (tipoff->possession_actor >= 0 &&
                      tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT) ||
                     (tipoff->ball.owner_actor >= 0 &&
                      tipoff->ball.owner_actor < NBA_GAMEPLAY_ACTOR_COUNT);
    if (tipoff->possession_actor >= 0 &&
        tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT) {
        NbaTipoffActor *owner = &tipoff->actors[tipoff->possession_actor];
        if (owner->control_mode != 8u) owner->control_mode = 2u;
    }
    if (had_owner) {
        /* `$87:9BA9-$9BAC` stops an owned ball. An already ownerless pass,
         * shot, or bounce keeps its current object routine and velocities. */
        tipoff->ball.velocity_x = 0;
        tipoff->ball.velocity_y = 0;
    }
    cpu_cancel_rom_pass_activity(tipoff);
    tipoff->ball.owner_actor = -1;
    tipoff->possession_actor = -1;
    tipoff->pass_active_raw = 0u;
    tipoff->rim_raw_097c = 0u;
    if (had_owner) tipoff->ball.state = NBA_BALL_BOUNCE;

    tipoff->offense_side = (uint8_t)side;
    tipoff->possession_team = (int8_t)side;
    tipoff->handler_actor = (uint8_t)tipoff->inbound_actor_raw;
    tipoff->receiver_actor = (uint8_t)(side_group + 4u);
    NbaGameplayInboundTarget target;
    int16_t context_anchor = side ? 336 : -336;
    if (nba_gameplay_inbound_target(
            tipoff->inbound_layout_raw, fp_round(tipoff->ball.x_fp),
            fp_round(tipoff->ball.y_fp), context_anchor,
            fp_round(tipoff->ball.x_fp), &tipoff->rng, &target)) {
        tipoff->inbound_target_x_raw = target.x;
        tipoff->inbound_target_y_raw = target.y;
        tipoff->inbound_direction_raw = target.direction;
        /* `$85:C5AD-$C5BD` installs the boundary target before the later
         * `$85:AD86-$AD95` formation pass deliberately skips this actor. */
        tipoff->actors[tipoff->inbound_actor_raw].target_x = target.x;
        tipoff->actors[tipoff->inbound_actor_raw].target_y = target.y;
    }
    tipoff->inbound_ready_raw = 0u;
    tipoff->inbound_transfer_raw = 0u;
}

/* `$87:9426-$9478`: `$09BC` is a deferred shooting-foul event. Phase 2 in
 * `$0A02` bypasses the live-shot wait; otherwise the ROM waits for `$0948`
 * to clear and for an owner to exist or the loose ball to fall below Z=16. */
static bool cpu_resolve_deferred_shooting_foul(NbaTipoff *tipoff) {
    if (!tipoff ||
        (tipoff->fouls.shooting_foul_raw_09bc & 0x7FFFu) !=
            NBA_GAMEPLAY_FOUL_DEFENSIVE)
        return false;
    bool ready = tipoff->deferred_shot_foul_phase_raw_0a02 == 2u ||
        (tipoff->ball_activity_raw == 0u &&
         (tipoff->possession_actor >= 0 ||
          fp_integer_word(tipoff->ball.z_fp) < 16));
    if (!ready) return false;
    uint16_t deferred = tipoff->fouls.shooting_foul_raw_09bc;
    tipoff->ball_activity_raw = 0u;
    tipoff->fouls.free_throw_state_raw_0978 = 1u;
    tipoff->fouls.free_throw_sequence_raw_097a =
        (deferred & 0x8000u) != 0u ? 1u : 2u;
    tipoff->fouls.foul_event_raw_0964 = deferred & 0x7FFFu;
    return true;
}

static void cpu_dispatch_pending_event(NbaTipoff *tipoff) {
    if (!tipoff || tipoff->fouls.whistle_active_raw_09b6 != 0u) return;
    (void)cpu_resolve_deferred_shooting_foul(tipoff);
    if (tipoff->fouls.foul_event_raw_0964 == 0u) return;
    uint16_t event = tipoff->fouls.foul_event_raw_0964;
    if (event == NBA_GAMEPLAY_VIOLATION_INTERFERENCE) {
        if (tipoff->session->config.rules[5] == 0u ||
            tipoff->fouls.offender_actor_raw < 0) return;
        uint8_t actor = (uint8_t)tipoff->fouls.offender_actor_raw;
        cpu_begin_dead_ball(tipoff, actor, (uint16_t)((actor / 5u) * 5u),
                            0, true);
    } else {
        /* `$87:9411-$949E`: ordinary and already-seeded bonus contact uses layout 4
         * (3 when `$0966` is nonzero). Code 1 awards the ball opposite the
         * offender; charging/offensive codes 2/13 use `$093A ^ 5`. */
        if ((event != NBA_GAMEPLAY_FOUL_DEFENSIVE &&
             event != NBA_GAMEPLAY_FOUL_CHARGING &&
             event != NBA_GAMEPLAY_FOUL_OFFENSIVE) ||
            tipoff->fouls.offender_actor_raw < 0) return;
        uint8_t offender = (uint8_t)tipoff->fouls.offender_actor_raw;
        uint16_t side_group = event == NBA_GAMEPLAY_FOUL_DEFENSIVE ?
            (uint16_t)(offender < 5u ? 5u : 0u) :
            (uint16_t)(tipoff->camera_side_group_raw ^ 5u);
        uint8_t selected_actor = tipoff->possession_actor >= 0 &&
                tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT ?
            (uint8_t)tipoff->possession_actor : offender;
        int16_t layout = tipoff->dead_ball_raw_0966 != 0u ? 3 : 4;
        cpu_begin_dead_ball(tipoff, selected_actor, side_group, layout, false);
    }
    (void)nba_gameplay_foul_consume_pending(
        &tipoff->fouls, tipoff->camera_side_group_raw,
        &tipoff->rim_raw_13e7, &tipoff->inbound_ready_raw,
        false);
}

static bool cpu_deferred_shooting_foul_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.possession_actor = -1;
    state.fouls.shooting_foul_raw_09bc = NBA_GAMEPLAY_FOUL_DEFENSIVE;
    state.deferred_shot_foul_phase_raw_0a02 = 1u;
    state.ball_activity_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.ball.z_fp = 15 * 256;
    if (cpu_resolve_deferred_shooting_foul(&state)) return false;
    state.ball_activity_raw = 0u;
    state.ball.z_fp = 16 * 256;
    if (cpu_resolve_deferred_shooting_foul(&state)) return false;
    state.ball.z_fp = 15 * 256;
    if (!cpu_resolve_deferred_shooting_foul(&state) ||
        state.fouls.foul_event_raw_0964 != NBA_GAMEPLAY_FOUL_DEFENSIVE ||
        state.fouls.free_throw_state_raw_0978 != 1u ||
        state.fouls.free_throw_sequence_raw_097a != 2u) return false;

    memset(&state, 0, sizeof(state));
    state.possession_actor = -1;
    state.fouls.shooting_foul_raw_09bc =
        0x8000u | NBA_GAMEPLAY_FOUL_DEFENSIVE;
    state.deferred_shot_foul_phase_raw_0a02 = 2u;
    state.ball_activity_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    state.ball.z_fp = 88 * 256;
    return cpu_resolve_deferred_shooting_foul(&state) &&
           state.ball_activity_raw == 0u &&
           state.fouls.foul_event_raw_0964 ==
               NBA_GAMEPLAY_FOUL_DEFENSIVE &&
           state.fouls.free_throw_state_raw_0978 == 1u &&
           state.fouls.free_throw_sequence_raw_097a == 1u;
}

static bool cpu_dead_ball_dispatch_self_test(void) {
    NbaTipoff state;
    NbaSession session;
    memset(&state, 0, sizeof(state));
    memset(&session, 0, sizeof(session));
    state.session = &session;
    state.session->config.rules[5] = 1u;
    state.ball.x_fp = 121 * 256;
    state.ball.y_fp = -19 * 256;
    state.ball.z_fp = 88 * 256;
    state.ball.owner_actor = 8;
    state.ball.state = NBA_BALL_ATTACHED;
    state.possession_actor = 8;
    state.actors[8].control_mode = 11u;
    nba_gameplay_rng_seed(&state.rng, 0x9146u);
    nba_gameplay_foul_init(&state.fouls);
    if (!nba_gameplay_foul_record_violation(
            &state.fouls, NBA_GAMEPLAY_VIOLATION_INTERFERENCE, 8u, 2u))
        return false;
    cpu_dispatch_pending_event(&state);
    bool interference_ok = state.live_state_raw == 0x82u &&
           state.camera_side_group_raw == 5u &&
           state.inbound_state_raw == 5u &&
           state.inbound_actor_raw == 7u &&
           state.team_context[1].dead_ball_actor_raw_3f == 8u &&
           state.inbound_layout_raw == 0 &&
           state.inbound_timer_raw == 300u &&
           state.role_rebuild_raw_09d6 == 300u &&
           state.rim_raw_092c == 0x05A0u &&
           state.shot_clock_mirror_raw_09c6 == 0x05A0u &&
           state.dead_ball_raw_0966 == NBA_GAMEPLAY_UNKNOWN_WORD &&
           state.dead_ball_x_raw_09b0 == 121 &&
           state.dead_ball_y_raw_09b2 == -19 &&
           state.possession_actor == -1 && state.ball.owner_actor == -1 &&
           state.ball.state == NBA_BALL_BOUNCE &&
           state.ball.velocity_x == 0 && state.ball.velocity_y == 0 &&
           state.actors[8].control_mode == 2u &&
           state.fouls.foul_event_raw_0964 == 0u &&
           state.fouls.latched_event_raw_08f0 ==
               NBA_GAMEPLAY_VIOLATION_INTERFERENCE &&
           state.fouls.whistle_active_raw_09b6 == 1u;
    if (!interference_ok) return false;

    NbaTipoff defensive;
    memset(&defensive, 0, sizeof(defensive));
    defensive.session = &session;
    defensive.camera_side_group_raw = 0u;
    defensive.possession_actor = 8;
    defensive.ball.owner_actor = 8;
    defensive.ball.state = NBA_BALL_ATTACHED;
    defensive.actors[8].control_mode = 11u;
    nba_gameplay_rng_seed(&defensive.rng, 0x9146u);
    nba_gameplay_foul_init(&defensive.fouls);
    if (!nba_gameplay_foul_record_contact(
            &defensive.fouls, NBA_GAMEPLAY_FOUL_DEFENSIVE,
            2u, 8u, 0u, false, 0u)) return false;
    cpu_dispatch_pending_event(&defensive);
    bool defensive_ok = defensive.live_state_raw == 0x82u &&
        defensive.inbound_state_raw == 5u &&
        defensive.camera_side_group_raw == 0u &&
        defensive.inbound_actor_raw == 7u &&
        defensive.inbound_layout_raw == 4 &&
        defensive.actors[8].control_mode == 2u &&
        defensive.fouls.latched_event_raw_08f0 ==
            NBA_GAMEPLAY_FOUL_DEFENSIVE &&
        defensive.fouls.whistle_active_raw_09b6 == 1u &&
        defensive.fouls.team_fouls[0] == 1u;
    if (!defensive_ok) return false;

    NbaTipoff charging;
    memset(&charging, 0, sizeof(charging));
    charging.session = &session;
    charging.camera_side_group_raw = 0u;
    charging.possession_actor = 2;
    charging.ball.owner_actor = 2;
    charging.ball.state = NBA_BALL_ATTACHED;
    charging.actors[2].control_mode = 11u;
    nba_gameplay_rng_seed(&charging.rng, 0x9146u);
    nba_gameplay_foul_init(&charging.fouls);
    if (!nba_gameplay_foul_record_contact(
            &charging.fouls, NBA_GAMEPLAY_FOUL_CHARGING,
            2u, 7u, 0u, false, 0u)) return false;
    cpu_dispatch_pending_event(&charging);
    bool charging_ok = charging.live_state_raw == 0x82u &&
           charging.camera_side_group_raw == 0u &&
           charging.inbound_state_raw == 5u &&
           charging.inbound_actor_raw == 7u &&
           charging.inbound_layout_raw == 4 &&
           charging.actors[2].control_mode == 2u &&
           charging.fouls.latched_event_raw_08f0 ==
               NBA_GAMEPLAY_FOUL_CHARGING &&
           charging.fouls.whistle_active_raw_09b6 == 1u;
    if (!charging_ok) return false;

    /* `$86:C686-$C68F` seeds the bonus before the ordinary event reaches
     * `$87:9411`. It still runs dead-ball placement and consumes `$0964`;
     * `$87:922E` then diverts actors into the free-throw scene. */
    NbaTipoff bonus;
    memset(&bonus, 0, sizeof(bonus));
    bonus.session = &session;
    bonus.camera_side_group_raw = 0u;
    bonus.possession_actor = 8;
    bonus.ball.owner_actor = 8;
    bonus.actors[8].control_mode = 11u;
    nba_gameplay_rng_seed(&bonus.rng, 0x9146u);
    nba_gameplay_foul_init(&bonus.fouls);
    bonus.fouls.team_fouls[0] = 4u;
    if (!nba_gameplay_foul_record_contact(
            &bonus.fouls, NBA_GAMEPLAY_FOUL_DEFENSIVE,
            2u, 8u, 0u, false, 0u)) return false;
    cpu_dispatch_pending_event(&bonus);
    return bonus.live_state_raw == 0x82u &&
           bonus.camera_side_group_raw == 0u &&
           bonus.inbound_state_raw == 5u &&
           bonus.inbound_actor_raw == 7u &&
           bonus.inbound_layout_raw == 4 &&
           bonus.fouls.free_throw_state_raw_0978 == 1u &&
           bonus.fouls.free_throw_sequence_raw_097a == 2u &&
           bonus.fouls.foul_event_raw_0964 == 0u &&
           bonus.fouls.whistle_active_raw_09b6 == 1u;
}

static NbaGameplayRimResult cpu_update_live_ball(NbaTipoff *tipoff) {
    /* The same `$87:9B0D` 30-Hz logical pass drives `$85:9ACB+` ball
     * collision/integration. Flight-table durations count these due passes. */
    if ((tipoff->simulation_tick & 1u) != 0u)
        return NBA_GAMEPLAY_RIM_FLIGHT;
    if (tipoff->ball.owner_actor < 0 &&
        tipoff->free_throw_flight_timer_raw_0930 != 0u)
        --tipoff->free_throw_flight_timer_raw_0930;
    /* `$85:9A2C-$9A34`: this counter advances by the live scheduler quantum,
     * once before the two free-ball substeps (not once per substep). */
    if (tipoff->rim_raw_094a != 0u)
        tipoff->rim_raw_094a = (uint16_t)(tipoff->rim_raw_094a + 2u);
    /* Oracle frames 1652..1683 write `$0970` 15..0 once per 30-Hz ball
     * pass. Decrement before contact so a newly installed response remains
     * at 15 for its complete first interval. */
    if (tipoff->rim_raw_0970 != 0u) --tipoff->rim_raw_0970;
    NbaTipoffBall *ball = &tipoff->ball;
    int32_t old_x = ball->x_fp, old_y = ball->y_fp, old_z = ball->z_fp;
    int16_t old_velocity_z = ball->velocity_z;
    bool attached = ball->state == NBA_BALL_ATTACHED;
    if (attached) {
        ball_attach_to_actor(tipoff, tipoff->handler_actor);
    } else if (ball->state == NBA_BALL_PASS || ball->state == NBA_BALL_SHOT ||
               ball->state == NBA_BALL_BOUNCE) {
        NbaGameplayRimResult accumulated = NBA_GAMEPLAY_RIM_FLIGHT;
        /* `$85:9A6A/$A7A1`: DP C6 enters as two and drives two complete
         * ownerless substeps. PASS, SHOT and BOUNCE are host labels only;
         * native gravity/rim/ground physics is shared by every free ball. */
        for (unsigned substep = 0; substep < 2u; ++substep) {
        bool made_response = false;
        NbaGameplayRimResult result = NBA_GAMEPLAY_RIM_FLIGHT;

        /* `$85:9A78-$9AC3`: ownerless descending/grounded records release
         * the activity latch, and an intended receiver or live play clears
         * the dead-ball marker before rim classification. */
        int16_t integer_z = fp_integer_word(ball->z_fp);
        if ((ball->velocity_z < 0 && integer_z < 64) || integer_z == 0)
            tipoff->ball_activity_raw = 0u;
        if (tipoff->pass_receiver_raw >= 0 || tipoff->live_state_raw == 1u)
            tipoff->dead_ball_raw_0968 = 0u;

        /* `$85:A009-$A036`: scripted resolution consumes one table entry
         * per substep, retaining all fractional bytes and velocities. */
        if (tipoff->free_throw_resolution_raw_0972 != 0u) {
            static const int16_t scripted_xy[16][2] = {
                {336,0},{336,0},{332,4},{332,4},{332,5},{332,5},
                {328,2},{328,2},{328,2},{328,2},{332,-1},{332,-1},
                {336,-1},{336,-1},{338,1},{338,1}
            };
            unsigned index = (tipoff->free_throw_resolution_raw_0972 >> 1) & 15u;
            int16_t scripted_x = scripted_xy[index][0];
            if (fp_integer_word(ball->x_fp) < 0) scripted_x = (int16_t)-scripted_x;
            ball->x_fp = fp_replace_integer_word(ball->x_fp, scripted_x);
            ball->y_fp = fp_replace_integer_word(ball->y_fp,
                                                  scripted_xy[index][1]);
            ball->z_fp = fp_replace_integer_word(ball->z_fp, 0x53);
            --tipoff->free_throw_resolution_raw_0972;
            continue;
        }

        /* `$85:9C42-$9C5B`: a descending live shot below rim height ends
         * live-shot classification before ordinary free-ball physics. */
        if (integer_z < 73 && tipoff->live_state_raw == 1u &&
            ball->velocity_z < 0) {
            tipoff->ball_activity_raw = 0u;
            tipoff->rim_raw_094a = 0u;
            tipoff->live_state_raw = 0u;
        }
        /* `$85:9A78-$A656`: the current integer position is tested against
         * the rim first. Any reflected velocity and snapped integer axis are
         * then consumed by gravity/integration on this same logical pass. */
        {
            int hoop_x = basket_x_for_side(tipoff->offense_side);
            int hoop_y = basket_y_for_side(tipoff->offense_side);
            bool right_basket = tipoff->offense_side != 0u;
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
                tipoff->offense_side != 0u, tipoff->live_state_raw,
                tipoff->rim_force_raw_1866 != 0u,
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
                /* `$85:A079-$A345` runs inline in the detecting substep;
                 * substep two therefore observes live state `$82`. */
                score_made_basket(tipoff);
                rim_context.raw_0936 = tipoff->live_state_raw;
                rim_context.raw_0948 = tipoff->ball_activity_raw;
                rim_context.raw_094a = tipoff->rim_raw_094a;
                rim_context.raw_09b8 = tipoff->inbound_transfer_raw;
                nba_gameplay_rim_apply_made_response(
                    &rim, right_basket, &rim_context);
                /* Score bookkeeping at `$85:A33C-$A342` occurs before the
                 * common physics tail. Keep those freshly raised bits when
                 * the local rim snapshot is copied back below. */
                rim.raw_13e7 = tipoff->rim_raw_13e7;
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
                    /* `$85:A34A-$A3B3` writes integer X/Y only. */
                    ball->x_fp = fp_replace_integer_word(ball->x_fp, rim.x);
                    ball->y_fp = fp_replace_integer_word(ball->y_fp, rim.y);
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
        /* `$85:A3D7-$A3DD` applies gravity on every common-tail pass,
         * including the detecting made-basket substep. */
        ball->velocity_z = (int16_t)(ball->velocity_z - 0x18);
        ball->x_fp += ball->velocity_x;
        ball->z_fp += ball->velocity_z;
        if (nba_gameplay_court_finish_y_step(
                &ball->x_fp, &ball->y_fp,
                &ball->velocity_x, &ball->velocity_y))
            cpu_cancel_rom_pass_activity(tipoff);
        if (ball->z_fp < 0) {
            ball->z_fp = 0;
            {
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
            }
        }
        /* Preserve the first terminal/inner result when substep two merely
         * flies, but allow a later inner result to supersede outer contact. */
        if (result == NBA_GAMEPLAY_RIM_MAKE ||
            (accumulated != NBA_GAMEPLAY_RIM_MAKE &&
             result != NBA_GAMEPLAY_RIM_FLIGHT &&
             (accumulated == NBA_GAMEPLAY_RIM_FLIGHT ||
              result != NBA_GAMEPLAY_RIM_OUTER_CONTACT)))
            accumulated = result;
        }
        /* `$85:A7A8-$A7B5`: free balls below 56 clear the rim latch after
         * both substeps. */
        if (fp_integer_word(ball->z_fp) < 56) tipoff->rim_raw_0962 = 0u;
        /* `$87:8F95-$8FA9` schedules `$85:9A24` ball physics before
         * `$87:AA02`, so a miss-started effect receives its first dt=2 step
         * on this same logical pass. */
        nba_gameplay_effect_step(
            &tipoff->rim_effect, fp_integer_word(ball->y_fp),
            fp_integer_word(ball->z_fp), ball->velocity_z, 2u);
        return accumulated;
    }
    if (attached) {
        ball->velocity_x = (int16_t)(ball->x_fp - old_x);
        ball->velocity_y = (int16_t)(ball->y_fp - old_y);
        ball->velocity_z = (int16_t)(ball->z_fp - old_z);
        NbaTipoffActor *owner = &tipoff->actors[tipoff->handler_actor];
        if (owner->upper_animation_phase_raw >= 3u) {
            /* `$85:9A99->$A4F2->$A532` retains the prior ball Z words and
             * velocity once the owner pose reaches phase three. Attachment
             * still supplies X/Y, but its Z snap is not an impulse. */
            ball->z_fp = old_z;
            NbaGameplayAttachedVerticalState vertical = {
                .attachment_state_raw_09f6 =
                    tipoff->attached_ball_state_raw_09f6,
                .dead_ball_raw_0968 = tipoff->dead_ball_raw_0968,
                .velocity_z = old_velocity_z,
                .z_fraction = (uint16_t)((old_z & 0xFF) << 8),
                .z = fp_integer_word(old_z),
                .impact_raw_13e5 = tipoff->rim_impact_raw_13e5,
                .event_bits_raw_13e7 = tipoff->rim_raw_13e7
            };
            nba_gameplay_ball_apply_attached_vertical(&vertical);
            tipoff->attached_ball_state_raw_09f6 =
                vertical.attachment_state_raw_09f6;
            tipoff->dead_ball_raw_0968 = vertical.dead_ball_raw_0968;
            ball->velocity_z = vertical.velocity_z;
            ball->z_fp = (int32_t)vertical.z * 256 +
                         (vertical.z_fraction >> 8);
            tipoff->rim_impact_raw_13e5 = vertical.impact_raw_13e5;
            tipoff->rim_raw_13e7 = vertical.event_bits_raw_13e7;
        }
    }
    nba_gameplay_effect_step(
        &tipoff->rim_effect, fp_integer_word(ball->y_fp),
        fp_integer_word(ball->z_fp), ball->velocity_z, 2u);
    return NBA_GAMEPLAY_RIM_FLIGHT;
}

NbaGameplayRimResult nba_tipoff_replay_ownerless_ball_entry(
        NbaTipoff *tipoff) {
    if (!tipoff) return NBA_GAMEPLAY_RIM_FLIGHT;
    /* The capture enters at `$85:9A6A`, after the wrapper's once-per-pass
     * counters. Back the represented counters up so the production driver
     * reaches that exact state before executing the ownerless core. */
    if (tipoff->free_throw_flight_timer_raw_0930 != 0u)
        ++tipoff->free_throw_flight_timer_raw_0930;
    if (tipoff->rim_raw_094a != 0u)
        tipoff->rim_raw_094a = (uint16_t)(tipoff->rim_raw_094a - 2u);
    if (tipoff->rim_raw_0970 != 0u) ++tipoff->rim_raw_0970;
    tipoff->simulation_tick &= ~1u;
    tipoff->ball.owner_actor = -1;
    if (tipoff->ball.state == NBA_BALL_ATTACHED ||
        tipoff->ball.state == NBA_BALL_HIDDEN ||
        tipoff->ball.state == NBA_BALL_TOSS)
        tipoff->ball.state = NBA_BALL_BOUNCE;
    return cpu_update_live_ball(tipoff);
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
           state.ball.x_fp == 345 * 256 + 72 &&
           state.ball.y_fp == -27 * 256 + 34 &&
           state.ball.z_fp == 81 * 256 + 68 &&
           state.ball.velocity_x == 100 &&
           state.ball.velocity_y == -15 &&
           state.ball.velocity_z == 102 &&
           state.rim_raw_096e == 7u &&
           state.rim_raw_0970 == 3u &&
           (state.rim_raw_13e7 & 0x0008u) == 0u;
}

/* Recomp `$85:9A24-$A7B5` orchestration vectors. These lock behavior that
 * cannot be proven by the isolated rim helpers: two substeps, inline score,
 * scripted `$0972`, preliminary live-latch clears, and the final tail. */
static bool cpu_ball_substep_self_test(void) {
    NbaSession session;
    NbaTipoff state;

    memset(&session, 0, sizeof(session));
    memset(&state, 0, sizeof(state));
    state.session = &session;
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_SHOT;
    state.offense_side = 1u;
    state.live_state_raw = 1u;
    state.shot_value_raw = 2u;
    nba_gameplay_effect_init(&state.rim_effect);
    state.rim_effect.effect_raw_401b = 4u;
    state.ball.x_fp = 336 * 256;
    state.ball.z_fp = 83 * 256;
    NbaGameplayRimResult result = cpu_update_live_ball(&state);
    if (result != NBA_GAMEPLAY_RIM_MAKE || session.score[1] != 2u ||
        state.live_state_raw != 0x82u || state.shot_value_raw != 0u ||
        state.ball.x_fp != 336 * 256 || state.ball.y_fp != 0 ||
        state.ball.z_fp != 82 * 256 + 205 ||
        state.ball.velocity_z != -27 ||
        (state.rim_raw_13e7 & 0x0004u) == 0u) return false;

    memset(&state, 0, sizeof(state));
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_BOUNCE;
    state.ball.z_fp = 0x0032;
    state.rim_raw_0962 = 0x05A0u;
    (void)cpu_update_live_ball(&state);
    if (state.ball.z_fp != 0 || state.ball.velocity_z != 42 ||
        state.rim_impact_raw_13e5 != 42u ||
        (state.rim_raw_13e7 & 1u) != 0u || state.rim_raw_0962 != 0u)
        return false;

    memset(&state, 0, sizeof(state));
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_SHOT;
    state.live_state_raw = 1u;
    state.ball_activity_raw = 5u;
    state.rim_raw_094a = 7u;
    state.ball.z_fp = 72 * 256;
    state.ball.velocity_z = -1;
    (void)cpu_update_live_ball(&state);
    if (state.live_state_raw != 0u || state.ball_activity_raw != 0u ||
        state.rim_raw_094a != 0u) return false;

    memset(&state, 0, sizeof(state));
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_BOUNCE;
    state.ball.z_fp = 55 * 256;
    state.rim_raw_0962 = 0x05A0u;
    (void)cpu_update_live_ball(&state);
    if (state.rim_raw_0962 != 0u) return false;

    memset(&state, 0, sizeof(state));
    state.ball.owner_actor = -1;
    state.ball.state = NBA_BALL_SHOT;
    state.free_throw_resolution_raw_0972 = 4u;
    state.ball.x_fp = -100 * 256 + 0x55;
    state.ball.y_fp = 9 * 256 + 0x66;
    state.ball.z_fp = 2 * 256 + 0x77;
    state.ball.velocity_x = 11;
    state.ball.velocity_y = -12;
    state.ball.velocity_z = 13;
    (void)cpu_update_live_ball(&state);
    return state.free_throw_resolution_raw_0972 == 2u &&
           state.ball.x_fp == -336 * 256 + 0x55 &&
           state.ball.y_fp == 0 * 256 + 0x66 &&
           state.ball.z_fp == 83 * 256 + 0x77 &&
           state.ball.velocity_x == 11 && state.ball.velocity_y == -12 &&
           state.ball.velocity_z == 13;
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

void nba_tipoff_update_play_control_end_frame(NbaTipoff *tipoff) {
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
                /* `$85:B25E-$B288/$B353`: DP $AA initially holds the
                 * countdown underflow, then becomes the descending actor
                 * loop counter.  The shared exit stores that live scratch
                 * value, not the original underflow. */
                tipoff->play_countdown_raw = (int16_t)(4u - i);
                return;
            }
        }
        tipoff->play_event_wait_raw = 0u; /* `$85:B288` */
    }
    cpu_advance_play_control(tipoff);
}

/* `$86:BAA2-$BAFA`, stopping immediately before `$86:BAFD`: common
 * player-grab prefix shared by the opening tip, pass catches, and rebounds.
 * Live Mesen vectors prove both movement branches: a catcher below `$0080`
 * loses lateral velocity, while a faster catcher retains it verbatim. */
static void cpu_apply_catch_prefix(NbaTipoff *tipoff, uint8_t catcher) {
    NbaTipoffActor *actor = &tipoff->actors[catcher];
    unsigned side = catcher / 5u;
    NbaGameplayTeamContext *context = &tipoff->team_context[side];
    NbaGameplayCatchPrefixState prefix = {
        .catcher = catcher,
        .controller_actor = actor->controller_assignment_raw,
        .velocity_x = actor->velocity_x,
        .velocity_y = actor->velocity_y,
        .movement_magnitude = actor->movement_magnitude_raw,
        .catcher_latch = actor->catcher_latch_raw_ae,
        .rim_force_raw_1866 = tipoff->rim_force_raw_1866,
        .dead_ball_raw_0968 = tipoff->dead_ball_raw_0968,
        .rim_raw_096a = tipoff->rim_raw_096a,
        .context_actor_raw_3f = context->dead_ball_actor_raw_3f,
        .context_controller_raw_41 = context->controller_actor_raw_41,
        .context_previous_actor_raw_43 =
            context->previous_dead_ball_actor_raw_43,
        .context_previous_controller_raw_45 =
            context->previous_controller_actor_raw_45,
        .special_actor_raw_09a2 = (int16_t)tipoff->special_actor_raw,
        .play_aux_raw_09a6 = tipoff->play_aux_selector_raw_09a6,
        .play_selector_raw = {
            tipoff->play_selector_raw[0], tipoff->play_selector_raw[1],
            tipoff->play_selector_raw[2]
        },
        .possession_actor_raw_093e = tipoff->possession_actor,
        .actor_record_raw_0910 = tipoff->catch_actor_record_raw_0910,
        .context_record_raw_0912 = tipoff->catch_context_record_raw_0912,
    };
    nba_gameplay_apply_catch_prefix(&prefix);
    actor->velocity_x = prefix.velocity_x;
    actor->velocity_y = prefix.velocity_y;
    actor->movement_magnitude_raw = prefix.movement_magnitude;
    actor->catcher_latch_raw_ae = prefix.catcher_latch;
    tipoff->rim_force_raw_1866 = prefix.rim_force_raw_1866;
    tipoff->dead_ball_raw_0968 = prefix.dead_ball_raw_0968;
    tipoff->rim_raw_096a = prefix.rim_raw_096a;
    context->dead_ball_actor_raw_3f = prefix.context_actor_raw_3f;
    context->controller_actor_raw_41 = prefix.context_controller_raw_41;
    context->previous_dead_ball_actor_raw_43 =
        prefix.context_previous_actor_raw_43;
    context->previous_controller_actor_raw_45 =
        prefix.context_previous_controller_raw_45;
    tipoff->special_actor_raw = (uint16_t)prefix.special_actor_raw_09a2;
    tipoff->play_aux_selector_raw_09a6 = prefix.play_aux_raw_09a6;
    for (unsigned i = 0; i < 3u; ++i)
        tipoff->play_selector_raw[i] = prefix.play_selector_raw[i];
    tipoff->possession_actor = (int8_t)prefix.possession_actor_raw_093e;
    tipoff->catch_actor_record_raw_0910 = prefix.actor_record_raw_0910;
    tipoff->catch_context_record_raw_0912 = prefix.context_record_raw_0912;
}

static void cpu_apply_catch_mode(NbaTipoff *tipoff, uint8_t catcher) {
    NbaTipoffActor *actor = &tipoff->actors[catcher];
    NbaGameplayTeamContext *context = &tipoff->team_context[catcher / 5u];
    uint16_t mode = actor->control_mode;
    uint16_t timer = actor->contact_action_timer_raw_60;
    uint16_t flags = actor->behavior_flags_raw;
    nba_gameplay_apply_catch_mode(
        tipoff->match_clock_raw_0928, &context->match_clock_raw_47,
        &mode, &timer, &flags);
    actor->control_mode = (uint8_t)mode;
    actor->contact_action_timer_raw_60 = timer;
    actor->behavior_flags_raw = flags;
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
    cpu_reset_play_control(tipoff);
    unsigned base = offense_side ? 5u : 0u;
    tipoff->handler_actor = (uint8_t)(base + handler_slots[tipoff->possession_number % 4u]);
    tipoff->receiver_actor = (uint8_t)(base +
        ((handler_slots[tipoff->possession_number % 4u] + 1u) % 5u));
    /* The `$86:D3F9 -> $86:BAA2` jump-ball ownership commit consumes play
     * `$35` record zero's pending selectors before installing actor 8. */
    cpu_apply_catch_prefix(tipoff, tipoff->handler_actor);
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
    cpu_apply_catch_mode(tipoff, tipoff->handler_actor);
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

static bool cpu_inbound_completion_witness(const NbaTipoff *tipoff) {
    return tipoff->live_state_raw == 0x82u &&
        (tipoff->inbound_transfer_raw != 0u ||
         tipoff->pass_receiver_raw >= 0);
}

static bool cpu_inbound_completion_witness_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.live_state_raw = 0x82u;
    state.pass_receiver_raw = -1;
    if (cpu_inbound_completion_witness(&state)) return false;
    state.inbound_transfer_raw = 1u;
    if (!cpu_inbound_completion_witness(&state)) return false;
    state.inbound_transfer_raw = 0u;
    state.pass_receiver_raw = 4;
    if (!cpu_inbound_completion_witness(&state)) return false;
    state.live_state_raw = 0u;
    return !cpu_inbound_completion_witness(&state);
}

static void cpu_apply_ball_acquisition_core(NbaTipoff *tipoff,
                                            uint8_t catcher) {
    /* `$86:BAA2-$BC99` is shared by pass catches and loose rebounds. It
     * installs the collision winner without mass-resetting the other nine
     * actor modes, clears the old play opportunities, and requests the next
     * ROM strategy through `$0994`. */
    unsigned side = catcher / 5u;
    uint16_t side_group = side ? 5u : 0u;
    bool side_change = (int16_t)tipoff->dead_ball_raw_097e >= 0 ?
        tipoff->dead_ball_raw_097e != side_group :
        tipoff->camera_side_group_raw != side_group;
    NbaTipoffActor *catcher_state = &tipoff->actors[catcher];
    cpu_apply_catch_prefix(tipoff, catcher);
    cpu_apply_catch_mode(tipoff, catcher);
    bool special_finish = catcher_state->control_mode == 14u;
    if (side_change) {
        ++tipoff->possession_number;
        /* `$86:BB3B-$BB6E` restores all actor +$74 assignments from +$76
         * and raises `$09D6`; the end-frame AF5C/BC07 pair owns modes. */
        for (unsigned actor = 0;
             actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            NbaTipoffActor *state = &tipoff->actors[actor];
            state->assignment_current_raw = state->assignment_base_raw;
            state->assignment_actor =
                (uint8_t)(state->assignment_current_raw >> 1);
        }
    }
    /* `$86:BB17-$BB81` reaches the rebuild/clock block only when the
     * catcher changes the active side (or completes the live `$82`
     * inbound). Ordinary same-side passes preserve both clocks and the
     * existing assignments. */
    if (side_change) {
        tipoff->role_rebuild_raw_09d6 = NBA_GAMEPLAY_UNKNOWN_WORD;
        tipoff->play_request_raw = 1u;
        tipoff->rim_raw_092c = 0x05A0u;
        tipoff->free_throw_flight_timer_raw_0930 = 0x0258u;
        tipoff->dead_ball_raw_096c = 0u;
        tipoff->team_context[side].previous_dead_ball_actor_raw_43 =
            NBA_GAMEPLAY_UNKNOWN_WORD;
        tipoff->team_context[side].previous_controller_actor_raw_45 = -1;
    }
    tipoff->offense_side = (uint8_t)side;
    tipoff->possession_team = (int8_t)side;
    tipoff->camera_side_group_raw = (uint8_t)side_group;
    tipoff->possession_frame = 0u;
    tipoff->play_state_frame = 0u;
    tipoff->handler_actor = catcher;
    tipoff->receiver_actor = catcher;
    /* BAA2 installs `$093E` but does not rewrite the ball record. The later
     * common attached-ball tail performs that projection. Preserve the
     * collision-frame position and velocity here while exposing ownership
     * to the host state machine. */
    tipoff->ball.owner_actor = (int8_t)catcher;
    tipoff->ball.state = NBA_BALL_ATTACHED;
    tipoff->cpu_play_state = special_finish ?
        NBA_CPU_PLAY_SHOT : NBA_CPU_PLAY_ATTACK;
    tipoff->shot_result_resolved = false;
    tipoff->shot_inner_veto_raw = false;
    tipoff->shot_miss_index_raw = 0xFFu;
    /* `$86:BC81-$BC90` runs after rebound/stat classification. In particular,
     * BC8A clears the rim context which otherwise traps formation roles 3/4
     * in `$85:AF2A` on the next possession. `$096E` is a separate cooldown
     * and is deliberately preserved. Owner installation makes `$85:A7A8`
     * clear `$0962` at the following common ball tail. */
    tipoff->ball_activity_raw = 0u;
    tipoff->rim_raw_094a = 0u;
    tipoff->shot_value_raw = 0u;
    tipoff->free_throw_resolution_raw_0972 = 0u;
    tipoff->rim_raw_096a = 0u;
    tipoff->rim_raw_097c = 0u;
    tipoff->dead_ball_raw_097e = NBA_GAMEPLAY_UNKNOWN_WORD;
}

static void cpu_commit_ball_acquisition(NbaTipoff *tipoff, uint8_t catcher) {
    /* `$86:D34A-$D3C5`: mark the collision, run the shared BAA2 ownership
     * installer, then clear only the pass globals owned by this continuation.
     * `$09C4/$09DA`, the inbound timer, and the ball record are not reset here. */
    bool completion_witness = cpu_inbound_completion_witness(tipoff);
    tipoff->rim_raw_13e7 |= 0x0010u;
    cpu_apply_ball_acquisition_core(tipoff, catcher);
    if (completion_witness) {
        if (tipoff->live_state_raw == 0x82u && tipoff->play_code < 6u)
            tipoff->play_request_raw = 1u;
        if (tipoff->fouls.whistle_active_raw_09b6 == 0u)
            tipoff->live_state_raw = 0u;
    } else if (tipoff->live_state_raw != 0x81u &&
               tipoff->live_state_raw != 0x82u &&
               tipoff->fouls.whistle_active_raw_09b6 == 0u) {
        tipoff->live_state_raw = 0u;
    }
    tipoff->pass_actor_raw = -1;
    tipoff->pass_aux_raw = -1;
    tipoff->pass_receiver_raw = -1;
    tipoff->inbound_transfer_raw = 0u;
}

void nba_tipoff_replay_ball_acquisition(NbaTipoff *tipoff, uint8_t catcher) {
    if (!tipoff || catcher >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    cpu_commit_ball_acquisition(tipoff, catcher);
}

void nba_tipoff_replay_ball_acquisition_core(NbaTipoff *tipoff,
                                             uint8_t catcher) {
    if (!tipoff || catcher >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    cpu_apply_ball_acquisition_core(tipoff, catcher);
}

static bool cpu_ball_acquisition_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.offense_side = 0u;
    state.handler_actor = 1u;
    state.possession_actor = 1;
    state.pass_actor_raw = 1;
    state.pass_aux_raw = 3;
    state.pass_receiver_raw = 7;
    state.ball_activity_raw = 0xFFFFu;
    state.rim_raw_094a = 29u;
    state.rim_force_raw_1866 = 1u;
    state.dead_ball_raw_0968 = 2u;
    state.rim_raw_096a = 2u;
    state.play_aux_selector_raw_09a6 = 3;
    state.team_context[1].dead_ball_actor_raw_3f = 9u;
    state.team_context[1].controller_actor_raw_41 = 2;
    state.dead_ball_raw_096c = 3u;
    state.dead_ball_raw_097e = 0u;
    state.actors[7].control_mode = 3u;
    state.actors[7].controller_assignment_raw = -1;
    state.actors[7].velocity_x = 0x30;
    state.actors[7].velocity_y = -0x20;
    state.actors[7].velocity_z = 0x123;
    state.actors[7].movement_magnitude_raw = 0x80u;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        state.actors[i].assignment_base_raw = (uint16_t)(i * 2u);
        state.actors[i].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    }
    cpu_commit_ball_acquisition(&state, 7u);
    if (state.possession_actor != 7 || state.ball.owner_actor != 7 ||
        state.actors[7].control_mode != 11u ||
        state.actors[7].velocity_z != 0x123 ||
        state.actors[7].catcher_latch_raw_ae != 1u ||
        state.actors[7].velocity_x != 0x30 ||
        state.actors[7].velocity_y != -0x20 ||
        state.role_rebuild_raw_09d6 != NBA_GAMEPLAY_UNKNOWN_WORD ||
        state.rim_raw_092c != 0x05A0u ||
        state.free_throw_flight_timer_raw_0930 != 0x0258u ||
        state.pass_actor_raw != -1 || state.pass_aux_raw != -1 ||
        state.pass_receiver_raw != -1 || state.ball_activity_raw != 0u ||
        state.rim_raw_094a != 0u || state.rim_raw_096a != 0u ||
        state.rim_force_raw_1866 != 0u ||
        state.dead_ball_raw_0968 != 0u || state.dead_ball_raw_096c != 0u ||
        state.dead_ball_raw_097e != NBA_GAMEPLAY_UNKNOWN_WORD ||
        state.play_aux_selector_raw_09a6 != -1 ||
        state.catch_actor_record_raw_0910 != 0x3BEBu ||
        state.catch_context_record_raw_0912 != 0x476Bu ||
        state.team_context[1].previous_dead_ball_actor_raw_43 !=
            NBA_GAMEPLAY_UNKNOWN_WORD ||
        state.team_context[1].previous_controller_actor_raw_45 != -1 ||
        state.team_context[1].dead_ball_actor_raw_3f != 7u ||
        state.team_context[1].controller_actor_raw_41 != -1)
        return false;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        if (state.actors[i].assignment_current_raw !=
            state.actors[i].assignment_base_raw) return false;
    NbaTipoff slow;
    memset(&slow, 0, sizeof(slow));
    slow.actors[2].velocity_x = 0x20;
    slow.actors[2].velocity_y = -0x10;
    slow.actors[2].movement_magnitude_raw = 0x7Fu;
    cpu_apply_catch_prefix(&slow, 2u);
    if (slow.actors[2].velocity_x != 0 ||
        slow.actors[2].velocity_y != 0 ||
        slow.actors[2].movement_magnitude_raw != 0u) return false;
    return true;
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
        (int16_t)(fp_integer_word(other->x_fp) -
                  fp_integer_word(state->x_fp)),
        (int16_t)(fp_integer_word(other->y_fp) -
                  fp_integer_word(state->y_fp)),
        &distance);
    state->assignment_actor = (uint8_t)paired;
    /* `$85:BC6B-$BC7A`: coincident actors (direction 8) retain both prior
     * +$86 direction words while still receiving the zero +$8A distance. */
    if (direction != 8u) state->assignment_direction = direction;
    state->assignment_distance = distance;
    state->pair_distance = distance;
    if (direction != 8u)
        other->assignment_direction = (uint8_t)(direction ^ 4u);
    other->assignment_distance = distance;
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
    tipoff->actors[paired].assignment_distance = 0x0140u;
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

void nba_tipoff_refresh_offense_roles_end_frame(NbaTipoff *tipoff) {
    unsigned offense = (tipoff->live_state_raw == 0x82u ?
        tipoff->inbound_state_raw : tipoff->camera_side_group_raw) != 0u ?
        1u : 0u;
    bool scan_predicted_ball = tipoff->live_state_raw == 0x82u ||
        (tipoff->possession_actor < 0 && tipoff->pass_receiver_raw < 0);
    int16_t predicted_ball_x = tipoff->role_focal_x_raw_0918;
    int16_t predicted_ball_y = tipoff->role_focal_y_raw_091a;
    uint16_t nearest_distance = 0x7FFFu;
    unsigned nearest_actor = offense * 5u;
    NbaGameplayLoosePursuitActor offense_actors[5];
    uint8_t offense_modes[5];
    for (unsigned i = 0; i < 5u; ++i) {
        NbaTipoffActor *state = &tipoff->actors[offense * 5u + i];
        uint16_t distance = 0u;
        uint8_t direction = nba_gameplay_pass_direction(
            (int16_t)(tipoff->team_context[offense].anchor_x_raw_0a -
                      fp_integer_word(state->x_fp)),
            (int16_t)(0 - fp_integer_word(state->y_fp)), &distance);
        state->anchor_distance_raw = distance;
        if (direction != 0x10u) state->anchor_direction_raw = direction;
        if (distance < nearest_distance) {
            nearest_distance = distance;
            nearest_actor = offense * 5u + i;
        }
        offense_actors[i] = (NbaGameplayLoosePursuitActor){
            fp_round(state->x_fp), fp_round(state->y_fp), state->control_mode
        };
    }
    tipoff->role_nearest_offense_raw_09de =
        (uint16_t)(0x34EBu + nearest_actor * 0x100u);
    tipoff->role_ownerless_raw_09d8 = scan_predicted_ball ? 1u : 0u;
    (void)nba_gameplay_select_no_owner_pursuer(
        offense_actors, predicted_ball_x, predicted_ball_y, offense_modes);
    if (!scan_predicted_ball) {
        for (unsigned i = 0; i < 5u; ++i)
            offense_modes[i] = offense_actors[i].control_mode < 7u ?
                1u : offense_actors[i].control_mode;
    }
    for (unsigned i = 0; i < 5u; ++i)
        tipoff->actors[offense * 5u + i].control_mode = offense_modes[i];
}

static void cpu_rebuild_role_assignments(NbaTipoff *tipoff,
                                         unsigned offense,
                                         unsigned defense) {
    /* `$85:BD0D-$BE03`: reset offense alternates, then restore both teams'
     * base pairings. B95C only refreshes +$7E/+60 and therefore has no
     * represented side effect in this planner boundary. */
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned actor = offense * 5u + i;
        NbaTipoffActor *state = &tipoff->actors[actor];
        state->assignment_current_raw = state->assignment_alternate_raw;
        if (state->control_mode < 7u &&
            actor != (unsigned)(uint8_t)tipoff->possession_actor)
            state->behavior_timer = 0x2Fu;
    }
    for (unsigned i = 0; i < 5u; ++i) {
        NbaTipoffActor *state = &tipoff->actors[defense * 5u + i];
        if (state->control_mode < 7u) state->control_mode = 2u;
        state->saved_control_mode = state->control_mode;
        state->assignment_current_raw = state->assignment_base_raw;
        if (state->control_mode < 7u) state->behavior_timer = 0u;
    }
    for (unsigned i = 0; i < 5u; ++i) {
        NbaTipoffActor *state = &tipoff->actors[offense * 5u + i];
        if (state->control_mode < 7u) {
            state->control_mode =
                tipoff->live_state_raw == 0x82u &&
                state->team_group_raw_6e == tipoff->inbound_state_raw ?
                1u : 2u;
        }
        state->saved_control_mode = state->control_mode;
        state->assignment_current_raw = state->assignment_base_raw;
        if (state->control_mode < 7u) state->behavior_timer = 0u;
    }
    tipoff->role_rebuild_raw_09d6 = 0u;
}

void nba_tipoff_refresh_defense_roles_end_frame(NbaTipoff *tipoff) {
    /* `$85:BC07-$C0F5`, entered after `$85:AF5C` has already normalized
     * offense. Keep this boundary independently replayable against Mesen. */
    unsigned offense = (tipoff->live_state_raw == 0x82u ?
        tipoff->inbound_state_raw : tipoff->camera_side_group_raw) != 0u ?
        1u : 0u;
    unsigned defense = offense ^ 1u;
    uint16_t focal_distance[5] = {0};
    unsigned nearest_focal = defense * 5u;
    unsigned nearest_anchor = defense * 5u;
    unsigned nearest_offense_anchor = offense * 5u;
    uint16_t nearest_focal_distance = 0x7FFFu;
    uint16_t nearest_anchor_distance = 0x7FFFu;
    uint16_t nearest_offense_distance = 0x7FFFu;
    int16_t focal_x = tipoff->role_focal_x_raw_0918;
    int16_t focal_y = tipoff->role_focal_y_raw_091a;
    if (tipoff->possession_actor >= 0 &&
        tipoff->possession_actor < NBA_GAMEPLAY_ACTOR_COUNT) {
        focal_x = fp_integer_word(
            tipoff->actors[(unsigned)tipoff->possession_actor].x_fp);
        focal_y = fp_integer_word(
            tipoff->actors[(unsigned)tipoff->possession_actor].y_fp);
    }
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned actor = defense * 5u + i;
        NbaTipoffActor *state = &tipoff->actors[actor];
        uint16_t focal = 0u;
        (void)nba_gameplay_target_direction(
            (int16_t)(fp_integer_word(state->x_fp) - focal_x),
            (int16_t)(fp_integer_word(state->y_fp) - focal_y), &focal);
        focal_distance[i] = focal;
        state->focal_distance_raw_8e = focal;
        if (focal < nearest_focal_distance) {
            nearest_focal_distance = focal;
            nearest_focal = actor;
        }
        cpu_refresh_pair_geometry(tipoff, actor);
    }

    if (tipoff->role_rebuild_raw_09d6 != 0u) {
        tipoff->role_cadence_raw_09d2 = 0x1Eu;
        cpu_rebuild_role_assignments(tipoff, offense, defense);
    } else {
        tipoff->role_cadence_raw_09d2 =
            (uint16_t)(tipoff->role_cadence_raw_09d2 - 2u);
        if ((int16_t)tipoff->role_cadence_raw_09d2 >= 0) return;
        tipoff->role_cadence_raw_09d2 =
            (uint16_t)(tipoff->role_cadence_raw_09d2 + 0x1Eu);
        if ((int8_t)tipoff->camera_side_group_raw < 0) return;
    }

    /* `$85:BE06` deliberately uses the *entry/offense* context +$0A while
     * measuring the five defenders. It is the basket they protect, not the
     * defender context's own anchor. +$88 is untouched in this routine. */
    int16_t defense_basket_x =
        tipoff->team_context[offense].anchor_x_raw_0a;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        NbaTipoffActor *state = &tipoff->actors[actor];
        unsigned side = actor / 5u;
        if (side == offense &&
            state->anchor_distance_raw < nearest_offense_distance) {
            nearest_offense_distance = state->anchor_distance_raw;
            nearest_offense_anchor = actor;
        }
        if (side == defense) {
            (void)nba_gameplay_target_direction(
                (int16_t)(defense_basket_x -
                          fp_integer_word(state->x_fp)),
                (int16_t)-fp_integer_word(state->y_fp),
                &state->anchor_distance_raw);
            if (state->anchor_distance_raw < nearest_anchor_distance) {
                nearest_anchor_distance = state->anchor_distance_raw;
                nearest_anchor = actor;
            }
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
            if ((int16_t)(fp_round(other->x_fp) ^ defense_basket_x) < 0 ||
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
        /* Assignment helpers `$85:BAE4/$BB6C` read the active defense
         * context's own +$0A. This is intentionally the opposite sign from
         * `$BA`, used by the preceding protected-basket cleanup. */
        int16_t anchor = tipoff->team_context[defense].anchor_x_raw_0a;
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
        /* `$85:BF89-$BF8E` reloads the owner's +$74 after the repair above;
         * that repair can displace the owner's former primary defender. */
        primary = owner->assignment_current_raw >> 1;
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
    int16_t defense_anchor =
        tipoff->team_context[defense].anchor_x_raw_0a;
    tipoff->pass_distance_raw = 5u; /* `$09DA` is reused as loop counter. */
    for (unsigned i = 0; i < 5u; ++i) {
        unsigned target = tipoff->team_context[defense].actor_order_raw_49[i] >> 1;
        if (target < NBA_GAMEPLAY_ACTOR_COUNT &&
            (int16_t)tipoff->actors[target].assignment_current_raw < 0 &&
            !cpu_try_base_defender(tipoff, target, defense, defense_anchor) &&
            !cpu_bind_nearest_unassigned(tipoff, target, defense))
            return; /* `$85:C051`: retain the remaining counter. */
        --tipoff->pass_distance_raw;
    }
    tipoff->role_rebuild_raw_09d6 = 0u;
}

void nba_tipoff_refresh_team_roles_end_frame(NbaTipoff *tipoff) {
    /* `$87:8FA1-$8FA9`: offense `$85:AF5C`, then defense `$85:BC07`. */
    nba_tipoff_refresh_offense_roles_end_frame(tipoff);
    nba_tipoff_refresh_defense_roles_end_frame(tipoff);
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
        cpu_ease_display_direction(state);
        if (state->contact_inhibit_raw_5a != 0u)
            state->contact_inhibit_raw_5a =
                state->contact_inhibit_raw_5a > 2u ?
                (uint16_t)(state->contact_inhibit_raw_5a - 2u) : 0u;
        if (state->recovery_inhibit_raw != 0u)
            state->recovery_inhibit_raw = state->recovery_inhibit_raw > 2u ?
                (uint16_t)(state->recovery_inhibit_raw - 2u) : 0u;
        cpu_move_actor(tipoff, actor);
        cpu_clamp_actor_to_court(&state->x_fp, &state->y_fp,
                                 &state->velocity_x, &state->velocity_y);
        cpu_advance_actor_animation(tipoff, state);
        (void)cpu_actor_contact_height(
            tipoff, actor, &state->contact_height_raw_aa);
        /* The renderer resolves the post-dispatch resource phase. Mirror
         * `$87:B649/$B832` against that same phase so an attached mode-15
         * ball cannot lag the visible passing hand by one host tick. */
        if (state->control_mode == 15u &&
            tipoff->ball.owner_actor == (int8_t)actor)
            ball_attach_to_actor(tipoff, actor);
    }
}

static void cpu_update_actor_behaviors(NbaTipoff *tipoff) {
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        if (cpu_move_inbound_actor(tipoff, actor)) continue;
        cpu_dispatch_normal_actor_behavior(tipoff, actor);
    }
}

static void cpu_schedule_actor_behaviors(NbaTipoff *tipoff,
                                         bool acquisition_boundary) {
    /* `$87:9244` normally follows the even-tick physics/global passes. A
     * BAA2 ownership install at that boundary can cross the outer-frame
     * callback: Mesen sees its mode changes on the following odd frame, then
     * the next even pass both consumes and replaces that motion. Preserve
     * that single deferred dispatch without delaying every behavior pass. */
    if ((tipoff->simulation_tick & 1u) == 0u) {
        if (acquisition_boundary) {
            tipoff->actor_behavior_pending = 1u;
            return;
        }
        cpu_update_actor_behaviors(tipoff);
        return;
    }
    if (tipoff->actor_behavior_pending != 0u) {
        tipoff->actor_behavior_pending = 0u;
        cpu_update_actor_behaviors(tipoff);
    }
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

typedef struct {
    int16_t x, y;
    uint8_t direction;
} CpuFreeThrowTarget;

/* `$87:A15C-$A360`: the shooting side occupies four lane records plus the
 * stripe record; each player's +$76 pair occupies the matching defender
 * record.  Only X mirrors when the shooting basket is on the left. */
static void cpu_free_throw_targets(const NbaTipoff *tipoff, uint8_t shooter,
                                   CpuFreeThrowTarget out[10]) {
    static const CpuFreeThrowTarget primary[5] = {
        {298, -56, 0}, {298, 56, 4}, {112, -104, 7},
        {112, 88, 6}, {210, 0, 6}
    };
    static const CpuFreeThrowTarget paired[5] = {
        {330, -56, 0}, {330, 56, 4}, {136, -104, 7},
        {136, 104, 6}, {274, -56, 0}
    };
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        out[i] = (CpuFreeThrowTarget){fp_round(tipoff->actors[i].x_fp),
                                     fp_round(tipoff->actors[i].y_fp),
                                     tipoff->actors[i].direction};
    unsigned base = (shooter / 5u) * 5u;
    unsigned shooter_local = shooter % 5u;
    for (unsigned local = 0; local < 5u; ++local) {
        unsigned actor = base + local;
        unsigned record = local == shooter_local ? 4u :
            (local > shooter_local ? local - 1u : local);
        out[actor] = primary[record];
        unsigned pair = tipoff->actors[actor].assignment_base_raw >> 1;
        if (pair < NBA_GAMEPLAY_ACTOR_COUNT) out[pair] = paired[record];
    }
    if (shooter < 5u) {
        for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
            out[i].x = (int16_t)-out[i].x;
    }
}

/* `$85:B3AA` with speed argument three, consumed on the 30-Hz actor pass.
 * The ROM's dt=2 makes the largest world-coordinate step six pixels. */
static bool cpu_free_throw_move_actor(NbaTipoffActor *actor,
                                      const CpuFreeThrowTarget *target) {
    int x = fp_round(actor->x_fp), y = fp_round(actor->y_fp);
    int dx = target->x - x, dy = target->y - y;
    int step_x = dx < -6 ? -6 : dx > 6 ? 6 : dx;
    int step_y = dy < -6 ? -6 : dy > 6 ? 6 : dy;
    if (dx == 0 && dy == 0) {
        actor->velocity_x = actor->velocity_y = 0;
        actor->movement_magnitude_raw = 0u;
        actor->direction = target->direction;
        actor->requested_direction = target->direction;
        return true;
    }
    actor->velocity_x = (int16_t)(step_x * 128);
    actor->velocity_y = (int16_t)(step_y * 128);
    actor->x_fp += (int32_t)step_x * 256;
    actor->y_fp += (int32_t)step_y * 256;
    actor->movement_magnitude_raw = actor_distance(step_x, step_y);
    actor->direction = nba_gameplay_target_direction(
        (int16_t)dx, (int16_t)dy, NULL);
    actor->requested_direction = actor->direction;
    return false;
}

static uint8_t cpu_free_throw_threshold(uint8_t rating) {
    static const uint8_t thresholds[8] = {
        130u, 145u, 160u, 185u, 200u, 215u, 230u, 245u
    };
    unsigned index = rating > 0x80u ? (rating - 0x80u) >> 4 : 0u;
    if (index > 7u) index = 7u;
    return thresholds[index];
}

static uint8_t cpu_free_throw_rating(const NbaTipoff *tipoff,
                                     uint8_t shooter) {
    uint8_t rating = 0x80u;
    uint8_t team = shooter < 5u ? tipoff->session->left_team :
                                 tipoff->session->right_team;
    (void)nba_player_gameplay_free_throw_rating(
        tipoff->assets, team, tipoff->actors[shooter].roster_slot, &rating);
    return rating;
}

static bool cpu_free_throw_launch_vector(uint8_t rating, uint8_t roll,
                                         uint8_t choice, uint8_t half,
                                         bool left_basket, int16_t *vx,
                                         int16_t *vy, int16_t *vz) {
    static const int16_t miss[2][4][3] = {
        {{512, 0, 800}, {608, 0, 800}, {576, 0, 848}, {608, 0, 800}},
        {{512, 0, 800}, {512, 0, 800}, {544, 0, 864}, {512, 0, 880}}
    };
    bool missed = roll >= cpu_free_throw_threshold(rating);
    unsigned table_half = half != 0u;
    unsigned table_choice = choice & 3u;
    *vx = missed ? miss[table_half][table_choice][0] : 512;
    *vy = missed ? miss[table_half][table_choice][1] : 0;
    *vz = missed ? miss[table_half][table_choice][2] : 864;
    if (left_basket) *vx = (int16_t)-*vx;
    return missed;
}

/* `$86:A2A7-$A45E`: the free throw has a second, independent rating roll.
 * Success uses the fixed stripe launch; failure selects one of the four
 * ROM velocity records from actor +$A8's half. */
static void cpu_release_free_throw(NbaTipoff *tipoff, uint8_t shooter) {
    NbaTipoffActor *actor = &tipoff->actors[shooter];
    uint8_t rating = cpu_free_throw_rating(tipoff, shooter);
    uint8_t roll = (uint8_t)nba_gameplay_rng_next(&tipoff->rng);
    uint8_t choice = roll >= cpu_free_throw_threshold(rating) ?
        (uint8_t)(nba_gameplay_rng_next(&tipoff->rng) & 3u) : 0u;
    int16_t vx, vy, vz;
    bool miss_roll = cpu_free_throw_launch_vector(
        rating, roll, choice,
        (uint8_t)actor->free_throw_launch_half_raw_a8,
        fp_round(actor->x_fp) < 0, &vx, &vy, &vz);
    ball_position_at_actor(tipoff, shooter);
    tipoff->ball.velocity_x = vx;
    tipoff->ball.velocity_y = vy;
    tipoff->ball.velocity_z = vz;
    tipoff->ball.owner_actor = -1;
    tipoff->ball.state = NBA_BALL_SHOT;
    tipoff->possession_actor = -1;
    tipoff->shot_actor_raw_09c8 = (int16_t)shooter;
    tipoff->shot_value_raw = 1u;
    tipoff->rim_raw_096a = 1u;
    tipoff->live_state_raw = 1u;
    tipoff->free_throw_flight_timer_raw_0930 = 1800u;
    tipoff->dead_ball_raw_0966 = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->shot_inner_veto_raw = miss_roll;
    tipoff->shot_result_resolved = false;
    actor_set_upper_animation(actor, 23u);
    actor->control_mode = 11u;
}

/* `$87:9CBF-$A017`: portable CPU free-throw scene. Human aiming graphics,
 * PPU scheduling, and controller reassignment are intentionally outside the
 * C gameplay boundary; lineup, possession, timing, ratings and ball launch
 * remain literal game behavior. */
static bool cpu_update_free_throw_scene(NbaTipoff *tipoff) {
    uint16_t *state = &tipoff->fouls.free_throw_state_raw_0978;
    if (*state == 0u) return false;
    if ((tipoff->simulation_tick & 1u) != 0u) return true;
    uint8_t shooter = tipoff->fouls.victim_actor_raw >= 0 &&
                      tipoff->fouls.victim_actor_raw < NBA_GAMEPLAY_ACTOR_COUNT ?
        (uint8_t)tipoff->fouls.victim_actor_raw : tipoff->handler_actor;
    if (shooter >= NBA_GAMEPLAY_ACTOR_COUNT) shooter = 0u;
    tipoff->handler_actor = shooter;
    tipoff->offense_side = shooter / 5u;
    tipoff->possession_team = (int8_t)tipoff->offense_side;

    if (*state == 1u) {
        CpuFreeThrowTarget targets[10];
        cpu_free_throw_targets(tipoff, shooter, targets);
        tipoff->rim_raw_097c = 0u;
        tipoff->pass_receiver_raw = -1;
        tipoff->fouls.whistle_active_raw_09b6 = 0u;
        if (tipoff->ball.owner_actor >= 0 &&
            tipoff->ball.owner_actor != (int8_t)shooter) {
            ball_position_at_actor(tipoff, (unsigned)tipoff->ball.owner_actor);
            tipoff->ball.owner_actor = -1;
            tipoff->possession_actor = -1;
            tipoff->ball.state = NBA_BALL_BOUNCE;
        }
        bool ready = true;
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            CpuFreeThrowTarget target = targets[actor];
            if (actor == shooter && tipoff->ball.owner_actor < 0) {
                target.x = fp_round(tipoff->ball.x_fp);
                target.y = fp_round(tipoff->ball.y_fp);
            }
            if (!cpu_free_throw_move_actor(&tipoff->actors[actor], &target))
                ready = false;
            cpu_advance_actor_animation(tipoff, &tipoff->actors[actor]);
        }
        if (tipoff->ball.owner_actor < 0) {
            (void)cpu_update_live_ball(tipoff);
            if (cpu_actor_body_contacts_ball(tipoff, shooter, true, 16u)) {
                tipoff->possession_actor = (int8_t)shooter;
                tipoff->ball.owner_actor = (int8_t)shooter;
                tipoff->ball.state = NBA_BALL_ATTACHED;
                tipoff->actors[shooter].control_mode = 11u;
                ball_attach_to_actor(tipoff, shooter);
            }
            ready = false;
        } else if (tipoff->ball.owner_actor == (int8_t)shooter) {
            ball_attach_to_actor(tipoff, shooter);
        }
        if (ready && tipoff->possession_actor >= 0 &&
            tipoff->fouls.whistle_timer_raw_08de < 0) {
            tipoff->free_throw_start_tick_raw_09be =
                (uint16_t)tipoff->simulation_tick;
            tipoff->shot_origin_x = fp_round(tipoff->actors[shooter].x_fp);
            tipoff->shot_origin_y = fp_round(tipoff->actors[shooter].y_fp);
            *state = 3u; /* `$85:9530` consumes transient state 2. */
        }
        return true;
    }
    if (*state == 3u) {
        NbaTipoffActor *actor = &tipoff->actors[shooter];
        actor_set_upper_animation(actor, 2u);
        uint16_t elapsed = (uint16_t)(tipoff->simulation_tick -
                                      tipoff->free_throw_start_tick_raw_09be);
        if (elapsed < 120u) return true;
        actor_set_upper_animation(actor, 12u);
        if (elapsed < 360u &&
            (nba_gameplay_rng_next(&tipoff->rng) & 0x0B2Au) != 0x0B2Au)
            return true;
        uint8_t threshold = cpu_free_throw_threshold(
            cpu_free_throw_rating(tipoff, shooter));
        uint8_t roll = (uint8_t)nba_gameplay_rng_next(&tipoff->rng);
        if (roll < threshold) {
            tipoff->free_throw_aim_y_raw_0982 = 29u;
            tipoff->free_throw_aim_x_raw_0980 = 16u;
        } else {
            tipoff->free_throw_aim_y_raw_0982 =
                (uint16_t)((tipoff->rng.state & 31u) + 12u);
            tipoff->free_throw_aim_x_raw_0980 =
                (uint16_t)((nba_gameplay_rng_next(&tipoff->rng) & 31u) + 12u);
        }
        *state = 9u;
        tipoff->ball_activity_raw = 1u;
        actor_set_animation(actor, 22u, 22u);
        actor->behavior_flags_raw |= 4u;
        actor->control_mode = 20u;
        return true;
    }
    if (*state == 9u) {
        NbaTipoffActor *actor = &tipoff->actors[shooter];
        cpu_advance_actor_animation(tipoff, actor);
        if (actor->lower_animation_tick < 6u) {
            ball_attach_to_actor(tipoff, shooter);
            return true;
        }
        cpu_release_free_throw(tipoff, shooter);
        if (tipoff->fouls.free_throw_sequence_raw_097a != 0u)
            --tipoff->fouls.free_throw_sequence_raw_097a;
        *state = 10u;
        return true;
    }
    if (*state == 10u) {
        NbaGameplayRimResult result = cpu_update_live_ball(tipoff);
        /* A make already ran `$85:A079-$A345` inline in its physics
         * substep. Only terminal misses need host-scene acknowledgement. */
        if (!tipoff->shot_result_resolved &&
            (result == NBA_GAMEPLAY_RIM_EDGE_CONTACT ||
             result == NBA_GAMEPLAY_RIM_MISS)) {
            tipoff->shot_result_resolved = true;
        }
        if (tipoff->fouls.free_throw_sequence_raw_097a != 0u) {
            if (tipoff->rim_raw_097c == 0u && tipoff->shot_value_raw != 0u)
                return true;
            if (shooter >= NBA_GAMEPLAY_ACTOR_COUNT ||
                fp_integer_word(tipoff->ball.z_fp) >= 8) return true;
            tipoff->ball.x_fp = tipoff->actors[shooter].x_fp;
            tipoff->ball.y_fp = tipoff->actors[shooter].y_fp;
            tipoff->ball.z_fp = 32 * 256;
            tipoff->ball.velocity_x = tipoff->ball.velocity_y = 0;
            tipoff->ball.velocity_z = 0;
            tipoff->ball.owner_actor = -1;
            tipoff->ball.state = NBA_BALL_LOOSE;
            *state = 11u;
            return true;
        }
        if (tipoff->ball.owner_actor < 0 && tipoff->ball.velocity_z < 0 &&
            (fp_integer_word(tipoff->ball.z_fp) < 24 ||
             tipoff->rim_raw_097c != 0u ||
             tipoff->free_throw_resolution_raw_0972 != 0u ||
             tipoff->shot_value_raw == 0u))
            *state = 0u;
        return true;
    }
    if (*state >= 11u) {
        ++*state;
        if (*state >= 25u) *state = 1u;
        return true;
    }
    return true;
}

static bool cpu_free_throw_scene_self_test(void) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        state.actors[i].assignment_base_raw =
            (uint16_t)(((i + 5u) % NBA_GAMEPLAY_ACTOR_COUNT) * 2u);
    CpuFreeThrowTarget target[10];
    cpu_free_throw_targets(&state, 2u, target);
    static const CpuFreeThrowTarget expected_left[10] = {
        {-298, -56, 0}, {-298, 56, 4}, {-210, 0, 6},
        {-112, -104, 7}, {-112, 88, 6}, {-330, -56, 0},
        {-330, 56, 4}, {-274, -56, 0}, {-136, -104, 7},
        {-136, 104, 6}
    };
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        if (target[i].x != expected_left[i].x ||
            target[i].y != expected_left[i].y ||
            target[i].direction != expected_left[i].direction)
            return false;
    /* The same local-slot layout mirrors to the positive basket; paired
     * defenders occupy the opposite five physical actor records. */
    cpu_free_throw_targets(&state, 7u, target);
    static const CpuFreeThrowTarget expected_right[10] = {
        {330, -56, 0}, {330, 56, 4}, {274, -56, 0},
        {136, -104, 7}, {136, 104, 6}, {298, -56, 0},
        {298, 56, 4}, {210, 0, 6}, {112, -104, 7},
        {112, 88, 6}
    };
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        if (target[i].x != expected_right[i].x ||
            target[i].y != expected_right[i].y ||
            target[i].direction != expected_right[i].direction)
            return false;
    if (cpu_free_throw_threshold(0x80u) != 130u ||
        cpu_free_throw_threshold(0xFFu) != 245u) return false;
    int16_t vx, vy, vz;
    if (cpu_free_throw_launch_vector(
            0xFFu, 0u, 3u, 1u, false, &vx, &vy, &vz) ||
        vx != 512 || vy != 0 || vz != 864) return false;
    if (cpu_free_throw_launch_vector(
            0xFFu, 0u, 0u, 0u, true, &vx, &vy, &vz) ||
        vx != -512 || vy != 0 || vz != 864) return false;
    if (!cpu_free_throw_launch_vector(
            0x80u, 130u, 1u, 0u, false, &vx, &vy, &vz) ||
        vx != 608 || vy != 0 || vz != 800) return false;
    state.fouls.free_throw_state_raw_0978 = 24u;
    state.handler_actor = 0u;
    state.simulation_tick = 2u;
    if (!cpu_update_free_throw_scene(&state) ||
        state.fouls.free_throw_state_raw_0978 != 1u) return false;
    return true;
}

/* `$86:F43A-$F653`: execute the inbound arrival/candidate/pass gates. The
 * surrounding 60-Hz clock decrements `$092E`; this 30-Hz actor pass reloads
 * it to 300 whenever the raw target box has not been reached. */
static void cpu_update_rom_inbound(NbaTipoff *tipoff) {
    /* `$86:F3D2` reaches F43A through the current actor dispatch (`X/$96`).
     * It does not compare that actor with provisional `$0954`. After an
     * A613 boundary cancellation, D353->BAA2 may install a teammate in
     * ownership `$093E`; that mode-11 carrier must be allowed to approach
     * `$0958/$095A` and retry even while `$0954` names the old inbounder. */
    if (tipoff->possession_actor < 0 ||
        tipoff->possession_actor >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    uint8_t inbounder = (uint8_t)tipoff->possession_actor;
    NbaTipoffActor *actor = &tipoff->actors[inbounder];
    /* `$86:F3F6-$F43A` reaches the inbound mode-11 branch only for the
     * collision-installed `$093E` owner. `$0954` starts as provisional slot
     * 2/7 and must not manufacture possession merely by reaching `$0958/A`. */
    if (actor->control_mode != 11u) return;
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

    /* `$86:F54F-$F555`: arrival writes 2 to `$0968` and the currently
     * unrepresented `$09F6` before freezing the inbounder. */
    tipoff->dead_ball_raw_0968 = 2u;
    actor->behavior_flags_raw |= 0x0040u;
    actor->velocity_x = actor->velocity_y = 0;
    actor->movement_magnitude_raw = 0u;
    actor->direction = (uint8_t)tipoff->inbound_direction_raw;
    actor->requested_direction = actor->direction;
    if (tipoff->inbound_ready_raw == 0u) {
        /* `$86:F56E-$F577`: arrival is the exact whistle/event-latch
         * release point, immediately before `$09BA` becomes one. */
        tipoff->fouls.whistle_active_raw_09b6 = 0u;
        tipoff->fouls.foul_event_raw_0964 = 0u;
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
    if (candidate >= 0 && candidate < NBA_GAMEPLAY_ACTOR_COUNT &&
        !cpu_inbound_side_gate(
            tipoff, inbounder, (uint8_t)candidate))
        candidate = -1;
    /* The port does not yet reproduce every `$85:AD6B` formation writer.
     * When its two cached selectors are both stranded on the forbidden side,
     * preserve the ROM F58F side gate and use the first eligible teammate
     * after the final 60-tick retry threshold. This prevents repeated
     * five-second violations without bypassing candidate validity. */
    if (candidate < 0 && tipoff->inbound_timer_raw < 60u) {
        unsigned first = (unsigned)(inbounder / 5u) * 5u;
        for (unsigned slot = first; slot < first + 5u; ++slot) {
            if (cpu_inbound_candidate_valid(
                    tipoff, inbounder, (int16_t)slot) &&
                cpu_inbound_side_gate(tipoff, inbounder, (uint8_t)slot)) {
                candidate = (int)slot;
                break;
            }
        }
    }
    if (candidate < 0 || candidate >= NBA_GAMEPLAY_ACTOR_COUNT) return;
    actor->reaction_threshold = 1u; /* `$86:F60B-$F610` */
    if (nba_tipoff_begin_rom_pass(
            tipoff, inbounder, (unsigned)candidate)) {
        tipoff->receiver_actor = (uint8_t)candidate;
        cpu_enter_play_state(tipoff, NBA_CPU_PLAY_PASS);
    }
}

static bool cpu_inbound_recovery_carrier_self_test(
        const NbaAssetPack *assets, NbaSession *session) {
    NbaTipoff state;
    memset(&state, 0, sizeof(state));
    state.assets = assets;
    state.session = session;
    state.live_state_raw = 0x82u;
    state.inbound_actor_raw = 1u; /* provisional `$0954` */
    state.possession_actor = 3;   /* recovered `$093E` carrier */
    state.inbound_target_x_raw = 246;
    state.inbound_target_y_raw = -22;
    state.inbound_direction_raw = 6u;
    state.inbound_timer_raw = 300u;
    state.actors[3].control_mode = 11u;
    state.actors[3].controller_assignment_raw = -1;
    state.actors[3].x_fp = 200 * 256;
    state.actors[3].y_fp = -22 * 256;
    if (!cpu_move_inbound_actor(&state, 3u) ||
        state.actors[3].velocity_x <= 0) return false;
    state.actors[3].x_fp = 246 * 256;
    state.actors[3].y_fp = -22 * 256;
    state.actors[3].velocity_x = 100;
    state.actors[3].velocity_y = -100;
    cpu_update_rom_inbound(&state);
    return state.inbound_actor_raw == 1u &&
           state.inbound_ready_raw == 1u &&
           state.dead_ball_raw_0968 == 2u &&
           state.actors[3].direction == 6u &&
           state.actors[3].velocity_x == 0 &&
           state.actors[3].velocity_y == 0;
}

static bool cpu_try_install_inbound_contact(NbaTipoff *tipoff) {
    if (tipoff->possession_actor >= 0 || tipoff->ball.owner_actor >= 0 ||
        tipoff->inbound_transfer_raw != 0u) return false;
    int contact = cpu_first_inbound_ball_contact(tipoff);
    if (contact < 0) return false;
    /* Initial dead-ball pickup is seeded by the `$85:A262` setup contract.
     * A post-release recovery instead reaches `$86:D353 -> BAA2`; that path
     * changes `$093E` ownership but does not reseed `$092E` or erase the
     * already-reached `$09BA` state. Reinitializing both here permits an
     * endless series of baseline A613 cancellations. */
    bool first_pickup = tipoff->inbound_ready_raw == 0u;
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
    tipoff->inbound_transfer_raw = 0u;
    if (first_pickup) {
        tipoff->inbound_ready_raw = 0u;
        tipoff->inbound_timer_raw = 300u;
    }
    return true;
}

static void cpu_update_possession(NbaTipoff *tipoff) {
    /* `$87:923D` globally diverts the actor pass to `$87:9CBF` while
     * `$0978` is active. This must precede the seeded `$0936=$82` inbound
     * path or that dead-ball scaffold steals the free-throw scene. */
    if (cpu_update_free_throw_scene(tipoff)) {
        ++tipoff->possession_frame;
        ++tipoff->play_state_frame;
        return;
    }
    if (tipoff->live_state_raw == 0x82u) {
        /* `$85:A262-$A268` seeds `$092E/$0A04=300`. `$86:F43A-$F653`
         * owns arrival, its sawtooth reload, receiver selection and AB2D. */
        if (tipoff->inbound_actor_raw < NBA_GAMEPLAY_ACTOR_COUNT) {
            /* `$85:C5AD-$C5BD` owns this target. `$85:AD86-$AD95` skips the
             * later formation overwrite whether `$0954` is still the
             * provisional slot or the collision-installed carrier. */
            tipoff->actors[tipoff->inbound_actor_raw].target_x =
                tipoff->inbound_target_x_raw;
            tipoff->actors[tipoff->inbound_actor_raw].target_y =
                tipoff->inbound_target_y_raw;
        }
        /* `$85:963D` completes the ten player integrations before the
         * `$86:D5DB/D652` sorted collision sweep. */
        cpu_update_all_actors(tipoff);
        (void)cpu_update_live_ball(tipoff);
        cpu_cache_predicted_ball_xy(tipoff);
        cpu_update_player_contacts(tipoff);
        nba_tipoff_update_play_control_end_frame(tipoff);
        nba_tipoff_refresh_team_roles_end_frame(tipoff);
        cpu_update_actor_behaviors(tipoff);
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
         * inbounder after the completed player pass. */
        if (cpu_try_install_inbound_contact(tipoff)) {
            /* The collision installs WRAM `$093E`, not the ball-record owner.
             * Keep that distinction on the installation frame as well as
             * subsequent dead-ball frames. */
            ball_position_at_actor(
                tipoff, (unsigned)tipoff->possession_actor);
            tipoff->ball.velocity_x = tipoff->ball.velocity_y = 0;
            tipoff->ball.velocity_z = 0;
            tipoff->ball.owner_actor = -1;
        }
        /* The inbound transfer still terminates through the shared
         * `$86:BAA2-$BC99` pass-catch acquisition boundary. */
        if (tipoff->cpu_play_state == NBA_CPU_PLAY_PASS &&
            tipoff->ball.owner_actor < 0) {
            int contact = cpu_first_pass_contact(tipoff);
            if (contact >= 0) {
                cpu_commit_ball_acquisition(tipoff, (uint8_t)contact);
                /* `$86:D3C5` returns to the common `$85:A7A8/$87:B649`
                 * owned-ball tail. BAA2 itself deliberately leaves the ball
                 * record at its collision point; the tail projects it. */
                ball_position_at_actor(tipoff, (unsigned)contact);
            }
        }
        if (tipoff->cpu_play_state == NBA_CPU_PLAY_PASS &&
            tipoff->ball.state == NBA_BALL_BOUNCE) {
            cpu_enter_play_state(tipoff, NBA_CPU_PLAY_REBOUND);
        }
        if (tipoff->live_state_raw == 0x82u &&
            tipoff->possession_actor >= 0 &&
            tipoff->inbound_transfer_raw == 0u)
            tipoff->ball.owner_actor = -1;
        if (tipoff->ball.owner_actor >= 0) {
            ++tipoff->possession_frame;
            ++tipoff->play_state_frame;
            return;
        }
        if (tipoff->inbound_timer_raw > 0u)
            --tipoff->inbound_timer_raw;
        /* `$87:9AA6` is the global `$092E` expiry consumer; it does not
         * require `$093E` to have acquired an inbounder first. */
        if (tipoff->inbound_timer_raw == 0u &&
            tipoff->inbound_transfer_raw == 0u) {
            cpu_reset_expired_inbound(tipoff);
        }
        cpu_update_rom_inbound(tipoff);
        if (tipoff->live_state_raw == 0x82u &&
            tipoff->possession_actor >= 0 &&
            tipoff->inbound_transfer_raw == 0u)
            tipoff->ball.owner_actor = -1;
        ++tipoff->possession_frame;
        ++tipoff->play_state_frame;
        return;
    }
    if (tipoff->possession_frame != 0u && tipoff->rim_raw_092c != 0u)
        --tipoff->rim_raw_092c;
    cpu_update_all_actors(tipoff);
    /* `$87:8F7D -> $85:963D` completes all ten actor commits before the
     * global jump-ball path reaches `$86:D3F9 -> $86:BAA2`. Installing the
     * owner here preserves the frame-220 pose while making mode 11 visible
     * to the following `$87:9244` behavior pass. */
    bool jump_ball_acquisition = tipoff->frame == NBA_TIPOFF_BREAK_FRAME;
    if (jump_ball_acquisition)
        cpu_begin_possession(tipoff, 1u);
    NbaGameplayRimResult rim_result = cpu_update_live_ball(tipoff);
    cpu_cache_predicted_ball_xy(tipoff);
    int8_t owner_before_contacts = tipoff->ball.owner_actor;
    cpu_update_player_contacts(tipoff);
    bool detached_contact = cpu_try_detached_shot_contact(tipoff);
    if (!detached_contact) (void)cpu_try_owned_ball_contact(tipoff);
    nba_tipoff_update_play_control_end_frame(tipoff);
    nba_tipoff_refresh_team_roles_end_frame(tipoff);

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
            if (rim_result == NBA_GAMEPLAY_RIM_MAKE) {
                /* Scoring is inline in the detecting physics substep. */
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

    /* `$86:D3C5` completes inside the collision pass, then the caller reaches
     * the shared attached-ball tail. Keep that ordering outside the isolated
     * BAA2/D25A replay functions, whose captured exits precede this write. */
    if (owner_before_contacts < 0 && tipoff->ball.owner_actor >= 0)
        ball_position_at_actor(tipoff,
            (unsigned)tipoff->ball.owner_actor);

    cpu_schedule_actor_behaviors(
        tipoff, jump_ball_acquisition ||
        (tipoff->rim_raw_13e7 & 0x0010u) != 0u);

    ++tipoff->possession_frame;
    ++tipoff->play_state_frame;
}

static void cpu_update_camera(NbaTipoff *tipoff) {
    /* `$87:A9D0-$A9E2/$87:95BB-$95D8`: signed `$093E` selects an actor;
     * FFFF substitutes the ball record before `$85:9192-$93F4` consumes the
     * proxy. `$093A` is persistent and independent from the free ball. */
    if (tipoff->frame < NBA_TIPOFF_POSSESSION_FRAME) return;
    /* The `$87:95BB` subject-proxy path reaches `$85:9192` only on the
     * 30-Hz logical pass. Mesen WRAM proof changes `$085C/$0860` at frames
     * 201,203,205... and holds them on the intervening outer frames. */
    if ((tipoff->simulation_tick & 1u) == 0u) return;
    /* `$093E` is a camera/control selector distinct from possession. During
     * the tip-result bridge it remains negative through frame 219 (ball),
     * then becomes actor 8 at the frame-220 handoff. Live ownership changes
     * use the represented possession actor until a separate control model
     * owns this raw selector. */
    int16_t selector = tipoff->frame < NBA_TIPOFF_BREAK_FRAME ?
        -1 : tipoff->possession_actor;
    if (selector >= 0 && selector < NBA_GAMEPLAY_ACTOR_COUNT) {
        unsigned subject = (unsigned)selector;
        tipoff->camera.subject_actor = (uint8_t)subject;
        nba_gameplay_camera_update(&tipoff->camera,
            tipoff->actors[subject].x_fp,
            tipoff->actors[subject].y_fp,
            tipoff->actors[subject].z_fp,
            tipoff->camera_side_group_raw, tipoff->live_state_raw == 1u);
    } else {
        tipoff->camera.subject_actor = NBA_GAMEPLAY_NO_ACTOR;
        nba_gameplay_camera_update(&tipoff->camera,
            tipoff->ball.x_fp, tipoff->ball.y_fp, tipoff->ball.z_fp,
            tipoff->camera_side_group_raw,
            tipoff->live_state_raw == 1u);
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
    if (!tipoff || !assets || !session) return false;
#define NBA_TIPOFF_REQUIRE(label, expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "[TIPOFF] initialization check failed: %s\n", label); \
        return false; \
    } \
} while (0)
    NBA_TIPOFF_REQUIRE("gameplay RNG", nba_gameplay_rng_self_test());
    NBA_TIPOFF_REQUIRE("gameplay AI", nba_gameplay_ai_self_test());
    NBA_TIPOFF_REQUIRE("ball physics", nba_gameplay_ball_self_test());
    NBA_TIPOFF_REQUIRE("effect", nba_gameplay_effect_self_test());
    NBA_TIPOFF_REQUIRE("foul", nba_gameplay_foul_self_test());
    NBA_TIPOFF_REQUIRE("rim contact tick", cpu_rim_contact_tick_self_test());
    NBA_TIPOFF_REQUIRE("two-substep ball physics", cpu_ball_substep_self_test());
    NBA_TIPOFF_REQUIRE("deferred shooting foul", cpu_deferred_shooting_foul_self_test());
    NBA_TIPOFF_REQUIRE("free throw scene", cpu_free_throw_scene_self_test());
    NBA_TIPOFF_REQUIRE("special receiver", cpu_special_receiver_self_test());
    NBA_TIPOFF_REQUIRE("boundary pass recovery", cpu_boundary_pass_recovery_self_test());
    NBA_TIPOFF_REQUIRE("inbound completion", cpu_inbound_completion_witness_self_test());
    NBA_TIPOFF_REQUIRE("ball acquisition", cpu_ball_acquisition_self_test());
    NBA_TIPOFF_REQUIRE("dead ball dispatch", cpu_dead_ball_dispatch_self_test());
    NBA_TIPOFF_REQUIRE("contact orchestration", cpu_contact_orchestration_self_test());
    NBA_TIPOFF_REQUIRE("player contact", cpu_player_contact_self_test());
    NBA_TIPOFF_REQUIRE("defensive planner", cpu_defensive_planner_self_test());
    NBA_TIPOFF_REQUIRE("expired inbound", cpu_expired_inbound_self_test());
    NBA_TIPOFF_REQUIRE("inbound recovery carrier",
        cpu_inbound_recovery_carrier_self_test(assets, session));
    NBA_TIPOFF_REQUIRE("attachment assets", ball_attachment_assets_valid(assets));
    NBA_TIPOFF_REQUIRE("ROM animation cadence", nba_player_animation_self_test(assets));
    NBA_TIPOFF_REQUIRE("boosted pass", cpu_boosted_pass_self_test(assets, session));
    NBA_TIPOFF_REQUIRE("court panorama", nba_assets_gameplay_court_panorama(assets, session->right_team));
    NBA_TIPOFF_REQUIRE("tipoff ball asset", nba_assets_get(assets, NBA_ASSET_TIPOFF_BALL));
#undef NBA_TIPOFF_REQUIRE
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
        tipoff->team_context[side].anchor_x_raw_0a =
            side ? 336 : -336;
        tipoff->team_context[side].mode_raw_30 = 4u;
        tipoff->team_context[side].flags_raw_32 = 1u;
        tipoff->team_context[side].activity_raw_39 = 1u;
        tipoff->team_context[side].dead_ball_actor_raw_3f =
            NBA_GAMEPLAY_UNKNOWN_WORD;
        tipoff->team_context[side].controller_actor_raw_41 = -1;
        tipoff->team_context[side].previous_dead_ball_actor_raw_43 =
            NBA_GAMEPLAY_UNKNOWN_WORD;
        tipoff->team_context[side].previous_controller_actor_raw_45 = -1;
        tipoff->team_context[side].help_distance_raw_4e = 0x00A0u;
        for (unsigned i = 0; i < 5u; ++i)
            tipoff->team_context[side].actor_order_raw_49[i] =
                context_actor_order[side][i];
    }
    tipoff->period_raw_0926 = 0u;
    tipoff->match_clock_raw_0928 = 43200u;
    tipoff->possession_actor = -1;
    tipoff->possession_team = -1;
    tipoff->collision_actor_a_raw = -1;
    tipoff->collision_actor_b_raw = -1;
    tipoff->player_contact_actor_a_raw = -1;
    tipoff->player_contact_actor_b_raw = -1;
    session->score[0] = session->score[1] = 0u;
    session->game_clock_ticks = 0u;
    tipoff->live_state_raw = 1u;
    tipoff->inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff->pass_actor_raw = -1;
    tipoff->pass_receiver_raw = -1;
    tipoff->play_aux_selector_raw_09a6 = -1;
    tipoff->shot_actor_raw_09c8 = -1;
    tipoff->ball.x_fp = 0;
    tipoff->ball.y_fp = 0;
    tipoff->ball.z_fp = 80 * 256;
    tipoff->ball.owner_actor = -1;
    static const uint8_t active_lineup[5] = {2u, 0u, 1u, 3u, 4u};
    uint8_t appearance_teams[NBA_PLAYER_APPEARANCE_COUNT];
    uint8_t appearance_roster[NBA_PLAYER_APPEARANCE_COUNT];
    for (unsigned i = 0; i < NBA_PLAYER_APPEARANCE_COUNT; ++i) {
        appearance_teams[i] = i < 5u ? session->left_team : session->right_team;
        appearance_roster[i] = active_lineup[i % 5u];
    }
    NbaPlayerAppearanceSetup appearance;
    if (!nba_player_appearance_setup(assets, appearance_teams, appearance_roster,
                                     &appearance)) return false;
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
        state->animation_upper_queue_cursor_raw_18 = 0xFFFFu;
        state->animation_lower_queue_cursor_raw_1a = 0xFFFFu;
        /* `$86:D86C-$D89B`: +$76 is derived from the active-lineup
         * permutation, then copied to mutable +$74. It is an even byte
         * offset into `$87:9C7B`, not a same-index matchup. */
        state->assignment_actor = actor < 5u ?
            (uint8_t)(5u + state->roster_slot) : state->roster_slot;
        state->team_group_raw_6e = actor < 5u ? 0u : 5u;
        state->assignment_base_raw = (uint16_t)(state->assignment_actor * 2u);
        state->assignment_current_raw = state->assignment_base_raw;
        state->assignment_alternate_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        uint8_t team = actor >= 5u ? session->right_team : session->left_team;
        state->assignment_role_raw_92 = (uint8_t)(actor % 5u);
        (void)nba_player_gameplay_position(
            assets, team, state->roster_slot, &state->assignment_role_raw_92);
        state->free_throw_launch_half_raw_a8 = appearance.players[actor].alternate_lower;
        state->animation_variant_raw_6c = appearance.players[actor].upper_variant;
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
    tipoff->collision_actor_a_raw = -1;
    tipoff->collision_actor_b_raw = -1;
    tipoff->collision_routine_raw = 0u;
    tipoff->player_contact_count_raw = 0u;
    tipoff->player_contact_actor_a_raw = -1;
    tipoff->player_contact_actor_b_raw = -1;
    tipoff->player_contact_routine_raw = 0u;
    ++tipoff->frame;
    ++tipoff->simulation_tick;
    ++tipoff->session->game_clock_ticks;
    /* `$85:EDB3`: presentation timer and master tick advance on every
     * outer update, independent of actor scheduling or gameplay state. */
    tipoff->fouls.whistle_timer_raw_08de = (int16_t)(uint16_t)(
        (uint16_t)tipoff->fouls.whistle_timer_raw_08de - 1u);
    /* Live Mesen capture: `$0928` is 43200 at frame 220, then decrements
     * once per outer frame (43020 at 400; 41620 at 1800). */
    if (tipoff->frame > NBA_TIPOFF_BREAK_FRAME &&
        tipoff->match_clock_raw_0928 != 0u)
        --tipoff->match_clock_raw_0928;
    if (tipoff->frame < NBA_TIPOFF_BREAK_FRAME)
        cpu_update_tip_ball(tipoff);
    if (tipoff->frame == NBA_TIPOFF_POSSESSION_FRAME) {
        /* `$85:B100-$B28B` resolves the tip and writes play code $35.
         * `$0946` selects actor 8 as the prospective receiver, but signed
         * owner `$093E` remains FFFF through frame 219. `$86:D3F9/BAA2`
         * installs actor 8 only after the frame-220 physics loop. */
        tipoff->handler_actor = 8u;
        tipoff->receiver_actor = 8u;
        tipoff->possession_team = 1;
        tipoff->camera_side_group_raw = 5u;
        tipoff->play_code = 0x35u;
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor)
            tipoff->actors[actor].control_mode = actor >= 5u ? 1u : 2u;
        tipoff->actors[8].control_mode = 10u;
    }
    if (tipoff->frame >= NBA_TIPOFF_BREAK_FRAME) {
        cpu_update_possession(tipoff);
        /* `$86:F357-$F364`: once a detached owner and pending `$09BC` have
         * both been observed, `$0A02` becomes the immediate-resolution phase. */
        if (tipoff->fouls.shooting_foul_raw_09bc != 0u &&
            tipoff->deferred_shot_foul_phase_raw_0a02 != 0u)
            tipoff->deferred_shot_foul_phase_raw_0a02 = 2u;
        /* `$87:92A5-$95E6` performs dead-ball setup before `$85:93F5`
         * consumes the pending event later in the same outer pass. */
        cpu_dispatch_pending_event(tipoff);
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
    telemetry->match_clock_raw_0928 = tipoff->match_clock_raw_0928;
    for (unsigned side = 0; side < 2u; ++side) {
        telemetry->team_context_mode_raw_30[side] =
            tipoff->team_context[side].mode_raw_30;
        telemetry->team_context_flags_raw_32[side] =
            tipoff->team_context[side].flags_raw_32;
        telemetry->team_context_activity_raw_39[side] =
            tipoff->team_context[side].activity_raw_39;
        telemetry->team_context_dead_ball_actor_raw_3f[side] =
            tipoff->team_context[side].dead_ball_actor_raw_3f;
    }
    telemetry->shot_clock_raw_092c = tipoff->rim_raw_092c;
    telemetry->shot_clock_mirror_raw_09c6 =
        tipoff->shot_clock_mirror_raw_09c6;
    telemetry->shot_value_raw = tipoff->shot_value_raw;
    telemetry->shot_actor_raw_09c8 = tipoff->shot_actor_raw_09c8;
    telemetry->interference_value_raw_096a = tipoff->rim_raw_096a;
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
    telemetry->dead_ball_raw_0966 = tipoff->dead_ball_raw_0966;
    telemetry->dead_ball_raw_0968 = tipoff->dead_ball_raw_0968;
    telemetry->dead_ball_raw_096c = tipoff->dead_ball_raw_096c;
    telemetry->dead_ball_x_raw_09b0 = tipoff->dead_ball_x_raw_09b0;
    telemetry->dead_ball_y_raw_09b2 = tipoff->dead_ball_y_raw_09b2;
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
    telemetry->free_throw_start_tick_raw_09be =
        tipoff->free_throw_start_tick_raw_09be;
    telemetry->free_throw_aim_x_raw_0980 =
        tipoff->free_throw_aim_x_raw_0980;
    telemetry->free_throw_aim_y_raw_0982 =
        tipoff->free_throw_aim_y_raw_0982;
    telemetry->free_throw_flight_timer_raw_0930 =
        tipoff->free_throw_flight_timer_raw_0930;
    telemetry->deferred_shot_foul_phase_raw_0a02 =
        tipoff->deferred_shot_foul_phase_raw_0a02;
    telemetry->latched_event_raw_08f0 =
        tipoff->fouls.latched_event_raw_08f0;
    telemetry->whistle_active_raw_09b6 =
        tipoff->fouls.whistle_active_raw_09b6;
    telemetry->whistle_timer_raw_08de =
        (uint16_t)tipoff->fouls.whistle_timer_raw_08de;
    telemetry->presentation_gate_raw_08e2 =
        tipoff->fouls.presentation_gate_raw_08e2;
    telemetry->whistle_presentation_queued_raw =
        tipoff->fouls.whistle_presentation_queued_raw;
    telemetry->ball_activity_raw = tipoff->ball_activity_raw;
    telemetry->pass_actor_raw = tipoff->pass_actor_raw;
    telemetry->pass_receiver_raw = tipoff->pass_receiver_raw;
    telemetry->pass_active_raw = tipoff->pass_active_raw;
    telemetry->pass_distance_raw = tipoff->pass_distance_raw;
    telemetry->collision_actor_a = tipoff->frame < NBA_TIPOFF_BREAK_FRAME &&
                                   tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME &&
                                   tipoff->frame < 211 ? 0 :
                                   tipoff->collision_actor_a_raw;
    telemetry->collision_actor_b = tipoff->frame < NBA_TIPOFF_BREAK_FRAME &&
                                   tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME &&
                                   tipoff->frame < 211 ? 5 :
                                   tipoff->collision_actor_b_raw;
    telemetry->player_contact_count_raw = tipoff->player_contact_count_raw;
    telemetry->player_contact_actor_a_raw = tipoff->player_contact_actor_a_raw;
    telemetry->player_contact_actor_b_raw = tipoff->player_contact_actor_b_raw;
    telemetry->player_contact_routine_raw = tipoff->player_contact_routine_raw;
    telemetry->controller_routine = 0x80CB8Fu;
    telemetry->selection_routine = 0x85C37Du;
    telemetry->collision_routine = tipoff->frame < NBA_TIPOFF_BREAK_FRAME &&
                                   tipoff->frame >= NBA_TIPOFF_CONTACT_FRAME ?
                                   SNES_ADDR_TIPOFF_CONTACT :
                                   tipoff->collision_routine_raw;
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
        if (live && actor_animation_resources(tipoff, state, state->direction,
                &upper_resource, &lower_resource)) {
            out->upper_resource_raw = upper_resource;
            out->lower_resource_raw = lower_resource;
        } else {
            out->upper_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
            out->lower_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        }
        out->head_resource_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        /* `$87:B572-$B648` consumes these exact actor words. Export the
         * represented values instead of hiding them behind FFFF so a Mesen
         * trace can distinguish locomotion-state errors from descriptor
         * phase errors. Historical motion_3c/lower_phase keys remain legacy
         * ticks for existing consumers; animation_rom below exports literal
         * ROM phases/accumulators/resources without conflating the clocks. */
        out->motion_38_raw = live ? state->base_animation_state_raw_38 : 0u;
        out->motion_3a_raw = live ? state->upper_animation_phase_raw : 0u;
        out->motion_3c_raw = live ? state->lower_animation_tick : 0u;
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
        bool close_finish = state->control_mode == 13u ||
                            state->control_mode == 14u;
        out->mode13_timer_60_raw = state->control_mode == 13u ?
            state->contact_action_timer_raw_60 :
            state->control_mode == 14u ? state->reaction_threshold : 0u;
        out->mode13_selector_56_raw = close_finish ?
            state->special_contact_raw_56 : -1;
        out->mode13_variant_58_raw = close_finish ?
            state->mode13_variant_raw_58 : 0u;
        out->mode13_baseline_vx_ba_raw = close_finish ?
            state->mode13_baseline_velocity_x : 0;
        out->mode13_baseline_vy_bc_raw = close_finish ?
            state->mode13_baseline_velocity_y : 0;
        out->contact_inhibit_5a_raw = state->contact_inhibit_raw_5a;
        out->contact_height_aa_raw = state->contact_height_raw_aa;
        out->recovery_inhibit_7a_raw = state->recovery_inhibit_raw;
        out->upper_restart_raw = out->lower_restart_raw = 0;
        out->upper_phase_raw = live ? state->upper_animation_phase_raw : 0u;
        out->lower_phase_raw = live ? state->lower_animation_tick : 0u;
        const uint16_t animation_words[10] = {
            state->upper_animation_resource_raw_2a, state->lower_animation_resource_raw_2c,
            state->rom_upper_animation_phase_raw_3a, state->rom_lower_animation_phase_raw_3c,
            state->upper_animation_accumulator_raw_42, state->lower_animation_accumulator_raw_44,
            state->upper_animation_lock_raw_46, state->lower_animation_lock_raw_48,
            state->animation_upper_queue_cursor_raw_18, state->animation_lower_queue_cursor_raw_1a
        };
        memcpy(out->animation_rom_words, animation_words, sizeof(animation_words));
        out->animation_resources_valid = live && state->animation_resources_valid;
        out->animation_action_integrated = live && state->exact_pass_animation;
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
        if (tipoff->frame >= NBA_TIPOFF_BREAK_FRAME &&
            tipoff->actors[actor].animation_resources_valid) {
            nba_player_sprite_render_resources(
                ren, tipoff->assets, team, slot, uniform_side, direction,
                tipoff->actors[actor].upper_animation_resource_raw_2a,
                tipoff->actors[actor].lower_animation_resource_raw_2c,
                screen_x[actor], screen_y[actor] - jump, 1);
        } else {
            nba_player_sprite_render_split(ren, tipoff->assets, team, slot,
                                           uniform_side, state, lower_state,
                                           direction, upper_tick, lower_tick,
                                           screen_x[actor],
                                           screen_y[actor] - jump, 1);
        }
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
