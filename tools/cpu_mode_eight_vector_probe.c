#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nba_tipoff.h"

#define FIELD_COUNT 27u

/* Host-only fixture helper; no direct native address. Rebuild the integer/fraction
 * Z coordinate represented by the `$86:C6AD-$C758` native witness. */
static int32_t fixed_from_words(uint16_t fraction, uint16_t integer) {
    return (int32_t)(int16_t)integer * 256 + (int32_t)(fraction >> 8);
}

/* Host-only fixture helper; no direct native address. Recover actor Z fraction for
 * the mode-eight strict output shape. */
static uint16_t fixed_fraction_word(int32_t value) {
    return (uint16_t)(((uint32_t)value & 0xffu) << 8);
}

/* Host-only fixture helper; no direct native address. Recover signed actor integer
 * Z for the mode-eight strict output shape. */
static uint16_t fixed_integer_word(int32_t value) {
    int32_t integer = value >= 0 ? value / 256 : -(((-value) + 255) / 256);
    return (uint16_t)(int16_t)integer;
}

/* Host-only fixture helper; no direct native address. Emit one exact 16-bit field
 * for the `$86:C6AD-$C758` production replay. */
static void emit(uint16_t value, unsigned *count) {
    printf("%s%04x", *count ? " " : "", value);
    ++*count;
}

typedef struct {
    unsigned actor;
    bool seen;
    bool install_mode_eight;
    int32_t x_fp;
    int32_t z_fp;
    int16_t velocity_z;
    uint8_t mode;
    uint16_t timer;
    uint16_t contact_inhibit;
    uint16_t recovery_inhibit;
    uint32_t animation_tick;
    bool resources_valid;
    uint16_t upper_resource;
    uint16_t lower_resource;
} ModeEightPhase;

/* Host-only fixture helper; no direct native address. Prepare a complete scheduler
 * pass around `$87:AAB2/$85:963D/$87:90A5-$90C2/$87:9244->$86:C6AD`. */
static bool prepare_scheduler_case(const NbaAssetPack *assets,
                                   NbaSession *session,
                                   NbaTipoff *tipoff,
                                   unsigned actor) {
    nba_session_init(session);
    session->right_team = 18u;
    session->left_team = 28u;
    if (!nba_tipoff_init(tipoff, assets, session)) return false;
    for (unsigned slot = 0; slot < NBA_GAMEPLAY_ACTOR_COUNT; ++slot) {
        tipoff->actors[slot].control_mode = 0u;
        tipoff->actors[slot].velocity_x = 0;
        tipoff->actors[slot].velocity_y = 0;
        tipoff->actors[slot].velocity_z = 0;
        tipoff->actors[slot].x_fp = (int32_t)(-350 + (int)slot * 70) * 256;
        tipoff->actors[slot].y_fp = (int32_t)(-180 + (int)slot * 35) * 256;
    }
    tipoff->simulation_tick = 1u;
    tipoff->tip_contact_actor = 0;
    tipoff->live_state_raw = 0x0082u;
    tipoff->camera_side_group_raw = 0u;
    tipoff->possession_actor = 3;
    NbaTipoffActor *state = &tipoff->actors[actor];
    state->roster_slot = 2u;
    state->team_group_raw_6e = 0u;
    state->x_fp = 100 * 256;
    state->y_fp = 20 * 256;
    state->z_fp = 0;
    state->velocity_x = 0x0100;
    state->velocity_y = 0;
    state->velocity_z = 0;
    state->control_mode = 8u;
    state->contact_action_timer_raw_60 = 66u;
    state->reaction_threshold = 66u;
    state->special_contact_raw_56 = -1;
    state->pass_direction_raw = 0x1234u;
    state->animation_state = 0x35u;
    state->lower_animation_state = 0x35u;
    state->base_animation_state_raw_38 = 0x35u;
    state->upper_animation_tick = 0u;
    state->lower_animation_tick = 0u;
    state->rom_upper_animation_phase_raw_3a = 0u;
    state->rom_lower_animation_phase_raw_3c = 0u;
    state->upper_animation_accumulator_raw_42 = 0u;
    state->lower_animation_accumulator_raw_44 = 0u;
    state->animation_upper_queue_cursor_raw_18 = 0xffffu;
    state->animation_lower_queue_cursor_raw_1a = 0xffffu;
    for (unsigned index = 0; index < 3u; ++index) {
        state->animation_upper_queue_raw_1c[index] = 0xffffu;
        state->animation_lower_queue_raw_22[index] = 0xffffu;
    }
    state->animation_resources_valid = false;
    state->exact_jump_animation = true;
    return true;
}

