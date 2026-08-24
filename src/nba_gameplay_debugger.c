#include "nba_gameplay_debugger.h"
#include "nba_font.h"
#include <stdio.h>
#include <string.h>

static void fill(NbaRenderer *renderer, int x, int y, int width, int height,
                 uint32_t color) {
    for (int py = y; py < y + height && py < NBA_SNES_HEIGHT; ++py)
        for (int px = x; px < x + width && px < NBA_SNES_WIDTH; ++px)
            if (px >= 0 && py >= 0)
                renderer->pixels[py * NBA_SNES_WIDTH + px] = color;
}

static void text(NbaRenderer *renderer, int x, int y, const char *value,
                 uint32_t color) {
    nba_font_render_text(renderer->pixels, NBA_SNES_WIDTH, x, y, value,
                         color, 0xFF081018u, 1);
}

void nba_gameplay_debugger_init(NbaGameplayDebugger *debugger) {
    if (!debugger) return;
    memset(debugger, 0, sizeof(*debugger));
}

void nba_gameplay_debugger_toggle(NbaGameplayDebugger *debugger) {
    if (!debugger) return;
    debugger->is_active = !debugger->is_active;
    debugger->step_requested = false;
    printf("[GAMEPLAY LAB] %s (F8), actor=%u page=%u paused=%s\n",
           debugger->is_active ? "opened" : "closed",
           (unsigned)debugger->selected_actor, (unsigned)debugger->page,
           debugger->is_paused ? "yes" : "no");
}

void nba_gameplay_debugger_update(NbaGameplayDebugger *debugger,
                                  const NbaInput *input) {
    if (!debugger || !debugger->is_active || !input) return;
    if (input->pressed & NBA_BTN_UP)
        debugger->selected_actor = (uint8_t)((debugger->selected_actor + 9u) % 10u);
    if (input->pressed & NBA_BTN_DOWN)
        debugger->selected_actor = (uint8_t)((debugger->selected_actor + 1u) % 10u);
    if (input->pressed & NBA_BTN_LEFT)
        debugger->page = (uint8_t)((debugger->page + 2u) % 3u);
    if (input->pressed & NBA_BTN_RIGHT)
        debugger->page = (uint8_t)((debugger->page + 1u) % 3u);
    if (input->pressed & NBA_BTN_A) debugger->is_paused = !debugger->is_paused;
    if (debugger->is_paused && (input->pressed & NBA_BTN_X))
        debugger->step_requested = true;
}

bool nba_gameplay_debugger_should_advance(NbaGameplayDebugger *debugger) {
    if (!debugger || !debugger->is_active || !debugger->is_paused) return true;
    if (!debugger->step_requested) return false;
    debugger->step_requested = false;
    return true;
}

static void mark_actor(NbaRenderer *renderer,
                       const NbaGameplayActorTelemetry *actor, bool selected) {
    if (!actor->visible) return;
    uint32_t color = selected ? 0xFFFFFF40u :
                     actor->control == NBA_GAMEPLAY_CONTROL_PLAYER_1 ?
                     0xFF40FF80u : 0xFF40C0FFu;
    int x = actor->screen_x, y = actor->screen_y;
    for (int d = -3; d <= 3; ++d) {
        int px = x + d, py = y + d;
        if (px >= 0 && px < NBA_SNES_WIDTH && y >= 0 && y < NBA_SNES_HEIGHT)
            renderer->pixels[y * NBA_SNES_WIDTH + px] = color;
        if (x >= 0 && x < NBA_SNES_WIDTH && py >= 0 && py < NBA_SNES_HEIGHT)
            renderer->pixels[py * NBA_SNES_WIDTH + x] = color;
    }
    char label[3];
    snprintf(label, sizeof(label), "%u", (unsigned)actor->index);
    text(renderer, x + 4, y - 6, label, color);
}

