#ifndef NBA_GAMEPLAY_AI_H
#define NBA_GAMEPLAY_AI_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t state;
} NbaGameplayRng;

typedef struct {
    uint16_t strategy_team_raw_00;
    int16_t anchor_x_raw_0a;
    uint16_t score_raw_26;
    uint16_t strategy_raw_2e;
    uint16_t mode_raw_30;
    uint16_t flags_raw_32;
    uint16_t activity_raw_39;
    uint16_t dead_ball_actor_raw_3f;
    int16_t controller_actor_raw_41;
    uint16_t previous_dead_ball_actor_raw_43;
    int16_t previous_controller_actor_raw_45;
    uint16_t match_clock_raw_47;
    uint16_t help_distance_raw_4e;
    uint16_t play_selection_raw_56;
    uint8_t actor_order_raw_49[5];
} NbaGameplayTeamContext;

/* Actor fields consumed by `$85:B60B-$B677`. `travel_direction` and
 * `travel_distance` correspond to actor +$86/+8A, not rendered facing or
 * instantaneous velocity. */
typedef struct {
    int16_t x, y;
    uint8_t control_mode;
    uint8_t travel_direction;
    uint16_t travel_distance;
} NbaGameplayReceiverState;

typedef struct {
    int16_t x, y;
    uint8_t team_group;
} NbaGameplayLaneActor;

typedef struct {
    int16_t x, y;
    uint8_t direction;
    uint8_t play_code;
    bool play_requested;
} NbaGameplayInboundTarget;

typedef struct {
    int16_t actor_x, actor_y;
    int16_t target_x, target_y;
    int16_t velocity_x, velocity_y;
    uint16_t boost_timer;
    uint8_t profile_42;
    uint16_t dispatch_dt;
    bool movement_blocked;
    int16_t owner_actor_raw_093e;
    uint8_t direction;
} NbaGameplayInboundMotion;

/* Owned state at the CPU-only arrival half of `$86:F54F-$F58E`.  Keeping
 * these native words together makes the attachment/transfer latches part of
 * the gameplay contract instead of incidental side effects in tipoff.c. */
typedef struct {
    uint16_t dead_ball_raw_0968;
    uint16_t attachment_raw_09f6;
    uint16_t behavior_flags_raw_7e;
    int16_t velocity_x_raw_0e, velocity_y_raw_10;
    uint16_t inbound_ready_raw_09ba;
    uint16_t whistle_raw_09b6;
    uint16_t foul_event_raw_0964;
    uint16_t transfer_raw_09b8;
    int16_t receiver_actor_raw_0946;
    uint16_t inbound_direction_raw_095c;
    uint8_t draw_direction_raw_4e;
} NbaGameplayInboundArrival;

typedef struct {
    uint8_t current_direction;
    uint8_t control_mode;
    uint16_t actor_status;
    uint16_t upper_state;
    uint16_t anchor_direction;
    bool candidate_valid;
    int16_t candidate_dx, candidate_dy;
} NbaGameplayDrawDirection;

typedef struct {
    NbaGameplayDrawDirection direction;
    uint16_t status, upper_resource, lower_resource;
    int16_t world_x, world_y, world_z;
    int16_t screen_x, screen_y;
    uint16_t head_base, palette_offset;
} NbaGameplayDrawPreparationInput;

typedef struct {
    uint8_t direction;
    uint16_t status, upper_resource, lower_resource, head_resource;
    uint16_t attribute;
    int16_t x, y;
} NbaGameplayDrawPreparation;

/* Portable boundary for `$86:E39A-$E3CA` and its wider
 * `$86:E3E1-$E4A6` defensive pose caller. Values retain their native actor
 * record meaning instead of being inferred from rendered sprites. */
typedef struct {
    int16_t actor_z;
    uint16_t free_throw_state_raw_0978;
    uint16_t live_state_raw_0936;
    int16_t owner_actor_raw_093e;
    int16_t receiver_actor_raw_0946;
    int16_t context_anchor_x_raw_0a;
    int16_t actor_x;
    uint8_t control_mode;
    uint16_t actor_movement_raw_4c;
    uint16_t paired_movement_raw_4c;
    uint16_t actor_pair_distance_raw_8a;
    uint8_t actor_pair_direction_raw_86;
    uint16_t actor_anchor_distance_raw_8c;
    uint16_t paired_anchor_distance_raw_8c;
    int16_t velocity_x, velocity_y;
    uint8_t upper_state_raw_30;
    uint8_t base_state_raw_38;
    uint8_t facing_raw_4e;
    uint8_t requested_direction_raw_50;
    uint16_t selected_count_raw_1868;
} NbaGameplayDefensivePoseInput;