/* Host-only test observer; no direct native address. Sample the real
 * post-physics boundary and optionally inject a synthetic mode-nine-to-eight
 * record there; that injected case does not exercise a contact writer. */
static void observe_after_physics(const NbaTipoff *observed,
                                  const char *boundary,
                                  void *raw_context) {
    ModeEightPhase *context = raw_context;
    if (context->seen || strcmp(boundary, "actors.end") != 0) return;
    NbaTipoff *tipoff = (NbaTipoff *)observed;
    NbaTipoffActor *actor = &tipoff->actors[context->actor];
    context->seen = true;
    context->x_fp = actor->x_fp;
    context->z_fp = actor->z_fp;
    context->velocity_z = actor->velocity_z;
    context->mode = actor->control_mode;
    context->timer = actor->contact_action_timer_raw_60;
    context->contact_inhibit = actor->contact_inhibit_raw_5a;
    context->recovery_inhibit = actor->recovery_inhibit_raw;
    context->animation_tick = actor->upper_animation_tick;
    context->resources_valid = actor->animation_resources_valid;
    context->upper_resource = actor->upper_animation_resource_raw_2a;
    context->lower_resource = actor->lower_animation_resource_raw_2c;
    if (context->install_mode_eight) {
        actor->control_mode = 8u;
        actor->contact_action_timer_raw_60 = 30u;
        actor->reaction_threshold = 30u;
        actor->contact_inhibit_raw_5a = 30u;
        actor->recovery_inhibit_raw = 8u;
        actor->special_contact_raw_56 = -1;
        actor->behavior_flags_raw = 0u;
        actor->actor_status_raw_28 = 0u;
    }
}

/* Host-only integration helper; no direct native address. Seed stale channel
 * state so the `$87:B538/$B555/$B3BD` publication sequence is observable. */
static NbaPlayerAnimationChannels seed_stale_knockdown_channels(
    NbaTipoffActor *actor) {
    NbaPlayerAnimationChannels channels = {0};
    channels.upper_queue_cursor = 2u;
    channels.lower_queue_cursor = 4u;
    channels.upper_state = 0x35u;
    channels.lower_state = 0x35u;
    channels.base_state = 3u;
    channels.upper_phase = 7u;
    channels.lower_phase = 9u;
    channels.upper_accumulator = 0x1111u;
    channels.lower_accumulator = 0x2222u;
    channels.upper_lock = 1u;
    channels.lower_lock = 2u;
    channels.upper_queue[0] = 0x0011u;
    channels.upper_queue[1] = 0x0022u;
    channels.upper_queue[2] = 0x0033u;
    channels.lower_queue[0] = 0x0044u;
    channels.lower_queue[1] = 0x0055u;
    channels.lower_queue[2] = 0x0066u;
    channels.upper_phase_target = 0x8007u;
    actor->animation_upper_queue_cursor_raw_18 = channels.upper_queue_cursor;
    actor->animation_lower_queue_cursor_raw_1a = channels.lower_queue_cursor;
    actor->animation_state = (uint8_t)channels.upper_state;
    actor->lower_animation_state = (uint8_t)channels.lower_state;
    actor->base_animation_state_raw_38 = (uint8_t)channels.base_state;
    actor->rom_upper_animation_phase_raw_3a = channels.upper_phase;
    actor->rom_lower_animation_phase_raw_3c = channels.lower_phase;
    actor->upper_animation_accumulator_raw_42 = channels.upper_accumulator;
    actor->lower_animation_accumulator_raw_44 = channels.lower_accumulator;
    actor->upper_animation_lock_raw_46 = channels.upper_lock;
    actor->lower_animation_lock_raw_48 = channels.lower_lock;
    memcpy(actor->animation_upper_queue_raw_1c, channels.upper_queue,
           sizeof(channels.upper_queue));
    memcpy(actor->animation_lower_queue_raw_22, channels.lower_queue,
           sizeof(channels.lower_queue));
    actor->upper_phase_target_raw_b0 = channels.upper_phase_target;
    return channels;
}

