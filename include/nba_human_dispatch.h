#ifndef NBA_HUMAN_DISPATCH_H
#define NBA_HUMAN_DISPATCH_H

#include <stdbool.h>
#include <stdint.h>

/* A bounded stage, not the complete $87:9106-$92A4 actor dispatcher.
 * Deliberately not in the production source manifest pending review. */
typedef enum {
    NBA_HUMAN_INPUT_NONE,
    NBA_HUMAN_INPUT_PUBLISH,
    NBA_HUMAN_INPUT_INVALID
} NbaHumanInputGate;
NbaHumanInputGate nba_human_input_gate(int16_t controller, uint16_t processed);

/* $84:E2AC-$E2F1 ends at one child call or the common return. All non-B
 * actions explicitly require the still-unimplemented E2F2 continuation. */
typedef enum {
    NBA_HUMAN_B_CONTINUE_OTHER_BUTTONS,
    NBA_HUMAN_B_RETURN,
    NBA_HUMAN_B_PASS,
    NBA_HUMAN_B_SWITCH
} NbaHumanBAction;
NbaHumanBAction nba_human_b_action(uint16_t newly_pressed, uint16_t actor,
    uint16_t owner, uint16_t free_throw, uint16_t receiver,
    uint16_t live_state, uint16_t movement_magnitude);

typedef enum {
    NBA_HUMAN_MOTION_FREE_THROW,
    NBA_HUMAN_MOTION_RECOVERY,
    NBA_HUMAN_MOTION_FLAG,
    NBA_HUMAN_MOTION_RECEIVER,
    NBA_HUMAN_MOTION_INBOUNDER,
    NBA_HUMAN_MOTION_LOOSE_MODE3,
    NBA_HUMAN_MOTION_MODE11,
    NBA_HUMAN_MOTION_MODE15,
    NBA_HUMAN_MOTION_ACCELERATE,
    NBA_HUMAN_MOTION_INVALID
} NbaHumanMotionRoute;

/* Inputs at $87:91C3, AFTER the action child returns. Calling with entry
 * values from before $84:E2AC would skip its possible state mutations. */
typedef struct {
    uint16_t actor, free_throw, recovery_7a, flags_7e, receiver;
    uint16_t live_state, inbound_group_0952, context_group_0c;
    uint16_t inbound_layout_0996, inbound_actor_0954, owner, mode;
    uint16_t direction, z, boost, dispatch_dt;
    uint8_t profile_42;
    int16_t velocity_x, velocity_y;
    /* Original carried-X path can address beyond the selected64-byte record.
     * This is WRAM[controller_pointer+0x72], not that actor's boost timer. */
    uint16_t controller_word_72;
} NbaHumanMotion;

/* $87:91C3-$922D including the existing $85:A82C acceleration helper.
 * This leaves behavior dispatch, processed-latch marking, actor commit and
 * action children to their own caller stages. No actor ownership is changed. */
NbaHumanMotionRoute nba_human_motion_step(NbaHumanMotion *motion);

#endif
