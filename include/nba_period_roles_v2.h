#ifndef NBA_PERIOD_ROLES_V2_H
#define NBA_PERIOD_ROLES_V2_H
#include "nba_period_roles.h"
/* Additional carried fields used after BD0D/BE06. No raw WRAM replacement. */
typedef struct {
 uint16_t id_00,mode_5e,reaction_60,behavior_64,team_6e,boost_72;
 uint16_t base_76,alternate_78,clear_7e,saved_84,anchor_distance_8c;
} NbaPeriodRoleExtraActor;
typedef struct {uint16_t anchor_0a,order_49[5];} NbaPeriodRoleExtraContext;
typedef struct {
 NbaPeriodRoleState prefix;
 NbaPeriodRoleExtraActor actors[10];
 NbaPeriodRoleExtraContext contexts[2];
 uint16_t ball_assignment_74,ball_alternate_78,ball_anchor_distance_8c;
 uint16_t live_0936,receiver_0946,side_0952,inbound_0954,rng_07f6;
 uint16_t nearest_offense_09de,nearest_anchor_09e2;
} NbaPeriodRoleStateV2;
typedef enum {
 NBA_PERIOD_ROLES_V2_INVALID=0,NBA_PERIOD_ROLES_V2_FIRST_RETURN,
 NBA_PERIOD_ROLES_V2_COMPLETE,NBA_PERIOD_ROLES_V2_RECORD_READ,
 NBA_PERIOD_ROLES_V2_ASSIGNMENT_CHILD
} NbaPeriodRoleKindV2;
typedef struct {
 NbaPeriodRoleKindV2 kind; uint32_t source_pc;
 unsigned completed_calls; uint16_t record_pointer;
} NbaPeriodRoleBoundaryV2;
typedef struct {
 NbaPeriodRoles prefix; bool valid,waiting; NbaPeriodRoleBoundaryV2 boundary;
} NbaPeriodRolesV2;
/* M=X=D=0, DP=0; live81/82; canonical actor IDs/teams, cross-context
 * reciprocal bijections in current/base/alternate assignment sets, and each
 * context's five order bytes is a permutation of the other context's IDs*2.
 * Ball pointer0910 is3EEB. All cadence/rebuild/RNG/owner/camera words carried.
 * This narrow period domain does not implement live-play help/assignment
 * repair. Unsupported indirect record reads stop BEFORE the read; assignment
 * children stop BEFORE their JSL (source_pc is that call site). No result or
 * elapsed-time adapter exists. State/work remain unchanged while waiting.
 * Use ordinary owned gameplay state in production, never captured snapshots. */
bool nba_period_roles_v2_begin(NbaPeriodRolesV2 *,const NbaPeriodRoleStateV2 *);
NbaPeriodRoleBoundaryV2 nba_period_roles_v2_advance(NbaPeriodRolesV2 *,NbaPeriodRoleStateV2 *);
bool nba_period_roles_v2_resume(NbaPeriodRolesV2 *);
#endif
