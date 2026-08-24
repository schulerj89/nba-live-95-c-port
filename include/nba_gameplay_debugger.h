#ifndef NBA_GAMEPLAY_DEBUGGER_H
#define NBA_GAMEPLAY_DEBUGGER_H

#include "nba_renderer.h"
#include "nba_types.h"
#include <stdio.h>

#define NBA_GAMEPLAY_ACTOR_COUNT 10
#define NBA_GAMEPLAY_NO_ACTOR    0xFFu
#define NBA_GAMEPLAY_PAD_COUNT    5
#define NBA_GAMEPLAY_UNKNOWN_WORD 0xFFFFu

typedef enum {
    NBA_GAMEPLAY_CONTROL_CPU = 0,
    NBA_GAMEPLAY_CONTROL_PLAYER_1,
    NBA_GAMEPLAY_CONTROL_PLAYER_2
} NbaGameplayControl;

typedef struct {
    uint8_t index;
    uint8_t team_side;
    uint8_t roster_slot;
    uint8_t control;
    bool visible;
    int16_t world_x, world_y, world_z;
    int16_t screen_x, screen_y;
    int16_t velocity_x, velocity_y, velocity_z;
    uint8_t direction;
    uint8_t animation_state;
    uint8_t lower_animation_state;
    uint8_t ai_state;
    uint8_t ai_target_actor;
    uint16_t actor_base;
    uint16_t id_raw;
    uint16_t action_raw;
    uint16_t flags_raw;
    uint16_t upper_resource_raw;
    uint16_t lower_resource_raw;
    uint16_t head_resource_raw;
    uint16_t motion_38_raw;
    uint16_t motion_3a_raw;
    uint16_t motion_3c_raw;
    uint16_t direction_4e_raw;
    uint16_t direction_50_raw;
    uint16_t direction_52_raw;
    uint16_t control_mode_raw;
    uint16_t control_mode_saved_raw;
    uint16_t side_group_raw;
    uint16_t assignment_base_raw;
    uint16_t assignment_current_raw;
    uint16_t assignment_alternate_raw;
    uint16_t assignment_distance_raw;
    uint16_t assignment_direction_raw;
    uint16_t pair_distance_raw;
    uint16_t reaction_threshold_raw;
    uint16_t upper_restart_raw;
    uint16_t lower_restart_raw;
    uint16_t upper_phase_raw;
    uint16_t lower_phase_raw;
    uint16_t behavior_flags_raw;
    uint16_t palette_raw;
    uint32_t actor_routine;
    uint32_t ai_routine;
} NbaGameplayActorTelemetry;

typedef struct {
    int16_t world_x, world_y, world_z;
    int16_t screen_x, screen_y;
    int16_t velocity_x, velocity_y, velocity_z;
    int8_t owner_actor;
    uint8_t state;
    uint16_t flags_raw;
    uint32_t routine;
} NbaGameplayBallTelemetry;

typedef struct {
    uint32_t global_frame;
    uint32_t scene_frame;
    uint32_t input_pressed;
    uint32_t input_held;
    uint32_t input_released;
    uint32_t simulation_tick;
    uint16_t pad_held_raw[NBA_GAMEPLAY_PAD_COUNT];
    uint16_t controller_assignment_raw[NBA_GAMEPLAY_PAD_COUNT];
    uint16_t controller_repeat_raw[NBA_GAMEPLAY_PAD_COUNT];
    int16_t active_controller_raw;
    int16_t selected_controller_raw;
    uint8_t phase;
    uint8_t controlled_actor;
    int16_t controlled_side_raw;
    int16_t initial_controlled_slot_raw;
    int16_t selected_slot_raw;
    uint16_t controlled_actor_pointer_raw;
    int8_t possession_actor;
    int8_t possession_team;
    int16_t possession_candidate_raw;
    uint16_t play_code_raw;
    uint16_t rng_state_raw;
    uint16_t score_left_raw;
    uint16_t score_right_raw;
    uint16_t shot_value_raw;
    uint16_t live_state_raw;
    uint16_t inbound_state_raw;
    uint16_t inbound_actor_raw;
    uint16_t inbound_timer_raw;
    int8_t collision_actor_a;
    int8_t collision_actor_b;
    int16_t camera_x, camera_y;
    uint16_t camera_085c_raw;
    uint16_t camera_085e_raw;
    uint16_t camera_0860_raw;
    uint16_t camera_0862_raw;
    uint16_t camera_086c_raw;
    uint16_t camera_086e_raw;
    uint16_t camera_0874_raw;
    uint16_t camera_0876_raw;
    uint16_t camera_0878_raw;
    uint16_t camera_087a_raw;
    uint32_t controller_routine;
    uint32_t selection_routine;
    uint32_t camera_routine;
    uint32_t collision_routine;
    uint32_t possession_routine;
    NbaGameplayBallTelemetry ball;
    NbaGameplayActorTelemetry actors[NBA_GAMEPLAY_ACTOR_COUNT];
} NbaGameplayTelemetry;

typedef struct {
    bool is_active;
    bool is_paused;
    bool step_requested;
    uint8_t selected_actor;
    uint8_t page;
} NbaGameplayDebugger;

void nba_gameplay_debugger_init(NbaGameplayDebugger *debugger);
void nba_gameplay_debugger_toggle(NbaGameplayDebugger *debugger);
void nba_gameplay_debugger_update(NbaGameplayDebugger *debugger,
                                  const NbaInput *input);
bool nba_gameplay_debugger_should_advance(NbaGameplayDebugger *debugger);
void nba_gameplay_debugger_render(const NbaGameplayDebugger *debugger,
                                  const NbaGameplayTelemetry *telemetry,
                                  NbaRenderer *renderer);
void nba_gameplay_telemetry_write_jsonl(FILE *stream,
                                        const NbaGameplayTelemetry *telemetry);

#endif
