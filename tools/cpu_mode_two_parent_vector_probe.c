#include <fcntl.h>
#include <io.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nba_tipoff.h"

#define FIELD_COUNT 197u
#define GLOBAL_BASE 2u
#define ACTOR_BASE 34u
#define PAIRED_BASE 98u
#define BALL_BASE 162u
#define CONTEXT_BASE 171u
#define LINEUP_BASE 187u

/* Host-only CPU-logic fixed-point adapter for `$86:F6CD-$F793`; no direct native
 * address. Decode the native integer word and represented high fraction byte. */
static int32_t fixed_word(uint16_t fraction, uint16_t integer) {
    return (int32_t)(int16_t)integer * 256 + (fraction >> 8);
}

/* Host-only CPU-logic actor adapter for `$86:F6CD-$F793`; no direct native address.
 * Load every represented actor word consumed or mutated by the parent/children. */
static void load_actor(NbaTipoffActor *actor, const uint16_t *p) {
    actor->x_fp = fixed_word(p[0], p[1]);
    actor->y_fp = fixed_word(p[2], p[3]);
    actor->z_fp = fixed_word(p[4], p[5]);
    actor->velocity_x = (int16_t)p[6];
    actor->velocity_y = (int16_t)p[7];
    actor->velocity_z = (int16_t)p[8];
    actor->controller_assignment_raw = (int16_t)p[9];
    actor->animation_upper_queue_cursor_raw_18 = p[10];
    actor->animation_lower_queue_cursor_raw_1a = p[11];
    for (unsigned i = 0; i < 3u; ++i) {
        actor->animation_upper_queue_raw_1c[i] = p[12 + i];
        actor->animation_lower_queue_raw_22[i] = p[15 + i];
    }
    actor->actor_status_raw_28 = p[18];
    actor->upper_animation_resource_raw_2a = p[19];
    actor->lower_animation_resource_raw_2c = p[20];
    actor->animation_resources_valid = true;
    actor->animation_state = (uint8_t)p[21];
    actor->lower_animation_state = (uint8_t)p[22];
    actor->upper_state_snapshot_raw_34 = p[23];
    actor->lower_state_snapshot_raw_36 = p[24];
    actor->base_animation_state_raw_38 = (uint8_t)p[25];
    actor->rom_upper_animation_phase_raw_3a = p[26];
    actor->rom_lower_animation_phase_raw_3c = p[27];
    actor->upper_phase_snapshot_raw_3e = p[28];
    actor->lower_phase_snapshot_raw_40 = p[29];
    actor->upper_animation_accumulator_raw_42 = p[30];
    actor->lower_animation_accumulator_raw_44 = p[31];
    actor->upper_animation_lock_raw_46 = p[32];
    actor->lower_animation_lock_raw_48 = p[33];
    actor->movement_speed_raw_4a = p[34];
    actor->movement_magnitude_raw = p[35];
    actor->movement_direction = (uint8_t)p[36];
    actor->requested_direction = (uint8_t)p[37];
    actor->direction = (uint8_t)p[38];
    actor->target_x = (int16_t)p[39];
    actor->target_y = (int16_t)p[40];
    actor->contact_inhibit_raw_5a = p[41];
    actor->formation_timer_raw_5c = p[42];
    actor->control_mode = (uint8_t)p[43];
    actor->reaction_threshold = p[44];
    actor->pass_band_raw = p[45];
    actor->behavior_timer = p[46];
    actor->pass_direction_raw = p[47];
    actor->animation_variant_raw_6c = p[48];
    actor->team_group_raw_6e = p[49];
    actor->movement_boost_timer = p[50];
    actor->assignment_base_raw = p[51];
    actor->assignment_current_raw = p[52];
    actor->recovery_inhibit_raw = p[53];
    actor->behavior_flags_raw = p[54];
    actor->assignment_direction = (uint8_t)p[55];
    actor->anchor_direction_raw = (uint8_t)p[56];
    actor->assignment_distance = p[57];
    actor->anchor_distance_raw = p[58];
    actor->focal_distance_raw_8e = p[59];
    actor->assignment_role_raw_92 = p[60];
    actor->velocity_direction_raw_a2 = (uint8_t)p[61];
    actor->free_throw_launch_half_raw_a8 = p[62];
    actor->upper_phase_target_raw_b0 = p[63];
}

