#ifndef NBA_PERIOD_FORMATION_H
#define NBA_PERIOD_FORMATION_H
#include "nba_period_restart_v2.h"
#include "nba_period_roles_v2.h"
#include "nba_controller.h"
#include "nba_assets.h"

/* Unique channel/pose/assignment fields not already owned by parent. */
typedef struct {
 uint16_t upper_queue_cursor,lower_queue_cursor,upper_queue[3],lower_queue[3];
 uint16_t upper_state,lower_state,base_state,upper_phase_target;
 uint16_t mirror_flags,upper_resource,lower_resource;
 uint16_t resolved_upper_state,resolved_lower_state,resolved_upper_phase,resolved_lower_phase;
 uint16_t alternate_lower,variant,catcher_latch;
 uint16_t current_assignment,base_assignment,alternate_assignment,help,role;
 uint16_t saved_mode,pair_direction,pair_distance,anchor_distance,depth;
} NbaPeriodFormationActorExtra;
typedef struct {
 uint16_t team,opponent_pointer,first_actor_pointer,roster[5];
 uint8_t selector[5],order[5];
} NbaPeriodFormationContext;
typedef struct {
 NbaPeriodRestartState parent; /* single owner for overlapping child words */
 NbaPeriodRestartInput input; /* period/tip plus the two canonical anchors */
 NbaPeriodFormationActorExtra actors[10];
 NbaPeriodFormationContext contexts[2];
 NbaControllerState controllers; /* sole owner of actor+16/context counts */
 uint32_t roster_table[2][12]; /* carried3471/34A1 addresses, validated */
 uint32_t active_roster_pointer[10];
 uint16_t statistic_pointer[10];
 /* D73E keys09DA..09EC, later BC07 latches: slots0=09DA,2=09DE,4=09E2.
  * Never keep duplicate nearest fields beside this shared canonical array. */
 uint16_t assignment_sort_slots[10];
 uint16_t ball_assignment,ball_alternate_assignment,ball_anchor_distance,ball_depth;
 int16_t extra_draw_x,extra_draw_y;uint16_t extra_draw_depth; /* record3FEB */
 uint16_t draw_order[12],leading_sentinel,camera_y,frame_low,frame_high;
 uint16_t delta,rng,previous_ball_x;
 int16_t predicted_x,predicted_y;
 uint16_t role_cadence,role_rebuild;
} NbaPeriodFormationState;
typedef enum {
 NBA_PERIOD_FORMATION_INVALID=0,NBA_PERIOD_FORMATION_CHECKPOINT,
 NBA_PERIOD_FORMATION_COMPLETE,NBA_PERIOD_FORMATION_REFUSED,
 NBA_PERIOD_FORMATION_ROLE_STOP
} NbaPeriodFormationKind;
typedef enum {
 NBA_PERIOD_FORMATION_NO_REFUSAL=0,NBA_PERIOD_FORMATION_APPEARANCE_DOMAIN,
 NBA_PERIOD_FORMATION_ASSIGNMENT_DOMAIN,NBA_PERIOD_FORMATION_SORT_DOMAIN,
 NBA_PERIOD_FORMATION_ATTACHMENT_DOMAIN,NBA_PERIOD_FORMATION_CONTROLLER_DOMAIN,
 NBA_PERIOD_FORMATION_ROLE_DOMAIN,NBA_PERIOD_FORMATION_ROLE_CARRIED_NEAREST,
 NBA_PERIOD_FORMATION_RENDER_DOMAIN
} NbaPeriodFormationRefusal;
typedef struct {
 NbaPeriodFormationKind kind;uint32_t source_pc;uint16_t actor;
 NbaPeriodFormationRefusal refusal;NbaPeriodRoleBoundaryV2 role;
} NbaPeriodFormationBoundary;
typedef struct {
 NbaPeriodRestart parent;unsigned phase;bool valid,waiting;
 NbaPeriodFormationBoundary boundary;
} NbaPeriodFormation;
/* DD97 entry only: earlier DCA6..DD97 caller has completed; no generic WRAM
 * or CPU state is accepted. M=X=D=0, DP0 are source caller preconditions.
 * All state is caller-owned current data. No native after-state callbacks.
 * Work/state/assets must not alias; caller preserves them while waiting.
 * Each child is projected from current canonical fields and committed only
 * on success. CPU/DP/stack/video timing is excluded. Unmodeled carried92
 * dependence refuses before roles instead of using a fabricated actor.
 * COMPLETE is E207. Unsupported role reads/assignment children stay stopped.
 * Parent actor fractions/A6/09BA/09B0/B2 quirks remain unchanged. */
bool nba_period_formation_begin(NbaPeriodFormation *,const NbaPeriodFormationState *);
NbaPeriodFormationBoundary nba_period_formation_advance(NbaPeriodFormation *,NbaPeriodFormationState *,const NbaAssetPack *);
bool nba_period_formation_resume(NbaPeriodFormation *);
#endif
