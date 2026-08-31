#ifndef NBA_CONTROLLER_H
#define NBA_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#define NBA_CONTROLLER_COUNT 5
#define NBA_CONTROLLER_ACTOR_COUNT 10

/* Native $47EB+$40*pad. Untranslated tail words are preserved by the
 * allocator/input routines; this is data, not a CPU/emulator register file. */
typedef struct {
    int16_t group;             /* +00: -1 unassigned, 0 home, 5 visitor */
    uint16_t actor;            /* +02 */
    uint16_t processed;        /* +04: once per native actor sweep */
    uint16_t direction;        /* +06: 0..7, 8 neutral */
    uint16_t held;             /* +08: SNES bits, not NBA_BTN_* */
    uint16_t previous;         /* +0A: current sample after publication */
    uint16_t changed;          /* +0C: held XOR previous on entry */
    uint16_t pressed;          /* +0E: held AND changed */
    uint16_t alternate_direction; /* +10 */
    uint16_t reserved[23];
} NbaControllerRecord;

typedef struct {
    NbaControllerRecord record[NBA_CONTROLLER_COUNT];
    uint16_t previous_selection[NBA_CONTROLLER_COUNT]; /* $1677 */
    int16_t actor_assignment[NBA_CONTROLLER_ACTOR_COUNT]; /* actor +16 */
    uint16_t count[2];         /* context +3B: $4726/$47A6 */
    uint16_t cursor[2];        /* context +3D: $4728/$47A8 */
} NbaControllerState;

/* $86:E208-E389; selection $166D is 0 left, 1 neutral, 2 right.
 * $07F8 is an override, not a controller-selection value. Returns false for
 * a corrupt out-of-domain host actor index rather than accessing past data. */
bool nba_controller_initialize(NbaControllerState *state,
    const uint16_t selection[5], const uint16_t flags[5], uint16_t override_07f8);
bool nba_controller_allocate(NbaControllerState *state,
    const uint16_t selection[5], const uint16_t flags[5], uint16_t override_07f8);

/* $86:BC9B-BD1E, called with the target actor and its native context. */
bool nba_controller_transfer(NbaControllerState *state, unsigned target,
                              uint16_t actor_group);
/* $86:D25A-D349: ownership prefix before the ordinary BAA2 acquisition.
 * Designated pass receivers use $0944, including its bit-$10 veto. */
bool nba_controller_acquire(NbaControllerState *state, unsigned target,
    uint16_t actor_group, uint16_t receiver_0946, uint16_t controller_0944,
    uint16_t *previous_controller_0a00);
/* $87:9075-9086, called once at the actor-behavior sweep boundary. */
void nba_controller_begin_sweep(NbaControllerState *state);

/* Full $85:EF3A-EFEC state effects. Caller supplies the active roster's
 * stamina +18, not the actor animation-queue word sharing that offset. */
typedef struct {
    uint16_t actor, actor_z, stamina, boost;
    uint16_t free_throw_0978, live_state_0936, attachment_09f6;
    uint16_t traveling_17d9, owner_093e;
    uint16_t contact_09bc, event_0964, whistle_09b6, event_actor_492d;
} NbaControllerInputContext;
void nba_controller_publish_input(NbaControllerRecord *record, uint16_t held,
                                   NbaControllerInputContext *context);
uint16_t nba_controller_native_buttons(uint32_t host_buttons);
uint16_t nba_controller_host_buttons(uint16_t native_buttons);

#endif
