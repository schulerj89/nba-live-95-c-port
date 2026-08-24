#ifndef NBA_GAMEPLAY_AI_H
#define NBA_GAMEPLAY_AI_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t state;
} NbaGameplayRng;

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
bool nba_gameplay_decision_timer_step(uint16_t *timer, uint8_t profile_byte,
                                      uint16_t reload_base,
                                      bool add_half_court_delay);
uint8_t nba_gameplay_target_direction(int16_t dx, int16_t dy,
                                      uint16_t *distance);
uint8_t nba_gameplay_pass_direction(int16_t dx, int16_t dy,
                                    uint16_t *distance);
bool nba_gameplay_predictive_arrival(int16_t actor_x, int16_t actor_y,
                                     int16_t velocity_x, int16_t velocity_y,
                                     int16_t target_x, int16_t target_y,
                                     uint16_t tolerance,
                                     uint8_t *steering_direction,
                                     uint16_t *distance);
void nba_gameplay_velocity_step(int16_t *velocity_x, int16_t *velocity_y,
                                uint16_t *boost_timer, uint8_t direction,
                                uint8_t profile_42, uint16_t dispatch_dt,
                                bool movement_blocked,
                                int16_t global_093e);

#endif
