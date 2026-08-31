#include "nba_human_pass_init.h"
#include "nba_gameplay_ai.h"
#include "nba_gameplay_ball.h"

bool nba_human_pass_init_prefix(NbaHumanPassInitState *s,
    uint16_t passer, uint16_t identity, uint16_t receiver,
    uint16_t receiver_identity, NbaHumanPassInitPrefix *out) {
    if (!s || !out || passer >= 10u || receiver >= 10u ||
        s->actors[passer].identity >= 10u) return false;
    NbaHumanPassInitActor *actor = &s->actors[passer];
    uint16_t request = 0u;
    /* $86:AB3D -> $87:B538-B554: any nonzero lock cancels, including
     * negative locks. Do not use the scene wrapper, which also resolves a
     * pose: this leaf only changes +18,+30,+3A,+42,+46 when locked. */
    if (!nba_player_animation_command_scratch(NULL, &actor->animation,
            NBA_ANIMATION_CANCEL_UPPER, &request, false, false, NULL))
        return false;
    out->profile_lo_e6 = s->profile_pointers[actor->identity][0];
    out->profile_hi_e8 = s->profile_pointers[actor->identity][1];
    out->receiver_slot = receiver;
    s->passer_0942 = identity; /* AB55 reads C2, not the descriptor index. */
    s->receiver_0946 = receiver_identity;
    s->active_09c4 = 1u;
    if (s->live_0936 == 0x82u) s->inbound_transfer_09b8 = 1u;
    s->actors[receiver].mode = 10u;
    s->actors[receiver].timer = 0x28u;
    return true;
}

bool nba_human_pass_init_geometry(NbaHumanPassInitState *s,
    uint16_t passer, uint16_t receiver, NbaHumanPassInitGeometry *out) {
    if (!s || !out || passer >= 10u || receiver >= 10u) return false;
    const NbaHumanPassInitActor *a = &s->actors[passer];
    const NbaHumanPassInitActor *b = &s->actors[receiver];
    /* AB85-ABE5 uses signed shifts and wraps each sum/subtraction at 16 bits.
     * Integer coordinates are read directly; no subpixel rounding. */
    uint16_t ax = (uint16_t)((uint16_t)a->x +
        (uint16_t)nba_gameplay_arithmetic_shift_right(a->velocity_x, 4u));
    uint16_t ay = (uint16_t)((uint16_t)a->y +
        (uint16_t)nba_gameplay_arithmetic_shift_right(a->velocity_y, 4u));
    uint16_t bx = (uint16_t)((uint16_t)b->x +
        (uint16_t)nba_gameplay_arithmetic_shift_right(b->velocity_x, 3u));
    uint16_t by = (uint16_t)((uint16_t)b->y +
        (uint16_t)nba_gameplay_arithmetic_shift_right(b->velocity_y, 3u));
    out->dx = (int16_t)(uint16_t)(bx - ax);
    out->dy = (int16_t)(uint16_t)(by - ay);
    out->fine_direction = nba_gameplay_pass_direction(out->dx, out->dy,
                                                      &out->distance);
    out->coarse_direction = out->fine_direction >> 1;
    out->band = 0u;
    static const uint16_t limits[] = {0x41u, 0x79u, 0xc9u, 0x119u, 0x191u};
    for (unsigned i = 0u; i < sizeof(limits) / sizeof(limits[0]); ++i) {
        /* ABFE-AC25 deliberately tests N after CPX, not unsigned carry.
         * Extreme wrapped distances preserve that source behavior; normal
         * captures only establish the ordinary court coordinate domain. */
        if (((uint16_t)(out->distance - limits[i]) & 0x8000u) != 0u) break;
        out->band = (uint16_t)(out->band + 6u);
    }
    s->distance_09da = out->distance;
    s->actors[passer].pass_band = out->band;
    /* A coincident endpoint yields fine16/coarse8, then FFFF at AC40.
     * Native prefix effects still occur; there is no early rejection. */
    out->relative = out->coarse_direction >= 8u ? 0xffffu :
        (uint16_t)((out->coarse_direction - a->movement_direction) & 7u);
    return true;
}

NbaHumanPassRoute nba_human_pass_prepare(const NbaHumanPassInput *selection,
    uint16_t passer_identity_c2, NbaHumanPassInitState *state,
    NbaHumanPassSelection *selected,
    NbaHumanPassInitPrefix *prefix, NbaHumanPassInitGeometry *geometry) {
    if (!selection || !state || !selected || !prefix || !geometry)
        return NBA_HUMAN_PASS_INVALID;
    *selected = nba_human_pass_select(selection);
    if (selected->route == NBA_HUMAN_PASS_INVALID) return selected->route;
    state->controller_tag_0944 = selected->controller_tag_0944;
    if (selected->route == NBA_HUMAN_PASS_NO_RECEIVER) return selected->route;
    if (!nba_human_pass_init_prefix(state, selection->actor,
            passer_identity_c2, selected->receiver_slot,
            selected->receiver_identity, prefix) ||
        !nba_human_pass_init_geometry(state, selection->actor,
            selected->receiver_slot, geometry)) return NBA_HUMAN_PASS_INVALID;
    return NBA_HUMAN_PASS_CONTINUE_INITIALIZER;
}