void nba_gameplay_debugger_render(const NbaGameplayDebugger *debugger,
                                  const NbaGameplayTelemetry *telemetry,
                                  NbaRenderer *renderer) {
    if (!debugger || !debugger->is_active || !telemetry || !renderer) return;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i)
        mark_actor(renderer, &telemetry->actors[i], i == debugger->selected_actor);

    fill(renderer, 2, 164, 252, 58, 0xE0081018u);
    fill(renderer, 2, 164, 252, 1, 0xFF40C0FFu);
    const NbaGameplayActorTelemetry *actor =
        &telemetry->actors[debugger->selected_actor];
    char line[96];
    snprintf(line, sizeof(line), "GAMEPLAY LAB [F8] %s P%u A%u %s",
             debugger->is_paused ? "PAUSE" : "RUN",
             (unsigned)debugger->page + 1u, (unsigned)actor->index,
             actor->control == NBA_GAMEPLAY_CONTROL_PLAYER_1 ? "P1" : "CPU");
    text(renderer, 6, 168, line, 0xFFFFD760u);
    if (debugger->page == 0u) {
        snprintf(line, sizeof(line), "W:%d,%d,%d S:%d,%d V:%d,%d D:%u U:%02X L:%02X",
                 actor->world_x, actor->world_y, actor->world_z,
                 actor->screen_x, actor->screen_y, actor->velocity_x,
                 actor->velocity_y, actor->direction,
                 actor->animation_state, actor->lower_animation_state);
        text(renderer, 6, 180, line, 0xFFFFFFFFu);
        snprintf(line, sizeof(line), "BALL:%d,%d,%d V:%d,%d,%d O:%d M:%02X",
                 telemetry->ball.world_x, telemetry->ball.world_y,
                 telemetry->ball.world_z, telemetry->ball.velocity_x,
                 telemetry->ball.velocity_y, telemetry->ball.velocity_z,
                 telemetry->ball.owner_actor,
                 telemetry->ball.state);
        text(renderer, 6, 192, line, 0xFF9EF7A9u);
    } else if (debugger->page == 1u) {
        snprintf(line, sizeof(line), "AI:%02X MODE:%02X TGT:%d D:%u V:%d,%d",
                 actor->ai_state, actor->control_mode_raw,
                 actor->ai_target_actor, actor->assignment_direction_raw,
                 actor->velocity_x, actor->velocity_y);
        text(renderer, 6, 180, line, 0xFFFFFFFFu);
        if (actor->control_mode_raw == 15u)
            snprintf(line, sizeof(line),
                     "PASS B:%u D:%u F:%d PH:%u/%u REL:%u",
                     actor->pass_band_62_raw, actor->pass_direction_66_raw,
                     actor->pass_family_c0_raw, actor->upper_phase_raw,
                     actor->pass_release_threshold_raw,
                     actor->pass_released_raw);
        else
            snprintf(line, sizeof(line), "H:%u POS:%d ASG:%u DST:%u R:%u B:%u",
                     telemetry->controlled_actor, telemetry->possession_actor,
                     actor->assignment_current_raw, actor->assignment_distance_raw,
                     actor->reaction_threshold_raw, actor->movement_boost_raw);
        text(renderer, 6, 192, line, 0xFF9EF7A9u);
    } else {
        snprintf(line, sizeof(line), "CAM:%d,%d S:%d G:%u R:$%06X",
                 telemetry->camera_x, telemetry->camera_y,
                 telemetry->camera_subject_raw,
                 telemetry->camera_side_group_raw,
                 telemetry->camera_routine);
        text(renderer, 6, 180, line, 0xFFFFFFFFu);
        snprintf(line, sizeof(line), "SCORE:%u-%u SC:%u VAL:%u C:%u M:%u/%u",
                 telemetry->score_left_raw, telemetry->score_right_raw,
                 telemetry->shot_clock_raw_092c,
                 telemetry->shot_value_raw, telemetry->shot_chance_raw,
                 telemetry->shot_inner_veto_raw, telemetry->shot_miss_index_raw);
        text(renderer, 6, 192, line, 0xFF9EF7A9u);
        snprintf(line, sizeof(line), "PLAY:%02X/%d T:%d W:%u R:%u A:%02X F:%u",
                 telemetry->play_code_raw, telemetry->play_step_raw,
                 telemetry->play_countdown_raw,
                 telemetry->play_event_wait_raw, telemetry->play_request_raw,
                 telemetry->special_actor_raw & 0xFFu,
                 telemetry->scene_frame);
        text(renderer, 6, 204, line, 0xFF79D7FFu);
    }
    text(renderer, 6, 216, "UP/DN ACTOR LT/RT PAGE A PAUSE X STEP",
         0xFF79D7FFu);
}

