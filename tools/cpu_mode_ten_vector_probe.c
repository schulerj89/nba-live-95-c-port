#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nba_tipoff.h"

#define FIELD_COUNT 68u

/* Host-only CPU-logic fixture helper; no direct native address. Rebuild a signed 16.8
 * coordinate for the `$86:A5B0-$A628` production replay. */
static int32_t fixed_from_words(uint16_t subpixel, uint16_t integer) {
    return (int32_t)(int16_t)integer * 256 + (int32_t)(subpixel >> 8);
}

/* Host-only CPU-logic fixture helper; no direct native address. Recover a replayed
 * record's represented subpixel word for the mode-ten strict output. */
static uint16_t fixed_subpixel_word(int32_t value) {
    return (uint16_t)(((uint32_t)value & 0xffu) << 8);
}

/* Host-only CPU-logic fixture helper; no direct native address. Recover the signed
 * integer word consumed by mode ten's `$87:9C71` actor entries. */
static uint16_t fixed_integer_word(int32_t value) {
    int32_t integer = value >= 0 ? value / 256 : -(((-value) + 255) / 256);
    return (uint16_t)(int16_t)integer;
}

/* Host-only CPU-logic fixture helper; no direct native address. Emit one exact 16-bit
 * field for the `$86:A5B0-$A628` strict replay. */
static void emit(uint16_t value, unsigned *count) {
    printf("%s%04x", *count ? " " : "", value);
    ++*count;
}

/* Host-only CPU-logic fixture helper; no direct native address. Load every represented
 * field in a compact `$86:A5B0-$A628` witness. */
static bool load_case(NbaTipoff *tipoff, NbaSession *session,
                      const NbaAssetPack *assets, const uint16_t input[]) {
    nba_session_init(session);
    session->right_team = 18u;
    session->left_team = 28u;
    session->config.rules[2] = input[10];
    if (!nba_tipoff_init(tipoff, assets, session)) return false;
    unsigned slot = input[11];
    if (slot >= NBA_GAMEPLAY_ACTOR_COUNT) return false;
    NbaTipoffActor *actor = &tipoff->actors[slot];
    tipoff->live_state_raw = input[0];
    tipoff->camera_side_group_raw = (uint8_t)input[1];
    tipoff->possession_actor = (int8_t)(int16_t)input[2];
    tipoff->pass_actor_raw = (int16_t)input[3];
    tipoff->pass_aux_raw = (int16_t)input[4];
    tipoff->pass_receiver_raw = (int16_t)input[5];
    tipoff->ball_activity_raw = input[6];
    tipoff->rim_raw_094a = input[7];
    tipoff->inbound_transfer_raw = input[8];
    tipoff->pass_active_raw = input[9];
    tipoff->scratch_0046 = input[12];
    tipoff->scratch_0047 = input[13];
    for (unsigned pad = 0; pad < NBA_CONTROLLER_COUNT; ++pad)
        tipoff->controllers.record[pad].held = input[14 + pad];

    actor->x_fp = fixed_from_words(input[25], input[26]);
    actor->y_fp = fixed_from_words(input[27], input[28]);
    actor->z_fp = fixed_from_words(input[29], input[30]);
    actor->velocity_x = (int16_t)input[31];
    actor->velocity_y = (int16_t)input[32];
    actor->velocity_z = (int16_t)input[33];
    actor->animation_upper_queue_cursor_raw_18 = input[34];
    actor->animation_lower_queue_cursor_raw_1a = input[35];
    for (unsigned index = 0; index < 3u; ++index) {
        actor->animation_upper_queue_raw_1c[index] = input[36 + index];
        actor->animation_lower_queue_raw_22[index] = input[39 + index];
    }
    actor->actor_status_raw_28 = input[42];
    actor->upper_animation_resource_raw_2a = input[43];
    actor->lower_animation_resource_raw_2c = input[44];
    actor->animation_state = (uint8_t)input[45];
    actor->lower_animation_state = (uint8_t)input[46];
    actor->base_animation_state_raw_38 = (uint8_t)input[47];
    actor->rom_upper_animation_phase_raw_3a = input[48];
    actor->rom_lower_animation_phase_raw_3c = input[49];
    actor->upper_animation_accumulator_raw_42 = input[50];
    actor->lower_animation_accumulator_raw_44 = input[51];
    actor->upper_animation_lock_raw_46 = input[52];
    actor->lower_animation_lock_raw_48 = input[53];
    actor->control_mode = (uint8_t)input[54];
    actor->reaction_threshold = input[55];
    actor->behavior_timer = input[56];
    actor->team_group_raw_6e = input[57];
    actor->behavior_flags_raw = input[58];
    actor->animation_resources_valid = true;

    /* `$87:9C71` can alias the selected record. Apply its three actor-table
     * inputs after the selected actor so the native overlap stays exact. */
    for (unsigned lookup = 0; lookup < 3u; ++lookup)
        tipoff->actors[lookup].y_fp =
            fixed_from_words(input[19 + lookup * 2],
                             input[20 + lookup * 2]);

    tipoff->ball.x_fp = fixed_from_words(input[59], input[60]);
    tipoff->ball.y_fp = fixed_from_words(input[61], input[62]);
    tipoff->ball.z_fp = fixed_from_words(input[63], input[64]);
    tipoff->ball.velocity_x = (int16_t)input[65];
    tipoff->ball.velocity_y = (int16_t)input[66];
    tipoff->ball.velocity_z = (int16_t)input[67];
    return true;
}

