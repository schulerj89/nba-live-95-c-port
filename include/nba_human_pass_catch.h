#ifndef NBA_HUMAN_PASS_CATCH_H
#define NBA_HUMAN_PASS_CATCH_H
#include "nba_types.h"

typedef struct {
    uint16_t x, y, team, order_cursor; /* cursor1..11 in original34D1 list */
    uint16_t mode, timer, flags, pass_band, axis_88;
} NbaHumanPassCatchActor;
typedef struct {
    NbaHumanPassCatchActor actors[11];
    uint16_t order[13]; /* slot0..10, or FFFF for original zero sentinel */
    uint16_t source_slot, receiver_slot, basket_x;
    /* Actual [00]+42 and [E0]+39 words, never assumed roster identities. */
    uint16_t indirect_word_42, profile_word_39;
    uint16_t rng_07f6, attempt_0904;
    uint16_t aa, ae, ac, b2, b6, ba, be, candidate_92;
} NbaHumanPassCatchState;
typedef enum {
    NBA_HUMAN_PASS_CATCH_AE10,
    NBA_HUMAN_PASS_CATCH_AF66,
    NBA_HUMAN_PASS_CATCH_B468,
    NBA_HUMAN_PASS_CATCH_INVALID
} NbaHumanPassCatchRoute;
bool nba_human_pass_catch_geometry(NbaHumanPassCatchState *); /* AD3D -> AD98 */
bool nba_human_pass_catch_rng(NbaHumanPassCatchState *);      /* CEE7 -> RTL */
bool nba_human_pass_catch_direction(NbaHumanPassCatchState *);/* F02D, not F34F */
bool nba_human_pass_catch_lane(NbaHumanPassCatchState *);     /* F5E4 -> F727 */
NbaHumanPassCatchRoute nba_human_pass_catch_attempt(NbaHumanPassCatchState *); /* AD98 -> AE10/AF66 */
NbaHumanPassCatchRoute nba_human_pass_catch_receiver(NbaHumanPassCatchState *);/* AF66 -> B468 unexecuted */
NbaHumanPassCatchRoute nba_human_pass_catch_prepare(NbaHumanPassCatchState *); /* AD3D -> AE10/B468 */
#endif
