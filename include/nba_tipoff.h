#ifndef NBA_TIPOFF_H
#define NBA_TIPOFF_H

#include "nba_assets.h"
#include "nba_renderer.h"
#include "nba_session.h"
#include "nba_gameplay_debugger.h"
#include "nba_gameplay_camera.h"
#include "nba_court_presentation.h"
#include "nba_gameplay_ai.h"
#include "nba_gameplay_ball.h"
#include "nba_gameplay_effect.h"
#include "nba_gameplay_foul.h"
#include "nba_gameplay_free_throw.h"
#include "nba_shot_launch.h"
#include "nba_shot_state.h"
#include "nba_tipoff_flow.h"

/* ROM routines correlated with live Mesen execution. */
#define SNES_ADDR_TIPOFF_PLAYER_FORMATION 0x86DDA7
#define SNES_ADDR_TIPOFF_BALL_INIT        0x86E056
#define SNES_ADDR_TIPOFF_JUMP_ANIMATION   0x86ECF4
#define SNES_ADDR_TIPOFF_CONTACT          0x86CCFC
#define SNES_ADDR_TIPOFF_POSSESSION       0x86D25A

typedef enum {
    NBA_TIPOFF_FORMATION = 0,
    NBA_TIPOFF_JUMP_BALL,
    NBA_TIPOFF_POSSESSION,
    NBA_TIPOFF_LIVE
} NbaTipoffPhase;

typedef enum {
    NBA_CPU_PLAY_BREAK = 0,
    NBA_CPU_PLAY_DRIVE,
    NBA_CPU_PLAY_PASS,
    NBA_CPU_PLAY_ATTACK,
    NBA_CPU_PLAY_SHOT,
    NBA_CPU_PLAY_REBOUND
} NbaCpuPlayState;

typedef enum {
    NBA_BALL_HIDDEN = 0,
    NBA_BALL_TOSS,
    NBA_BALL_LOOSE,
    NBA_BALL_PASS,
    NBA_BALL_ATTACHED,
    NBA_BALL_SHOT,
    NBA_BALL_BOUNCE,
    NBA_BALL_DEAD
} NbaBallMode;