/* Host-only CPU-logic fixture helper; no direct native address. Emit every represented
 * field after the production mode-ten replay. */
static void emit_case(const NbaTipoff *tipoff, const NbaSession *session,
                      unsigned slot) {
    const NbaTipoffActor *actor = &tipoff->actors[slot];
    unsigned count = 0;
    emit(tipoff->live_state_raw, &count);
    emit(tipoff->camera_side_group_raw, &count);
    emit((uint16_t)(int16_t)tipoff->possession_actor, &count);
    emit((uint16_t)tipoff->pass_actor_raw, &count);
    emit((uint16_t)tipoff->pass_aux_raw, &count);
    emit((uint16_t)tipoff->pass_receiver_raw, &count);
    emit(tipoff->ball_activity_raw, &count);
    emit(tipoff->rim_raw_094a, &count);
    emit(tipoff->inbound_transfer_raw, &count);
    emit(tipoff->pass_active_raw, &count);
    emit(session->config.rules[2], &count);
    emit((uint16_t)slot, &count);
    emit(tipoff->scratch_0046, &count);
    emit(tipoff->scratch_0047, &count);
    for (unsigned pad = 0; pad < NBA_CONTROLLER_COUNT; ++pad)
        emit(tipoff->controllers.record[pad].held, &count);
    for (unsigned lookup = 0; lookup < 3u; ++lookup) {
        emit(fixed_subpixel_word(tipoff->actors[lookup].y_fp), &count);
        emit(fixed_integer_word(tipoff->actors[lookup].y_fp), &count);
    }
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
    for (unsigned index = 0; index < 3u; ++index)
        emit(actor->animation_upper_queue_raw_1c[index], &count);
    for (unsigned index = 0; index < 3u; ++index)
        emit(actor->animation_lower_queue_raw_22[index], &count);
    emit(actor->actor_status_raw_28, &count);
    emit(actor->upper_animation_resource_raw_2a, &count);
    emit(actor->lower_animation_resource_raw_2c, &count);
    emit(actor->animation_state, &count);
    emit(actor->lower_animation_state, &count);
    emit(actor->base_animation_state_raw_38, &count);
    emit(actor->rom_upper_animation_phase_raw_3a, &count);
    emit(actor->rom_lower_animation_phase_raw_3c, &count);
    emit(actor->upper_animation_accumulator_raw_42, &count);
    emit(actor->lower_animation_accumulator_raw_44, &count);
    emit(actor->upper_animation_lock_raw_46, &count);
    emit(actor->lower_animation_lock_raw_48, &count);
    emit(actor->control_mode, &count);
    emit(actor->reaction_threshold, &count);
    emit(actor->behavior_timer, &count);
    emit(actor->team_group_raw_6e, &count);
    emit(actor->behavior_flags_raw, &count);
    emit(fixed_subpixel_word(tipoff->ball.x_fp), &count);
    emit(fixed_integer_word(tipoff->ball.x_fp), &count);
    emit(fixed_subpixel_word(tipoff->ball.y_fp), &count);
    emit(fixed_integer_word(tipoff->ball.y_fp), &count);
    emit(fixed_subpixel_word(tipoff->ball.z_fp), &count);
    emit(fixed_integer_word(tipoff->ball.z_fp), &count);
    emit((uint16_t)tipoff->ball.velocity_x, &count);
    emit((uint16_t)tipoff->ball.velocity_y, &count);
    emit((uint16_t)tipoff->ball.velocity_z, &count);
    if (count != FIELD_COUNT) fprintf(stderr, "mode-ten field count=%u\n", count);
    putchar('\n');
}

