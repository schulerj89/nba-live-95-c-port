#ifndef NBA_OWNER_FLOW_H
#define NBA_OWNER_FLOW_H
#include <stdint.h>
#include <stdbool.h>

/* Literal persistent words at the boundary of $86:F34F-$F439. CPU/pose
 * callees own additional actor/ball state in their runtime context. */
typedef struct {
    uint16_t actor_id, owner_093e, veto_09f8, shooting_09bc, deferred_0a02;
    uint16_t team_6e, owner_team_09f4, play_0996, inbound_actor_0954;
    uint16_t transfer_09b8, request_0994, dead_0968, attached_09f6;
    uint16_t vx_0e, vy_10, facing_4e, pair_74, live_0936, offense_093a;
    uint16_t timer_60, delta_c8, mode_5e, behavior_64, flags_7e;
    uint16_t controller_16, recovery_7a, rating_3f;
} NbaOwnerFlow;

typedef enum {
    NBA_OWNER_CALL_POSE, NBA_OWNER_CALL_CPU,
    NBA_OWNER_CALL_FORMATION, NBA_OWNER_CALL_RECEIVER
} NbaOwnerCall;
typedef enum {
    NBA_OWNER_FLOW_RETURN, NBA_OWNER_FLOW_HOLD, NBA_OWNER_FLOW_LOST,
    NBA_OWNER_FLOW_INBOUND, NBA_OWNER_FLOW_ESCAPED, NBA_OWNER_FLOW_INVALID
} NbaOwnerFlowResult;
/* Callee writes are committed to state before the next caller instruction.
 * CPU false means its native nonlocal return bypasses F42C-F439. */
typedef bool (*NbaOwnerFlowCall)(void *context,NbaOwnerFlow *state,
                                NbaOwnerCall call,unsigned paired_actor);
NbaOwnerFlowResult nba_owner_flow_run(NbaOwnerFlow *state,
                                     NbaOwnerFlowCall call,void *context);
#endif