/* Host-only CPU-logic actor projection for `$86:F6CD-$F793`; no direct native address.
 * Emit the same represented words after the production parent returns. */
static void emit_actor(uint16_t *p, const NbaTipoffActor *actor) {
    p[0] = (uint16_t)((uint32_t)actor->x_fp & 0xFFu) << 8;
    p[1] = (uint16_t)(int16_t)(actor->x_fp >> 8);
    p[2] = (uint16_t)((uint32_t)actor->y_fp & 0xFFu) << 8;
    p[3] = (uint16_t)(int16_t)(actor->y_fp >> 8);
    p[4] = (uint16_t)((uint32_t)actor->z_fp & 0xFFu) << 8;
    p[5] = (uint16_t)(int16_t)(actor->z_fp >> 8);
    p[6] = (uint16_t)actor->velocity_x;
    p[7] = (uint16_t)actor->velocity_y;
    p[8] = (uint16_t)actor->velocity_z;
    p[9] = (uint16_t)(int16_t)actor->controller_assignment_raw;
    p[10] = actor->animation_upper_queue_cursor_raw_18;
    p[11] = actor->animation_lower_queue_cursor_raw_1a;
    for (unsigned i = 0; i < 3u; ++i) {
        p[12 + i] = actor->animation_upper_queue_raw_1c[i];
        p[15 + i] = actor->animation_lower_queue_raw_22[i];
    }
    p[18] = actor->actor_status_raw_28;
    p[19] = actor->upper_animation_resource_raw_2a;
    p[20] = actor->lower_animation_resource_raw_2c;
    p[21] = actor->animation_state;
    p[22] = actor->lower_animation_state;
    p[23] = actor->upper_state_snapshot_raw_34;
    p[24] = actor->lower_state_snapshot_raw_36;
    p[25] = actor->base_animation_state_raw_38;
    p[26] = actor->rom_upper_animation_phase_raw_3a;
    p[27] = actor->rom_lower_animation_phase_raw_3c;
    p[28] = actor->upper_phase_snapshot_raw_3e;
    p[29] = actor->lower_phase_snapshot_raw_40;
    p[30] = actor->upper_animation_accumulator_raw_42;
    p[31] = actor->lower_animation_accumulator_raw_44;
    p[32] = actor->upper_animation_lock_raw_46;
    p[33] = actor->lower_animation_lock_raw_48;
    p[34] = actor->movement_speed_raw_4a;
    p[35] = actor->movement_magnitude_raw;
    p[36] = actor->movement_direction;
    p[37] = actor->requested_direction;
    p[38] = actor->direction;
    p[39] = (uint16_t)actor->target_x;
    p[40] = (uint16_t)actor->target_y;
    p[41] = actor->contact_inhibit_raw_5a;
    p[42] = actor->formation_timer_raw_5c;
    p[43] = actor->control_mode;
    p[44] = actor->reaction_threshold;
    p[45] = actor->pass_band_raw;
    p[46] = actor->behavior_timer;
    p[47] = actor->pass_direction_raw;
    p[48] = actor->animation_variant_raw_6c;
    p[49] = actor->team_group_raw_6e;
    p[50] = actor->movement_boost_timer;
    p[51] = actor->assignment_base_raw;
    p[52] = actor->assignment_current_raw;
    p[53] = actor->recovery_inhibit_raw;
    p[54] = actor->behavior_flags_raw;
    p[55] = actor->assignment_direction;
    p[56] = actor->anchor_direction_raw;
    p[57] = actor->assignment_distance;
    p[58] = actor->anchor_distance_raw;
    p[59] = actor->focal_distance_raw_8e;
    p[60] = actor->assignment_role_raw_92;
    p[61] = actor->velocity_direction_raw_a2;
    p[62] = actor->free_throw_launch_half_raw_a8;
    p[63] = actor->upper_phase_target_raw_b0;
}

