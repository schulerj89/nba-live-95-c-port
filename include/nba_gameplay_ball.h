#ifndef NBA_GAMEPLAY_BALL_H
#define NBA_GAMEPLAY_BALL_H

#include <stdbool.h>
#include <stdint.h>
#include "nba_gameplay_ai.h"

typedef enum NbaGameplayRimResult {
    NBA_GAMEPLAY_RIM_FLIGHT = 0,
    NBA_GAMEPLAY_RIM_OUTER_CONTACT,
    NBA_GAMEPLAY_RIM_EDGE_CONTACT,
    NBA_GAMEPLAY_RIM_MISS,
    NBA_GAMEPLAY_RIM_MAKE
} NbaGameplayRimResult;

typedef struct NbaGameplayRimState {
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t velocity_x;
    int16_t velocity_y;
    int16_t velocity_z;
    uint16_t raw_092c;
    uint16_t raw_0962;
    uint16_t raw_096a;
    uint16_t raw_097c;
    uint16_t raw_096e;
    uint16_t raw_13e7;
} NbaGameplayRimState;

typedef struct NbaGameplayPosePoint {
    int16_t x, y, z;
} NbaGameplayPosePoint;

typedef enum NbaGameplayOwnedContactResult {
    NBA_GAMEPLAY_OWNED_CONTACT_NONE = 0,
    NBA_GAMEPLAY_OWNED_CONTACT_FOUL,
    NBA_GAMEPLAY_OWNED_CONTACT_STRIP
} NbaGameplayOwnedContactResult;

/* Globals consumed by the inner-cylinder responses at `$85:9DAC-$A006`.
 * They remain explicit until surrounding writers give them stable semantic
 * names; this prevents a host collision shortcut from silently replacing a
 * gameplay state transition. */
typedef struct NbaGameplayRimContext {
    uint16_t raw_0920;
    uint16_t raw_0936;
    uint16_t raw_0948;
    uint16_t raw_094a;
    uint16_t raw_0970;
    uint16_t raw_0978;
    uint16_t raw_09f8;
    uint16_t raw_09b8;
    uint16_t raw_1866;
    uint16_t raw_07f6;
    uint16_t effect_raw_401b;
} NbaGameplayRimContext;

typedef struct NbaGameplaySettleContext {
    uint16_t raw_0936;
    uint16_t raw_0942;
    uint16_t raw_0944;
    uint16_t raw_0946;
    uint16_t raw_0948;
    uint16_t raw_094a;
    uint16_t raw_0978;
    uint16_t raw_09b8;
} NbaGameplaySettleContext;

/* Raw vertical state owned by the attached-ball response at
 * `$85:A4F2-$A517/$85:A532-$A597`. */
typedef struct NbaGameplayAttachedVerticalState {
    uint16_t attachment_state_raw_09f6;
    uint16_t dead_ball_raw_0968;
    int16_t velocity_z;
    uint16_t z_fraction;
    int16_t z;
    uint16_t impact_raw_13e5;
    uint16_t event_bits_raw_13e7;
} NbaGameplayAttachedVerticalState;

/* Normalized state for the live-covered player-grab prefix at
 * `$86:BAA2-$BAFA`. Raw actor/team-record pointers are retained because the
 * ROM publishes them in `$0910/$0912` for the following ownership tail. */
typedef struct NbaGameplayCatchPrefixState {
    uint8_t catcher;
    int16_t controller_actor;
    int16_t velocity_x, velocity_y;
    uint16_t movement_magnitude;
    uint16_t catcher_latch;
    uint16_t rim_force_raw_1866;
    uint16_t dead_ball_raw_0968;
    uint16_t rim_raw_096a;
    uint16_t context_actor_raw_3f;
    int16_t context_controller_raw_41;
    uint16_t context_previous_actor_raw_43;
    int16_t context_previous_controller_raw_45;
    int16_t special_actor_raw_09a2;
    int16_t play_aux_raw_09a6;
    int16_t play_selector_raw[3];
    int16_t possession_actor_raw_093e;
    uint16_t actor_record_raw_0910;
    uint16_t context_record_raw_0912;
} NbaGameplayCatchPrefixState;

uint16_t nba_gameplay_hoop_distance(int16_t dx, int16_t dy);
void nba_gameplay_apply_catch_prefix(NbaGameplayCatchPrefixState *state);
void nba_gameplay_apply_catch_mode(uint16_t match_clock,
                                   uint16_t *context_match_clock,
                                   uint16_t *control_mode,
                                   uint16_t *decision_timer,
                                   uint16_t *behavior_flags);
uint8_t nba_gameplay_owner_dribble_fallback_pose(
    uint16_t dead_ball_raw_0968, uint16_t catcher_latch_raw_ae);
