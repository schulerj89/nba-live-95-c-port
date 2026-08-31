#include "nba_human_pass_aligned.h"

static bool negative_difference(uint16_t a, uint16_t b) {
    return ((uint16_t)(a - b) & 0x8000u) != 0u;
}
static void expand_endpoints(uint16_t *a, uint16_t *b) {
    if (negative_difference(*a, *b)) {
        *a = (uint16_t)(*a - 24u); *b = (uint16_t)(*b + 24u);
    } else {
        *a = (uint16_t)(*a + 24u); *b = (uint16_t)(*b - 24u);
    }
}
static bool between(uint16_t value, uint16_t a, uint16_t b) {
    /* F525-F539/F542-F556 use XOR of wrapped subtraction signs. This
     * includes the lower endpoint and excludes the upper one in ordinary
     * nonwrapped ranges; do not replace it with inclusive geometry tests.
     * Extreme/endpoint cases are source-derived, not natural witnesses. */
    return (((uint16_t)(value - a) ^ (uint16_t)(value - b)) & 0x8000u) != 0u;
}

bool nba_human_pass_lane_obstructed(const NbaHumanPassLaneInput *s,
                                   uint16_t *result) {
    if (!s || !result || s->source_slot >= 10u || s->receiver_slot >= 10u ||
        s->source_cursor < 1u || s->source_cursor > 11u ||
        s->receiver_cursor < 1u || s->receiver_cursor > 11u ||
        s->order[0] != 0xffffu || s->order[12] != 0xffffu) return false;
    for (unsigned i = 0u; i < 13u; ++i)
        if (s->order[i] != 0xffffu && s->order[i] > 10u) return false;
    uint16_t x1 = s->actors[s->source_slot].x, x2 = s->actors[s->receiver_slot].x;
    uint16_t y1 = s->actors[s->source_slot].y, y2 = s->actors[s->receiver_slot].y;
    expand_endpoints(&x1, &x2); expand_endpoints(&y1, &y2);
    uint16_t team = s->actors[s->source_slot].team;
    /* F4FD-F55F scans forward from the source's actual +14 cursor.
     * F539's X miss immediately reverses the scan. A Y miss merely advances.
     * Preserve this even for unsorted source inputs: do not improve the ROM. */
    for (unsigned cursor = s->source_cursor + 1u; cursor < 13u; ++cursor) {
        if (cursor == s->receiver_cursor) continue;
        uint16_t slot = s->order[cursor];
        if (slot == 0xffffu) break;
        if (slot == 10u || s->actors[slot].team == team) continue;
        if (!between(s->actors[slot].x, x1, x2)) break;
        if (between(s->actors[slot].y, y1, y2)) { *result = 1u; return true; }
    }
    /* F568-F5CC starts over at source-1; F5A4's X miss now returns clear. */
    for (int cursor = (int)s->source_cursor - 1; cursor >= 0; --cursor) {
        if ((unsigned)cursor == s->receiver_cursor) continue;
        uint16_t slot = s->order[cursor];
        if (slot == 0xffffu) break;
        if (slot == 10u || s->actors[slot].team == team) continue;
        if (!between(s->actors[slot].x, x1, x2)) break;
        if (between(s->actors[slot].y, y1, y2)) { *result = 1u; return true; }
    }
    *result = 0u; return true;
}

static bool valid_state(const NbaHumanPassAlignedState *s) {
    return s && s->action.passer_slot < 10u && s->action.receiver_slot < 10u;
}