/* Host-only CPU-logic fixture loader for `$86:F6CD-$F793`; no direct native address.
 * Bind compact native state to the production NbaTipoff representation. */
static bool load_case(NbaTipoff *tipoff, NbaSession *session,
                      const NbaAssetPack *assets, const uint16_t *in) {
    unsigned slot = in[0], paired = in[1];
    if (slot >= NBA_GAMEPLAY_ACTOR_COUNT || paired >= NBA_GAMEPLAY_ACTOR_COUNT)
        return false;
    nba_session_init(session);
    session->right_team = (uint8_t)in[CONTEXT_BASE];
    session->left_team = (uint8_t)in[CONTEXT_BASE + 8u];
    session->config.main_values[2] = (uint8_t)in[30];
    if (!nba_tipoff_init(tipoff, assets, session)) return false;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        tipoff->actors[i].roster_slot = (uint8_t)in[LINEUP_BASE + i];
    load_actor(&tipoff->actors[slot], in + ACTOR_BASE);
    load_actor(&tipoff->actors[paired], in + PAIRED_BASE);
    tipoff->scratch_0046 = in[2];
    tipoff->scratch_0047 = in[3];
    tipoff->formation_override_raw_005c = in[4];
    tipoff->rng.state = in[5];
    tipoff->catch_actor_record_raw_0910 = in[6];
    tipoff->period_raw_0926 = in[7];
    tipoff->match_clock_raw_0928 = in[8];
    tipoff->live_state_raw = in[9];
    tipoff->camera_side_group_raw = (uint8_t)in[10];
    tipoff->possession_actor = (int8_t)(int16_t)in[11];
    tipoff->pass_receiver_raw = (int16_t)in[12];
    tipoff->ball_activity_raw = in[13];
    tipoff->rim_raw_094a = in[14];
    tipoff->shot_value_raw = in[15];
    tipoff->inbound_state_raw = (int16_t)in[16];
    tipoff->inbound_actor_raw = in[17];
    tipoff->inbound_target_x_raw = (int16_t)in[18];
    tipoff->inbound_target_y_raw = (int16_t)in[19];
    tipoff->rim_raw_0962 = in[20];
    tipoff->dead_ball_raw_0968 = in[21];
    tipoff->fouls.free_throw_state_raw_0978 = in[22];
    tipoff->rim_raw_097c = in[23];
    tipoff->play_code = in[24];
    tipoff->play_step_raw = (int16_t)in[25];
    tipoff->play_mirror_raw = in[26];
    tipoff->special_actor_raw = in[27];
    tipoff->play_cycle_raw = in[28];
    tipoff->role_ownerless_raw_09d8 = in[29];
    tipoff->defensive_pose_count_raw_1868 = in[31];
    tipoff->collision_actor_b_raw = (int8_t)(int16_t)in[32];
    tipoff->hud.shot_category_raw_4939 = in[33];
    tipoff->ball.x_fp = fixed_word(in[BALL_BASE], in[BALL_BASE + 1u]);
    tipoff->ball.y_fp = fixed_word(in[BALL_BASE + 2u], in[BALL_BASE + 3u]);
    tipoff->ball.z_fp = fixed_word(in[BALL_BASE + 4u], in[BALL_BASE + 5u]);
    tipoff->ball.velocity_x = (int16_t)in[BALL_BASE + 6u];
    tipoff->ball.velocity_y = (int16_t)in[BALL_BASE + 7u];
    tipoff->ball.velocity_z = (int16_t)in[BALL_BASE + 8u];
    tipoff->ball.owner_actor = tipoff->possession_actor;
    for (unsigned side = 0; side < 2u; ++side) {
        unsigned p = CONTEXT_BASE + side * 8u;
        tipoff->team_context[side].strategy_team_raw_00 = in[p];
        tipoff->team_context[side].anchor_x_fraction_raw_08 = in[p + 1u];
        tipoff->team_context[side].anchor_x_raw_0a = (int16_t)in[p + 2u];
        tipoff->team_context[side].score_raw_26 = in[p + 3u];
        tipoff->team_context[side].mode_raw_30 = in[p + 4u];
        tipoff->team_context[side].flags_raw_32 = in[p + 5u];
        tipoff->team_context[side].activity_raw_39 = in[p + 6u];
        tipoff->team_pose_contact_count_raw[side] = in[p + 7u];
    }
    tipoff->cpu_play_state = NBA_CPU_PLAY_ATTACK;
    return true;
}