/* Host-only integration helper; no direct native address. Compare all channel
 * words affected by `$87:B538/$B555/$B3BD` with their verified command model. */
static bool actor_matches_channels(const NbaTipoffActor *actor,
                                   const NbaPlayerAnimationChannels *channels) {
    return actor->animation_upper_queue_cursor_raw_18 ==
               channels->upper_queue_cursor &&
        actor->animation_lower_queue_cursor_raw_1a ==
               channels->lower_queue_cursor &&
        actor->animation_state == channels->upper_state &&
        actor->lower_animation_state == channels->lower_state &&
        actor->base_animation_state_raw_38 == channels->base_state &&
        actor->rom_upper_animation_phase_raw_3a == channels->upper_phase &&
        actor->rom_lower_animation_phase_raw_3c == channels->lower_phase &&
        actor->upper_animation_accumulator_raw_42 ==
               channels->upper_accumulator &&
        actor->lower_animation_accumulator_raw_44 ==
               channels->lower_accumulator &&
        actor->upper_animation_lock_raw_46 == channels->upper_lock &&
        actor->lower_animation_lock_raw_48 == channels->lower_lock &&
        memcmp(actor->animation_upper_queue_raw_1c, channels->upper_queue,
               sizeof(channels->upper_queue)) == 0 &&
        memcmp(actor->animation_lower_queue_raw_22, channels->lower_queue,
               sizeof(channels->lower_queue)) == 0 &&
        actor->upper_phase_target_raw_b0 == channels->upper_phase_target;
}

/* Host-only production contact check; no direct native address. Exercise the
 * real `$86:BFBA-$C236` writer and its next mode-eight scheduler pass. */
