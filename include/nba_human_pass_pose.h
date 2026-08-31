#ifndef NBA_HUMAN_PASS_POSE_H
#define NBA_HUMAN_PASS_POSE_H
#include "nba_assets.h"

typedef struct {
    uint16_t x, y, z, flags_28, upper_2a, lower_2c;
    uint16_t upper_30, lower_32, previous_upper_34, previous_lower_36;
    uint16_t phase_3a, phase_3c, previous_phase_3e, previous_phase_40;
    uint16_t facing_4e, resolved_facing_52, mode_5e, variant_6c;
    uint16_t flags_7e, alternate_lower_a8;
} NbaHumanPassPoseActor;

typedef struct {
    NbaHumanPassPoseActor actor;
    uint16_t ball_x, ball_y, ball_z, previous_ball_x_0922, live_0936;
    uint16_t scratch_00, scratch_02, scratch_04, scratch_06;
    uint16_t pointer_47, bank_49, direction_index_ac;
} NbaHumanPassPoseState;

/* Valid native table domain: states0..56, facing0..8, literal phase lookups
 * inside bank84; attachment resource indices0..82F. Invalid asset/input
 * addresses fail instead of fabricating a resource or emulating bank wrap. */
bool nba_human_pass_pose_resolve(const NbaAssetPack *, NbaHumanPassPoseState *);
/* Complete B832 result, including source word-width mirror/shift behavior.
 * scratch00=0 selects point0; any nonzero word selects point1. */
bool nba_human_pass_pose_offset(const NbaAssetPack *, NbaHumanPassPoseState *);
/* B649-B669 writes previous X, ball X/Y and scratch offsets; not ball Z. */
bool nba_human_pass_pose_attach(const NbaAssetPack *, NbaHumanPassPoseState *);
/* AF1D-AF30: resolver, attachment, then actor-relative ball Z. */
bool nba_human_pass_pose_prefix(const NbaAssetPack *, NbaHumanPassPoseState *);
/* AF30-AF4D state commit only; the stack epilogue is unexecuted. */
bool nba_human_pass_pose_commit(NbaHumanPassPoseState *);
bool nba_human_pass_pose_prepare(const NbaAssetPack *, NbaHumanPassPoseState *);
#endif
