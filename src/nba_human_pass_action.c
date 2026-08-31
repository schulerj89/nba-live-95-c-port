#include "nba_human_pass_action.h"

bool nba_human_pass_action_upper(const NbaAssetPack *assets,
                                NbaHumanPassActionState *s) {
    if (!s || s->passer_slot >= 10u) return false;
    NbaPlayerAnimationChannels *c = &s->common.actors[s->passer_slot].animation;
    /* B481/B486 return without touching either descriptor word. In
     * particular, a negative lock keeps the installed action even when the
     * next request differs. B48E-B491 writes the full bank word $0084. */
    bool resolves_descriptor = c->upper_state != s->request_00 &&
                               (c->upper_lock & 0x8000u) == 0u;
    if (!nba_player_animation_command_scratch(assets, c,
            NBA_ANIMATION_INSTALL_UPPER, &s->request_00, false, false,
            &s->descriptor_47)) return false;
    if (resolves_descriptor) s->descriptor_bank_49 = 0x0084u;
    return true;
}

bool nba_human_pass_action_grounded(const NbaAssetPack *assets,
                                   NbaHumanPassActionState *s) {
    if (!s || s->passer_slot >= 10u || s->receiver_slot >= 10u) return false;
    NbaHumanPassInitActor *actor = &s->common.actors[s->passer_slot];
    NbaHumanPassActionActor *extra = &s->extra[s->passer_slot];
    s->common.actors[s->receiver_slot].timer = 0x50u;
    actor->velocity_x = actor->velocity_y = 0;
    extra->magnitude = 0u;
    extra->family = 5u;
    extra->pass_direction = s->coarse_be; /* B024-B026, not a pose side effect. */
    extra->flags |= 6u;
    if (s->distance_4f >= 0x00f1u) {
        s->request_00 = 0x2cu;
        if (!nba_human_pass_action_upper(assets, s)) return false;
    }
    /* B042-B047 unconditionally requests 2F after the optional 2C request.
     * Do not collapse this to one final state assignment: B47A's lock test
     * can retain 2C while DP00 still contains the later request 2F.
     * Original right-route court800/870, upper entries84/110: state2C and
     * lockFFFF reach B486's early return. Keep this observed behavior. */
    s->request_00 = 0x2fu;
    return nba_human_pass_action_upper(assets, s);
}

NbaHumanPassActionRoute nba_human_pass_action_select(const NbaAssetPack *assets,
                                                   NbaHumanPassActionState *s) {
    if (!s || s->passer_slot >= 10u || s->receiver_slot >= 10u)
        return NBA_HUMAN_PASS_ACTION_INVALID;
    /* AC50's N comes from the preceding relative-direction result. */
    if ((s->relative_51 & 0x8000u) || s->relative_51 < 3u || s->relative_51 >= 6u)
        return NBA_HUMAN_PASS_ACTION_OFFAXIS_AD0E;
    const NbaHumanPassInitActor *actor = &s->common.actors[s->passer_slot];
    const NbaHumanPassActionActor *extra = &s->extra[s->passer_slot];
    if ((extra->z | extra->velocity_z) != 0u)
        return NBA_HUMAN_PASS_ACTION_NORMAL_ACA9;
    bool grounded_special = s->profile_3e < 0x55u ||
        actor->animation.lower_state == 0x0bu || actor->animation.lower_state == 9u;
    if (!grounded_special) {
        if (s->distance_4f >= 0x119u && extra->boost != 0u)
            return NBA_HUMAN_PASS_ACTION_BOOST_AFC4;
        if (((uint16_t)actor->velocity_x | (uint16_t)actor->velocity_y) != 0u)
            return NBA_HUMAN_PASS_ACTION_NORMAL_ACA9;
    }
    if (!nba_human_pass_action_grounded(assets, s))
        return NBA_HUMAN_PASS_ACTION_INVALID;
    return NBA_HUMAN_PASS_ACTION_POSE_AF1D;
}