/* Host-only CPU-logic fixture projection for `$86:F6CD-$F793`; no direct native
 * address. Emit production state in the strict native field order. */
static void emit_case(const NbaTipoff *tipoff, const uint16_t *in) {
    uint16_t out[FIELD_COUNT] = {0};
    unsigned slot = in[0], paired = in[1];
    out[0] = (uint16_t)slot;
    out[1] = (uint16_t)paired;
    out[2] = tipoff->scratch_0046;
    out[3] = tipoff->scratch_0047;
    out[4] = tipoff->formation_override_raw_005c;
    out[5] = tipoff->rng.state;
    out[6] = tipoff->catch_actor_record_raw_0910;
    out[7] = tipoff->period_raw_0926;
    out[8] = tipoff->match_clock_raw_0928;
    out[9] = tipoff->live_state_raw;
    out[10] = tipoff->camera_side_group_raw == 0xFFu
        ? 0xFFFFu : tipoff->camera_side_group_raw;
    out[11] = (uint16_t)(int16_t)tipoff->possession_actor;
    out[12] = (uint16_t)tipoff->pass_receiver_raw;
    out[13] = tipoff->ball_activity_raw;
    out[14] = tipoff->rim_raw_094a;
    out[15] = tipoff->shot_value_raw;
    out[16] = (uint16_t)tipoff->inbound_state_raw;
    out[17] = tipoff->inbound_actor_raw;
    out[18] = (uint16_t)tipoff->inbound_target_x_raw;
    out[19] = (uint16_t)tipoff->inbound_target_y_raw;
    out[20] = tipoff->rim_raw_0962;
    out[21] = tipoff->dead_ball_raw_0968;
    out[22] = tipoff->fouls.free_throw_state_raw_0978;
    out[23] = tipoff->rim_raw_097c;
    out[24] = tipoff->play_code;
    out[25] = (uint16_t)tipoff->play_step_raw;
    out[26] = tipoff->play_mirror_raw;
    out[27] = tipoff->special_actor_raw;
    out[28] = tipoff->play_cycle_raw;
    out[29] = tipoff->role_ownerless_raw_09d8;
    out[30] = tipoff->session->config.main_values[2];
    out[31] = tipoff->defensive_pose_count_raw_1868;
    out[32] = (uint16_t)(int16_t)tipoff->collision_actor_b_raw;
    out[33] = tipoff->hud.shot_category_raw_4939;
    emit_actor(out + ACTOR_BASE, &tipoff->actors[slot]);
    emit_actor(out + PAIRED_BASE, &tipoff->actors[paired]);
    out[BALL_BASE] = (uint16_t)((uint32_t)tipoff->ball.x_fp & 0xFFu) << 8;
    out[BALL_BASE + 1u] = (uint16_t)(int16_t)(tipoff->ball.x_fp >> 8);
    out[BALL_BASE + 2u] = (uint16_t)((uint32_t)tipoff->ball.y_fp & 0xFFu) << 8;
    out[BALL_BASE + 3u] = (uint16_t)(int16_t)(tipoff->ball.y_fp >> 8);
    out[BALL_BASE + 4u] = (uint16_t)((uint32_t)tipoff->ball.z_fp & 0xFFu) << 8;
    out[BALL_BASE + 5u] = (uint16_t)(int16_t)(tipoff->ball.z_fp >> 8);
    out[BALL_BASE + 6u] = (uint16_t)tipoff->ball.velocity_x;
    out[BALL_BASE + 7u] = (uint16_t)tipoff->ball.velocity_y;
    out[BALL_BASE + 8u] = (uint16_t)tipoff->ball.velocity_z;
    for (unsigned side = 0; side < 2u; ++side) {
        unsigned p = CONTEXT_BASE + side * 8u;
        out[p] = tipoff->team_context[side].strategy_team_raw_00;
        out[p + 1u] = tipoff->team_context[side].anchor_x_fraction_raw_08;
        out[p + 2u] = (uint16_t)tipoff->team_context[side].anchor_x_raw_0a;
        out[p + 3u] = tipoff->team_context[side].score_raw_26;
        out[p + 4u] = tipoff->team_context[side].mode_raw_30;
        out[p + 5u] = tipoff->team_context[side].flags_raw_32;
        out[p + 6u] = tipoff->team_context[side].activity_raw_39;
        out[p + 7u] = tipoff->team_pose_contact_count_raw[side];
    }
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        out[LINEUP_BASE + i] = tipoff->actors[i].roster_slot;
    for (unsigned i = 0; i < FIELD_COUNT; ++i)
        printf(i + 1u == FIELD_COUNT ? "%04x\n" : "%04x ", out[i]);
}

