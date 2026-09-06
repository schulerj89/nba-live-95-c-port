#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nba_tipoff.h"

#define FIELD_COUNT 52u

/* Host-only fixture helper; no native address. Rebuild a signed 16.8 actor
 * coordinate for the `$86:F0B7-$F0FC` production replay. */
static int32_t fixed_from_words(uint16_t subpixel, uint16_t integer) {
    return (int32_t)(int16_t)integer * 256 + (int32_t)(subpixel >> 8);
}

/* Host-only fixture helper; no native address. Recover a replayed actor's
 * represented subpixel word for the mode-nine strict output. */
static uint16_t fixed_subpixel_word(int32_t value) {
    return (uint16_t)(((uint32_t)value & 0xffu) << 8);
}

/* Host-only fixture helper; no native address. Recover the signed integer
 * word consumed by mode nine and its movement children. */
static uint16_t fixed_integer_word(int32_t value) {
    int32_t integer = value >= 0 ? value / 256 : -(((-value) + 255) / 256);
    return (uint16_t)(int16_t)integer;
}

/* Host-only fixture helper; no native address. Emit one exact 16-bit field
 * for the `$86:F0B7-$F0FC` strict replay. */
static void emit(uint16_t value, unsigned *count) {
    printf("%s%04x", *count ? " " : "", value);
    ++*count;
}

typedef struct {
    unsigned actor;
    uint16_t mutate_inhibit;
    bool seen;
    int32_t committed_x_fp;
    int32_t committed_y_fp;
    uint16_t timer;
    int16_t velocity_x;
    int16_t velocity_y;
    uint8_t velocity_direction;
    uint8_t animation_state;
    uint32_t animation_tick;
} ModeNinePhase;

/* Host-only fixture helper; no native address. Prepare a complete scheduler
 * pass around `$87:AAB2/$85:963D/$87:9244 -> $86:F0B7`. */
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
    tipoff->possession_actor = 3;
    NbaTipoffActor *state = &tipoff->actors[actor];
    state->roster_slot = 2u;
    state->team_group_raw_6e = 0u;
    state->x_fp = 100 * 256;
    state->y_fp = 20 * 256;
    state->z_fp = 0;
    state->velocity_x = 0x0100;
    state->velocity_y = 0;
    state->control_mode = 9u;
    state->pass_band_raw = 4u;
    state->reaction_threshold = 0x000cu;
    state->target_x = 200;
    state->target_y = 20;
    state->velocity_direction_raw_a2 = 4u;
    state->animation_state = 5u;
    state->lower_animation_state = 5u;
    state->base_animation_state_raw_38 = 5u;
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

/* Host-only test observer; no native address. Sample the real post-physics
 * boundary and optionally publish a recovery inhibit for later mode nine. */
static void observe_after_physics(const NbaTipoff *observed,
                                  const char *boundary,
                                  void *raw_context) {
    ModeNinePhase *context = raw_context;
    if (context->seen || strcmp(boundary, "actors.end") != 0) return;
    NbaTipoff *tipoff = (NbaTipoff *)observed;
    NbaTipoffActor *actor = &tipoff->actors[context->actor];
    context->seen = true;
    context->committed_x_fp = actor->x_fp;
    context->committed_y_fp = actor->y_fp;
    context->timer = actor->reaction_threshold;
    context->velocity_x = actor->velocity_x;
    context->velocity_y = actor->velocity_y;
    context->velocity_direction = actor->velocity_direction_raw_a2;
    context->animation_state = actor->animation_state;
    context->animation_tick = actor->upper_animation_tick;
    if (context->mutate_inhibit)
        actor->recovery_inhibit_raw = context->mutate_inhibit;
}

/* Host-only production scheduler check; no direct native address. Verify
 * mode nine consumes old motion before choosing target/final/restore motion. */
