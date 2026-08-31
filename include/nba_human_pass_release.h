#ifndef NBA_HUMAN_PASS_RELEASE_H
#define NBA_HUMAN_PASS_RELEASE_H
#include "nba_human_pass_pose.h"

typedef enum {
    NBA_PASS_RELEASE_INVALID=0,
    NBA_PASS_RELEASE_TIMER=1,
    NBA_PASS_RELEASE_NORMALIZED=2,
    NBA_PASS_RELEASE_ATTACHED=3,
    NBA_PASS_RELEASE_LAUNCH_99C4=4,
    NBA_PASS_RELEASE_CANCELLED=5,
    NBA_PASS_RELEASE_AIRBORNE_A629=6,
    NBA_PASS_RELEASE_POSE_A6F8=7,
    NBA_PASS_RELEASE_STEER_AD6B=8
} NbaHumanPassReleaseBoundary;

typedef struct {
    uint8_t thresholds_a7a0[8];
    uint16_t actor_pointers_9c7b[10];
} NbaHumanPassReleaseTables;

typedef struct {
    NbaHumanPassPoseState pose;
    uint16_t controller_16, timer_60, behavior_64, direction_66, group_6e, family_c0;
    uint16_t actor_index_c2, delta_c6, offense_093a, owner_093e;
    uint16_t source_0942, source_0944, receiver_0946, flag_09c4;
    uint16_t pointer_8e, bank_90, scratch_aa, scratch_ae;
} NbaHumanPassReleaseState;

/* 87:9244-9258, mode15 only. The ROM table resolves to wrapper87:9C53,
 * not directly86:A6B3. CPU stack frames remain the caller's responsibility. */
bool nba_human_pass_release_dispatch(NbaHumanPassReleaseState *);
/* 86:A7A8-A7D9, including its raw-difference-before-mask quirk. */
void nba_human_pass_release_turn(NbaHumanPassReleaseState *);
/* 86:9846-986C; group identity comes from actor+6E. */
void nba_human_pass_release_normalize(NbaHumanPassReleaseState *);
/* 86:A6B3 mode15. Binary16 domain, DP0/DBR7E. Returns at an explicit
 * original boundary; LAUNCH/AIRBORNE/POSE/STEER children are UNEXECUTED.
 * Invalid resource/table domain leaves the input unchanged. */
NbaHumanPassReleaseBoundary nba_human_pass_release_step(
    const NbaAssetPack *, const NbaHumanPassReleaseTables *, NbaHumanPassReleaseState *);
/* 86:A75F-A76F only; input must be actual post99C4 state. Does not launch. */
void nba_human_pass_release_after_launch(NbaHumanPassReleaseState *);
#endif