typedef struct {
    uint8_t base_state_raw_38;
    uint8_t facing_raw_4e;
    uint8_t requested_direction_raw_50;
    uint16_t selected_count_raw_1868;
    uint8_t selector_result_raw_aa;
    bool install_both;
    uint8_t install_state;
} NbaGameplayDefensivePoseOutput;

bool nba_gameplay_stationary_defensive_pose(
    const NbaGameplayDefensivePoseInput *input,
    NbaGameplayDefensivePoseOutput *output);
bool nba_gameplay_defensive_pose(
    const NbaGameplayDefensivePoseInput *input,
    NbaGameplayDefensivePoseOutput *output);

/* Portable inputs consumed by the live-covered `$85:B734-$B820` tail of
 * the CPU ballhandler's mode-11 shot decision. */
typedef struct {
    int16_t play_step_raw_0998;
    uint16_t play_cycle_raw_09a4;
    uint16_t play_hold_raw_09d0;
    uint16_t dead_ball_raw_0968;
    uint16_t shot_clock_rule_raw_17e1;
    uint16_t difficulty_raw_17af;
    uint16_t assignment_distance_raw_8a;
    uint16_t anchor_distance_raw_8c;
    uint8_t two_point_rating_raw_36;
    uint8_t three_point_rating_raw_37;
    uint8_t shot_range_raw_49;
    int16_t actor_z;
    bool same_attack_half;
} NbaGameplayMode11ShotInput;

typedef struct {
    int16_t actor_x, actor_y;
    uint8_t actor_pair_direction_raw_86;
    uint16_t actor_pair_distance_raw_8a;
    int16_t paired_x, paired_y;
    int16_t paired_velocity_x, paired_velocity_y;
    uint8_t paired_anchor_direction_raw_88;
    uint16_t paired_anchor_distance_raw_8c;
    uint8_t paired_position_raw_92;
    int16_t context_anchor_x;
    uint16_t context_mode_raw_30;
    uint16_t context_flags_raw_32;
    bool paired_on_three_point_arc;
    uint8_t paired_three_point_rating;
} NbaGameplayDefenseTargetInput;

typedef struct {
    int16_t target_x, target_y;
    bool target_written;
    bool stop_velocity;
} NbaGameplayDefenseTargetOutput;

/* Inputs retained by the close-range half of mode four's `$86:EF09`
 * special policy. The far-range score/contact policy remains a separate
 * branch; this boundary returns false for it after preserving the first RNG
 * gate shared by both paths. */
typedef struct {
    uint16_t difficulty_raw_17af;
    int16_t controller_assignment_raw_16;
    uint16_t live_state_raw_0936;
    uint16_t period_raw_0926;
    uint16_t match_clock_raw_0928;
    uint16_t current_score_raw_26;
    uint16_t opponent_score_raw_26;
    uint16_t personal_fouls_raw_14;
    uint16_t paired_anchor_distance_raw_8c;
    int16_t paired_x, paired_y;
    int16_t paired_velocity_x, paired_velocity_y;
} NbaGameplayModeFourCloseInput;

typedef struct {
    int16_t target_x, target_y;
    uint8_t upper_animation_request;
} NbaGameplayModeFourCloseOutput;

/* Five-player side input consumed by the no-owner role pass at
 * `$85:AFB2-$B128`. */
typedef struct {
    int16_t x, y;
    uint8_t control_mode;
} NbaGameplayLoosePursuitActor;

/* Pure branch inputs for `$86:F0FD-$F1AF`. Actor IDs are the ROM's 0..9
 * logical slots; team groups use raw values 0/5. */
typedef struct {
    uint16_t live_state_raw_0936;
    uint16_t ball_activity_raw_0948;
    uint16_t bounce_age_raw_094a;
    uint16_t free_throw_state_raw_0978;
    uint16_t play_code_raw_0996;
    int16_t foul_actor_raw_7e492f;
    uint8_t actor_id;
    uint8_t actor_control_mode;
    uint8_t actor_team_group_raw_6e;
    uint8_t offense_group_raw_093a;
    uint8_t inbound_group_raw_0952;
} NbaGameplayLoosePursuitGateInput;

