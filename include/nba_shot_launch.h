#ifndef NBA_SHOT_LAUNCH_H
#define NBA_SHOT_LAUNCH_H
#include "nba_shot_action.h"
#include "nba_gameplay_ai.h"

/* Native launch inputs; references name WRAM/roster offsets, not host rules. */
typedef struct {
    int16_t actor_x, actor_y, controller, basket_x, origin_x, origin_y;
    uint16_t basket_fraction, team_group, distance_8c, defense_8a, movement_4c;
    uint16_t modifier_b2, stamina_18, difficulty, shot_control_17c3;
    uint16_t shot_assistance_17bf, hot_team_09c0, free_throw_0978;
    uint16_t aim_0982, power_0980, clock_0928, period_0926, assist_clock_47;
    uint16_t roster_low, roster_bank;
    uint8_t rating_two, rating_three, rating_free, range_49;
    bool special_entry, boosted, alternate_lower;
} NbaShotLaunchInput;

typedef struct {
    NbaShotAction actor;
    uint16_t facing, contact_inhibit;
    uint16_t x_fraction, x, y_fraction, y, z_fraction, z;
    int16_t velocity_x, velocity_y, velocity_z;
    uint16_t owner, last_owner, display_shooter, attempt_latch;
    uint16_t dead_0966, height_0968, dead_096c, bounce_0920, inner_veto;
    uint16_t live_state, timeout_0930, value, display_value, initial_value;
    uint16_t roster_low, roster_bank, ball_record, assist_43, assist_45;
    uint16_t player_stats[5], controller_stats[5];
    NbaGameplayRng rng;
    /* Diagnostics only; do not use these to force a made basket. */
    uint16_t chance, miss_index;
} NbaShotLaunchState;

bool nba_shot_launch(const NbaAssetPack *assets, const NbaShotLaunchInput *input,
                      NbaShotLaunchState *state);
#endif
