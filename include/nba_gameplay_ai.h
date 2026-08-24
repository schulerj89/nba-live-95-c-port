#ifndef NBA_GAMEPLAY_AI_H
#define NBA_GAMEPLAY_AI_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t state;
} NbaGameplayRng;

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
    uint8_t direction;
    uint8_t play_code;
    bool play_requested;
} NbaGameplayInboundTarget;

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
bool nba_gameplay_decision_timer_step(uint16_t *timer, uint8_t profile_byte,
                                      uint16_t reload_base,
                                      bool add_half_court_delay);
bool nba_gameplay_same_x_half(int16_t actor_x, int16_t context_anchor_x);
uint8_t nba_gameplay_target_direction(int16_t dx, int16_t dy,
                                      uint16_t *distance);
uint8_t nba_gameplay_pass_direction(int16_t dx, int16_t dy,
                                    uint16_t *distance);
bool nba_gameplay_receiver_candidate_valid(
    uint8_t passer_actor, uint8_t candidate_actor,
    const NbaGameplayReceiverState *actors, uint8_t actor_count);
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
