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
        snprintf(line, sizeof(line), "W:%d,%d,%d S:%d,%d V:%d,%d,%d D:%u AN:%02X",
                 actor->world_x, actor->world_y, actor->world_z,
                 actor->screen_x, actor->screen_y, actor->velocity_x,
                 actor->velocity_y, actor->velocity_z, actor->direction,
                 actor->animation_state);
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
        snprintf(line, sizeof(line), "H:%u POS:%d TEAM:%d ASG:%u REACT:%u",
                 telemetry->controlled_actor, telemetry->possession_actor,
                 telemetry->possession_team, actor->assignment_current_raw,
                 actor->reaction_threshold_raw);
        text(renderer, 6, 192, line, 0xFF9EF7A9u);
    } else {
        snprintf(line, sizeof(line), "CAM:%d,%d R:$%06X COLL:$%06X POS:$%06X",
                 telemetry->camera_x, telemetry->camera_y,
                 telemetry->camera_routine, telemetry->collision_routine,
                 telemetry->possession_routine);
        text(renderer, 6, 180, line, 0xFFFFFFFFu);
        snprintf(line, sizeof(line), "IN P:%03X H:%03X R:%03X F:%u",
                 telemetry->input_pressed & 0xFFFu,
                 telemetry->input_held & 0xFFFu,
                 telemetry->input_released & 0xFFFu, telemetry->scene_frame);
        text(renderer, 6, 192, line, 0xFF9EF7A9u);
    }
    text(renderer, 6, 207, "UP/DN ACTOR  LT/RT PAGE  A PAUSE  X STEP",
         0xFF79D7FFu);
}

void nba_gameplay_telemetry_write_jsonl(FILE *stream,
                                        const NbaGameplayTelemetry *telemetry) {
    if (!stream || !telemetry) return;
    fprintf(stream,
            "{\"frame\":%u,\"scene_frame\":%u,\"simulation_tick\":%u,"
            "\"phase\":%u,"
            "\"input\":{\"pressed\":%u,\"held\":%u,\"released\":%u},"
            "\"controllers\":{\"active_raw\":%d,\"selected_raw\":%d,"
            "\"held_raw\":[",
            telemetry->global_frame, telemetry->scene_frame,
            telemetry->simulation_tick, telemetry->phase,
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
            "\"rng_state_raw\":%u},\"camera\":{\"x\":%d,\"y\":%d,"
            "\"routine\":%u,\"raw_085c\":%u,\"raw_085e\":%u,"
            "\"raw_0860\":%u,\"raw_0862\":%u,\"raw_086c\":%u,"
            "\"raw_086e\":%u,\"raw_0874\":%u,\"raw_0876\":%u,"
            "\"raw_0878\":%u,\"raw_087a\":%u},"
            "\"collision\":{\"a\":%d,\"b\":%d,\"routine\":%u},"
            "\"ball\":{\"x\":%d,\"y\":%d,\"z\":%d,"
            "\"screen_x\":%d,\"screen_y\":%d,\"vx\":%d,\"vy\":%d,"
            "\"vz\":%d,\"owner\":%d,\"state\":%u,\"flags_raw\":%u,"
            "\"routine\":%u},"
            "\"routines\":{\"controller\":%u,\"selection\":%u,"
            "\"possession\":%u},\"actors\":[",
            telemetry->controlled_actor, telemetry->controlled_side_raw,
            telemetry->initial_controlled_slot_raw, telemetry->selected_slot_raw,
            telemetry->controlled_actor_pointer_raw, telemetry->possession_actor,
            telemetry->possession_team, telemetry->possession_candidate_raw,
            telemetry->play_code_raw, telemetry->rng_state_raw,
            telemetry->camera_x, telemetry->camera_y, telemetry->camera_routine,
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
                "\"direction_52\":%u,\"control_mode\":%u,"
                "\"control_mode_saved\":%u,\"side_group\":%u,"
                "\"assignment_base\":%u,\"assignment_current\":%u,"
                "\"assignment_alternate\":%u,\"assignment_distance\":%u,"
                "\"assignment_direction\":%u,\"pair_distance\":%u,"
                "\"reaction_threshold\":%u,\"upper_restart\":%u,"
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
                a->direction_52_raw, a->control_mode_raw,
                a->control_mode_saved_raw, a->side_group_raw,
                a->assignment_base_raw, a->assignment_current_raw,
                a->assignment_alternate_raw, a->assignment_distance_raw,
                a->assignment_direction_raw, a->pair_distance_raw,
                a->reaction_threshold_raw, a->upper_restart_raw,
                a->lower_restart_raw, a->upper_phase_raw, a->lower_phase_raw,
                a->behavior_flags_raw, a->palette_raw);
    }
    fputs("]}\n", stream);
}
