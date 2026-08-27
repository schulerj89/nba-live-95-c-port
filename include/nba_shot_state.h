#ifndef NBA_SHOT_STATE_H
#define NBA_SHOT_STATE_H
#include "nba_assets.h"

typedef struct {
    uint16_t made_run[10], defensive_run[10], team_group[10];
    uint16_t assistance_team, owner;
} NbaShotMomentum;

typedef struct {
    uint16_t stamina[24], playing_seconds[24];
    uint16_t active_roster[10], boost[10], rating[24];
    uint16_t timer, live_state, enabled, quarter;
} NbaShotFatigue;

typedef struct {
    uint16_t live_state, period, clock, shot_clock, shot_clock_mirror;
    uint16_t dead_clock_enabled, fatigue_timer, flight_timer;
    uint16_t shot_clock_enabled, elapsed_clock, elapsed_shot_clock;
} NbaShotClock;

bool nba_shot_state_assets_valid(const NbaAssetPack *assets);
bool nba_shot_momentum_make(NbaShotMomentum *state,uint16_t shooter,
    uint16_t assistance_enabled,uint16_t clock,uint16_t left_score,uint16_t right_score);
void nba_shot_momentum_reset(NbaShotMomentum *state);
void nba_shot_stamina_init(NbaShotFatigue *state);
void nba_shot_fatigue_timer_init(NbaShotFatigue *state);
void nba_shot_stamina_grant(NbaShotFatigue *state,uint16_t amount);
void nba_shot_stamina_fixed_grant(NbaShotFatigue *state);
bool nba_shot_stamina_recover(const NbaAssetPack *assets,NbaShotFatigue *state);
bool nba_shot_fatigue_step(const NbaAssetPack *assets,NbaShotFatigue *state);
void nba_shot_clock_step(NbaShotClock *state);
#endif