/* Raw actor words consumed by the common `$85:96B5-$9A13` physics commit.
 * Positions intentionally retain the ROM's split 16.16 representation so
 * vector replay does not discard the low fractional byte used by carries. */
typedef struct {
    uint16_t x_fraction, y_fraction, z_fraction;
    int16_t x, y, z;
    int16_t velocity_x, velocity_y, velocity_z;
    uint16_t control_mode_raw_5e;
    uint16_t reaction_timer_raw_60;
    uint16_t behavior_flags_raw_7e;
    uint16_t speed_raw_4a;
    uint16_t movement_distance_raw_4c;
    uint8_t facing_raw_4e;
    uint8_t velocity_direction_raw_a2;
    uint16_t previous_x_fraction_raw_94;
    int16_t previous_x_raw_96;
    uint16_t previous_y_fraction_raw_98;
    int16_t previous_y_raw_9a;
    uint16_t planar_scratch_raw_a0;
} NbaGameplayActorCommit;

void nba_gameplay_rng_seed(NbaGameplayRng *rng, uint16_t seed);
uint16_t nba_gameplay_rng_next(NbaGameplayRng *rng);
bool nba_gameplay_rng_self_test(void);
bool nba_gameplay_ai_self_test(void);
uint16_t nba_gameplay_reaction_threshold(NbaGameplayRng *rng,
                                         int16_t actor_x, int16_t actor_y,
                                         int16_t ball_x, int16_t ball_y);
uint32_t nba_gameplay_behavior_routine(uint8_t mode);
void nba_gameplay_target_from_pair(int16_t paired_x, int16_t paired_y,
                                   int16_t paired_velocity_x,
                                   int16_t paired_velocity_y,
                                   int16_t offset_x, int16_t offset_y,
                                   int16_t *target_x, int16_t *target_y);
void nba_gameplay_defense_pair_target(
    int16_t paired_x, int16_t paired_y,
    int16_t paired_velocity_x, int16_t paired_velocity_y,
    uint8_t paired_direction, uint16_t context_raw_30,
    uint16_t context_raw_32, bool force_close_table,
    int16_t *target_x, int16_t *target_y);
bool nba_gameplay_defense_mode_target(
    uint8_t actor_mode, const NbaGameplayDefenseTargetInput *input,
    NbaGameplayDefenseTargetOutput *output);
bool nba_gameplay_mode_four_close_override(
    const NbaGameplayModeFourCloseInput *input, NbaGameplayRng *rng,
    NbaGameplayModeFourCloseOutput *output);
uint16_t nba_gameplay_weighted_distance(int16_t dx, int16_t dy);
int8_t nba_gameplay_select_no_owner_pursuer(
    const NbaGameplayLoosePursuitActor actors[5],
    int16_t predicted_ball_x, int16_t predicted_ball_y,
    uint8_t normalized_modes[5]);
bool nba_gameplay_loose_ball_pursuit_allowed(
    const NbaGameplayLoosePursuitGateInput *input);
bool nba_gameplay_decision_timer_step(uint16_t *timer, uint8_t profile_byte,
                                      uint16_t reload_base,
                                      bool add_half_court_delay);
bool nba_gameplay_same_x_half(int16_t actor_x, int16_t context_anchor_x);
bool nba_gameplay_defense_context_reselect(
    uint16_t current_score, uint16_t opponent_score,
    uint16_t period_raw_0926, uint16_t opponent_activity_raw_39,
    uint16_t random_word, uint16_t *opponent_mode_raw_30);
uint8_t nba_gameplay_target_direction(int16_t dx, int16_t dy,
                                      uint16_t *distance);
uint8_t nba_gameplay_contact_facing(int16_t dx, int16_t dy);
uint8_t nba_gameplay_pass_direction(int16_t dx, int16_t dy,
                                    uint16_t *distance);
bool nba_gameplay_receiver_candidate_valid(
    uint8_t passer_actor, uint8_t candidate_actor,
    const NbaGameplayReceiverState *actors, uint8_t actor_count);
int8_t nba_gameplay_select_inbound_receiver_cpu(
    uint8_t inbounder, uint16_t timer, int16_t context_anchor_x,
    const int16_t selectors[3],
    const NbaGameplayReceiverState *actors, uint8_t actor_count);
bool nba_gameplay_inbound_side_allows(int16_t context_anchor_x,
                                     int16_t owner_x, int16_t receiver_x);