typedef struct {
    unsigned actor;
    bool invalidate_receiver;
    bool seen;
    int32_t committed_x_fp;
    int32_t committed_y_fp;
    uint16_t timer;
    uint16_t auxiliary;
    uint8_t animation_state;
    uint32_t animation_tick;
} ModeTenPhase;

/* Host-only CPU-logic test observer; no direct native address. Sample the real
 * post-physics boundary and publish inputs consumed by later `$86:A5B0`. */
static void observe_after_physics(const NbaTipoff *observed,
                                  const char *boundary,
                                  void *raw_context) {
    ModeTenPhase *context = raw_context;
    if (context->seen || strcmp(boundary, "actors.end") != 0) return;
    NbaTipoff *tipoff = (NbaTipoff *)observed;
    NbaTipoffActor *actor = &tipoff->actors[context->actor];
    context->seen = true;
    context->committed_x_fp = actor->x_fp;
    context->committed_y_fp = actor->y_fp;
    context->timer = actor->reaction_threshold;
    context->auxiliary = (uint16_t)tipoff->pass_aux_raw;
    context->animation_state = actor->animation_state;
    context->animation_tick = actor->upper_animation_tick;
    tipoff->pass_actor_raw = 3;
    tipoff->pass_aux_raw = 14;
    tipoff->pass_receiver_raw = context->invalidate_receiver ? -1 :
                                (int16_t)context->actor;
    if (context->invalidate_receiver) tipoff->live_state_raw = 0x0082u;
    tipoff->ball_activity_raw = 0x1111u;
    tipoff->rim_raw_094a = 0x2222u;
    tipoff->inbound_transfer_raw = 0x3333u;
    tipoff->pass_active_raw = 0x4444u;
    tipoff->actors[1].y_fp = 1 * 256 + 0xabu;
}

/* Host-only CPU-logic production caller check; no direct native address. Start a pass
 * through `$86:AB2D-$AF65`, then run each preinstalled passer/receiver slot
 * order through the production dispatcher once and prove `$87:9244 ->
 * $86:A5B0` decrements its mode-ten receiver exactly once. */