void nba_gameplay_telemetry_write_jsonl(FILE *stream,
                                        const NbaGameplayTelemetry *telemetry) {
    if (!stream || !telemetry) return;
    fprintf(stream,
            "{\"frame\":%u,\"scene_frame\":%u,\"simulation_tick\":%u,"
            "\"phase\":%u,\"scheduler\":{\"due_raw\":%u,"
            "\"actor_pass_dt_raw\":%u,\"actor_pass_mask_raw\":%u,"
            "\"actor_pass_order_raw\":[",
            telemetry->global_frame, telemetry->scene_frame,
            telemetry->simulation_tick, telemetry->phase,
            telemetry->scheduler_due_raw, telemetry->actor_pass_dt_raw,
            telemetry->actor_pass_mask_raw);
    unsigned scheduled_actors = telemetry->scheduler_due_raw ?
        NBA_GAMEPLAY_ACTOR_COUNT : 0u;
    for (unsigned actor = 0; actor < scheduled_actors; ++actor)
        fprintf(stream, "%s%u", actor ? "," : "",
                telemetry->actor_pass_order_raw[actor]);
    fprintf(stream,
            "]},\"input\":{\"pressed\":%u,\"held\":%u,\"released\":%u},"
            "\"controllers\":{\"active_raw\":%d,\"selected_raw\":%d,"
            "\"held_raw\":[",
            telemetry->input_pressed & 0xFFFu, telemetry->input_held & 0xFFFu,
            telemetry->input_released & 0xFFFu,
            telemetry->active_controller_raw, telemetry->selected_controller_raw);
    for (unsigned i = 0; i < NBA_GAMEPLAY_PAD_COUNT; ++i)
        fprintf(stream, "%s%u", i ? "," : "", telemetry->pad_held_raw[i]);
    fputs("],\"assignment_raw\":[", stream);
    for (unsigned i = 0; i < NBA_GAMEPLAY_PAD_COUNT; ++i)
        fprintf(stream, "%s%u", i ? "," : "",
                telemetry->controller_assignment_raw[i]);
    fputs("],\"repeat_raw\":[", stream);
    for (unsigned i = 0; i < NBA_GAMEPLAY_PAD_COUNT; ++i)
        fprintf(stream, "%s%u", i ? "," : "",
                telemetry->controller_repeat_raw[i]);
    fprintf(stream,
            "]},\"control\":{\"actor\":%u,\"side_raw\":%d,"
            "\"initial_slot_raw\":%d,\"selected_slot_raw\":%d,"
            "\"actor_pointer_raw\":%u},\"possession\":{\"actor\":%d,"
            "\"team\":%d,\"candidate_raw\":%d,\"play_code_raw\":%u,"
            "\"play_step_raw\":%d,\"play_countdown_raw\":%d,"
            "\"play_mirror_raw\":%u,\"play_event_wait_raw\":%u,"
            "\"play_request_raw\":%u,\"play_cycle_raw\":%u,"
            "\"play_hold_raw\":%u,\"special_actor_raw\":%u,"
            "\"role_rebuild_raw_09d6\":%u,"
            "\"pass_actor_raw\":%d,\"pass_receiver_raw\":%d,"
            "\"pass_active_raw\":%u,\"pass_distance_raw\":%u,"
            "\"play_selector_raw\":[%d,%d,%d],"
            "\"rng_state_raw\":%u},\"match\":{\"score_left_raw\":%u,"
            "\"score_right_raw\":%u,\"period_raw_0926\":%u,"
            "\"team_context_mode_raw_30\":[%u,%u],"
            "\"team_context_flags_raw_32\":[%u,%u],"
            "\"team_context_activity_raw_39\":[%u,%u],"
            "\"shot_clock_raw_092c\":%u,"
            "\"shot_value_raw\":%u,"
            "\"shot_chance_raw\":%u,\"shot_miss_index_raw\":%u,"
            "\"shot_inner_veto_raw\":%u,"
            "\"live_state_raw\":%u,\"inbound_state_raw\":%u,"
            "\"inbound_actor_raw\":%u,\"inbound_timer_raw\":%u,"
            "\"inbound_layout_raw\":%d,\"inbound_target_x_raw\":%d,"
            "\"inbound_target_y_raw\":%d,\"inbound_direction_raw\":%u,"
            "\"inbound_ready_raw\":%u,\"inbound_transfer_raw\":%u,"
            "\"rim_context_raw_097c\":%u,"
            "\"rim_contact_count_raw_0920\":%u,"
            "\"rim_response_raw_0970\":%u,"
            "\"effect_gate_raw_3f33\":%u,"
            "\"effect_resource_raw_4015\":%u,"
            "\"rim_effect_raw_401b\":%u,"
            "\"effect_frame_raw_4025\":%u,"
            "\"effect_timer_raw_402d\":%u,"
            "\"rim_impact_raw_13e5\":%u,"
            "\"event_bits_raw_13e7\":%u},"
            "\"fouls\":{\"event_raw\":%u,\"shooting_raw\":%u,"
            "\"offender_raw\":%d,\"victim_raw\":%d,"
            "\"team_raw\":[%u,%u],\"personal_raw\":["
            "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u],"
            "\"free_throw_state_raw\":%u,"
            "\"free_throw_sequence_raw\":%u},"
            "\"camera\":{\"x\":%d,\"y\":%d,"
            "\"subject_raw\":%d,\"side_group_raw\":%u,"
            "\"routine\":%u,\"raw_085c\":%u,\"raw_085e\":%u,"
            "\"raw_0860\":%u,\"raw_0862\":%u,\"raw_086c\":%u,"
            "\"raw_086e\":%u,\"raw_0874\":%u,\"raw_0876\":%u,"
            "\"raw_0878\":%u,\"raw_087a\":%u},"
            "\"collision\":{\"a\":%d,\"b\":%d,\"routine\":%u},"
            "\"ball\":{\"x\":%d,\"y\":%d,\"z\":%d,"
            "\"screen_x\":%d,\"screen_y\":%d,\"vx\":%d,\"vy\":%d,"
            "\"vz\":%d,\"owner\":%d,\"state\":%u,\"flags_raw\":%u,"
            "\"activity_raw\":%u,"
            "\"routine\":%u},"
            "\"routines\":{\"controller\":%u,\"selection\":%u,"
            "\"possession\":%u},\"actors\":[",
            telemetry->controlled_actor, telemetry->controlled_side_raw,
            telemetry->initial_controlled_slot_raw, telemetry->selected_slot_raw,
            telemetry->controlled_actor_pointer_raw, telemetry->possession_actor,
            telemetry->possession_team, telemetry->possession_candidate_raw,
            telemetry->play_code_raw, telemetry->play_step_raw,
            telemetry->play_countdown_raw, telemetry->play_mirror_raw,
            telemetry->play_event_wait_raw, telemetry->play_request_raw,
            telemetry->play_cycle_raw,
            telemetry->play_hold_raw, telemetry->special_actor_raw,
            telemetry->role_rebuild_raw_09d6,
            telemetry->pass_actor_raw, telemetry->pass_receiver_raw,
            telemetry->pass_active_raw, telemetry->pass_distance_raw,
            telemetry->play_selector_raw[0],
            telemetry->play_selector_raw[1], telemetry->play_selector_raw[2],
            telemetry->rng_state_raw,
            telemetry->score_left_raw, telemetry->score_right_raw,
            telemetry->period_raw_0926,
            telemetry->team_context_mode_raw_30[0],
            telemetry->team_context_mode_raw_30[1],
            telemetry->team_context_flags_raw_32[0],
            telemetry->team_context_flags_raw_32[1],
            telemetry->team_context_activity_raw_39[0],
            telemetry->team_context_activity_raw_39[1],
            telemetry->shot_clock_raw_092c,
            telemetry->shot_value_raw, telemetry->shot_chance_raw,
            telemetry->shot_miss_index_raw, telemetry->shot_inner_veto_raw,
            telemetry->live_state_raw,
            telemetry->inbound_state_raw, telemetry->inbound_actor_raw,
            telemetry->inbound_timer_raw, telemetry->inbound_layout_raw,
            telemetry->inbound_target_x_raw,
            telemetry->inbound_target_y_raw,
            telemetry->inbound_direction_raw, telemetry->inbound_ready_raw,
            telemetry->inbound_transfer_raw,
            telemetry->rim_context_raw_097c,
            telemetry->rim_contact_count_raw_0920,
            telemetry->rim_response_raw_0970,
            telemetry->effect_gate_raw_3f33,
            telemetry->effect_resource_raw_4015,
            telemetry->rim_effect_raw_401b,
            telemetry->effect_frame_raw_4025,
            telemetry->effect_timer_raw_402d,
            telemetry->rim_impact_raw_13e5,
            telemetry->event_bits_raw_13e7,
            telemetry->foul_event_raw, telemetry->shooting_foul_raw,
            telemetry->foul_offender_raw, telemetry->foul_victim_raw,
            telemetry->team_fouls_raw[0], telemetry->team_fouls_raw[1],
            telemetry->personal_fouls_raw[0],
            telemetry->personal_fouls_raw[1],
            telemetry->personal_fouls_raw[2],
            telemetry->personal_fouls_raw[3],
            telemetry->personal_fouls_raw[4],
            telemetry->personal_fouls_raw[5],
            telemetry->personal_fouls_raw[6],
            telemetry->personal_fouls_raw[7],
            telemetry->personal_fouls_raw[8],
            telemetry->personal_fouls_raw[9],
            telemetry->free_throw_state_raw,
            telemetry->free_throw_sequence_raw,
            telemetry->camera_x, telemetry->camera_y,
            telemetry->camera_subject_raw, telemetry->camera_side_group_raw,
            telemetry->camera_routine,
            telemetry->camera_085c_raw, telemetry->camera_085e_raw,
            telemetry->camera_0860_raw, telemetry->camera_0862_raw,
            telemetry->camera_086c_raw, telemetry->camera_086e_raw,
            telemetry->camera_0874_raw, telemetry->camera_0876_raw,
            telemetry->camera_0878_raw, telemetry->camera_087a_raw,
            telemetry->collision_actor_a, telemetry->collision_actor_b,
            telemetry->collision_routine, telemetry->ball.world_x,
            telemetry->ball.world_y, telemetry->ball.world_z,
            telemetry->ball.screen_x, telemetry->ball.screen_y,
            telemetry->ball.velocity_x, telemetry->ball.velocity_y,
            telemetry->ball.velocity_z, telemetry->ball.owner_actor,
            telemetry->ball.state, telemetry->ball.flags_raw,
            telemetry->ball_activity_raw,
            telemetry->ball.routine,
            telemetry->controller_routine, telemetry->selection_routine,
            telemetry->possession_routine);
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        const NbaGameplayActorTelemetry *a = &telemetry->actors[i];
        fprintf(stream,
                "%s{\"id\":%u,\"team\":%u,\"roster\":%u,\"control\":%u,"
                "\"visible\":%s,\"x\":%d,\"y\":%d,\"z\":%d,"
                "\"screen_x\":%d,\"screen_y\":%d,\"vx\":%d,\"vy\":%d,"
                "\"vz\":%d,\"direction\":%u,\"animation\":%u,"
                "\"lower_animation\":%u,"
                "\"ai_state\":%u,\"ai_target\":%u,\"actor_routine\":%u,"
                "\"ai_routine\":%u,\"raw\":{\"base\":%u,\"id\":%u,"
                "\"action\":%u,\"flags\":%u,\"upper_resource\":%u,"
                "\"lower_resource\":%u,\"head_resource\":%u,"
                "\"motion_38\":%u,\"motion_3a\":%u,\"motion_3c\":%u,"
                "\"direction_4e\":%u,\"direction_50\":%u,"
                "\"direction_52\":%u,\"target_x_56\":%d,"
                "\"target_y_58\":%d,\"control_mode\":%u,"
                "\"mode_saved_62\":%u,\"pass_band_62\":%u,"
                "\"pass_direction_66\":%u,"
                "\"control_mode_saved\":%u,\"saved_mode_84\":%u,"
                "\"pass_family_c0\":%d,\"pass_release_threshold\":%u,"
                "\"pass_released\":%u,\"side_group\":%u,"
                "\"assignment_base\":%u,\"assignment_current\":%u,"
                "\"assignment_alternate\":%u,\"assignment_distance\":%u,"
                "\"assignment_direction\":%u,\"anchor_direction_88\":%u,"
                "\"assignment_role_92\":%u,\"pair_distance\":%u,"
                "\"anchor_distance_8c\":%u,"
                "\"reaction_threshold\":%u,\"movement_boost_72\":%u,"
                "\"controller_assignment_16\":%d,"
                "\"movement_magnitude_4c\":%u,"
                "\"recovery_inhibit_7a\":%u,"
                "\"upper_restart\":%u,"
                "\"lower_restart\":%u,\"upper_phase\":%u,"
                "\"lower_phase\":%u,\"behavior_flags\":%u,"
                "\"palette\":%u}}",
                i ? "," : "", a->index, a->team_side, a->roster_slot,
                a->control, a->visible ? "true" : "false", a->world_x,
                a->world_y, a->world_z, a->screen_x, a->screen_y,
                a->velocity_x, a->velocity_y, a->velocity_z, a->direction,
                a->animation_state, a->lower_animation_state, a->ai_state,
                a->ai_target_actor, a->actor_routine, a->ai_routine,
                a->actor_base, a->id_raw, a->action_raw, a->flags_raw,
                a->upper_resource_raw, a->lower_resource_raw,
                a->head_resource_raw, a->motion_38_raw, a->motion_3a_raw,
                a->motion_3c_raw, a->direction_4e_raw, a->direction_50_raw,
                a->direction_52_raw, a->target_x_56_raw, a->target_y_58_raw,
                a->control_mode_raw, a->mode_saved_62_raw,
                a->pass_band_62_raw, a->pass_direction_66_raw,
                a->control_mode_saved_raw, a->saved_mode_84_raw,
                a->pass_family_c0_raw, a->pass_release_threshold_raw,
                a->pass_released_raw, a->side_group_raw,
                a->assignment_base_raw, a->assignment_current_raw,
                a->assignment_alternate_raw, a->assignment_distance_raw,
                a->assignment_direction_raw, a->anchor_direction_raw_88,
                a->assignment_role_raw_92, a->pair_distance_raw,
                a->anchor_distance_raw_8c,
                a->reaction_threshold_raw, a->movement_boost_raw,
                a->controller_assignment_16_raw,
                a->movement_magnitude_4c_raw, a->recovery_inhibit_7a_raw,
                a->upper_restart_raw,
                a->lower_restart_raw, a->upper_phase_raw, a->lower_phase_raw,
                a->behavior_flags_raw, a->palette_raw);
    }
    fputs("]}\n", stream);
}