typedef struct {
    int32_t x_fp, y_fp, z_fp;
    int16_t velocity_x, velocity_y, velocity_z;
    int16_t target_x, target_y;
    uint8_t direction;
    uint8_t roster_slot; /* active `$46F9/$4779` lineup entry */
    uint8_t animation_state;
    uint8_t lower_animation_state;
    uint16_t upper_animation_tick;
    uint16_t lower_animation_tick;
    uint16_t upper_animation_phase_raw; /* compatibility action-gate phase */
    uint16_t lower_animation_phase_raw;
    uint16_t rom_upper_animation_phase_raw_3a;
    uint16_t rom_lower_animation_phase_raw_3c;
    uint16_t upper_animation_accumulator_raw_42;
    uint16_t lower_animation_accumulator_raw_44;
    uint16_t upper_animation_resource_raw_2a;
    uint16_t lower_animation_resource_raw_2c;
    bool animation_resources_valid;
    uint8_t base_animation_state_raw_38; /* actor `+$38`, locomotion intent */
    uint16_t upper_animation_lock_raw_46; /* actor `+$46` */
    uint16_t lower_animation_lock_raw_48; /* actor `+$48` */
    uint16_t actor_status_raw_28;       /* actor `+$28`, `$86:C476` */
    uint8_t control_mode;
    uint8_t saved_control_mode;         /* actor `+$84` */
    uint16_t pass_band_raw;             /* actor `+$62`: 0,6,...,30 */
    uint16_t pass_direction_raw;        /* actor `+$66` */
    int16_t special_contact_raw_56;     /* actor `+$56`, `$86:C943-$C951` */
    uint16_t mode13_variant_raw_58;     /* shared close-finish `+$58` word */
    int16_t pass_family_raw;            /* actor `+$C0` */
    uint8_t pass_release_threshold_raw;
    bool pass_released_raw;
    uint8_t requested_direction;
    uint8_t movement_direction;
    uint8_t velocity_direction_raw_a2; /* actor `+$A2` */
    uint16_t planar_edge_raw_a0; /* actor `+$A0`, common rectangle edge/corner */
    uint8_t facing_ease_timer_raw_be;  /* actor `+$BE` */
    uint8_t assignment_actor;
    uint8_t assignment_direction;
    uint8_t anchor_direction_raw;       /* actor `+$88` */
    uint8_t assignment_role_raw_92;     /* actor `+$92` */
    uint16_t assignment_base_raw;
    uint16_t assignment_current_raw;
    uint16_t assignment_alternate_raw;
    uint16_t team_group_raw_6e;          /* actor `+$6E`: context group 0/5 */
    uint16_t assignment_distance;
    uint16_t anchor_distance_raw;       /* actor `+$8C` */
    uint16_t focal_distance_raw_8e;     /* actor `+$8E`, `$85:BC84` */
    uint16_t pair_distance;
    uint16_t reaction_threshold;
    uint16_t movement_boost_timer; /* actor `+$72`, consumed by `$85:A82C` */
    int8_t controller_assignment_raw; /* signed actor `+$16` */
    uint16_t movement_magnitude_raw;  /* actor `+$4C` */
    uint16_t movement_speed_raw_4a;
    uint16_t shot_modifier_raw_b2;
    uint16_t defensive_run_raw_b4;
    uint16_t shot_stamina_raw_18; /* read-only mirror of active roster statistics +$18 */
    uint16_t shot_statistics[5]; /* player stats +$00..+$08 */
    int16_t mode13_baseline_velocity_x; /* modes 13/14 actor `+$BA` */
    int16_t mode13_baseline_velocity_y; /* modes 13/14 actor `+$BC` */
    uint16_t contact_inhibit_raw_5a;  /* actor `+$5A`, `$86:CD03/D460` */
    uint16_t formation_timer_raw_5c;  /* actor `+$5C`, `$85:AE97-$AEBB` */
    uint16_t contact_action_timer_raw_60; /* actor `+$60`, `$86:C0DF/C12F` */
    uint16_t contact_height_raw_aa;   /* actor `+$AA`, `$87:A6A9-A6B2` */
    uint16_t catcher_latch_raw_ae;    /* actor `+$AE`, `$86:BAE0` */
    uint16_t free_throw_launch_half_raw_a8; /* actor `+$A8`, `$86:A2A7` */
    uint16_t animation_variant_raw_6c; /* roster +$08, `$87:AD3D-$AD57` */
    uint16_t upper_phase_target_raw_b0; /* `$87:ADC6-$AE75` held-ball phase target */
    uint16_t animation_upper_queue_cursor_raw_18;
    uint16_t animation_lower_queue_cursor_raw_1a;
    uint16_t animation_upper_queue_raw_1c[3];
    uint16_t animation_lower_queue_raw_22[3];
    bool exact_pass_animation; /* ordinary live-play adoption, not inbound */
    bool exact_shot_animation; /* ordinary mode-12 startup/wind-up */
    bool exact_jump_animation; /* EC32/EAA8 channels share contact/render pose */
    uint16_t recovery_inhibit_raw;    /* actor `+$7A` */
    uint16_t behavior_flags_raw;      /* actor `+$7E` */
    uint16_t help_request_raw_80;     /* actor `+$80`, `$85:C006` */
    uint16_t behavior_timer;          /* actor `+$64` */
    uint16_t action_state;
    bool visible;
} NbaTipoffActor;

typedef struct {
    int32_t x_fp, y_fp, z_fp;
    int16_t velocity_x, velocity_y, velocity_z;
    int8_t owner_actor;
    uint8_t state;
} NbaTipoffBall;

/* Exact `$87:8F13-$8F61` facing-ease state transition. */
void nba_tipoff_ease_display_direction(uint8_t desired,
                                       uint16_t upper_animation_lock,
                                       uint8_t *shown, uint8_t *timer);

