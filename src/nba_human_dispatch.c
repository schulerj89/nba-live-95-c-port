#include "nba_human_dispatch.h"
#include "nba_gameplay_ai.h"

/* Original ROM 2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870.
 * This module translates semantic caller stages, not CPU instructions.
 * Native source/boundary evidence: docs/human-dispatch-checkpoint.md. */
NbaHumanInputGate nba_human_input_gate(int16_t controller, uint16_t processed) {
    /* $87:9138-$915D: signed actor +16, then selected record +04. */
    if (controller < 0) return NBA_HUMAN_INPUT_NONE;
    if (controller >= 5) return NBA_HUMAN_INPUT_INVALID;
    return processed ? NBA_HUMAN_INPUT_NONE : NBA_HUMAN_INPUT_PUBLISH;
}

NbaHumanBAction nba_human_b_action(uint16_t newly_pressed, uint16_t actor,
    uint16_t owner, uint16_t free_throw, uint16_t receiver,
    uint16_t live_state, uint16_t movement_magnitude) {
    if (!(newly_pressed & 0x8000u)) return NBA_HUMAN_B_CONTINUE_OTHER_BUTTONS;
    if (actor != owner) return NBA_HUMAN_B_SWITCH;
    if (free_throw) return NBA_HUMAN_B_RETURN;
    /* Native BPL is a sign test, including positive out-of-range values. */
    if (!(receiver & 0x8000u)) return NBA_HUMAN_B_SWITCH;
    if (live_state == 0x82u && movement_magnitude) return NBA_HUMAN_B_RETURN;
    return NBA_HUMAN_B_PASS;
}

NbaHumanMotionRoute nba_human_motion_step(NbaHumanMotion *s) {
    if (!s) return NBA_HUMAN_MOTION_INVALID;
    if (s->free_throw) return NBA_HUMAN_MOTION_FREE_THROW;
    if (s->recovery_7a) return NBA_HUMAN_MOTION_RECOVERY;
    if (s->flags_7e & 4u) return NBA_HUMAN_MOTION_FLAG;
    if (s->actor == s->receiver) return NBA_HUMAN_MOTION_RECEIVER;
    /* $91E8-$91EB BMI after CMP preserves wrapped 16-bit subtraction. */
    if (!((uint16_t)(s->live_state - 0x80u) & 0x8000u) &&
        s->inbound_group_0952 == s->context_group_0c) {
        if (s->inbound_layout_0996 >= 6u) {
            if (s->actor == s->inbound_actor_0954) return NBA_HUMAN_MOTION_INBOUNDER;
        } else if ((s->owner & 0x8000u) && s->mode == 3u) {
            return NBA_HUMAN_MOTION_LOOSE_MODE3;
        }
        if (s->mode == 11u) return NBA_HUMAN_MOTION_MODE11;
        if (s->mode == 15u) return NBA_HUMAN_MOTION_MODE15;
    }
    if (s->direction > 8u) return NBA_HUMAN_MOTION_INVALID;
    /* Keep A82C's original cap quirk: AA1B loads its expired loop counter
     * from DP C2 and AA1D compares that with owner093E. The existing helper
     * preserves this; do not substitute the current actor index here. */
    nba_gameplay_velocity_step(&s->velocity_x,&s->velocity_y,&s->boost,
        (uint8_t)s->direction,s->profile_42,s->dispatch_dt,
        s->live_state == 0x81u || s->z != 0,(int16_t)s->owner);
    return NBA_HUMAN_MOTION_ACCELERATE;
}
