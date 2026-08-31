#ifndef NBA_HUMAN_PASS_ALIGNED_H
#define NBA_HUMAN_PASS_ALIGNED_H

#include "nba_human_pass_action.h"

typedef struct { uint16_t x, y, team; } NbaHumanPassLaneActor;
typedef struct {
    NbaHumanPassLaneActor actors[11]; /* slot10 is the ball, skipped by F473 */
    uint16_t order[13]; /* $34D1..34E9: slot0..10 or FFFF for native null */
    uint16_t source_slot, receiver_slot, source_cursor, receiver_cursor;
} NbaHumanPassLaneInput;

/* Complete $85:F473-F5E3 result. Preserve native sorted-list order and early
 * stops; this is not an unordered search of every opposing actor. The native
 * saved DP words/persistent records are read-only to this semantic helper. */
bool nba_human_pass_lane_obstructed(const NbaHumanPassLaneInput *input,
                                   uint16_t *result_aa);

typedef struct {
    NbaHumanPassActionState action;
    NbaHumanPassLaneInput lane;
    uint16_t fine_c0, band_b2, scratch_aa, family_ae;
    uint16_t receiver_anchor_8c, passer_anchor_8a;
    uint16_t options_07f6, layout_0956;
} NbaHumanPassAlignedState;

typedef enum {
    NBA_HUMAN_PASS_ALIGNED_CATCH_AD3D,
    NBA_HUMAN_PASS_ALIGNED_POSE_AF1D,
    NBA_HUMAN_PASS_ALIGNED_BOTH_B3BD,
    NBA_HUMAN_PASS_ALIGNED_COMMIT_AF30,
    NBA_HUMAN_PASS_ALIGNED_INVALID
} NbaHumanPassAlignedRoute;

/* AE10 -> AED9: choose upper request/family, including the F473 child. */
bool nba_human_pass_aligned_choose(NbaHumanPassAlignedState *state);
/* AED9 -> upper child -> AF1D, or unexecuted B3BD/AF30 continuation. */
NbaHumanPassAlignedRoute nba_human_pass_aligned_install(
    const NbaAssetPack *assets, NbaHumanPassAlignedState *state);
/* AD0E -> the four early catch gates -> choose/install. AD3D itself and
 * the catch, both-channel, pose/attachment and final commit remain gated. */
NbaHumanPassAlignedRoute nba_human_pass_aligned_prepare(
    const NbaAssetPack *assets, NbaHumanPassAlignedState *state);

#endif