typedef struct NbaTipoff {
    const NbaAssetPack *assets;
    NbaSession *session;
    /* Optional read-only test observer. NULL in the normal game. These are
     * actual sweep boundaries, not cadence-derived telemetry predictions. */
    void (*differential_observer)(const struct NbaTipoff *, const char *, void *);
    void *differential_context;
    int frame;
    NbaTipoffPhase phase;
    NbaTipoffActor actors[NBA_GAMEPLAY_ACTOR_COUNT];
    NbaTipoffBall ball;
    NbaTipBallInitialization ball_initialization; /* immutable startup bookkeeping */
    uint16_t context_raw_4933, context_raw_4935;
    int16_t camera_x, camera_y;
    NbaGameplayCamera camera;
    /* `$87:A357-$A47A` prepares object origins only on the scheduled OAM
     * submission pass. The SNES keeps those coordinates between passes. */
    int16_t player_screen_x[NBA_GAMEPLAY_ACTOR_COUNT];
    int16_t player_screen_y[NBA_GAMEPLAY_ACTOR_COUNT];
    int16_t ball_screen_x, ball_screen_y;
    bool player_screen_visible[NBA_GAMEPLAY_ACTOR_COUNT];
    NbaCourtPlayerIndicator player_indicator[NBA_GAMEPLAY_ACTOR_COUNT];
    NbaCourtPresentation court_presentation;
    NbaCourtStream court_stream;
    NbaGameplayRng rng;
    NbaGameplayFoulState fouls;
    uint16_t team_pose_contact_count_raw[2]; /* team context +$50 */
    uint16_t defensive_pose_count_raw_1868; /* `$86:E39A` successful selects */
    uint16_t deferred_shot_foul_phase_raw_0a02;
    uint16_t free_throw_start_tick_raw_09be;
    uint16_t free_throw_aim_x_raw_0980;
    uint16_t free_throw_aim_y_raw_0982;
    uint16_t free_throw_aim_accumulator_raw_0984;
    uint16_t free_throw_aim_step_raw_0986;
    uint16_t free_throw_resolution_raw_0972;
    uint16_t free_throw_flight_timer_raw_0930;
    uint16_t free_throw_clock_mirror_raw_493f;
    uint16_t free_throw_upload_raw_180b;
    uint16_t free_throw_upload_raw_180c;
    /* Native `$46EB/$476B`: context0 home/right, context1 visitor/left.
     * Published team IDs drive actor ratings, appearance and uniforms. */
    NbaGameplayTeamContext team_context[2];
    /* `$85:B83E-$B85D` scans the five $47EB+$40n controller records when
     * team context +$3B requests the alternate mode-11 path. */
    uint16_t mode11_context_raw_3b[2];
    int16_t mode11_control_group_raw[5];
    uint16_t mode11_control_flags_raw[5];
    uint16_t period_raw_0926;
    uint16_t match_clock_raw_0928;
    int8_t possession_actor;
    int8_t tip_contact_actor;
    uint32_t tip_contact_frame;
    uint32_t tip_possession_frame;
    uint16_t tip_reach_mask;
    uint16_t tip_toss_countdown_raw_09f2;
    uint16_t scratch_0046, scratch_0047;
    uint16_t formation_override_raw_005c; /* DP `$5C`, formation timer gate */
    NbaGraphicsScratchState graphics_scratch;
    uint32_t jump_decision_calls, jump_launches, jump_rejected_contexts;
    NbaJumpReachInput last_jump_input;
    NbaJumpReachResult last_jump_result;
    NbaTipEvent tip_event;
    uint16_t tip_event_bits_raw_13e9;
    uint16_t tip_winner_group_raw_0932;
    uint16_t overtime_tie_marker_raw_15bd;
    NbaTipLaunch tip_last_launch; /* diagnostics: $99C4 result before wrapper */
    int8_t possession_team;
    uint8_t camera_side_group_raw; /* persistent `$093A`: 0/5, FF before tip */
    uint16_t camera_alternate_raw_08bc, camera_alternate_mode_raw_08cc;
    uint16_t owner_team_group_raw_09f4; /* current mode-11 actor's +6E */
    uint16_t play_code;
    int16_t play_step_raw;       /* `$0998` */
    int16_t play_countdown_raw;  /* `$099A` */
    uint16_t play_mirror_raw;    /* `$099C` */
    uint16_t play_event_wait_raw;/* `$099E` */
    uint16_t play_request_raw;   /* `$0994` */
    uint32_t play_consumed_serial; /* host diagnostic, not ROM state */
    uint16_t play_cycle_raw;     /* `$09A4` */
    uint16_t play_hold_raw;      /* `$09D0` */
    uint16_t role_rebuild_raw_09d6;
    uint16_t role_ownerless_raw_09d8;
    uint16_t role_cadence_raw_09d2;
    uint16_t role_near_orientation_raw_09d4;
    uint16_t role_nearest_offense_raw_09de;
    int16_t role_focal_x_raw_0918; /* cached `$85:A76D` lookahead X */
    int16_t role_focal_y_raw_091a; /* cached `$85:A76D` lookahead Y */
    uint16_t special_actor_raw;  /* `$09A2`, clear-lane cutter or `$FFFF` */
    int16_t play_aux_selector_raw_09a6; /* cleared by `$86:BADC` */
    int16_t play_selector_raw[3];/* `$09AA/$09AC/$09AE` */
    uint16_t catch_actor_record_raw_0910; /* `$86:BAEE` */
    uint16_t catch_context_record_raw_0912; /* `$86:BAF1` */
    uint32_t simulation_tick;
    uint32_t actor_update_tick;
    uint8_t actor_behavior_pending;
    uint16_t possession_frame;
    uint16_t play_state_frame;
    uint16_t possession_number;
    uint16_t inbound_timer_raw;    /* `$092E`, separate from run-clock latch `$0A04` */
    uint16_t dead_clock_enabled_raw_0a04;
    uint16_t elapsed_clock_raw_13f9, elapsed_shot_clock_raw_13f7;
    NbaShotFatigue fatigue;
    /* Player-record-owned shot counters. Actors mirror the active record;
     * substitution saves/loads through this table instead of transferring
     * the outgoing player's counters to the promoted bench player. */
    uint16_t roster_shot_statistics[24][5];
    uint8_t roster_personal_fouls[24];
    uint16_t shot_value_raw;       /* `$094C` */
    uint16_t live_state_raw;       /* `$0936` */
    uint16_t inbound_state_raw;    /* `$0952` */
    uint16_t inbound_actor_raw;    /* `$0954` */
    int16_t inbound_layout_raw;    /* `$0956` */
    int16_t inbound_target_x_raw;  /* `$0958` */
    int16_t inbound_target_y_raw;  /* `$095A` */
    uint16_t inbound_direction_raw;/* `$095C` */
    uint16_t pad_held_raw;         /* controller 0 `$090C+$08`, current frame */
    uint16_t inbound_ready_raw;    /* represented `$09BA` arrival latch */
    uint16_t inbound_transfer_raw; /* `$09B8` */
    uint16_t ball_activity_raw;    /* `$0948`, canonical shot detach */
    uint16_t shot_bounce_timer_raw_091c;
    int16_t pass_actor_raw;        /* `$0942` */
    int16_t pass_aux_raw;          /* `$0944`, shared activity auxiliary */
    int16_t pass_receiver_raw;     /* `$0946` */
    uint16_t pass_active_raw;      /* `$09C4` */
    uint16_t pass_distance_raw;    /* `$09DA` */
    /* Persistent globals consumed/mutated by `$85:9ACB-$A081`. Keep their
     * raw names until the surrounding writers establish narrower labels. */
    uint16_t rim_raw_092c;
    uint16_t shot_clock_mirror_raw_09c6;
    uint16_t rim_raw_0962;
    uint16_t rim_raw_096a;
    uint16_t rim_raw_097c;
    uint16_t rim_raw_096e;
    uint16_t rim_impact_raw_13e5;
    uint16_t rim_raw_13e7;
    uint16_t rim_raw_0920;
    uint16_t rim_raw_094a;
    uint16_t rim_raw_0970;
    uint16_t attached_ball_state_raw_09f6;
    uint16_t dead_ball_raw_0966;
    uint16_t dead_ball_raw_0968;
    uint16_t dead_ball_raw_096c;
    uint16_t dead_ball_raw_097e;
    int16_t dead_ball_x_raw_09b0;
    int16_t dead_ball_y_raw_09b2;
    uint16_t dead_ball_dispatch_busy_raw_09b4;
    uint16_t rim_force_raw_1866;
    int16_t shot_actor_raw_09c8;
    uint16_t leading_side_raw_1403;
    uint16_t left_lead_change_count_raw_1405;
    uint16_t right_lead_change_count_raw_1407;
    NbaGameplayEffectState rim_effect;
    int16_t shot_origin_x, shot_origin_y;
    uint8_t shot_chance_raw;
    uint8_t shot_miss_index_raw;
    bool shot_inner_veto_raw;  /* `$09F8` */
    uint16_t assistance_team_raw_09c0; /* late-game trailing-team CPU Assistance */
    uint32_t shot_selection_serial;
    uint16_t shot_selection_inputs[8]; /* lane,move,range,direction,facing,variant,mode,actor */
    uint16_t shot_previous_actor_x_raw_0922;
    uint16_t ball_previous_z_raw_0924; /* `$85:A59A-$A59D`, pre-substep integer Z */
    NbaShotLaunchState last_shot_launch; /* diagnostic snapshot, not actor/ball authority */
    uint32_t shot_launch_serial;
    uint8_t last_scoring_side;
    bool shot_result_resolved;
    uint8_t offense_side;
    uint8_t handler_actor;
    uint8_t receiver_actor;
    uint8_t cpu_play_state;
    int8_t collision_actor_a_raw; /* `$492D`, current contact candidate */
    int8_t collision_actor_b_raw; /* `$492F`, current owner/victim */
    uint32_t collision_routine_raw;
    uint16_t player_contact_count_raw;
    int8_t player_contact_actor_a_raw;
    int8_t player_contact_actor_b_raw;
    uint32_t player_contact_routine_raw;
    bool cpu_vs_cpu;
    bool is_initialized;
} NbaTipoff;

