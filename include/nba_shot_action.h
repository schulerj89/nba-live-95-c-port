#ifndef NBA_SHOT_ACTION_H
#define NBA_SHOT_ACTION_H
#include "nba_player_lab.h"

typedef struct {
    NbaPlayerAnimationChannels animation;
    int16_t velocity_x, velocity_y, velocity_z;
    uint16_t speed, mode, flags, timer, status, behavior_timer;
    uint16_t activity, bounce_count, bounce_timer;
} NbaShotAction;

bool nba_shot_action_start(const NbaAssetPack *assets, NbaShotAction *state,
                           bool boosted, bool alternate_lower);
bool nba_shot_action_jump(const NbaAssetPack *assets, NbaShotAction *state,
                          bool free_throw, bool alternate_lower);
void nba_shot_action_restore(NbaShotAction *state, uint16_t team_group,
                             uint16_t active_group);
void nba_shot_action_clear(NbaShotAction *state);

typedef enum { NBA_SHOT_WAIT, NBA_SHOT_RELEASE, NBA_SHOT_GROUNDED } NbaShotGate;
typedef struct {
    int16_t x, y, z, velocity_z, controller;
    uint16_t lower_accumulator, free_throw, rng, buttons, facing;
    int16_t basket_x;
} NbaShotGateInput;
NbaShotGate nba_shot_action_gate(NbaShotGateInput *state);
typedef enum { NBA_SHOT_DELAY, NBA_SHOT_JUMP, NBA_SHOT_AIRBORNE } NbaShotStage;
NbaShotStage nba_shot_action_delay(uint16_t *activity, uint16_t delta,
                                  bool free_throw);

typedef struct {
    int16_t x, y, basket_x;
    uint16_t movement, anchor_distance, free_throw, rng;
} NbaShotSidestepInput;
bool nba_shot_action_sidestep(NbaShotAction *state,
                              const NbaShotSidestepInput *input);
uint16_t nba_shot_action_release_facing(int16_t x, int16_t y, int16_t basket_x);

typedef struct {
    uint16_t live_state, ball_z, attachment_state, height_latch;
    int16_t ball_velocity_z;
} NbaShotCancelBall;
bool nba_shot_action_cancel(const NbaAssetPack *assets, NbaShotAction *state,
                            NbaShotCancelBall *ball, bool alternate_lower);
void nba_shot_action_windup_button(NbaShotAction *state, int16_t controller,
                                   uint16_t free_throw, uint16_t buttons);
typedef enum {
    NBA_SHOT_CONTINUE, NBA_SHOT_LOST_OWNER, NBA_SHOT_PUMP_WAIT,
    NBA_SHOT_PUMP_CANCEL
} NbaShotOwnerGate;
NbaShotOwnerGate nba_shot_action_owner_gate(const NbaShotAction *state,
                                            bool owns_ball);
#endif
