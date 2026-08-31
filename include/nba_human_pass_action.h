#ifndef NBA_HUMAN_PASS_ACTION_H
#define NBA_HUMAN_PASS_ACTION_H

#include "nba_human_pass_init.h"

typedef struct {
    uint16_t z, velocity_z, boost, magnitude, family, pass_direction, flags;
} NbaHumanPassActionActor;

typedef struct {
    NbaHumanPassInitState common;
    NbaHumanPassActionActor extra[10];
    uint16_t passer_slot, receiver_slot;
    uint16_t distance_4f, coarse_be, relative_51;
    uint16_t request_00, descriptor_47, descriptor_bank_49;
    uint8_t profile_3e; /* native byte at [E6]+3E; input, never an output */
} NbaHumanPassActionState;

typedef enum {
    NBA_HUMAN_PASS_ACTION_OFFAXIS_AD0E,
    NBA_HUMAN_PASS_ACTION_NORMAL_ACA9,
    NBA_HUMAN_PASS_ACTION_BOOST_AFC4,
    NBA_HUMAN_PASS_ACTION_POSE_AF1D,
    NBA_HUMAN_PASS_ACTION_INVALID
} NbaHumanPassActionRoute;

/* $87:B47A child invoked by B00B, including its DP47/49 descriptor writes.
 * The shared semantic animation command owns channels; no pose is resolved. */
bool nba_human_pass_action_upper(const NbaAssetPack *assets,
                                NbaHumanPassActionState *state);

/* Complete $86:B00B-B04B grounded-special child, before its caller's AF1D. */
bool nba_human_pass_action_grounded(const NbaAssetPack *assets,
                                   NbaHumanPassActionState *state);

/* First AC50 through the side/back gate. The B00B child is closed here.
 * All other routes are exact unexecuted continuations: AD0E, ACA9 or AFC4.
 * No production dispatch or source-manifest registration is added. */
NbaHumanPassActionRoute nba_human_pass_action_select(const NbaAssetPack *assets,
                                                   NbaHumanPassActionState *state);

#endif