bool nba_tipoff_init(NbaTipoff *tipoff, const NbaAssetPack *assets,
                     NbaSession *session);
void nba_tipoff_update(NbaTipoff *tipoff, const NbaInput *input);
/* `$86:8300-$857B` bounded TIMEOUT/RESUME dispatcher. */
bool nba_tipoff_pause_active(const NbaTipoff *tipoff);
bool nba_tipoff_pause_can_enter(const NbaTipoff *tipoff);
/* `$86:97CD-$97F2 -> $87:8EB2-$8ECF -> $87:95E9-$979D`.
 * Returns true while the normal gameplay pass is frozen. */
bool nba_tipoff_step_match_lifecycle(NbaTipoff *tipoff);
bool nba_tipoff_match_horn_transition_ready(const NbaTipoff *tipoff);
bool nba_tipoff_try_tip_contact(NbaTipoff *tipoff);
bool nba_tipoff_jump_reach(NbaTipoff *tipoff, unsigned actor);
bool nba_tipoff_select_tip_receiver(NbaTipoff *tipoff);
bool nba_tipoff_launch_tip_ball(NbaTipoff *tipoff);
void nba_tipoff_render(const NbaTipoff *tipoff, NbaRenderer *renderer);
void nba_tipoff_capture_telemetry(const NbaTipoff *tipoff,
                                  const NbaInput *input,
                                  NbaGameplayTelemetry *telemetry);
