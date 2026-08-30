#ifndef NBA_GAMEPLAY_FREE_THROW_H
#define NBA_GAMEPLAY_FREE_THROW_H

#include "nba_gameplay_ai.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t state_raw_0978;
    uint16_t attempts_raw_097a;
    int16_t whistle_timer_raw_08de;
    uint16_t audio_raw_08e6;
    uint16_t audio_mirror_raw_08e8;
    uint16_t upload_raw_180b;
    uint16_t upload_raw_180c;
    uint16_t ball_z_raw_3ef7;
    int16_t ball_x_raw_3eef;
    int16_t ball_y_raw_3ef3;
    int16_t ball_vx_raw_3ef9;
    int16_t ball_vy_raw_3efb;
    int16_t ball_vz_raw_3efd;
} NbaGameplayFreeThrowCompletion;

/* `$87:9E39-$9F11` and `$87:A018-$A045`: controller-owned two-press
 * free-throw aim.  The first B/Y press locks one axis, releasing the button
 * advances state 4 to state 5, and the second press commits state 9. */
typedef struct {
    uint16_t state_raw_0978;
    uint16_t aim_x_raw_0980;
    uint16_t aim_y_raw_0982;
    uint16_t accumulator_raw_0984;
    uint16_t step_raw_0986;
    int16_t controller_assignment_raw_16;
    uint16_t human_context_raw_3b;
    bool shoot_held;
} NbaGameplayHumanFreeThrowAim;

typedef enum {
    NBA_HUMAN_FREE_THROW_WAIT,
    NBA_HUMAN_FREE_THROW_FIRST_LOCK,
    NBA_HUMAN_FREE_THROW_RELEASED_FIRST,
    NBA_HUMAN_FREE_THROW_LAUNCH,
    NBA_HUMAN_FREE_THROW_CPU_FALLBACK
} NbaGameplayHumanFreeThrowResult;

uint8_t nba_gameplay_free_throw_threshold(uint8_t rating);
uint16_t nba_gameplay_free_throw_human_aim_step(uint8_t rating);
void nba_gameplay_free_throw_human_aim_begin(
    NbaGameplayHumanFreeThrowAim *state, uint8_t rating);
NbaGameplayHumanFreeThrowResult nba_gameplay_free_throw_human_aim_step_frame(
    NbaGameplayHumanFreeThrowAim *state);
bool nba_gameplay_free_throw_cpu_aim_step(
    NbaGameplayFreeThrowCompletion *state, NbaGameplayRng *rng,
    uint16_t elapsed, uint8_t rating, uint16_t *aim_x_raw_0980,
    uint16_t *aim_y_raw_0982);
bool nba_gameplay_free_throw_presentation_gate(
    NbaGameplayFreeThrowCompletion *state, bool clock_changed,
    uint16_t native_audio_word);
bool nba_gameplay_free_throw_release_complete(
    NbaGameplayFreeThrowCompletion *state);
bool nba_gameplay_free_throw_resolution_step(
    NbaGameplayFreeThrowCompletion *state, bool shooter_pass,
    bool ownerless, uint16_t shot_value_raw_094c,
    uint16_t rim_raw_097c, uint16_t resolution_raw_0972,
    int16_t shooter_x, int16_t shooter_y);

#endif