static bool real_pass_order_cases(const NbaAssetPack *assets) {
    static const unsigned order[2][2] = {{3u, 1u}, {1u, 3u}};
    for (unsigned case_index = 0; case_index < 2u; ++case_index) {
        unsigned passer_slot = order[case_index][0];
        unsigned receiver_slot = order[case_index][1];
        NbaSession session;
        NbaTipoff tipoff;
        nba_session_init(&session);
        session.right_team = 18u;
        session.left_team = 28u;
        if (!nba_tipoff_init(&tipoff, assets, &session)) return false;
        for (unsigned actor_index = 0;
             actor_index < NBA_GAMEPLAY_ACTOR_COUNT; ++actor_index) {
            NbaTipoffActor *actor = &tipoff.actors[actor_index];
            actor->control_mode = 0u;
            actor->velocity_x = actor->velocity_y = actor->velocity_z = 0;
        }
        NbaTipoffActor *passer = &tipoff.actors[passer_slot];
        NbaTipoffActor *receiver = &tipoff.actors[receiver_slot];
        passer->control_mode = 1u;
        passer->x_fp = 0;
        passer->y_fp = 0;
        passer->z_fp = 0;
        passer->movement_direction = 2u;
        passer->lower_animation_state = 3u;
        receiver->control_mode = 1u;
        receiver->x_fp = 100 * 256;
        receiver->y_fp = 0;
        receiver->z_fp = 0;
        tipoff.simulation_tick = 1u;
        tipoff.tip_contact_actor = 0;
        tipoff.live_state_raw = 0x0080u;
        tipoff.camera_side_group_raw = 0u;
        tipoff.possession_actor = (int8_t)passer_slot;
        tipoff.ball.owner_actor = (int8_t)passer_slot;
        tipoff.ball.state = NBA_BALL_ATTACHED;
        tipoff.cpu_play_state = NBA_CPU_PLAY_PASS;
        if (!nba_tipoff_begin_rom_pass(
                &tipoff, passer_slot, receiver_slot)) return false;
        uint16_t installed_timer = receiver->reaction_threshold;
        nba_tipoff_update(&tipoff, NULL);
        receiver = &tipoff.actors[receiver_slot];
        if (receiver->control_mode != 10u ||
            receiver->reaction_threshold !=
                (uint16_t)(installed_timer - 2u) ||
            tipoff.pass_receiver_raw != (int16_t)receiver_slot) {
            fprintf(stderr, "[CPU MODE TEN PASS ORDER] case=%u "
                    "passer=%u receiver=%u installed=%04x final=%u/%04x/%04x\n",
                    case_index, passer_slot, receiver_slot, installed_timer,
                    receiver->control_mode, receiver->reaction_threshold,
                    (uint16_t)tipoff.pass_receiver_raw);
            return false;
        }
    }
    return true;
}

/* Host-only CPU-logic production scheduler probe; no direct native address.
 * Exercise the dead-ball caller around `$87:8EFB/$87:9244`: ordinary odd
 * frames must not run physics, animation, or mode ten; an explicitly pending
 * odd dispatch consumes exactly one behavior pass without another prepare;
 * and a real inbound pass must release before its receiver timer expires. */
