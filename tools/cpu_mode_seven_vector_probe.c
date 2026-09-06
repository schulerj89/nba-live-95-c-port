#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nba_gameplay_ai.h"
#include "nba_tipoff.h"

#define FIELD_COUNT 50u

/* Host-only fixture helper; no native address. Rebuild a signed 16.8 actor
 * coordinate for the `$86:994C-$99C3` production replay. */
static int32_t fixed_from_words(uint16_t subpixel, uint16_t integer) {
    return (int32_t)(int16_t)integer * 256 + (int32_t)(subpixel >> 8);
}

/* Host-only fixture helper; no native address. Recover the represented
 * subpixel word from a replayed mode-seven coordinate. */
static uint16_t fixed_subpixel_word(int32_t value) {
    return (uint16_t)(((uint32_t)value & 0xffu) << 8);
}

/* Host-only fixture helper; no native address. Recover the signed integer
 * word read by `$86:9964` and the `$85:B3AA` steering child. */
static uint16_t fixed_integer_word(int32_t value) {
    int32_t integer = value >= 0 ? value / 256 : -(((-value) + 255) / 256);
    return (uint16_t)(int16_t)integer;
}

/* Host-only fixture helper; no native address. Emit one exact 16-bit field
 * for the `$86:994C-$99C3` strict replay. */
static void emit(uint16_t value, unsigned *count) {
    printf("%s%04x", *count ? " " : "", value);
    ++*count;
}

#ifndef NBA_MODE_SEVEN_DIRECT_ONLY
typedef struct {
    unsigned actor;
    bool mutate;
    bool seen;
    int32_t committed_x_fp;
    uint8_t animation_state;
    uint32_t animation_tick;
    uint8_t direction;
    bool resources_valid;
    uint16_t upper_resource;
    uint16_t lower_resource;
} ModeSevenPhaseMutation;

/* Host-only fixture helper; no native address. Prepare one full scheduler
 * pass while leaving all unrelated actors passive. */
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
    }
    tipoff->simulation_tick = 1u;
    tipoff->tip_contact_actor = 0;
    tipoff->live_state_raw = 0x0082u;
    tipoff->camera_side_group_raw = 0u;
    tipoff->possession_actor = 3;
    NbaTipoffActor *state = &tipoff->actors[actor];
    state->roster_slot = 2u;
    state->team_group_raw_6e = 5u;
    state->x_fp = 100 * 256;
    state->y_fp = 200 * 256;
    state->z_fp = 0;
    state->velocity_x = 0x0100;
    state->controller_assignment_raw = 0;
    state->movement_direction = 4u;
    state->requested_direction = 4u;
    state->direction = 4u;
    state->facing_ease_timer_raw_be = 0u;
    state->control_mode = 7u;
    state->reaction_threshold = 0x20u;
    state->animation_state = 0x0eu;
    state->lower_animation_state = 0x0eu;
    state->base_animation_state_raw_38 = 0x0eu;
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
    state->animation_resources_valid = true;
    state->exact_jump_animation = true;
    return true;
}

/* Host-only test observer; no native address. At the real actors.end
 * boundary, publish globals that `$87:9244 -> $86:994C` must read later in
 * the same scheduler pass. */
static void mutate_after_physics(const NbaTipoff *observed,
                                 const char *boundary,
                                 void *raw_context) {
    ModeSevenPhaseMutation *context = raw_context;
    if (context->seen || strcmp(boundary, "actors.end") != 0) return;
    NbaTipoff *tipoff = (NbaTipoff *)observed;
    context->seen = true;
    context->committed_x_fp = tipoff->actors[context->actor].x_fp;
    context->animation_state = tipoff->actors[context->actor].animation_state;
    context->animation_tick = tipoff->actors[context->actor].upper_animation_tick;
    context->direction = tipoff->actors[context->actor].direction;
    context->resources_valid =
        tipoff->actors[context->actor].animation_resources_valid;
    context->upper_resource =
        tipoff->actors[context->actor].upper_animation_resource_raw_2a;
    context->lower_resource =
        tipoff->actors[context->actor].lower_animation_resource_raw_2c;
    if (!context->mutate) return;
    tipoff->live_state_raw = 0x0182u;
    tipoff->camera_side_group_raw = 0u;
    tipoff->possession_actor = (int8_t)context->actor;
    tipoff->actors[context->actor].team_group_raw_6e = 0u;
}

/* Host-only production scheduler check; no direct native address. Verify the
 * `$87:AAB2 -> $85:963D -> globals -> $87:9244 -> $86:994C` order using
 * complete nba_tipoff_update passes. */