/* Host-only CPU-logic production-caller test for `$86:F6CD-$F793`; no
 * direct native address. Exercise the real update sweep in rebound state. */
static bool mode_two_caller_self_test(const NbaAssetPack *assets) {
    NbaSession session;
    NbaTipoff tipoff;
    nba_session_init(&session);
    session.right_team = 18u;
    session.left_team = 28u;
    if (!nba_tipoff_init(&tipoff, assets, &session)) return false;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        NbaTipoffActor *actor = &tipoff.actors[i];
        actor->control_mode = 0u;
        actor->controller_assignment_raw = -1;
        actor->velocity_x = 0;
        actor->velocity_y = 0;
        actor->velocity_z = 0;
        actor->x_fp = (int32_t)(-300 + (int)i * 50) * 256;
        actor->y_fp = (int32_t)(-180 + (int)i * 35) * 256;
        actor->z_fp = 0;
    }
    tipoff.simulation_tick = 1u;
    tipoff.tip_contact_actor = 0;
    tipoff.tip_possession_frame = 1u;
    tipoff.phase = NBA_TIPOFF_LIVE;
    tipoff.live_state_raw = 0x82u;
    tipoff.camera_side_group_raw = 0u;
    tipoff.inbound_state_raw = 0;
    tipoff.inbound_actor_raw = 0xFFFFu;
    tipoff.possession_actor = -1;
    tipoff.pass_receiver_raw = 0;
    tipoff.ball.owner_actor = -1;
    tipoff.ball.state = NBA_BALL_LOOSE;
    tipoff.ball.x_fp = 300 * 256;
    tipoff.ball.y_fp = 180 * 256;
    tipoff.ball.z_fp = 100 * 256;
    tipoff.cpu_play_state = NBA_CPU_PLAY_REBOUND;
    tipoff.role_rebuild_raw_09d6 = 0u;
    tipoff.role_cadence_raw_09d2 = 0x0100u;
    NbaTipoffActor *actor = &tipoff.actors[5];
    actor->control_mode = 2u;
    actor->assignment_base_raw = 0u;
    actor->assignment_current_raw = 0u;
    actor->reaction_threshold = 0x20u;
    actor->behavior_timer = 0x1234u;
    actor->recovery_inhibit_raw = 0u;
    actor->x_fp = -100 * 256;
    actor->y_fp = 0;
    actor->velocity_x = 0x0100;
    actor->target_x = 0x5555;
    actor->target_y = 0x5555;
    int32_t initial_x = actor->x_fp;
    int16_t initial_velocity_x = actor->velocity_x;
    nba_tipoff_update(&tipoff, NULL);
    actor = &tipoff.actors[5];
    bool ok = tipoff.simulation_tick == 2u && tipoff.actor_pass_executed &&
        tipoff.cpu_play_state == NBA_CPU_PLAY_REBOUND &&
        actor->control_mode == 2u && actor->x_fp == initial_x + 0x0200 &&
        actor->velocity_x != initial_velocity_x &&
        actor->behavior_timer == 0x1234u && actor->reaction_threshold != 0x20u &&
        (actor->target_x != 0x5555 || actor->target_y != 0x5555);
    actor->controller_assignment_raw = 0x7FFF;
    bool positive_controller = nba_tipoff_transfer_controller(&tipoff, 5u) &&
        actor->controller_assignment_raw == 5;
    actor->controller_assignment_raw = (int16_t)0x8000u;
    bool negative_controller = nba_tipoff_transfer_controller(&tipoff, 5u) &&
        actor->controller_assignment_raw == -1;
    ok = ok && positive_controller && negative_controller;
    if (!ok)
        fprintf(stderr, "mode2 caller tick=%lu pass=%u play=%u mode=%u "
                "x=%ld/%ld velocity=%04x/%04x timer=%04x reaction=%04x target=%d,%d "
                "controller=%d positive=%u negative=%u\n",
                (unsigned long)tipoff.simulation_tick,
                tipoff.actor_pass_executed, tipoff.cpu_play_state,
                actor->control_mode, (long)actor->x_fp,
                (long)(initial_x + 0x0200), (uint16_t)actor->velocity_x,
                (uint16_t)initial_velocity_x, actor->behavior_timer,
                actor->reaction_threshold, actor->target_x, actor->target_y,
                actor->controller_assignment_raw, positive_controller,
                negative_controller);
    return ok;
}

