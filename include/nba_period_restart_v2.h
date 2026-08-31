#ifndef NBA_PERIOD_RESTART_V2_H
#define NBA_PERIOD_RESTART_V2_H
#include <stdbool.h>
#include <stdint.h>

/* Typed words owned/carried by $86:DDA4-$E183, not an image of WRAM.
 * Coordinates retain the original integer/fraction words separately. */
typedef struct {
    uint16_t id, x_fraction; int16_t x;
    uint16_t y_fraction; int16_t y;
    uint16_t z_fraction; int16_t z;
    int16_t velocity_x, velocity_y, velocity_z;
    uint16_t list_link;
    uint16_t upper_phase, lower_phase;       /* +3A/+3C */
    uint16_t upper_accumulator, lower_accumulator; /* +42/+44 */
    uint16_t upper_lock, lower_lock, speed;  /* +46/+48/+4A */
    uint16_t direction, requested_direction, movement_direction; /* +4E/+50/+52 */
    int16_t target_x, target_y;              /* +56/+58 */
    uint16_t contact_inhibit, formation_timer, mode; /* +5A/+5C/+5E */
    uint16_t action_timer, behavior_timer;   /* +60/+64 */
    uint16_t team_group, team_context, boost_timer; /* +6E/+70/+72 */
    uint16_t recovery_inhibit, behavior_flags, focal_distance, field_a6;
} NbaPeriodRestartActor;

typedef struct {
    uint16_t id, x_fraction; int16_t x;
    uint16_t y_fraction; int16_t y;
    uint16_t z_fraction; int16_t z;
    int16_t velocity_x, velocity_y, velocity_z;
    uint16_t list_link, team_group;
} NbaPeriodRestartBall;

typedef struct {
    NbaPeriodRestartActor actors[10];
    NbaPeriodRestartBall ball;
    uint16_t object_list[12]; /* $34D3: ten interleaved actors, ball, zero */
    uint16_t list_cursor, list_flag_08fe, ball_pointer_0910;
    uint16_t context_4933, context_4935, event_08f0;
    uint16_t timer_092e, live_0936, camera_093a, last_side_093c;
    uint16_t owner_093e, owner_pointer_0940;
    uint16_t pass_actor_0942, pass_word_0944, receiver_0946;
    uint16_t activity_0948, pass_word_094a, last_actor_097e;
    uint16_t side_0952, actor_0954, layout_0956;
    int16_t target_x_0958, target_y_095a;
    uint16_t direction_095c, play_request_0994, play_0996;
    uint16_t attachment_0968, transfer_09b8, ready_09ba, attachment_09f6;
    int16_t dead_ball_x_09b0, dead_ball_y_09b2;
} NbaPeriodRestartState;

typedef struct {
    uint16_t period; /* already incremented: bounded 0..4 */
    uint16_t tip_winner; /* original $0932, exactly 0 or 5 */
    int16_t anchor_x[2]; /* already flipped by DD56..DD75 for period 2 */
} NbaPeriodRestartInput;

typedef enum {
    NBA_PERIOD_INVALID = 0,
    NBA_PERIOD_APPEARANCE, /* $87:AAB2; boundary.actor identifies the record */
    NBA_PERIOD_BALL_BEFORE, /* E056 observation checkpoint */
    NBA_PERIOD_APPEARANCE_GEOMETRY, /* $86:D85E */
    NBA_PERIOD_OBJECT_SORT, /* $86:D5DB */
    NBA_PERIOD_CANCEL_BEFORE, /* E0B4 observation checkpoint, child INCLUDED */
    NBA_PERIOD_CANCEL_AFTER, /* E0B8 observation checkpoint */
    NBA_PERIOD_TARGET_BEFORE, /* E102 observation checkpoint, child is INCLUDED */
    NBA_PERIOD_TARGET_AFTER,  /* E106 observation checkpoint */
    NBA_PERIOD_CONTROLLER,    /* E183 -> $86:BC9B, terminal excluded child */
    NBA_PERIOD_OPENING,       /* E1AC after included $E1A6 state81; children excluded */
    NBA_PERIOD_DONE
} NbaPeriodRestartBoundaryKind;

typedef struct {
    NbaPeriodRestartBoundaryKind kind;
    uint32_t source_pc, child_pc;
    uint16_t actor; /* UINT16_MAX when no actor belongs to this boundary */
} NbaPeriodRestartBoundary;

typedef struct {
    NbaPeriodRestartInput input;
    unsigned pair, phase;
    bool valid, waiting;
    NbaPeriodRestartBoundary boundary;
} NbaPeriodRestart;

/* Source CPU caller contract: M=0, X=0, decimal D=0, direct-page base=0.
 * Caller has completed DCA6..DD97, including DD56's period-2 anchor swap.
 * At DD97 original X=0, Y=34EB, scratch B6=context0 anchor, cursor 9A=34D3.
 * State is current gameplay state, never a captured native initialization.
 * work and input must not overlap. Do not edit work after begin. */
bool nba_period_restart_begin(NbaPeriodRestart *work, const NbaPeriodRestartInput *input);
/* Applies only this parent's writes through the next explicit boundary.
 * Repeating advance at a boundary is immutable. External children are never
 * executed or assigned a duration. Caller owns real child execution and state.
 * At checkpoints the caller preserves fields used by the next parent segment;
 * in particular side_0952/actor_0954 must retain the published legal values. */
NbaPeriodRestartBoundary nba_period_restart_advance(NbaPeriodRestart *work,
                                                  NbaPeriodRestartState *state);
/* Acknowledges a checkpoint or completed external child. It does not execute
 * that child. The terminal CONTROLLER/OPENING boundary cannot be resumed. */
bool nba_period_restart_resume(NbaPeriodRestart *work);
#endif
