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
    uint8_t control_mode;
    uint8_t saved_control_mode;         /* actor `+$84` */
    uint16_t pass_band_raw;             /* actor `+$62`: 0,6,...,30 */
    uint16_t pass_direction_raw;        /* actor `+$66` */
    int16_t pass_family_raw;            /* actor `+$C0` */
    uint8_t pass_release_threshold_raw;
    bool pass_released_raw;
    uint8_t requested_direction;
    uint8_t movement_direction;
    uint8_t assignment_actor;
    uint8_t assignment_direction;
    uint16_t assignment_base_raw;
    uint16_t assignment_current_raw;
    uint16_t assignment_alternate_raw;
    uint16_t assignment_distance;
    uint16_t pair_distance;
    uint16_t reaction_threshold;
    uint16_t movement_boost_timer; /* actor `+$72`, consumed by `$85:A82C` */
    int8_t controller_assignment_raw; /* signed actor `+$16` */
    uint16_t movement_magnitude_raw;  /* actor `+$4C` */
    uint16_t recovery_inhibit_raw;    /* actor `+$7A` */
    uint16_t behavior_flags_raw;      /* actor `+$7E` */
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
    uint16_t special_actor_raw;  /* `$09A2`, clear-lane cutter or `$FFFF` */
    int16_t play_selector_raw[3];/* `$09AA/$09AC/$09AE` */
    uint32_t simulation_tick;
    uint32_t actor_update_tick;
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
    uint16_t rim_raw_0962;
    uint16_t rim_raw_096a;
    uint16_t rim_raw_097c;
    uint16_t rim_raw_096e;
    uint16_t rim_raw_13e7;
    uint16_t rim_raw_0920;
    uint16_t rim_raw_094a;
    uint16_t rim_raw_0970;
    uint16_t rim_force_raw_1866;
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

#endif