/* Controlled CLI fixture, not normal gameplay/user-player control. */
bool nba_tipoff_debug_special_shot(NbaTipoff *tipoff, unsigned slot);
/* `$83:ECB0-$ED46` automatic foul-out continuation without the still-unknown
 * human substitution presentation. False leaves the request and all lineup,
 * actor, resource, role, fatigue and stat ownership state unchanged. */
bool nba_tipoff_apply_foul_out_substitution(NbaTipoff *tipoff);

/* ROM-vector replay boundaries. Runtime gameplay uses these same functions;
 * they are public so captured native calls can be replayed without copying
 * their decision logic into a test-only model. */
bool nba_tipoff_begin_rom_pass(NbaTipoff *tipoff, unsigned passer_slot,
                               unsigned receiver_slot);
bool nba_tipoff_update_rom_passer(NbaTipoff *tipoff, unsigned slot);
void nba_tipoff_refresh_team_roles_end_frame(NbaTipoff *tipoff);
void nba_tipoff_refresh_offense_roles_end_frame(NbaTipoff *tipoff);
void nba_tipoff_update_play_control_end_frame(NbaTipoff *tipoff);
void nba_tipoff_refresh_defense_roles_end_frame(NbaTipoff *tipoff);
/* Verification entry for the ownerless `$85:9A6A-$A7C7` ball driver. */
NbaGameplayRimResult nba_tipoff_replay_ownerless_ball_entry(
    NbaTipoff *tipoff);