static bool actual_contact_writer_case(const NbaAssetPack *assets,
                                       bool alternate) {
    NbaSession session;
    NbaTipoff tipoff;
    if (!prepare_scheduler_case(assets, &session, &tipoff, 0u)) return false;
    tipoff.live_state_raw = 0u;
    tipoff.inbound_actor_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
    tipoff.possession_actor = -1;
    tipoff.shot_actor_raw_09c8 = -1;
    NbaTipoffActor *victim = &tipoff.actors[0];
    NbaTipoffActor *hitter = &tipoff.actors[5];
    victim->x_fp = victim->y_fp = 0;
    victim->velocity_x = victim->velocity_y = victim->velocity_z = 0;
    victim->movement_magnitude_raw = 0u;
    victim->movement_boost_timer = alternate ? 1u : 0u;
    victim->control_mode = 2u;
    victim->behavior_flags_raw = 0u;
    victim->controller_assignment_raw = -1;
    hitter->x_fp = 8 * 256;
    hitter->y_fp = 0;
    hitter->velocity_x = 1800;
    hitter->velocity_y = hitter->velocity_z = 0;
    hitter->movement_magnitude_raw = 1800u;
    hitter->control_mode = 1u;
    hitter->behavior_flags_raw = 0u;
    hitter->controller_assignment_raw = -1;
    NbaPlayerAnimationChannels expected =
        seed_stale_knockdown_channels(victim);
    uint16_t request = 0u;
    if (!nba_player_animation_command(assets, &expected,
            NBA_ANIMATION_CANCEL_UPPER, &request, alternate, false) ||
        !nba_player_animation_command(assets, &expected,
            NBA_ANIMATION_CANCEL_LOWER, &request, alternate, false))
        return false;
    request = alternate ? 0x36u : 0x35u;
    if (!nba_player_animation_command(assets, &expected,
            NBA_ANIMATION_INSTALL_BOTH, &request, alternate, false))
        return false;
    nba_gameplay_rng_seed(&tipoff.rng, 0x0010u);
    const uint8_t order[2] = {0u, 5u};
    nba_tipoff_replay_player_contact_order(&tipoff, order, 2u);
    victim = &tipoff.actors[0];
    uint16_t wanted_action = alternate ? 0x36u : 0x35u;
    uint16_t wanted_timer = alternate ? 174u : 30u;
    if (victim->control_mode != 8u || victim->action_state != wanted_action ||
        victim->contact_action_timer_raw_60 != wanted_timer ||
        victim->reaction_threshold != wanted_timer ||
        !actor_matches_channels(victim, &expected)) {
        fprintf(stderr, "[CPU MODE EIGHT INTEGRATION] contact action=%u "
                "mode=%u state=%u/%u/%u timer=%u/%u lock=%04x/%04x\n",
                wanted_action, victim->control_mode, victim->animation_state,
                victim->lower_animation_state,
                victim->base_animation_state_raw_38,
                victim->contact_action_timer_raw_60,
                victim->reaction_threshold, victim->upper_animation_lock_raw_46,
                victim->lower_animation_lock_raw_48);
        return false;
    }
    if (alternate) return true;

    hitter->x_fp = 500 * 256;
    tipoff.live_state_raw = 0x82u;
    nba_tipoff_update(&tipoff, NULL);
    victim = &tipoff.actors[0];
    if (victim->control_mode != 8u || victim->animation_state != 0x35u ||
        victim->lower_animation_state != 0x35u ||
        victim->upper_animation_tick != 1u ||
        !victim->animation_resources_valid ||
        victim->contact_action_timer_raw_60 != 28u) {
        fprintf(stderr, "[CPU MODE EIGHT INTEGRATION] contact nextpass "
                "mode=%u state=%u/%u tick=%u valid=%u timer=%u\n",
                victim->control_mode, victim->animation_state,
                victim->lower_animation_state, victim->upper_animation_tick,
                victim->animation_resources_valid,
                victim->contact_action_timer_raw_60);
        return false;
    }
    return true;
}

/* Host-only production scheduler check; no direct native address. Verify
 * mode-eight physics, animation, cooldown, landing, and late dispatch order. */