/* Host-only CPU-logic executable fixture for `$86:F6CD-$F793`; no direct native
 * address. Replay compact native witnesses through the production parent. */
int main(int argc, char **argv) {
    bool self_test = argc == 3 && strcmp(argv[2], "--self-test") == 0;
    if (argc < 2 || argc > 3 || (argc == 3 && !self_test)) {
        fprintf(stderr, "usage: %s <asset-pack> [--self-test]\n", argv[0]);
        return 2;
    }
    NbaAssetPack assets = {0};
    if (!nba_assets_load(&assets, argv[1])) return 3;
    if (self_test) {
        bool ok = mode_two_caller_self_test(&assets);
        nba_assets_free(&assets);
        if (ok)
            puts("[CPU MODE TWO PARENT CALLER] PASS: common physics, rebound dispatch and parent cadence");
        return ok ? 0 : 4;
    }
    _setmode(_fileno(stdin), _O_BINARY);
    uint16_t in[FIELD_COUNT];
    for (;;) {
        size_t read_size = fread(in, 1, sizeof(in), stdin);
        if (read_size == 0u) break;
        if (read_size != sizeof(in)) {
            nba_assets_free(&assets);
            return 5;
        }
        NbaSession session;
        NbaTipoff tipoff;
        if (!load_case(&tipoff, &session, &assets, in) ||
                !nba_tipoff_replay_normal_actor(&tipoff, (uint8_t)in[0])) {
            nba_assets_free(&assets);
            return 4;
        }
        emit_case(&tipoff, in);
    }
    bool ok = feof(stdin) && !ferror(stdin);
    nba_assets_free(&assets);
    return ok ? 0 : 5;
}
