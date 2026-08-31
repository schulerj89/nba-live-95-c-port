#ifndef NBA_HUMAN_SWITCH_H
#define NBA_HUMAN_SWITCH_H

#include "nba_controller.h"

typedef struct {
    int16_t x, y;
    uint16_t group;
} NbaHumanSwitchActor;

/* Semantic inputs at $84:E141, after the caller has published controller
 * input. The neutral anchor comes from $0910, not necessarily owner093E.
 * fallback_actor represents incoming DP A6; native leaves it unseeded when
 * no candidate improves the initial $0640 score. It is never replaced by
 * an arbitrary owner or current actor by this implementation. */
typedef struct {
    NbaControllerState controllers;
    NbaHumanSwitchActor actors[10];
    uint16_t actor, group_first, group_end, controller_090c, direction;
    uint16_t context_controller_count, fallback_actor, incoming_index_c2;
    int16_t neutral_anchor_x, neutral_anchor_y;
} NbaHumanSwitchState;

typedef enum {
    NBA_HUMAN_SWITCH_ALL_CONTROLLED,
    NBA_HUMAN_SWITCH_RETAIN,
    NBA_HUMAN_SWITCH_TRANSFER,
    NBA_HUMAN_SWITCH_INVALID
} NbaHumanSwitchRoute;

/* Full $84:E141-$E2AB persistent effects. Does not change ball ownership,
 * processed latches, allocation cursors, input publication or CPU behavior.
 * The directional helper is the existing native $85:F34F translation.
 * Deliberately separate from the frozen human dispatch stage and not yet
 * included in the normal production source manifest. */
NbaHumanSwitchRoute nba_human_switch_control(NbaHumanSwitchState *state);

#endif