/* Verification entry for the ownership/resource dispatch at `$85:9A37`. */
NbaGameplayRimResult nba_tipoff_replay_ball_driver_entry(NbaTipoff *tipoff);
void nba_tipoff_replay_player_contact_sweep(NbaTipoff *tipoff);
void nba_tipoff_replay_player_contact_order(NbaTipoff *tipoff,
                                            const uint8_t *order,
                                            unsigned count);
void nba_tipoff_replay_collision_order(NbaTipoff *tipoff,
                                       const uint8_t *order,
                                       unsigned count);
void nba_tipoff_replay_ball_acquisition(NbaTipoff *tipoff, uint8_t catcher);
void nba_tipoff_replay_ball_acquisition_core(NbaTipoff *tipoff,
                                              uint8_t catcher);
bool nba_tipoff_replay_defensive_pose(NbaTipoff *tipoff, uint8_t actor);
bool nba_tipoff_replay_normal_actor(NbaTipoff *tipoff, uint8_t actor);
bool nba_tipoff_replay_requested_direction(NbaTipoff *tipoff, uint8_t actor);
bool nba_tipoff_replay_passive_mode(NbaTipoff *tipoff, uint8_t actor);
bool nba_tipoff_replay_mode13_close_finish(NbaTipoff *tipoff, uint8_t actor);
bool nba_tipoff_replay_mode14_close_finish(NbaTipoff *tipoff, uint8_t actor);
bool nba_tipoff_replay_close_finish_start(NbaTipoff *tipoff, uint8_t actor);
bool nba_tipoff_replay_matchup_helper(NbaTipoff *tipoff, uint16_t entry,
    uint8_t current_actor, uint8_t related_actor, uint8_t context_side,
    uint8_t *selected_actor);
/* Exact `$85:AD6B-$AF5B` formation target/steering boundary. */
bool nba_tipoff_replay_formation_route(NbaTipoff *tipoff, uint8_t actor,
                                       uint8_t *direction);
/* Readiness/selection continuation `$86:F4F2-$F653`, without host scheduling. */
void nba_tipoff_replay_inbound_continuation(NbaTipoff *tipoff);
/* Exact `$85:B678-$B8CA` mode-11 decision parent (0 return, 1 consumed,
 * 2 shot-started), including its native formation velocity continuation. */
uint8_t nba_tipoff_replay_mode11_dispatch(NbaTipoff *tipoff, uint8_t actor);
/* Exact early `$87:92A5-$949E` violation/dead-ball parent boundary. */
void nba_tipoff_replay_violation_dispatch(NbaTipoff *tipoff);

#endif
