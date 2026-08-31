#ifndef NBA_PERIOD_APPEARANCE_H
#define NBA_PERIOD_APPEARANCE_H
#include "nba_player_lab.h"
typedef struct {
    NbaPlayerAnimationChannels channels;
    uint16_t owner, controller, velocity_x, velocity_y, z, boost, speed;
    uint16_t direction, display_direction, status, alternate_lower, variant;
    uint16_t catcher_latch, delta, rng, upper_resource, lower_resource, owner_pointer;
} NbaPeriodAppearance;
/* $87:AAB2 period CPU-actor projection. Excludes CPU/DP/register residue,
 * human palette updates and source domains not entered by the period parent.
 * Native snapshots are diagnostic inputs only; this API owns no raw memory. */
bool nba_period_appearance(const NbaAssetPack *assets, NbaPeriodAppearance *state);
#endif