static bool integration_case(const NbaAssetPack *assets) {
    if (!actual_contact_writer_case(assets, false) ||
        !actual_contact_writer_case(assets, true)) return false;
    const unsigned slot = 4u;
    NbaSession session;
    NbaTipoff tipoff;
    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    NbaTipoffActor *actor = &tipoff.actors[slot];
    actor->contact_inhibit_raw_5a = 3u;
    actor->recovery_inhibit_raw = 0x8002u;
    actor->actor_status_raw_28 = 0xffffu;
    ModeEightPhase held = {.actor = slot};
    tipoff.differential_observer = observe_after_physics;
    tipoff.differential_context = &held;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (!held.seen || held.x_fp != 102 * 256 || held.timer != 66u ||
        held.contact_inhibit != 3u || held.recovery_inhibit != 0x8002u ||
        held.animation_tick != 1u || !held.resources_valid ||
        actor->x_fp != held.x_fp ||
        actor->control_mode != 8u || actor->contact_action_timer_raw_60 != 64u ||
        actor->reaction_threshold != 64u || actor->contact_inhibit_raw_5a != 1u ||
        actor->recovery_inhibit_raw != 0u || actor->behavior_flags_raw != 6u ||
        (actor->actor_status_raw_28 & 0x0018u) != 0x0010u) {
        fprintf(stderr, "[CPU MODE EIGHT INTEGRATION] held seen=%u "
                "boundary=%ld/%u/%04x/%04x/%lu/%u/%04x/%04x "
                "final=%ld/%u/%u/%04x/%04x/%04x\n",
                held.seen, (long)held.x_fp, held.timer, held.contact_inhibit,
                held.recovery_inhibit, (unsigned long)held.animation_tick,
                held.resources_valid, held.upper_resource, held.lower_resource,
                (long)actor->x_fp, actor->control_mode,
                actor->contact_action_timer_raw_60,
                actor->contact_inhibit_raw_5a, actor->recovery_inhibit_raw,
                actor->actor_status_raw_28);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    actor = &tipoff.actors[slot];
    actor->control_mode = 9u;
    actor->reaction_threshold = 0x0040u;
    actor->contact_action_timer_raw_60 = 0x7777u;
    ModeEightPhase installed = {.actor = slot, .install_mode_eight = true};
    tipoff.differential_observer = observe_after_physics;
    tipoff.differential_context = &installed;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (!installed.seen || installed.mode != 9u ||
        installed.x_fp != 102 * 256 || actor->x_fp != installed.x_fp ||
        actor->control_mode != 8u || actor->contact_action_timer_raw_60 != 28u ||
        actor->reaction_threshold != 28u || actor->contact_inhibit_raw_5a != 28u ||
        actor->recovery_inhibit_raw != 6u || actor->behavior_flags_raw != 6u) {
        fprintf(stderr, "[CPU MODE EIGHT INTEGRATION] installed seen=%u "
                "boundary=%u/%ld final=%u/%ld/%u/%u/%04x/%04x\n",
                installed.seen, installed.mode, (long)installed.x_fp,
                actor->control_mode, (long)actor->x_fp,
                actor->contact_action_timer_raw_60,
                actor->contact_inhibit_raw_5a, actor->recovery_inhibit_raw,
                actor->behavior_flags_raw);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    actor = &tipoff.actors[slot];
    actor->contact_inhibit_raw_5a = 0x8001u;
    actor->recovery_inhibit_raw = 0x8001u;
    actor->contact_action_timer_raw_60 = 80u;
    actor->reaction_threshold = 80u;
    ModeEightPhase wrapped = {.actor = slot};
    tipoff.differential_observer = observe_after_physics;
    tipoff.differential_context = &wrapped;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (!wrapped.seen || actor->contact_inhibit_raw_5a != 0x7fffu ||
        actor->recovery_inhibit_raw != 0x7fffu ||
        actor->contact_action_timer_raw_60 != 78u) {
        fprintf(stderr, "[CPU MODE EIGHT INTEGRATION] wrapped seen=%u "
                "final=%04x/%04x/%04x\n", wrapped.seen,
                actor->contact_inhibit_raw_5a, actor->recovery_inhibit_raw,
                actor->contact_action_timer_raw_60);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    actor = &tipoff.actors[slot];
    actor->z_fp = 64;
    actor->velocity_z = 0;
    actor->special_contact_raw_56 = 0;
    actor->pass_direction_raw = 0u;
    ModeEightPhase landing = {.actor = slot};
    tipoff.differential_observer = observe_after_physics;
    tipoff.differential_context = &landing;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (!landing.seen || landing.z_fp != 0 || landing.velocity_z != 0 ||
        actor->z_fp != 0 || actor->velocity_z != 0x00f0 ||
        actor->pass_direction_raw != 0xffffu || actor->x_fp != landing.x_fp) {
        fprintf(stderr, "[CPU MODE EIGHT INTEGRATION] landing seen=%u "
                "boundary=%ld/%d final=%ld/%d/%04x\n", landing.seen,
                (long)landing.z_fp, landing.velocity_z, (long)actor->z_fp,
                actor->velocity_z, actor->pass_direction_raw);
        return false;
    }
    return true;
}

/* Host-only native-vector probe; no direct native address. Load compact
 * non-asset witnesses and invoke the production mode-eight dispatcher. */
int main(int argc, char **argv) {
    NbaAssetPack assets = {0};
    uint16_t input[FIELD_COUNT];
    if ((argc != 2 && argc != 3) || !nba_assets_load(&assets, argv[1])) return 2;
    if (argc == 3) {
        bool passed = integration_case(&assets);
        nba_assets_free(&assets);
        puts(passed ? "[CPU MODE EIGHT INTEGRATION] PASS" :
                      "[CPU MODE EIGHT INTEGRATION] FAIL");
        return passed ? 0 : 7;
    }
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(input, sizeof(input[0]), FIELD_COUNT, stdin) == FIELD_COUNT) {
        NbaTipoff tipoff;
        memset(&tipoff, 0, sizeof(tipoff));
        tipoff.scratch_0046 = input[0];
        tipoff.scratch_0047 = input[1];
        tipoff.camera_side_group_raw = (uint8_t)input[2];
        tipoff.possession_actor = (int8_t)(int16_t)input[3];
        tipoff.rim_raw_13e7 = input[4];
        unsigned slot = input[5];
        if (slot >= NBA_GAMEPLAY_ACTOR_COUNT) return 4;
        NbaTipoffActor *actor = &tipoff.actors[slot];
        actor->x_fp = fixed_from_words(input[6], input[7]);
        actor->y_fp = fixed_from_words(input[8], input[9]);
        actor->z_fp = fixed_from_words(input[10], input[11]);
        actor->velocity_x = (int16_t)input[12];
        actor->velocity_y = (int16_t)input[13];
        actor->velocity_z = (int16_t)input[14];
        actor->actor_status_raw_28 = input[15];
        actor->special_contact_raw_56 = (int16_t)input[16];
        actor->contact_inhibit_raw_5a = input[17];
        actor->recovery_inhibit_raw = input[18];
        actor->control_mode = (uint8_t)input[19];
        actor->contact_action_timer_raw_60 = input[20];
        actor->reaction_threshold = input[21];
        actor->behavior_timer = input[22];
        actor->pass_direction_raw = input[23];
        actor->team_group_raw_6e = input[24];
        actor->behavior_flags_raw = input[25];
        actor->free_throw_launch_half_raw_a8 = input[26];
        if (!nba_tipoff_replay_passive_mode(&tipoff, (uint8_t)slot)) return 5;

        unsigned count = 0;
        emit(tipoff.scratch_0046, &count); emit(tipoff.scratch_0047, &count);
        emit(tipoff.camera_side_group_raw, &count);
        emit((uint16_t)(int16_t)tipoff.possession_actor, &count);
        emit(tipoff.rim_raw_13e7, &count); emit((uint16_t)slot, &count);
        emit(fixed_fraction_word(actor->x_fp), &count);
        emit(fixed_integer_word(actor->x_fp), &count);
        emit(fixed_fraction_word(actor->y_fp), &count);
        emit(fixed_integer_word(actor->y_fp), &count);
        emit(fixed_fraction_word(actor->z_fp), &count);
        emit(fixed_integer_word(actor->z_fp), &count);
        emit((uint16_t)actor->velocity_x, &count);
        emit((uint16_t)actor->velocity_y, &count);
        emit((uint16_t)actor->velocity_z, &count);
        emit(actor->actor_status_raw_28, &count);
        emit((uint16_t)actor->special_contact_raw_56, &count);
        emit(actor->contact_inhibit_raw_5a, &count);
        emit(actor->recovery_inhibit_raw, &count); emit(actor->control_mode, &count);
        emit(actor->contact_action_timer_raw_60, &count);
        emit(actor->reaction_threshold, &count); emit(actor->behavior_timer, &count);
        emit(actor->pass_direction_raw, &count); emit(actor->team_group_raw_6e, &count);
        emit(actor->behavior_flags_raw, &count);
        emit(actor->free_throw_launch_half_raw_a8, &count);
        if (count != FIELD_COUNT) return 6;
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
