#ifndef NBA_PERIOD_SUPPORT_H
#define NBA_PERIOD_SUPPORT_H
#include "nba_player_lab.h"
#include "nba_controller.h"

typedef struct {
    uint8_t team[2], roster[10], selector[10];
} NbaPeriodAssignmentInput;
typedef struct {
    uint16_t variant, current, base, alternate, help, role;
} NbaPeriodAssignmentActor;
typedef struct {
    NbaPeriodAssignmentActor actor[10];
    uint32_t roster_pointer[10];
    uint16_t statistic_pointer[10], keys[10];
    uint8_t order[2][5];
} NbaPeriodAssignment;
/* D85E through DA17 data projection, including D7B8 and D73E. The lineup
 * selectors must be permutations on each side; CPU/DP/stack excluded. */
bool nba_period_assignment(const NbaAssetPack *assets,
    const NbaPeriodAssignmentInput *input, NbaPeriodAssignment *state);

typedef struct {
    int16_t x[11];
    uint16_t link[11], object[12];
} NbaPeriodObjectSort;
/* D5DB sorts integer X with the original wrapped-sign CMP. */
bool nba_period_object_sort(NbaPeriodObjectSort *state);

typedef struct {
    NbaControllerState controllers;
    NbaPlayerAnimationChannels channels;
    NbaPlayerResolvedPose pose;
    uint16_t actor, owner, group, facing, alternate_lower, variant, boost;
    int16_t x, y, z, ball_x, ball_y, ball_z;
    uint16_t previous_ball_x, scratch_47;
} NbaPeriodAttachment;
/* E183-E1A4: actual BC9B, cancel/install, AEC3, B649, B66A children in order.
 * Input is the period parent's current state, not a native poststate. */
bool nba_period_attachment(const NbaAssetPack *assets, NbaPeriodAttachment *state);
#endif
