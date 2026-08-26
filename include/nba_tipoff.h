#ifndef NBA_TIPOFF_H
#define NBA_TIPOFF_H

#include "nba_assets.h"
#include "nba_renderer.h"
#include "nba_session.h"
#include "nba_gameplay_debugger.h"
#include "nba_gameplay_camera.h"
#include "nba_gameplay_ai.h"
#include "nba_gameplay_ball.h"
#include "nba_gameplay_effect.h"
#include "nba_gameplay_foul.h"

/* ROM routines correlated with live Mesen execution. */
#define SNES_ADDR_TIPOFF_PLAYER_FORMATION 0x86DDA7
#define SNES_ADDR_TIPOFF_BALL_INIT        0x86E054
#define SNES_ADDR_TIPOFF_JUMP_ANIMATION   0x86ECF4
#define SNES_ADDR_TIPOFF_CONTACT          0x86CF49
#define SNES_ADDR_TIPOFF_POSSESSION       0x86D3F9

#define NBA_TIPOFF_BALL_APPEAR_FRAME 140
#define NBA_TIPOFF_TOSS_FRAME       145
#define NBA_TIPOFF_JUMP_FRAME       156
#define NBA_TIPOFF_CONTACT_FRAME    198
#define NBA_TIPOFF_POSSESSION_FRAME 200
#define NBA_TIPOFF_BREAK_FRAME      220

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
    uint16_t upper_animation_phase_raw; /* actor `+$3A` */
    uint8_t base_animation_state_raw_38; /* actor `+$38`, locomotion intent */
    uint16_t upper_animation_lock_raw_46; /* actor `+$46` */
    uint16_t lower_animation_lock_raw_48; /* actor `+$48` */
    uint16_t actor_status_raw_28;       /* actor `+$28`, `$86:C476` */
    uint8_t control_mode;
    uint8_t saved_control_mode;         /* actor `+$84` */
    uint16_t pass_band_raw;             /* actor `+$62`: 0,6,...,30 */
    uint16_t pass_direction_raw;        /* actor `+$66` */
    int16_t special_contact_raw_56;     /* actor `+$56`, `$86:C943-$C951` */
    uint8_t mode13_variant_raw_58;      /* shared close-finish `+$58` */
    int16_t pass_family_raw;            /* actor `+$C0` */
    uint8_t pass_release_threshold_raw;
    bool pass_released_raw;
    uint8_t requested_direction;
    uint8_t movement_direction;
    uint8_t velocity_direction_raw_a2; /* actor `+$A2` */
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
    int16_t mode13_baseline_velocity_x; /* modes 13/14 actor `+$BA` */
    int16_t mode13_baseline_velocity_y; /* modes 13/14 actor `+$BC` */
    uint16_t contact_inhibit_raw_5a;  /* actor `+$5A`, `$86:CD03/D460` */
    uint16_t contact_action_timer_raw_60; /* actor `+$60`, `$86:C0DF/C12F` */
    uint16_t contact_height_raw_aa;   /* actor `+$AA`, `$87:A6A9-A6B2` */
    uint16_t catcher_latch_raw_ae;    /* actor `+$AE`, `$86:BAE0` */
    uint16_t free_throw_launch_half_raw_a8; /* actor `+$A8`, `$86:A2A7` */
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

typedef struct {
    const NbaAssetPack *assets;
    NbaSession *session;
    int frame;
    NbaTipoffPhase phase;
    NbaTipoffActor actors[NBA_GAMEPLAY_ACTOR_COUNT];
    NbaTipoffBall ball;
    int16_t camera_x, camera_y;
    NbaGameplayCamera camera;
    NbaGameplayRng rng;
    NbaGameplayFoulState fouls;
    uint16_t team_pose_contact_count_raw[2]; /* team context +$50 */
    uint16_t deferred_shot_foul_phase_raw_0a02;
    uint16_t free_throw_start_tick_raw_09be;
    uint16_t free_throw_aim_x_raw_0980;
    uint16_t free_throw_aim_y_raw_0982;
    uint16_t free_throw_resolution_raw_0972;
    uint16_t free_throw_flight_timer_raw_0930;
    NbaGameplayTeamContext team_context[2]; /* `$46EB/$476B` +$30/+32/+39 */
    uint16_t period_raw_0926;
    uint16_t match_clock_raw_0928;
    int8_t possession_actor;
    int8_t possession_team;
    uint8_t camera_side_group_raw; /* persistent `$093A`: 0 or 5 */
    uint16_t play_code;
    int16_t play_step_raw;       /* `$0998` */
    int16_t play_countdown_raw;  /* `$099A` */
    uint16_t play_mirror_raw;    /* `$099C` */
    uint16_t play_event_wait_raw;/* `$099E` */
    uint16_t play_request_raw;   /* `$0994` */
    uint16_t play_cycle_raw;     /* `$09A4` */
    uint16_t play_hold_raw;      /* `$09D0` */
    uint16_t role_rebuild_raw_09d6;
    uint16_t role_ownerless_raw_09d8;
    uint16_t role_cadence_raw_09d2;
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
    uint16_t inbound_timer_raw;    /* `$092E/$0A04` */
    uint16_t shot_value_raw;       /* `$094C` */
    uint16_t live_state_raw;       /* `$0936` */
    uint16_t inbound_state_raw;    /* `$0952` */
    uint16_t inbound_actor_raw;    /* `$0954` */
    int16_t inbound_layout_raw;    /* `$0956` */
    int16_t inbound_target_x_raw;  /* `$0958` */
    int16_t inbound_target_y_raw;  /* `$095A` */
    uint16_t inbound_direction_raw;/* `$095C` */
    uint16_t inbound_ready_raw;    /* represented `$09BA` arrival latch */
    uint16_t inbound_transfer_raw; /* `$09B8` */
    uint16_t ball_activity_raw;    /* `$0948`, canonical shot detach */
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
void nba_tipoff_render(const NbaTipoff *tipoff, NbaRenderer *renderer);
void nba_tipoff_capture_telemetry(const NbaTipoff *tipoff,
                                  const NbaInput *input,
                                  NbaGameplayTelemetry *telemetry);

/* ROM-vector replay boundaries. Runtime gameplay uses these same functions;
 * they are public so captured native calls can be replayed without copying
 * their decision logic into a test-only model. */
bool nba_tipoff_begin_rom_pass(NbaTipoff *tipoff, unsigned passer_slot,
                               unsigned receiver_slot);
bool nba_tipoff_update_rom_passer(NbaTipoff *tipoff, unsigned slot);
void nba_tipoff_refresh_team_roles_end_frame(NbaTipoff *tipoff);
void nba_tipoff_refresh_defense_roles_end_frame(NbaTipoff *tipoff);
/* Verification entry for the ownerless `$85:9A6A-$A7C7` ball driver. */
NbaGameplayRimResult nba_tipoff_replay_ownerless_ball_entry(
    NbaTipoff *tipoff);
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

#endif
