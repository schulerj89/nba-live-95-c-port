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
uint16_t nba_gameplay_reaction_threshold(NbaGameplayRng *rng,
                                         int16_t actor_x, int16_t actor_y,
                                         int16_t ball_x, int16_t ball_y);
uint32_t nba_gameplay_behavior_routine(uint8_t mode);

#endif