bool nba_human_pass_aligned_choose(NbaHumanPassAlignedState *s) {
    if (!valid_state(s)) return false;
    NbaHumanPassActionState *a = &s->action;
    const NbaHumanPassInitActor *actor = &a->common.actors[a->passer_slot];
    const NbaHumanPassInitActor *receiver = &a->common.actors[a->receiver_slot];
    uint16_t selector = a->relative_51;
    if (selector == 0u) {
        /* AE19/AE1C are consecutive SBCs, without a second SEC or an AND.
         * Preserve the first subtraction's borrow and the full wrapped word. */
        uint16_t first = (uint16_t)(s->fine_c0 - actor->movement_direction);
        selector = (uint16_t)(first - actor->movement_direction -
                    (s->fine_c0 < actor->movement_direction ? 1u : 0u));
    }
    if (selector != 0u) {
        s->family_ae = 0xffffu;
        bool choose30 = selector < 3u;
        if (actor->movement_direction < 3u) choose30 = !choose30;
        a->request_00 = choose30 ? 0x30u : 0x31u;
        return true;
    }
    if (receiver->mode != 14u) {
        if (!nba_human_pass_lane_obstructed(&s->lane, &s->scratch_aa)) return false;
        bool choose2b = s->scratch_aa == 0u ? (s->options_07f6 & 0x30u) == 0u :
            a->common.live_0936 == 0x82u &&
            (negative_difference(s->layout_0956, 2u) ||
             !negative_difference(s->layout_0956, 5u));
        if (choose2b) { a->request_00 = 0x2bu; s->family_ae = 0xffffu; return true; }
        if (a->profile_3e >= 0x50u && negative_difference(a->distance_4f, 0x79u) &&
            a->distance_4f >= 0x30u && s->passer_anchor_8a >= 0x20u &&
            a->common.live_0936 != 0x82u) {
            /* AEBA-AEBF: naturally witnessed at left court270, F473 AA=1.
             * The older bounded adapter's 2F/family1 was a port gap here. */
            a->request_00 = 0x2au; s->family_ae = 0u; return true;
        }
    }
    a->request_00 = 0x2cu; s->family_ae = 1u; return true;
}

NbaHumanPassAlignedRoute nba_human_pass_aligned_install(
    const NbaAssetPack *assets, NbaHumanPassAlignedState *s) {
    if (!valid_state(s)) return NBA_HUMAN_PASS_ALIGNED_INVALID;
    NbaHumanPassActionState *a = &s->action;
    const NbaHumanPassInitActor *actor = &a->common.actors[a->passer_slot];
    a->extra[a->passer_slot].family = s->family_ae; /* AEDB-AEDD precedes every branch. */
    if (a->relative_51 & 0x8000u) return NBA_HUMAN_PASS_ALIGNED_COMMIT_AF30;
    if (a->request_00 < 0x2du) {
        if (a->distance_4f < 0xf1u) {
            if (a->request_00 == 0x2cu) a->request_00 = 0x2fu;
        } else if (a->common.live_0936 != 0x82u &&
                   ((uint16_t)actor->velocity_x | (uint16_t)actor->velocity_y |
                    a->extra[a->passer_slot].z) == 0u)
            return NBA_HUMAN_PASS_ALIGNED_BOTH_B3BD;
    }
    if (!nba_human_pass_action_upper(assets, a)) return NBA_HUMAN_PASS_ALIGNED_INVALID;
    return NBA_HUMAN_PASS_ALIGNED_POSE_AF1D;
}

NbaHumanPassAlignedRoute nba_human_pass_aligned_prepare(
    const NbaAssetPack *assets, NbaHumanPassAlignedState *s) {
    if (!valid_state(s)) return NBA_HUMAN_PASS_ALIGNED_INVALID;
    const NbaHumanPassActionActor *receiver = &s->action.extra[s->action.receiver_slot];
    if (negative_difference(s->action.common.live_0936, 0x80u) &&
        negative_difference(s->band_b2, 0x19u) &&
        negative_difference(s->receiver_anchor_8c, 0xc8u) &&
        (receiver->z | receiver->velocity_z) == 0u)
        return NBA_HUMAN_PASS_ALIGNED_CATCH_AD3D;
    if (!nba_human_pass_aligned_choose(s)) return NBA_HUMAN_PASS_ALIGNED_INVALID;
    return nba_human_pass_aligned_install(assets, s);
}
