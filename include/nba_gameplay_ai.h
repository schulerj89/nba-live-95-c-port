#ifndef NBA_GAMEPLAY_AI_H
#define NBA_GAMEPLAY_AI_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t state;
} NbaGameplayRng;

typedef struct {
    int16_t anchor_x_raw_0a;
    uint16_t mode_raw_30;
    uint16_t flags_raw_32;
    uint16_t activity_raw_39;
    uint16_t dead_ball_actor_raw_3f;
    int16_t controller_actor_raw_41;
    uint16_t previous_dead_ball_actor_raw_43;
    int16_t previous_controller_actor_raw_45;
    uint16_t match_clock_raw_47;
    uint16_t help_distance_raw_4e;
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
uint8_t nba_gameplay_pass_direction(int16_t dx, int16_t dy,
                                    uint16_t *distance);
bool nba_gameplay_receiver_candidate_valid(
    uint8_t passer_actor, uint8_t candidate_actor,
    const NbaGameplayReceiverState *actors, uint8_t actor_count);
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
bool nba_gameplay_mode11_shot_rectangle(int16_t rom_x, int16_t y, int16_t z);
bool nba_gameplay_mode11_shot_decision(
    const NbaGameplayMode11ShotInput *input, NbaGameplayRng *rng);
bool nba_gameplay_court_clamp(int32_t *x_fp, int32_t *y_fp,
                              int16_t *velocity_x, int16_t *velocity_y);
bool nba_gameplay_inbound_target(
    int16_t layout_state, int16_t source_x, int16_t source_y,
    int16_t context_anchor_x, int16_t ball_x, NbaGameplayRng *rng,
    NbaGameplayInboundTarget *target);
bool nba_gameplay_inbound_arrived(int16_t actor_x, int16_t actor_y,
                                  int16_t target_x, int16_t target_y);
bool nba_gameplay_inbound_pass_due(uint16_t timer, uint16_t random_word);
void nba_gameplay_velocity_step(int16_t *velocity_x, int16_t *velocity_y,
                                uint16_t *boost_timer, uint8_t direction,
                                uint8_t profile_42, uint16_t dispatch_dt,
                                bool movement_blocked,
                                int16_t global_093e);

#endif
