#include "nba_controller.h"

/* Original ROM SHA256 2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870.
 * Fresh bounded recomp/Ghidra/native evidence and absolute artifact paths:
 * docs/controller-implementation-checkpoint.md.
 * Native routines preserve fields not explicitly written below. */
bool nba_controller_allocate(NbaControllerState *state,
    const uint16_t selection[5], const uint16_t flags[5], uint16_t override_07f8) {
    if (!state || !selection || !flags) return false;
    NbaControllerState next = *state;
    unsigned available[2] = {0, 5};
    for (unsigned i = 0; i < 10; ++i) next.actor_assignment[i] = -1;
    next.count[0] = next.count[1] = 0;
    for (unsigned pad = 0; pad < 5; ++pad) {
        uint16_t choice = selection[pad] & 0x7fffu;
        NbaControllerRecord *r = &next.record[pad];
        if (choice == next.previous_selection[pad]) {
            /* E294-E2C0 retains both selected actor and group. */
            if (override_07f8 || choice == 1) continue;
            if (r->actor >= 10) return false;
            ++next.count[choice < 1 ? 1 : 0];
            next.actor_assignment[r->actor] = (int16_t)pad;
        } else {
            r->group = -1;
            if (override_07f8 || choice == 1) continue;
            unsigned context = choice < 1 ? 1 : 0;
            ++next.count[context];
            unsigned actor = available[context];
            while (actor < 10 && next.actor_assignment[actor] >= 0) ++actor;
            if (actor >= 10) return false;
            next.actor_assignment[actor] = (int16_t)pad;
            r->actor = (uint16_t)actor;
            r->group = context ? 5 : 0;
            available[context] = actor + 1;
        }
    }
    for (unsigned pad = 0; pad < 5; ++pad) {
        /* The native copy is unmasked; the earlier comparison is masked. */
        next.previous_selection[pad] = selection[pad];
        next.record[pad].alternate_direction = (flags[pad] & 0x8000u) ? 1 : 0;
    }
    *state = next;
    return true;
}

bool nba_controller_initialize(NbaControllerState *state,
    const uint16_t selection[5], const uint16_t flags[5], uint16_t override_07f8) {
    if (!state || !selection || !flags) return false;
    NbaControllerState next = *state;
    for (unsigned pad = 0; pad < 5; ++pad) {
        next.record[pad].group = -1;
        next.previous_selection[pad] = 0xffffu;
    }
    for (unsigned actor = 0; actor < 10; ++actor) next.actor_assignment[actor] = -1;
    if (!nba_controller_allocate(&next, selection, flags, override_07f8)) return false;
    *state = next;
    return true;
}

static bool transfer_round_robin(NbaControllerState *state, unsigned target,
                                  uint16_t actor_group, bool clamp_cursor) {
    if (!state || target >= 10 || (actor_group != 0 && actor_group != 5)) return false;
    unsigned context = actor_group == 5 ? 1 : 0;
    if (state->actor_assignment[target] >= 0 || state->count[context] == 0) return true;
    unsigned pad = state->cursor[context];
    if (pad >= 5) {
        if (!clamp_cursor) return false;
        pad = 0;
    }
    /* The ROM loops until a matching record. A bounded search rejects corrupt
     * host state (count nonzero without a matching record) without hanging. */
    for (unsigned attempt = 0; attempt < 5; ++attempt, pad = (pad + 1) % 5) {
        NbaControllerRecord *r = &state->record[pad];
        if ((uint16_t)r->group != actor_group) continue;
        unsigned old = r->actor;
        if (old >= 10) return false;
        r->actor = (uint16_t)target;
        state->actor_assignment[target] = (int16_t)pad;
        state->cursor[context] = (uint16_t)((pad + 1) % 5);
        state->actor_assignment[old] = -1;
        return true;
    }
    return false;
}

bool nba_controller_transfer(NbaControllerState *state, unsigned target,
                              uint16_t actor_group) {
    return transfer_round_robin(state,target,actor_group,true);
}

bool nba_controller_acquire(NbaControllerState *state, unsigned target,
    uint16_t actor_group, uint16_t receiver_0946, uint16_t controller_0944,
    uint16_t *previous_controller_0a00) {
    if (!state || !previous_controller_0a00 || target >= 10 ||
        (actor_group != 0 && actor_group != 5)) return false;
    *previous_controller_0a00 = (uint16_t)state->actor_assignment[target];
    if (target == receiver_0946) {
        if ((controller_0944 & 0x8010u) || state->actor_assignment[target] >= 0) return true;
        if (controller_0944 >= 5) return false;
        NbaControllerRecord *r = &state->record[controller_0944];
        unsigned old = r->actor;
        if (old >= 10) return false;
        /* Native designated-pass transfer does not test or rewrite group,
         * and does not advance the round-robin cursor. Preserve that quirk. */
        state->actor_assignment[target] = (int16_t)controller_0944;
        r->actor = receiver_0946;
        state->actor_assignment[old] = -1;
        return true;
    }
    /* D2DC has no BCAD cursor>=5 repair; reject corrupt host input instead
     * of silently importing BC9B's different contract into this routine. */
    return transfer_round_robin(state,target,actor_group,false);
}

void nba_controller_begin_sweep(NbaControllerState *state) {
    if (!state) return;
    for (unsigned pad=0;pad<5;++pad) state->record[pad].processed=0;
}

void nba_controller_publish_input(NbaControllerRecord *r, uint16_t held,
                                   NbaControllerInputContext *c) {
    if (!r || !c) return;
    /* $85:F00D and $85:EFED, all 16 direction combinations including
     * simultaneous opposing keys. Values are original ROM uint16 tables. */
    static const uint16_t normal[16] = {8,1,5,8,7,0,6,8,3,2,4,8,8,8,8,8};
    static const uint16_t alternate[16] = {8,2,6,8,0,1,7,8,4,3,5,8,8,8,8,8};
    r->held = held;
    r->changed = held ^ r->previous;
    r->previous = held;
    r->pressed = held & r->changed;
    unsigned index = (held >> 8) & 15;
    r->direction = r->alternate_direction ? alternate[index] : normal[index];
    if (c->free_throw_0978) return;
    c->boost = c->stamina >= 0x0800u && (held & 0x0030u) ? 5 : 0;
    /* BPL after CMP tests the wrapped subtraction's sign, not unsigned >=. */
    if (((uint16_t)(c->live_state_0936 - 0x80u) & 0x8000u) == 0 ||
        c->attachment_09f6 != 3 || !c->traveling_17d9 || c->owner_093e != c->actor ||
        (c->free_throw_0978 | c->contact_09bc | c->event_0964 | c->whistle_09b6) ||
        c->actor_z || r->direction == 8) return;
    c->event_0964 = 5;
    c->event_actor_492d = c->actor;
}

uint16_t nba_controller_native_buttons(uint32_t host_buttons) {
    uint16_t result = 0;
    for (unsigned bit = 0; bit < 12; ++bit)
        if (host_buttons & (1u << bit)) result |= (uint16_t)(0x8000u >> bit);
    return result;
}

uint16_t nba_controller_host_buttons(uint16_t native_buttons) {
    uint16_t result = 0;
    for (unsigned bit = 0; bit < 12; ++bit)
        if (native_buttons & (0x8000u >> bit)) result |= (uint16_t)(1u << bit);
    return result;
}