static bool integration_case(const NbaAssetPack *assets) {
    const unsigned slot = 5u;
    /* `$80:CEE7-$CEFC`: $FFFF is a real point in the nonzero LFSR orbit,
     * while zero alone takes the native recovery seed. This protects the
     * full-trace regression from treating a valid word as missing data. */
    NbaGameplayRng rng = {0xf13cu};
    if (nba_gameplay_rng_next(&rng) != 0xffffu ||
        nba_gameplay_rng_next(&rng) != 0xe279u) return false;
    rng.state = 0u;
    if (nba_gameplay_rng_next(&rng) != 0x9146u) return false;
    NbaSession session;
    NbaTipoff tipoff;
    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    ModeSevenPhaseMutation baseline_phase = {
        .actor = slot, .mutate = false
    };
    tipoff.differential_observer = mutate_after_physics;
    tipoff.differential_context = &baseline_phase;
    nba_tipoff_update(&tipoff, NULL);
    NbaTipoffActor *actor = &tipoff.actors[slot];
    if (actor->x_fp != 102 * 256 || actor->y_fp != 200 * 256 ||
        actor->control_mode != 7u || actor->reaction_threshold != 0x1eu ||
        actor->animation_state != 0x0eu ||
        actor->lower_animation_state != 0x0eu) {
        fprintf(stderr, "[CPU MODE SEVEN INTEGRATION] hold x=%ld y=%ld "
                "mode=%u timer=%04x anim=%u/%u ticks=%lu/%lu accum=%04x/%04x valid=%u\n",
                (long)actor->x_fp, (long)actor->y_fp, actor->control_mode,
                actor->reaction_threshold, actor->animation_state,
                actor->lower_animation_state,
                (unsigned long)actor->upper_animation_tick,
                (unsigned long)actor->lower_animation_tick,
                actor->upper_animation_accumulator_raw_42,
                actor->lower_animation_accumulator_raw_44,
                actor->animation_resources_valid);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    actor = &tipoff.actors[slot];
    actor->direction = 3u;
    actor->facing_ease_timer_raw_be = 2u;
    ModeSevenPhaseMutation eased_phase = {
        .actor = slot, .mutate = false
    };
    tipoff.differential_observer = mutate_after_physics;
    tipoff.differential_context = &eased_phase;
    nba_tipoff_update(&tipoff, NULL);
    if (!baseline_phase.seen || !eased_phase.seen ||
        baseline_phase.direction != 4u || eased_phase.direction != 4u ||
        !baseline_phase.resources_valid || !eased_phase.resources_valid ||
        baseline_phase.upper_resource != eased_phase.upper_resource ||
        baseline_phase.lower_resource != eased_phase.lower_resource) {
        fprintf(stderr, "[CPU MODE SEVEN INTEGRATION] facing "
                "baseline=%u/%u/%04x/%04x eased=%u/%u/%04x/%04x\n",
                baseline_phase.direction, baseline_phase.resources_valid,
                baseline_phase.upper_resource, baseline_phase.lower_resource,
                eased_phase.direction, eased_phase.resources_valid,
                eased_phase.upper_resource, eased_phase.lower_resource);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    actor = &tipoff.actors[slot];
    actor->velocity_x = 0;
    actor->animation_state = 0u;
    actor->lower_animation_state = 0u;
    actor->base_animation_state_raw_38 = 0u;
    ModeSevenPhaseMutation stationary_phase = {
        .actor = slot, .mutate = false
    };
    tipoff.differential_observer = mutate_after_physics;
    tipoff.differential_context = &stationary_phase;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (actor->control_mode != 7u || actor->reaction_threshold != 0x1eu ||
        actor->animation_state != 0x10u ||
        actor->lower_animation_state != 0x10u ||
        actor->upper_animation_tick != 0u ||
        actor->lower_animation_tick != 0u ||
        actor->upper_animation_accumulator_raw_42 != 0u ||
        actor->lower_animation_accumulator_raw_44 != 0u ||
        actor->animation_resources_valid) {
        fprintf(stderr, "[CPU MODE SEVEN INTEGRATION] stationary "
                "mode=%u timer=%04x anim=%u/%u ticks=%lu/%lu "
                "accum=%04x/%04x valid=%u\n", actor->control_mode,
                actor->reaction_threshold, actor->animation_state,
                actor->lower_animation_state,
                (unsigned long)actor->upper_animation_tick,
                (unsigned long)actor->lower_animation_tick,
                actor->upper_animation_accumulator_raw_42,
                actor->lower_animation_accumulator_raw_44,
                actor->animation_resources_valid);
        fprintf(stderr, "[CPU MODE SEVEN INTEGRATION] stationary boundary "
                "seen=%u anim=%u tick=%lu\n", stationary_phase.seen,
                stationary_phase.animation_state,
                (unsigned long)stationary_phase.animation_tick);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    tipoff.camera_side_group_raw = 5u;
    ModeSevenPhaseMutation mutation = {
        .actor = slot, .mutate = true
    };
    tipoff.differential_observer = mutate_after_physics;
    tipoff.differential_context = &mutation;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    bool passed = mutation.seen && mutation.committed_x_fp == 102 * 256 &&
           actor->control_mode == 2u && actor->reaction_threshold == 0u &&
           actor->behavior_timer == 0x2fu &&
           actor->team_group_raw_6e == 0u &&
           actor->animation_state == 3u &&
           actor->upper_animation_tick == 0u &&
           actor->animation_resources_valid == false;
    if (!passed) {
        fprintf(stderr, "[CPU MODE SEVEN INTEGRATION] restore seen=%u "
                "committed_x=%ld mode=%u timer=%04x behavior=%04x anim=%u "
                "tick=%lu valid=%u live=%04x group=%u owner=%d\n",
                mutation.seen, (long)mutation.committed_x_fp,
                actor->control_mode, actor->reaction_threshold,
                actor->behavior_timer, actor->animation_state,
                (unsigned long)actor->upper_animation_tick,
                actor->animation_resources_valid, tipoff.live_state_raw,
                tipoff.camera_side_group_raw, tipoff.possession_actor);
        fprintf(stderr, "[CPU MODE SEVEN INTEGRATION] restore team_group=%u\n",
                actor->team_group_raw_6e);
    }
    return passed;
}
#endif

/* Host-only native-vector probe; no direct native address. Load compact,
 * non-asset witnesses and invoke the production mode-seven dispatcher. */
int main(int argc, char **argv) {
    NbaAssetPack assets = {0};
    uint16_t input[FIELD_COUNT];
    if ((argc != 2 && argc != 3) || !nba_assets_load(&assets, argv[1])) return 2;
#ifndef NBA_MODE_SEVEN_DIRECT_ONLY
    if (argc == 3) {
        bool passed = integration_case(&assets);
        nba_assets_free(&assets);
        puts(passed ? "[CPU MODE SEVEN INTEGRATION] PASS" :
                      "[CPU MODE SEVEN INTEGRATION] FAIL");
        return passed ? 0 : 7;
    }
#else
    if (argc == 3) return 8;
#endif
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(input, sizeof(input[0]), FIELD_COUNT, stdin) == FIELD_COUNT) {
        NbaSession session;
        NbaTipoff tipoff;
        nba_session_init(&session);
        session.right_team = (uint8_t)input[4];
        session.left_team = (uint8_t)input[5];
        if (!nba_tipoff_init(&tipoff, &assets, &session)) {
            nba_assets_free(&assets);
            return 3;
        }

        unsigned slot = input[3];
        if (slot >= NBA_GAMEPLAY_ACTOR_COUNT) {
            nba_assets_free(&assets);
            return 4;
        }
        NbaTipoffActor *actor = &tipoff.actors[slot];
        tipoff.live_state_raw = input[0];
        tipoff.camera_side_group_raw = (uint8_t)input[1];
        tipoff.possession_actor = (int8_t)(int16_t)input[2];
        tipoff.scratch_0046 = input[7];
        tipoff.scratch_0047 = input[7] >> 8;
        actor->roster_slot = (uint8_t)input[6];
        actor->x_fp = fixed_from_words(input[8], input[9]);
        actor->y_fp = fixed_from_words(input[10], input[11]);
        actor->z_fp = fixed_from_words(input[12], input[13]);
        actor->velocity_x = (int16_t)input[14];
        actor->velocity_y = (int16_t)input[15];
        actor->velocity_z = (int16_t)input[16];
        actor->controller_assignment_raw = (int8_t)(int16_t)input[17];
        actor->animation_upper_queue_cursor_raw_18 = input[18];
        actor->animation_lower_queue_cursor_raw_1a = input[19];
        for (unsigned i = 0; i < 3u; ++i) {
            actor->animation_upper_queue_raw_1c[i] = input[20 + i];
            actor->animation_lower_queue_raw_22[i] = input[23 + i];
        }
        actor->actor_status_raw_28 = input[26];
        actor->animation_state = (uint8_t)input[27];
        actor->lower_animation_state = (uint8_t)input[28];
        actor->base_animation_state_raw_38 = (uint8_t)input[29];
        actor->rom_upper_animation_phase_raw_3a = input[30];
        actor->rom_lower_animation_phase_raw_3c = input[31];
        actor->upper_animation_accumulator_raw_42 = input[32];
        actor->lower_animation_accumulator_raw_44 = input[33];
        actor->upper_animation_lock_raw_46 = input[34];
        actor->lower_animation_lock_raw_48 = input[35];
        actor->movement_direction = (uint8_t)input[36];
        actor->requested_direction = (uint8_t)input[37];
        actor->direction = (uint8_t)input[38];
        actor->control_mode = (uint8_t)input[39];
        actor->reaction_threshold = input[40];
        actor->behavior_timer = input[41];
        actor->pass_direction_raw = input[42];
        actor->team_group_raw_6e = input[43];
        actor->movement_boost_timer = input[44];
        actor->behavior_flags_raw = input[45];
        actor->free_throw_launch_half_raw_a8 = input[46];
        actor->upper_phase_target_raw_b0 = input[47];
        actor->upper_animation_resource_raw_2a = input[48];
        actor->lower_animation_resource_raw_2c = input[49];
        actor->animation_resources_valid = true;
        actor->exact_jump_animation = true;

        if (!nba_tipoff_replay_passive_mode(&tipoff, (uint8_t)slot)) {
            nba_assets_free(&assets);
            return 5;
        }

        unsigned count = 0;
        emit(tipoff.live_state_raw, &count);
        emit(tipoff.camera_side_group_raw, &count);
        emit((uint16_t)(int16_t)tipoff.possession_actor, &count);
        emit((uint16_t)slot, &count);
        emit(session.right_team, &count);
        emit(session.left_team, &count);
        emit(actor->roster_slot, &count);
        emit(tipoff.scratch_0046, &count);
        emit(fixed_subpixel_word(actor->x_fp), &count);
        emit(fixed_integer_word(actor->x_fp), &count);
        emit(fixed_subpixel_word(actor->y_fp), &count);
        emit(fixed_integer_word(actor->y_fp), &count);
        emit(fixed_subpixel_word(actor->z_fp), &count);
        emit(fixed_integer_word(actor->z_fp), &count);
        emit((uint16_t)actor->velocity_x, &count);
        emit((uint16_t)actor->velocity_y, &count);
        emit((uint16_t)actor->velocity_z, &count);
        emit((uint16_t)(int16_t)actor->controller_assignment_raw, &count);
        emit(actor->animation_upper_queue_cursor_raw_18, &count);
        emit(actor->animation_lower_queue_cursor_raw_1a, &count);
        for (unsigned i = 0; i < 3u; ++i)
            emit(actor->animation_upper_queue_raw_1c[i], &count);
        for (unsigned i = 0; i < 3u; ++i)
            emit(actor->animation_lower_queue_raw_22[i], &count);
        emit(actor->actor_status_raw_28, &count);
        emit(actor->animation_state, &count);
        emit(actor->lower_animation_state, &count);
        emit(actor->base_animation_state_raw_38, &count);
        emit(actor->rom_upper_animation_phase_raw_3a, &count);
        emit(actor->rom_lower_animation_phase_raw_3c, &count);
        emit(actor->upper_animation_accumulator_raw_42, &count);
        emit(actor->lower_animation_accumulator_raw_44, &count);
        emit(actor->upper_animation_lock_raw_46, &count);
        emit(actor->lower_animation_lock_raw_48, &count);
        emit(actor->movement_direction, &count);
        emit(actor->requested_direction, &count);
        emit(actor->direction, &count);
        emit(actor->control_mode, &count);
        emit(actor->reaction_threshold, &count);
        emit(actor->behavior_timer, &count);
        emit(actor->pass_direction_raw, &count);
        emit(actor->team_group_raw_6e, &count);
        emit(actor->movement_boost_timer, &count);
        emit(actor->behavior_flags_raw, &count);
        emit(actor->free_throw_launch_half_raw_a8, &count);
        emit(actor->upper_phase_target_raw_b0, &count);
        emit(actor->upper_animation_resource_raw_2a, &count);
        emit(actor->lower_animation_resource_raw_2c, &count);
        if (count != FIELD_COUNT) {
            nba_assets_free(&assets);
            return 6;
        }
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
