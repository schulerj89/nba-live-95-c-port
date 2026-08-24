#ifndef NBA_GAMEPLAY_EFFECT_H
#define NBA_GAMEPLAY_EFFECT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t gate_raw_3f33;
    uint16_t resource_raw_4015;
    uint16_t effect_raw_401b;
    uint16_t frame_raw_4025;
    uint16_t timer_raw_402d;
    int16_t reference_y_raw_3ff3;
} NbaGameplayEffectState;

void nba_gameplay_effect_init(NbaGameplayEffectState *state);
bool nba_gameplay_effect_start(NbaGameplayEffectState *state,
                               uint16_t effect_id);
void nba_gameplay_effect_step(NbaGameplayEffectState *state,
                              int16_t ball_y, int16_t ball_z,
                              int16_t velocity_z, uint16_t dt);
bool nba_gameplay_effect_self_test(void);

#endif
