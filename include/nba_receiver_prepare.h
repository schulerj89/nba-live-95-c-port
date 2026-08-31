#ifndef NBA_RECEIVER_PREPARE_H
#define NBA_RECEIVER_PREPARE_H
#include "nba_assets.h"

/* $86:B468-B624. Caller owns pointer exchange, +60 and later mode14.
 * All words retain their original width. The profile/stat inputs are those
 * inherited through E0/C2; AF66 does not replace them with receiver data. */
typedef struct {
    uint16_t x_fraction, x, y_fraction, y;
    uint16_t axis_88, timer_60, modifier_b2, team_6e;
    uint16_t velocity_x, velocity_y, baseline_x_ba, baseline_y_bc;
    uint16_t magnitude_4c, speed_4a, flags_7e, facing_4e, movement_50;
    uint16_t selector_56, variant_58, upper_66;
} NbaReceiverPrepareActor;

typedef struct {
    uint16_t profile_word_39, stamina_word_18, alternate_word_012c;
    uint16_t hot_team_09c0, basket_x_fraction, basket_x;
} NbaReceiverPrepareInput;

typedef struct {
    NbaReceiverPrepareActor actor;
    uint16_t rng_07f6, attempt_0904, live_0936, timeout_091c;
    uint16_t p00, p02, p04, p06, p14, p18, p1a, p47, p49, p4f, p51;
    uint16_t aa, ac, ae, b0, b2, b4, b6, b8, ba, cc, ce, d0;
    uint16_t math_0806, math_0808, math_080a, math_080c;
    uint16_t math_080e, math_0810, sign_0824;
} NbaReceiverPrepareState;

/* Binary arithmetic / 16-bit A,X,Y contract. Refuses unsupported direction
 * or asset domains without changing state. Does not emulate CPU timing. */
bool nba_receiver_prepare(const NbaAssetPack *, const NbaReceiverPrepareInput *,
                          NbaReceiverPrepareState *);

/* $86:AF66 -> AFA3 (jump to AE10), after the real catch gate selected it.
 * Slot/pointer identities remain caller-owned; this receiver value is the
 * record selected by original DP8E before the temporary DP96/8E exchange. */
typedef struct {
    NbaReceiverPrepareState receiver;
    uint16_t passer_band_62, passer_flags_7e, receiver_mode_5e;
} NbaReceiverPassState;
bool nba_receiver_pass_prepare(const NbaAssetPack *,const NbaReceiverPrepareInput *,
                               NbaReceiverPassState *);
#endif