typedef enum NbaGameplayOwnerDribbleGate {
    NBA_GAMEPLAY_OWNER_DRIBBLE_SKIP = 0,
    NBA_GAMEPLAY_OWNER_DRIBBLE_FALLBACK,
    NBA_GAMEPLAY_OWNER_DRIBBLE_CONTINUE
} NbaGameplayOwnerDribbleGate;
NbaGameplayOwnerDribbleGate nba_gameplay_owner_dribble_gate(
    int16_t actor_z, uint16_t free_throw_state_raw_0978,
    uint16_t live_state_raw_0936, uint16_t movement_magnitude_raw_4c);
typedef enum NbaGameplayOwnerProximityResult {
    NBA_GAMEPLAY_OWNER_PROXIMITY_FALLBACK = 0,
    NBA_GAMEPLAY_OWNER_PROXIMITY_LATCHED,
    NBA_GAMEPLAY_OWNER_PROXIMITY_UNLATCHED
} NbaGameplayOwnerProximityResult;
NbaGameplayOwnerProximityResult nba_gameplay_owner_dribble_proximity(
    int16_t context_anchor_x, int16_t actor_x,
    uint16_t paired_movement_magnitude, uint16_t assignment_distance,
    uint8_t paired_direction, uint16_t dead_ball_raw_0968,
    uint16_t catcher_latch_raw_ae, uint8_t *requested_direction);
uint8_t nba_gameplay_owner_unlatched_pose(
    int16_t velocity_x, int16_t velocity_y, uint8_t requested_direction,
    uint8_t *display_direction);
NbaGameplayRimResult nba_gameplay_rim_step(NbaGameplayRimState *state,
                                           uint16_t live_state,
                                           bool alternate_height,
                                           bool inner_veto,
                                           bool correct_basket_side);
NbaGameplayRimResult nba_gameplay_rim_world_step(
    NbaGameplayRimState *state, int16_t hoop_x, int16_t hoop_y,
    bool right_basket, uint16_t live_state, bool alternate_height,
    bool inner_veto, bool correct_basket_side);
void nba_gameplay_rim_apply_inner_response(
    NbaGameplayRimState *state, NbaGameplayRimResult result,
    NbaGameplayRimContext *context, NbaGameplayRng *rng);
void nba_gameplay_rim_apply_made_response(
    NbaGameplayRimState *state, bool right_basket,
    NbaGameplayRimContext *context);
bool nba_gameplay_ball_apply_settle(
    NbaGameplayRimState *state, NbaGameplaySettleContext *context);
void nba_gameplay_ball_apply_attached_vertical(
    NbaGameplayAttachedVerticalState *state);
void nba_gameplay_ball_apply_ground_impact(
    NbaGameplayRimState *state, uint16_t *impact_raw_13e5);
bool nba_gameplay_ball_is_make(uint16_t live_state, bool alternate_height,
                               bool inner_veto, bool correct_basket_side,
                               int16_t dx, int16_t dy, int16_t z);
uint16_t nba_gameplay_shot_value(bool one_point_attempt, int16_t shooter_x,
                                 int16_t shooter_y, bool right_basket);
uint8_t nba_gameplay_shot_chance(uint8_t rating, uint8_t raw_actor_8c,
                                 uint8_t difficulty,
                                 bool raw_actor_16_nonnegative);
void nba_gameplay_miss_offset(uint8_t index, bool left_basket,
                              int16_t *dx, int16_t *dy);
uint16_t nba_gameplay_shot_flight_duration(int16_t dx, int16_t dy);
void nba_gameplay_shot_launch(int32_t ball_x_fp, int32_t ball_y_fp,
                              int32_t ball_z_fp, int16_t target_x,
                              int16_t target_y, int16_t *velocity_x,
                              int16_t *velocity_y, int16_t *velocity_z);
int16_t nba_gameplay_arithmetic_shift_right(int16_t value, unsigned amount);
bool nba_gameplay_ball_coarse_contact(int16_t actor_x, int16_t actor_y,
                                      int16_t actor_z, int16_t ball_x,
                                      int16_t ball_y, int16_t ball_z,
                                      bool intended_receiver);
bool nba_gameplay_ball_pose_contact(const NbaGameplayPosePoint points[2],
                                    int16_t ball_x, int16_t ball_y,
                                    int16_t ball_z, uint8_t threshold);
int nba_gameplay_ball_pose_contact_index(
    const NbaGameplayPosePoint points[2], int16_t ball_x, int16_t ball_y,
    int16_t ball_z, uint8_t threshold);
void nba_gameplay_ball_apply_deflection(NbaGameplayRimState *state,
                                        NbaGameplayRng *rng);
NbaGameplayOwnedContactResult nba_gameplay_owned_contact_attempt(
    NbaGameplayRng *rng, uint8_t candidate_animation,
    uint8_t pose_point_index, uint8_t contact_rating_3a,
    uint16_t difficulty_raw_17af, uint16_t foul_rule_raw_17d1,
    bool foul_state_clear);
bool nba_gameplay_detached_shot_contact_attempt(
    NbaGameplayRng *rng, uint8_t pose_point_index,
    bool rim_context_nonzero);
bool nba_gameplay_ball_self_test(void);

#endif