static bool dead_ball_cadence_cases(const NbaAssetPack *assets) {
    NbaSession session;
    NbaTipoff tipoff;
    nba_session_init(&session);
    session.right_team = 18u;
    session.left_team = 28u;
    if (!nba_tipoff_init(&tipoff, assets, &session)) return false;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        tipoff.actors[i].control_mode = 0u;
        tipoff.actors[i].velocity_x = 0;
        tipoff.actors[i].velocity_y = 0;
        tipoff.actors[i].velocity_z = 0;
        tipoff.actors[i].x_fp = (int32_t)(-300 + (int)i * 60) * 256;
        tipoff.actors[i].y_fp = (int32_t)(-160 + (int)i * 32) * 256;
    }
    tipoff.tip_contact_actor = 0;
    tipoff.simulation_tick = 2u;
    tipoff.actor_behavior_pending = 0u;
    tipoff.live_state_raw = 0x0082u;
    tipoff.possession_actor = 3;
    tipoff.handler_actor = 3u;
    tipoff.inbound_actor_raw = 3u;
    tipoff.inbound_transfer_raw = 1u;
    tipoff.ball.owner_actor = 3;
    tipoff.ball.state = NBA_BALL_ATTACHED;
    tipoff.pass_actor_raw = 3;
    tipoff.pass_receiver_raw = 1;
    tipoff.pass_aux_raw = 0;
    NbaTipoffActor *receiver = &tipoff.actors[1];
    receiver->control_mode = 10u;
    receiver->reaction_threshold = 40u;
    receiver->velocity_x = 0x0100;
    receiver->animation_state = 0x0eu;
    receiver->lower_animation_state = 0x0eu;
    receiver->base_animation_state_raw_38 = 0x0eu;
    receiver->animation_upper_queue_cursor_raw_18 = 0xffffu;
    receiver->animation_lower_queue_cursor_raw_1a = 0xffffu;
    receiver->exact_jump_animation = true;
    receiver->animation_resources_valid = true;
    uint32_t odd_animation = receiver->upper_animation_tick;
    int32_t odd_x = receiver->x_fp;
    nba_tipoff_update(&tipoff, NULL);
    receiver = &tipoff.actors[1];
    if (tipoff.simulation_tick != 3u ||
        receiver->reaction_threshold != 40u || receiver->x_fp != odd_x ||
        receiver->upper_animation_tick != odd_animation) {
        fprintf(stderr, "[CPU MODE TEN DEAD BALL ODD] tick=%lu "
                "timer=%04x x=%ld/%ld animation=%lu/%lu\n",
                (unsigned long)tipoff.simulation_tick,
                receiver->reaction_threshold, (long)odd_x,
                (long)receiver->x_fp, (unsigned long)odd_animation,
                (unsigned long)receiver->upper_animation_tick);
        return false;
    }
    nba_tipoff_update(&tipoff, NULL);
    receiver = &tipoff.actors[1];
    if (tipoff.simulation_tick != 4u ||
        receiver->reaction_threshold != 38u ||
        receiver->x_fp != odd_x + 0x0200 ||
        receiver->upper_animation_tick == odd_animation) {
        fprintf(stderr, "[CPU MODE TEN DEAD BALL EVEN] tick=%lu "
                "timer=%04x x=%ld/%ld animation=%lu/%lu\n",
                (unsigned long)tipoff.simulation_tick,
                receiver->reaction_threshold, (long)(odd_x + 0x0200),
                (long)receiver->x_fp, (unsigned long)odd_animation,
                (unsigned long)receiver->upper_animation_tick);
        return false;
    }

    nba_session_init(&session);
    session.right_team = 18u;
    session.left_team = 28u;
    if (!nba_tipoff_init(&tipoff, assets, &session)) return false;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        tipoff.actors[i].control_mode = 0u;
        tipoff.actors[i].velocity_x = 0;
        tipoff.actors[i].velocity_y = 0;
        tipoff.actors[i].velocity_z = 0;
    }
    tipoff.tip_contact_actor = 0;
    tipoff.simulation_tick = 2u;
    tipoff.live_state_raw = 0x0082u;
    tipoff.possession_actor = 3;
    tipoff.handler_actor = 3u;
    tipoff.inbound_actor_raw = 3u;
    tipoff.inbound_transfer_raw = 1u;
    tipoff.ball.owner_actor = 3;
    tipoff.ball.state = NBA_BALL_ATTACHED;
    tipoff.pass_actor_raw = 3;
    tipoff.pass_receiver_raw = 1;
    tipoff.pass_aux_raw = 0;
    tipoff.actor_behavior_pending = 1u;
    receiver = &tipoff.actors[1];
    receiver->control_mode = 10u;
    receiver->reaction_threshold = 40u;
    receiver->velocity_x = 0x0100;
    receiver->animation_resources_valid = true;
    NbaTipoffActor *knockdown = &tipoff.actors[2];
    knockdown->control_mode = 8u;
    knockdown->special_contact_raw_56 = -1;
    knockdown->pass_direction_raw = 0xffffu;
    knockdown->reaction_threshold = 100u;
    knockdown->contact_action_timer_raw_60 = 100u;
    knockdown->recovery_inhibit_raw = 10u;
    knockdown->contact_inhibit_raw_5a = 10u;
    odd_animation = receiver->upper_animation_tick;
    odd_x = receiver->x_fp;
    nba_tipoff_update(&tipoff, NULL);
    receiver = &tipoff.actors[1];
    knockdown = &tipoff.actors[2];
    if (tipoff.simulation_tick != 3u || tipoff.actor_behavior_pending != 0u ||
        receiver->reaction_threshold != 38u || receiver->x_fp != odd_x ||
        receiver->upper_animation_tick != odd_animation ||
        knockdown->contact_action_timer_raw_60 != 98u ||
        knockdown->recovery_inhibit_raw != 10u ||
        knockdown->contact_inhibit_raw_5a != 10u) {
        fprintf(stderr, "[CPU MODE TEN DEAD BALL PENDING] tick=%lu "
                "pending=%u receiver=%04x/%ld/%lu knockdown=%04x/%04x/%04x\n",
                (unsigned long)tipoff.simulation_tick,
                tipoff.actor_behavior_pending, receiver->reaction_threshold,
                (long)receiver->x_fp,
                (unsigned long)receiver->upper_animation_tick,
                knockdown->contact_action_timer_raw_60,
                knockdown->recovery_inhibit_raw,
                knockdown->contact_inhibit_raw_5a);
        return false;
    }

    nba_session_init(&session);
    session.right_team = 18u;
    session.left_team = 28u;
    if (!nba_tipoff_init(&tipoff, assets, &session)) return false;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        NbaTipoffActor *actor = &tipoff.actors[i];
        actor->control_mode = 0u;
        actor->velocity_x = actor->velocity_y = actor->velocity_z = 0;
        actor->x_fp = (int32_t)(-320 + (int)i * 8) * 256;
        actor->y_fp = (int32_t)(-180 + (int)i * 8) * 256;
    }
    NbaTipoffActor *passer = &tipoff.actors[3];
    receiver = &tipoff.actors[1];
    passer->control_mode = 11u;
    passer->x_fp = 0;
    passer->y_fp = 0;
    passer->z_fp = 0;
    passer->movement_direction = 2u;
    passer->lower_animation_state = 3u;
    receiver->control_mode = 1u;
    receiver->x_fp = 100 * 256;
    receiver->y_fp = 0;
    receiver->z_fp = 0;
    tipoff.tip_contact_actor = 0;
    tipoff.simulation_tick = 1u;
    tipoff.live_state_raw = 0x0082u;
    tipoff.possession_actor = 3;
    tipoff.handler_actor = 3u;
    tipoff.inbound_actor_raw = 3u;
    tipoff.inbound_transfer_raw = 1u;
    tipoff.ball.owner_actor = 3;
    tipoff.ball.state = NBA_BALL_ATTACHED;
    tipoff.cpu_play_state = NBA_CPU_PLAY_PASS;
    if (!nba_tipoff_begin_rom_pass(&tipoff, 3u, 1u)) return false;
    uint16_t installed_timer = receiver->reaction_threshold;
    bool released = false;
    unsigned release_updates = 0u;
    for (unsigned frame = 0; frame < 40u; ++frame) {
        nba_tipoff_update(&tipoff, NULL);
        if (tipoff.actors[3].pass_released_raw) {
            released = true;
            release_updates = frame + 1u;
            break;
        }
    }
    receiver = &tipoff.actors[1];
    if (!released || receiver->control_mode != 10u ||
        receiver->reaction_threshold == 0u) {
        fprintf(stderr, "[CPU MODE TEN DEAD BALL RELEASE] released=%u "
                "passer=%u phase=%u/%u receiver=%u/%04x pass=%04x/%04x\n",
                released, tipoff.actors[3].control_mode,
                tipoff.actors[3].upper_animation_phase_raw,
                tipoff.actors[3].pass_release_threshold_raw,
                receiver->control_mode, receiver->reaction_threshold,
                tipoff.pass_active_raw,
                (uint16_t)tipoff.pass_receiver_raw);
        return false;
    }
    printf("[CPU MODE TEN DEAD BALL RELEASE] installed=%04x "
           "updates=%u tick=%lu remaining=%04x\n",
           installed_timer, release_updates,
           (unsigned long)tipoff.simulation_tick,
           receiver->reaction_threshold);
    return true;
}

