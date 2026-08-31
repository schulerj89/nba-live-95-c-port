#ifndef NBA_HUMAN_PASS_H
#define NBA_HUMAN_PASS_H

#include <stdint.h>

typedef struct {
    int16_t x,y;
    uint16_t mode,anchor_distance_8c;
} NbaHumanPassActor;
typedef struct {
    NbaHumanPassActor actors[10];
    uint16_t actor,group_first,context_group,direction,controller_id_090e;
} NbaHumanPassInput;
typedef enum {
    NBA_HUMAN_PASS_NO_RECEIVER,
    NBA_HUMAN_PASS_CONTINUE_INITIALIZER,
    NBA_HUMAN_PASS_INVALID
} NbaHumanPassRoute;
typedef struct {
    NbaHumanPassRoute route;
    uint16_t controller_tag_0944,receiver_slot,receiver_identity,score;
} NbaHumanPassSelection;

/* $85:F1C1-$F228 distance result, including its near-diagonal weighting and
 * wrapped16-bit comparisons. This differs from the F34F direction metric. */
uint16_t nba_human_pass_distance(int16_t dx,int16_t dy);

/* $84:DF7A selection through either the actual $86:AB2D call boundary or
 * $84:E09C with no receiver. This does NOT execute the AB2D initializer,
 * its animation/ownership changes or the DF7A epilogue after that child.
 * No normal human-play integration is enabled by this module. */
NbaHumanPassSelection nba_human_pass_select(const NbaHumanPassInput *input);

#endif
