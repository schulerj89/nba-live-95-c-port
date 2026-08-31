#ifndef NBA_HUMAN_PASS_INIT_H
#define NBA_HUMAN_PASS_INIT_H

#include "nba_human_pass.h"
#include "nba_player_lab.h"

/* Native AB2D caller state, deliberately independent of the scene's partially
 * translated pass initializer. No owner/controller reassignment occurs here. */
typedef struct {
    uint16_t identity; /* actor +00 indexes $7E:3449 four-byte descriptors */
    int16_t x, y, velocity_x, velocity_y;
    uint16_t movement_direction, mode, timer, pass_band;
    NbaPlayerAnimationChannels animation;
} NbaHumanPassInitActor;

typedef struct {
    NbaHumanPassInitActor actors[10];
    uint16_t profile_pointers[10][2]; /* full words; +2 includes bank/padding */
    uint16_t live_0936, passer_0942, receiver_0946, active_09c4;
    uint16_t inbound_transfer_09b8, distance_09da, controller_tag_0944;
} NbaHumanPassInitState;

typedef struct {
    uint16_t profile_lo_e6, profile_hi_e8, receiver_slot;
} NbaHumanPassInitPrefix;

typedef struct {
    int16_t dx, dy;
    uint16_t fine_direction, coarse_direction, distance, band, relative;
} NbaHumanPassInitGeometry;

/* $86:AB2D -> AB83, including the complete $87:B538 cancel child.
 * Caller-saved DP stack words are the eventual AF4D-AF65 continuation's job.
 * False is host input validation, not an invented native branch. */
bool nba_human_pass_init_prefix(NbaHumanPassInitState *state,
    uint16_t passer_slot, uint16_t passer_identity, uint16_t receiver_slot,
    uint16_t receiver_identity, NbaHumanPassInitPrefix *result);

/* $86:AB83 -> first AC50, including existing complete $85:F3C3 geometry.
 * The AC50 branch, catch-preinit, action installation, pose/attachment, mode15
 * commit and original stack restoration remain unexecuted continuations. */
bool nba_human_pass_init_geometry(NbaHumanPassInitState *state,
    uint16_t passer_slot, uint16_t receiver_slot,
    NbaHumanPassInitGeometry *result);

/* Bounded ordinary B-pass chain: DF7A receiver selection -> AB2D prefix ->
 * first AC50. NO_RECEIVER returns before initializer mutations. This API is
 * not registered in normal gameplay until the remaining children close. */
NbaHumanPassRoute nba_human_pass_prepare(const NbaHumanPassInput *selection,
    uint16_t passer_identity_c2, NbaHumanPassInitState *state,
    NbaHumanPassSelection *selected,
    NbaHumanPassInitPrefix *prefix, NbaHumanPassInitGeometry *geometry);

#endif