static bool integration_case(const NbaAssetPack *assets) {
    const unsigned slot = 4u;
    NbaSession session;
    NbaTipoff tipoff;
    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    NbaTipoffActor *actor = &tipoff.actors[slot];
    ModeNinePhase target = {.actor = slot};
    tipoff.differential_observer = observe_after_physics;
    tipoff.differential_context = &target;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (!target.seen || target.committed_x_fp != 102 * 256 ||
        target.committed_y_fp != 20 * 256 || target.timer != 0x000cu ||
        target.velocity_x != 0x0100 || target.velocity_y != 0 ||
        actor->control_mode != 9u || actor->reaction_threshold != 0x000au ||
        actor->velocity_x <= target.velocity_x || actor->velocity_y != 0) {
        fprintf(stderr, "[CPU MODE NINE INTEGRATION] target seen=%u "
                "boundary=%ld,%ld/%04x/%d,%d final=%u/%04x/%d,%d\n",
                target.seen, (long)target.committed_x_fp,
                (long)target.committed_y_fp, target.timer,
                target.velocity_x, target.velocity_y, actor->control_mode,
                actor->reaction_threshold, actor->velocity_x,
                actor->velocity_y);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    actor = &tipoff.actors[slot];
    actor->reaction_threshold = 0x000bu;
    actor->velocity_x = 0x0100;
    actor->target_x = -200;
    actor->target_y = 20;
    /* Deliberately stale: common physics must refresh eastward +$A2 before
     * the final window consumes it instead of using this negative-Y value. */
    actor->velocity_direction_raw_a2 = 4u;
    ModeNinePhase final = {.actor = slot};
    tipoff.differential_observer = observe_after_physics;
    tipoff.differential_context = &final;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (!final.seen || final.committed_x_fp != 102 * 256 ||
        final.velocity_x != 0x0100 || final.velocity_direction != 2u ||
        actor->reaction_threshold != 9u ||
        actor->velocity_x <= final.velocity_x || actor->velocity_y != 0) {
        fprintf(stderr, "[CPU MODE NINE INTEGRATION] final seen=%u "
                "boundary=%ld/%d,%d/a2=%u final=%04x/%d,%d\n", final.seen,
                (long)final.committed_x_fp, final.velocity_x,
                final.velocity_y, final.velocity_direction,
                actor->reaction_threshold,
                actor->velocity_x, actor->velocity_y);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    actor = &tipoff.actors[slot];
    ModeNinePhase continued = {.actor = slot, .mutate_inhibit = 1u};
    tipoff.differential_observer = observe_after_physics;
    tipoff.differential_context = &continued;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (!continued.seen || continued.committed_x_fp != 102 * 256 ||
        actor->recovery_inhibit_raw != 0u || actor->control_mode != 9u ||
        actor->reaction_threshold != 0x000au) {
        fprintf(stderr, "[CPU MODE NINE INTEGRATION] inhibit-one seen=%u "
                "boundary=%ld final=%u/%04x/%04x\n", continued.seen,
                (long)continued.committed_x_fp, actor->control_mode,
                actor->reaction_threshold, actor->recovery_inhibit_raw);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    ModeNinePhase high_continued = {
        .actor = slot, .mutate_inhibit = 0x8002u
    };
    tipoff.differential_observer = observe_after_physics;
    tipoff.differential_context = &high_continued;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (!high_continued.seen || actor->recovery_inhibit_raw != 0u ||
        actor->control_mode != 9u || actor->reaction_threshold != 0x000au) {
        fprintf(stderr, "[CPU MODE NINE INTEGRATION] inhibit-8002 "
                "seen=%u final=%u/%04x/%04x\n", high_continued.seen,
                actor->control_mode, actor->reaction_threshold,
                actor->recovery_inhibit_raw);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    ModeNinePhase high_restore = {
        .actor = slot, .mutate_inhibit = 0x8001u
    };
    tipoff.differential_observer = observe_after_physics;
    tipoff.differential_context = &high_restore;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (!high_restore.seen || actor->recovery_inhibit_raw != 0x7fffu ||
        actor->control_mode != 4u || actor->reaction_threshold != 0u) {
        fprintf(stderr, "[CPU MODE NINE INTEGRATION] inhibit-8001 "
                "seen=%u final=%u/%04x/%04x\n", high_restore.seen,
                actor->control_mode, actor->reaction_threshold,
                actor->recovery_inhibit_raw);
        return false;
    }

    if (!prepare_scheduler_case(assets, &session, &tipoff, slot)) return false;
    actor = &tipoff.actors[slot];
    ModeNinePhase restore = {.actor = slot, .mutate_inhibit = 3u};
    tipoff.differential_observer = observe_after_physics;
    tipoff.differential_context = &restore;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[slot];
    if (!restore.seen || restore.committed_x_fp != 102 * 256 ||
        restore.timer != 0x000cu || actor->recovery_inhibit_raw != 1u ||
        actor->control_mode != 4u ||
        actor->reaction_threshold != 0u || actor->movement_boost_timer != 0u ||
        actor->animation_state != 3u || actor->lower_animation_state != 3u ||
        actor->upper_animation_tick != 0u ||
        actor->animation_resources_valid) {
        fprintf(stderr, "[CPU MODE NINE INTEGRATION] restore seen=%u "
                "boundary=%ld/%04x/%u/%lu final=%u/%04x/%u/%u/%lu/%u\n",
                restore.seen, (long)restore.committed_x_fp, restore.timer,
                restore.animation_state,
                (unsigned long)restore.animation_tick, actor->control_mode,
                actor->reaction_threshold, actor->animation_state,
                actor->lower_animation_state,
                (unsigned long)actor->upper_animation_tick,
                actor->animation_resources_valid);
        return false;
    }
    return true;
}

/* Host-only native-vector probe; no direct native address. Load compact
 * non-asset witnesses and invoke the production mode-nine dispatcher. */
int main(int argc, char **argv) {
    NbaAssetPack assets = {0};
    uint16_t input[FIELD_COUNT];
    if ((argc != 2 && argc != 3) || !nba_assets_load(&assets, argv[1])) return 2;
    if (argc == 3) {
        bool passed = integration_case(&assets);
        nba_assets_free(&assets);
        puts(passed ? "[CPU MODE NINE INTEGRATION] PASS" :
                      "[CPU MODE NINE INTEGRATION] FAIL");
        return passed ? 0 : 7;
    }
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(input, sizeof(input[0]), FIELD_COUNT, stdin) == FIELD_COUNT) {
        NbaSession session;
        NbaTipoff tipoff;
        nba_session_init(&session);
        session.right_team = (uint8_t)input[3];
        session.left_team = (uint8_t)input[4];
        if (!nba_tipoff_init(&tipoff, &assets, &session)) return 3;
        unsigned slot = input[2];
        if (slot >= NBA_GAMEPLAY_ACTOR_COUNT) return 4;
        NbaTipoffActor *actor = &tipoff.actors[slot];
        tipoff.live_state_raw = input[0];
        tipoff.possession_actor = (int8_t)(int16_t)input[1];
        actor->roster_slot = (uint8_t)input[5];
        tipoff.scratch_0046 = input[6];
        tipoff.scratch_0047 = input[7];
        actor->x_fp = fixed_from_words(input[8], input[9]);
        actor->y_fp = fixed_from_words(input[10], input[11]);
        actor->z_fp = fixed_from_words(input[12], input[13]);
        actor->velocity_x = (int16_t)input[14];
        actor->velocity_y = (int16_t)input[15];
        actor->velocity_z = (int16_t)input[16];
        actor->animation_upper_queue_cursor_raw_18 = input[17];
        actor->animation_lower_queue_cursor_raw_1a = input[18];
        for (unsigned i = 0; i < 3u; ++i) {
            actor->animation_upper_queue_raw_1c[i] = input[19 + i];
            actor->animation_lower_queue_raw_22[i] = input[22 + i];
        }
        actor->actor_status_raw_28 = input[25];
        actor->animation_state = (uint8_t)input[26];
        actor->lower_animation_state = (uint8_t)input[27];
        actor->base_animation_state_raw_38 = (uint8_t)input[28];
        actor->rom_upper_animation_phase_raw_3a = input[29];
        actor->rom_lower_animation_phase_raw_3c = input[30];
        actor->upper_animation_accumulator_raw_42 = input[31];
        actor->lower_animation_accumulator_raw_44 = input[32];
        actor->upper_animation_lock_raw_46 = input[33];
        actor->lower_animation_lock_raw_48 = input[34];
        actor->movement_direction = (uint8_t)input[35];
        actor->requested_direction = (uint8_t)input[36];
        actor->direction = (uint8_t)input[37];
        actor->target_x = (int16_t)input[38];
        actor->target_y = (int16_t)input[39];
        actor->control_mode = (uint8_t)input[40];
        actor->reaction_threshold = input[41];
        actor->pass_band_raw = input[42];
        actor->behavior_timer = input[43];
        actor->movement_boost_timer = input[44];
        actor->behavior_flags_raw = input[45];
        actor->recovery_inhibit_raw = input[46];
        actor->velocity_direction_raw_a2 = (uint8_t)input[47];
        actor->free_throw_launch_half_raw_a8 = input[48];
        actor->upper_phase_target_raw_b0 = input[49];
        actor->upper_animation_resource_raw_2a = input[50];
        actor->lower_animation_resource_raw_2c = input[51];
        actor->animation_resources_valid = true;
        actor->exact_jump_animation = true;
        (void)nba_tipoff_replay_passive_mode(&tipoff, (uint8_t)slot);

        unsigned count = 0;
        emit(tipoff.live_state_raw, &count);
        emit((uint16_t)(int16_t)tipoff.possession_actor, &count);
        emit((uint16_t)slot, &count); emit(session.right_team, &count);
        emit(session.left_team, &count); emit(actor->roster_slot, &count);
        emit(tipoff.scratch_0046, &count); emit(tipoff.scratch_0047, &count);
        emit(fixed_subpixel_word(actor->x_fp), &count);
        emit(fixed_integer_word(actor->x_fp), &count);
        emit(fixed_subpixel_word(actor->y_fp), &count);
        emit(fixed_integer_word(actor->y_fp), &count);
        emit(fixed_subpixel_word(actor->z_fp), &count);
        emit(fixed_integer_word(actor->z_fp), &count);
        emit((uint16_t)actor->velocity_x, &count);
        emit((uint16_t)actor->velocity_y, &count);
        emit((uint16_t)actor->velocity_z, &count);
        emit(actor->animation_upper_queue_cursor_raw_18, &count);
        emit(actor->animation_lower_queue_cursor_raw_1a, &count);
        for (unsigned i = 0; i < 3u; ++i) emit(actor->animation_upper_queue_raw_1c[i], &count);
        for (unsigned i = 0; i < 3u; ++i) emit(actor->animation_lower_queue_raw_22[i], &count);
        emit(actor->actor_status_raw_28, &count);
        emit(actor->animation_state, &count); emit(actor->lower_animation_state, &count);
        emit(actor->base_animation_state_raw_38, &count);
        emit(actor->rom_upper_animation_phase_raw_3a, &count);
        emit(actor->rom_lower_animation_phase_raw_3c, &count);
        emit(actor->upper_animation_accumulator_raw_42, &count);
        emit(actor->lower_animation_accumulator_raw_44, &count);
        emit(actor->upper_animation_lock_raw_46, &count);
        emit(actor->lower_animation_lock_raw_48, &count);
        emit(actor->movement_direction, &count); emit(actor->requested_direction, &count);
        emit(actor->direction, &count); emit((uint16_t)actor->target_x, &count);
        emit((uint16_t)actor->target_y, &count); emit(actor->control_mode, &count);
        emit(actor->reaction_threshold, &count); emit(actor->pass_band_raw, &count);
        emit(actor->behavior_timer, &count); emit(actor->movement_boost_timer, &count);
        emit(actor->behavior_flags_raw, &count); emit(actor->recovery_inhibit_raw, &count);
        emit(actor->velocity_direction_raw_a2, &count);
        emit(actor->free_throw_launch_half_raw_a8, &count);
        emit(actor->upper_phase_target_raw_b0, &count);
        emit(actor->upper_animation_resource_raw_2a, &count);
        emit(actor->lower_animation_resource_raw_2c, &count);
        if (count != FIELD_COUNT) return 6;
        putchar('\n');
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
