#ifndef NBA_PERIOD_ROLES_H
#define NBA_PERIOD_ROLES_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t x,y; /* integer words +04/+08; no rounded fraction */
    uint16_t assignment_74,direction_86,pair_distance_8a,focal_distance_8e;
} NbaPeriodRoleActor;
typedef struct { uint16_t opponent_02,first_actor_04; } NbaPeriodRoleContext;
typedef struct {
    NbaPeriodRoleActor actors[10];
    NbaPeriodRoleContext contexts[2]; /* source records46EB,476B */
    int16_t ball_x,ball_y; /* record3EEB +04/+08 */
    uint16_t ball_pointer_0910;
    int16_t predicted_x_0918,predicted_y_091a;
    uint16_t camera_093a,owner_093e,cadence_09d2,rebuild_09d6,nearest_09da;
    uint16_t delta_00c6;
    /* Carried source scratch, including92 when no distance wins the scan. */
    uint16_t object_008e,nearest_0092,actor_0096,pair_009a,context_009e;
    uint16_t entry_context_00a6,dx_00aa,dy_00ae,direction_00b2;
    uint16_t focal_x_00b6,focal_y_00ba,best_00be,counter_00c2;
    uint16_t ready_09ba,dead_x_09b0,dead_y_09b2; /* explicitly preserved */
} NbaPeriodRoleState;

typedef enum {
    NBA_PERIOD_ROLES_INVALID=0,
    NBA_PERIOD_ROLES_FIRST_RETURN, /* E1EE, firstBC07 returned atBD06 */
    NBA_PERIOD_ROLES_COMPLETE,     /* E1F7, before excludedD5DB sort */
    NBA_PERIOD_ROLES_REBUILD,      /* BD0D, after9D2=30;9D6stillcarried */
    NBA_PERIOD_ROLES_PLANNER       /* BE06, aftercadencewrap; remainderexcluded */
} NbaPeriodRoleKind;
typedef struct {
    NbaPeriodRoleKind kind;
    uint32_t source_pc;
    unsigned completed_calls;
} NbaPeriodRoleBoundary;
typedef struct {
    unsigned calls;
    bool valid,waiting;
    NbaPeriodRoleBoundary boundary;
} NbaPeriodRoles;

/* Original source domain M=X=D=0, DP=0. Context links and actor bases are
 * the initialized46EB/476B records. Nonnegative assignments are even0..18;
 * other table indexes require actor records outside this bounded projection.
 * With nonnegative owner,0910mustpoint to the represented ball record3EEB.
 * No fixed cadence, delta, camera, rebuild or initial-nearest value is assumed.
 * State must come from current gameplay ownership, not native snapshots.
 * Keep state/work unchanged between FIRST_RETURN and resume; the original
 * paired caller has no intervening external operation. Storage must not alias. */
bool nba_period_roles_begin(NbaPeriodRoles *work,const NbaPeriodRoleState *state);
NbaPeriodRoleBoundary nba_period_roles_advance(NbaPeriodRoles *work,NbaPeriodRoleState *state);
/* Only FIRST_RETURN resumes. REBUILD/PLANNER are unresolved terminal source
 * boundaries; this helper never clears9D6 or skips their required work. */
bool nba_period_roles_resume(NbaPeriodRoles *work);
#endif
