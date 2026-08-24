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
    uint16_t raw_1866;
    uint16_t raw_07f6;
    uint16_t effect_raw_401b;
} NbaGameplayRimContext;

uint16_t nba_gameplay_hoop_distance(int16_t dx, int16_t dy);
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
bool nba_gameplay_ball_self_test(void);

#endif