bool nba_gameplay_lane_to_basket_clear(
    uint8_t subject_actor, int16_t basket_x,
    const NbaGameplayLaneActor *actors, uint8_t actor_count);
void nba_gameplay_special_actor_step(
    uint16_t *behavior_timer, uint8_t control_mode,
    uint16_t play_cycle_raw_09a4, bool possession_active, bool lane_clear,
    uint16_t owner_distance, uint8_t actor_id, uint16_t *special_actor_raw_09a2);
int8_t nba_gameplay_select_pass_receiver(
    uint8_t passer_actor, int16_t special_actor,
    const int16_t selectors[3], const NbaGameplayReceiverState *actors,
    uint8_t actor_count, bool attack_right);
bool nba_gameplay_predictive_arrival(int16_t actor_x, int16_t actor_y,
                                     int16_t velocity_x, int16_t velocity_y,
                                     int16_t target_x, int16_t target_y,
                                     uint16_t tolerance,
                                     uint8_t *steering_direction,
                                     uint16_t *distance);
bool nba_gameplay_direct_arrival(int16_t actor_x, int16_t actor_y,
                                 int16_t target_x, int16_t target_y,
                                 uint16_t tolerance,
                                 uint8_t *steering_direction,
                                 uint16_t *distance);
bool nba_gameplay_mode11_shot_rectangle(int16_t rom_x, int16_t y, int16_t z);
bool nba_gameplay_mode11_shot_decision(
    const NbaGameplayMode11ShotInput *input, NbaGameplayRng *rng);
bool nba_gameplay_court_clamp(int32_t *x_fp, int32_t *y_fp,
                              int16_t *velocity_x, int16_t *velocity_y);
bool nba_gameplay_court_finish_y_step(
    int32_t *x_fp, int32_t *y_fp, int16_t *velocity_x,
    int16_t *velocity_y);
bool nba_gameplay_inbound_target(
    int16_t layout_state, int16_t source_x, int16_t source_y,
    int16_t context_anchor_x, int16_t ball_x, NbaGameplayRng *rng,
    NbaGameplayInboundTarget *target);
bool nba_gameplay_inbound_arrived(int16_t actor_x, int16_t actor_y,
                                  int16_t target_x, int16_t target_y);
bool nba_gameplay_inbound_pass_due(uint16_t timer, uint16_t random_word);
void nba_gameplay_inbound_motion_step(NbaGameplayInboundMotion *motion);
void nba_gameplay_inbound_arrival_prepare(NbaGameplayInboundArrival *state);
/* `$86:F520-$F54E`: translate the controller direction nibble through the
 * ROM table at `$86:F669`. CPU actors and inactive human inbounders preserve
 * actor `+$56`; an active human inbounder writes the table result. */
int16_t nba_gameplay_human_inbound_direction(
    int8_t controller_assignment, uint16_t movement_boost_timer,
    uint16_t pad_held, int16_t current_direction);
typedef struct {
    uint16_t camera_side_group, owner_actor;
    int16_t ball_x, ball_y, ball_velocity_x, ball_velocity_y;
    uint16_t owner_mode;
    uint16_t award_side_group, live_state, inbound_timer, role_rebuild_timer;
    uint16_t game_clock, shot_clock_mirror, dead_ball, ball_aux;
    int16_t dead_ball_x, dead_ball_y;
    uint16_t rim_state, ball_record, selector, scene_phase;
} NbaGameplayDeadBallReset;
/* `$87:9B38-$9BC8`: common five-second/dead-ball state reset. */
void nba_gameplay_dead_ball_reset(NbaGameplayDeadBallReset *state);
/* `$87:A52C-$A5FA`: presentation-only facing selector. It deliberately does
 * not mutate the actor's movement direction. */
uint8_t nba_gameplay_draw_direction(const NbaGameplayDrawDirection *input);
void nba_gameplay_prepare_player_draw(
    const NbaGameplayDrawPreparationInput *input,
    NbaGameplayDrawPreparation *output);
void nba_gameplay_velocity_step(int16_t *velocity_x, int16_t *velocity_y,
                                uint16_t *boost_timer, uint8_t direction,
                                uint8_t profile_42, uint16_t dispatch_dt,
                                bool movement_blocked,
                                int16_t global_093e);
void nba_gameplay_actor_commit(NbaGameplayActorCommit *actor,
                               uint16_t dispatch_dt,
                               bool update_ground_facing);

#endif