/* Host-only CPU-logic production scheduler check; no direct native address. Verify
 * mode ten commits old motion/animation before consuming post-physics globals
 * and consumes the dispatch pass even when `$86:9846` restores the actor. */
static bool integration_case(const NbaAssetPack *assets) {
    const unsigned slot = 0u;
    for (unsigned variant = 0; variant < 3u; ++variant) {
        bool invalid = variant == 1u;
        bool edge = variant == 2u;
        NbaSession session;
        NbaTipoff tipoff;
        nba_session_init(&session);
        session.right_team = 18u;
        session.left_team = 28u;
        if (!nba_tipoff_init(&tipoff, assets, &session)) return false;
        for (unsigned actor_index = 0;
             actor_index < NBA_GAMEPLAY_ACTOR_COUNT; ++actor_index) {
            tipoff.actors[actor_index].control_mode = 0u;
            tipoff.actors[actor_index].velocity_x = 0;
            tipoff.actors[actor_index].velocity_y = 0;
            tipoff.actors[actor_index].velocity_z = 0;
        }
        tipoff.simulation_tick = 1u;
        tipoff.tip_contact_actor = 0;
        tipoff.live_state_raw = 0x0080u;
        tipoff.camera_side_group_raw = 0u;
        tipoff.possession_actor = 3;
        tipoff.ball.owner_actor = 3;
        tipoff.ball.state = NBA_BALL_ATTACHED;
        NbaTipoffActor *actor = &tipoff.actors[slot];
        actor->team_group_raw_6e = 0u;
        actor->x_fp = (edge ? 393 : 100) * 256;
        actor->y_fp = 20 * 256;
        actor->z_fp = 0;
        actor->velocity_x = 0x0100;
        actor->velocity_y = -0x0080;
        actor->control_mode = 10u;
        actor->reaction_threshold = 4u;
        actor->animation_state = 0x0eu;
        actor->lower_animation_state = 0x0eu;
        actor->base_animation_state_raw_38 = 0x0eu;
        actor->upper_animation_tick = 0u;
        actor->lower_animation_tick = 0u;
        actor->rom_upper_animation_phase_raw_3a = 0u;
        actor->rom_lower_animation_phase_raw_3c = 0u;
        actor->upper_animation_accumulator_raw_42 = 0u;
        actor->lower_animation_accumulator_raw_44 = 0u;
        actor->animation_upper_queue_cursor_raw_18 = 0xffffu;
        actor->animation_lower_queue_cursor_raw_1a = 0xffffu;
        for (unsigned queue = 0; queue < 3u; ++queue) {
            actor->animation_upper_queue_raw_1c[queue] = 0xffffu;
            actor->animation_lower_queue_raw_22[queue] = 0xffffu;
        }
        actor->animation_resources_valid = true;
        actor->exact_jump_animation = true;
        ModeTenPhase phase = {.actor = slot, .invalidate_receiver = invalid};
        tipoff.differential_observer = observe_after_physics;
        tipoff.differential_context = &phase;
        nba_tipoff_update(&tipoff, NULL);
        actor = &tipoff.actors[slot];
        bool common_ok = phase.seen &&
            phase.committed_x_fp == (edge ? 394 : 102) * 256 &&
            phase.committed_y_fp == 19 * 256 &&
            phase.timer == (edge ? 0u : 4u) &&
            phase.animation_state == 0x0eu && phase.animation_tick != 0u;
        bool result_ok = (invalid || edge) ?
            (actor->control_mode == 1u && actor->reaction_threshold == 0u &&
             actor->behavior_timer == 0x2fu &&
             tipoff.live_state_raw == (invalid ? 0x0082u : 0u) &&
             tipoff.pass_actor_raw == -1 && tipoff.pass_aux_raw == -1 &&
             tipoff.pass_receiver_raw == -1 &&
             tipoff.ball_activity_raw == 0u && tipoff.rim_raw_094a == 0u &&
             tipoff.inbound_transfer_raw == 0u &&
             tipoff.pass_active_raw == 0x4444u) :
            (actor->control_mode == 10u && actor->reaction_threshold == 2u &&
             tipoff.pass_aux_raw == 6 && tipoff.pass_receiver_raw == 0 &&
             tipoff.pass_active_raw == 0x4444u);
        if (!common_ok || !result_ok) {
            fprintf(stderr, "[CPU MODE TEN INTEGRATION] variant=%u "
                    "seen=%u boundary=%ld,%ld/%04x/%u/%lu "
                    "final=%u/%04x/%04x/%04x/%04x/%04x\n",
                    variant, phase.seen, (long)phase.committed_x_fp,
                    (long)phase.committed_y_fp, phase.timer,
                    phase.animation_state, (unsigned long)phase.animation_tick,
                    actor->control_mode, actor->reaction_threshold,
                    (uint16_t)tipoff.pass_aux_raw,
                    (uint16_t)tipoff.pass_receiver_raw,
                    tipoff.live_state_raw, tipoff.pass_active_raw);
            return false;
        }
        if (edge) {
            actor->exact_jump_animation = false;
            uint32_t restore_tick = actor->upper_animation_tick;
            tipoff.differential_observer = NULL;
            tipoff.differential_context = NULL;
            nba_tipoff_update(&tipoff, NULL);
            nba_tipoff_update(&tipoff, NULL);
            actor = &tipoff.actors[slot];
            if (actor->control_mode != 1u ||
                !actor->animation_resources_valid ||
                actor->exact_jump_animation ||
                actor->upper_animation_tick == restore_tick) {
                fprintf(stderr, "[CPU MODE TEN CONTINUATION] mode=%u "
                        "tick=%lu/%lu valid=%u exact=%u state=%u/%u\n",
                        actor->control_mode,
                        (unsigned long)restore_tick,
                        (unsigned long)actor->upper_animation_tick,
                        actor->animation_resources_valid,
                        actor->exact_jump_animation,
                        actor->animation_state, actor->lower_animation_state);
                return false;
            }
        }
    }
    return real_pass_order_cases(assets) && dead_ball_cadence_cases(assets);
}

/* Host-only CPU-logic native-vector probe; no direct native address. Load compact
 * non-asset witnesses and invoke the production mode-ten dispatcher. */
int main(int argc, char **argv) {
    NbaAssetPack assets = {0};
    uint16_t input[FIELD_COUNT];
    if ((argc != 2 && argc != 3) || !nba_assets_load(&assets, argv[1])) return 2;
    if (argc == 3) {
        bool passed = integration_case(&assets);
        nba_assets_free(&assets);
        puts(passed ? "[CPU MODE TEN INTEGRATION] PASS" :
                      "[CPU MODE TEN INTEGRATION] FAIL");
        return passed ? 0 : 7;
    }
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(input, sizeof(input[0]), FIELD_COUNT, stdin) == FIELD_COUNT) {
        NbaSession session;
        NbaTipoff tipoff;
        if (!load_case(&tipoff, &session, &assets, input)) return 3;
        unsigned slot = input[11];
        if (!nba_tipoff_replay_passive_mode(&tipoff, (uint8_t)slot)) return 4;
        emit_case(&tipoff, &session, slot);
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
