#ifndef NBA_GAMEPLAY_DEBUGGER_H
#define NBA_GAMEPLAY_DEBUGGER_H

#include "nba_renderer.h"
#include "nba_types.h"
#include "nba_shot_state.h"
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
    int32_t world_x_fp, world_y_fp; /* exact 24.8 coordinates for ROM integer-word gates */
    int32_t world_z_fp; /* exact 24.8 height; rounded z cannot prove landing */
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
    uint16_t draw_upper_resource_raw;
    uint16_t draw_lower_resource_raw;
    uint16_t head_resource_raw;
    uint16_t appearance_resource_raw[4]; /* lower, upper, head, number */
    uint32_t appearance_opaque_pixels[4];
    uint8_t appearance_flags_raw; /* palette, number gate/tile/palette, valid */
    uint16_t motion_38_raw;
    uint16_t motion_3a_raw;
    uint16_t motion_3c_raw;
    uint16_t direction_4e_raw;
    uint16_t direction_50_raw;
    uint16_t direction_52_raw;
    uint16_t draw_direction_raw;
    int16_t target_x_56_raw;
    int16_t target_y_58_raw;
    uint16_t control_mode_raw;
    uint16_t mode_saved_62_raw;
    uint16_t pass_band_62_raw;
    uint16_t pass_direction_66_raw;
    uint16_t control_mode_saved_raw;
    uint16_t saved_mode_84_raw;
    int16_t pass_family_c0_raw;
    uint8_t pass_release_threshold_raw;
    uint8_t pass_released_raw;
    uint16_t side_group_raw;
    uint16_t assignment_base_raw;
    uint16_t assignment_current_raw;
    uint16_t assignment_alternate_raw;
    uint16_t assignment_distance_raw;
    uint16_t assignment_direction_raw;
    uint16_t anchor_direction_raw_88;
    uint16_t assignment_role_raw_92;
    uint16_t pair_distance_raw;
    uint16_t anchor_distance_raw_8c;
    uint16_t reaction_threshold_raw;
    uint16_t movement_boost_raw;
    int16_t controller_assignment_16_raw;
    uint16_t movement_magnitude_4c_raw;
    uint16_t mode13_timer_60_raw;
    int16_t mode13_selector_56_raw;
    uint16_t mode13_variant_58_raw;
    int16_t mode13_baseline_vx_ba_raw;
    int16_t mode13_baseline_vy_bc_raw;
    uint16_t contact_inhibit_5a_raw;
    uint16_t contact_height_aa_raw;
    uint16_t recovery_inhibit_7a_raw;
    uint16_t upper_restart_raw;
    uint16_t lower_restart_raw;
    uint16_t upper_phase_raw;
    uint16_t lower_phase_raw;
    uint16_t animation_rom_words[10]; /* resources, phases, accumulators, locks, cursors */
    uint16_t animation_phase_target_raw_b0;
    bool animation_resources_valid;
    bool animation_action_integrated;
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
    int16_t tip_contact_actor;
    uint32_t tip_contact_frame;
    uint32_t tip_possession_frame;
    uint16_t tip_reach_mask;
    uint16_t tip_toss_countdown_raw, jump_scratch_raw;
    uint32_t jump_calls, jump_launches, jump_rejections;
    uint16_t ball_velocity_raw[3];
    uint8_t scheduler_due_raw;
    uint8_t actor_pass_dt_raw;
    uint16_t actor_pass_mask_raw;
    uint8_t actor_pass_order_raw[NBA_GAMEPLAY_ACTOR_COUNT];
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
    int16_t play_step_raw;
    int16_t play_countdown_raw;
    uint16_t play_mirror_raw;
    uint16_t play_event_wait_raw;
    uint16_t play_request_raw;
    uint32_t play_consumed_serial;
    uint16_t play_cycle_raw;
    uint16_t play_hold_raw;
    uint16_t role_rebuild_raw_09d6;
    uint16_t special_actor_raw;
    int16_t play_selector_raw[3];
    uint16_t rng_state_raw;
    uint16_t score_left_raw;
    uint16_t score_right_raw;
    uint16_t period_raw_0926;
    uint16_t match_clock_raw_0928;
    uint16_t team_context_mode_raw_30[2];
    uint16_t team_context_flags_raw_32[2];
    uint16_t team_context_activity_raw_39[2];
    uint16_t team_context_dead_ball_actor_raw_3f[2];
    uint16_t shot_clock_raw_092c;
    uint16_t shot_clock_mirror_raw_09c6;
    uint16_t shot_value_raw;
    int16_t shot_actor_raw_09c8;
    uint16_t interference_value_raw_096a;
    uint8_t shot_chance_raw;
    uint8_t shot_miss_index_raw;
    uint8_t shot_inner_veto_raw;
    uint32_t shot_selection_serial;
    uint32_t shot_launch_serial;
    uint16_t shot_launch_actor,shot_launch_value;
    uint16_t shot_selection_inputs[8];
    NbaShotFatigue shot_fatigue;
    uint16_t shot_made_run[10],shot_defensive_run[10],shot_assistance_team;
    uint16_t live_state_raw;
    uint16_t inbound_state_raw;
    uint16_t inbound_actor_raw;
    uint16_t inbound_timer_raw;
    int16_t inbound_layout_raw;
    int16_t inbound_target_x_raw;
    int16_t inbound_target_y_raw;
    uint16_t inbound_direction_raw;
    uint16_t inbound_ready_raw;
    uint16_t inbound_transfer_raw;
    uint16_t dead_ball_raw_0966;
    uint16_t dead_ball_raw_0968;
    uint16_t dead_ball_raw_096c;
    int16_t dead_ball_x_raw_09b0;
    int16_t dead_ball_y_raw_09b2;
    uint16_t rim_context_raw_097c;
    uint16_t rim_contact_count_raw_0920;
    uint16_t rim_response_raw_0970;
    uint16_t effect_gate_raw_3f33;
    uint16_t effect_resource_raw_4015;
    uint16_t rim_effect_raw_401b;
    uint16_t effect_frame_raw_4025;
    uint16_t effect_timer_raw_402d;
    uint16_t rim_impact_raw_13e5;
    uint16_t event_bits_raw_13e7;
    uint16_t foul_event_raw;
    uint16_t shooting_foul_raw;
    int16_t foul_offender_raw;
    int16_t foul_victim_raw;
    uint16_t team_fouls_raw[2];
    uint8_t personal_fouls_raw[NBA_GAMEPLAY_ACTOR_COUNT];
    uint16_t free_throw_state_raw;
    uint16_t free_throw_sequence_raw;
    uint16_t free_throw_start_tick_raw_09be;
    uint16_t free_throw_aim_x_raw_0980;
    uint16_t free_throw_aim_y_raw_0982;
    uint16_t free_throw_flight_timer_raw_0930;
    uint16_t deferred_shot_foul_phase_raw_0a02;
    uint16_t latched_event_raw_08f0;
    uint16_t whistle_active_raw_09b6;
    uint16_t whistle_timer_raw_08de;
    uint16_t presentation_gate_raw_08e2;
    uint16_t whistle_presentation_queued_raw;
    uint16_t ball_activity_raw;
    int16_t pass_actor_raw;
    int16_t pass_receiver_raw;
    uint16_t pass_active_raw;
    uint16_t pass_distance_raw;
    int8_t collision_actor_a;
    int8_t collision_actor_b;
    uint16_t player_contact_count_raw;
    int8_t player_contact_actor_a_raw;
    int8_t player_contact_actor_b_raw;
    uint32_t player_contact_routine_raw;
    int16_t camera_x, camera_y;
    int16_t camera_subject_raw;
    uint16_t camera_side_group_raw;
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
    uint16_t camera_087c_raw, camera_087e_raw, camera_0880_raw, camera_0882_raw;
    uint16_t camera_basket_x_raw, camera_stream_row_bytes;
    uint16_t camera_initialized_raw, camera_pointer_raw, camera_ticks_raw;
    uint16_t camera_alternate_raw, camera_alternate_mode_raw, camera_proxy_raw[4];
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
