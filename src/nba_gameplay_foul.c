#include "nba_gameplay_foul.h"
#include <string.h>

void nba_gameplay_foul_init(NbaGameplayFoulState *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->offender_actor_raw = -1;
    state->victim_actor_raw = -1;
    state->whistle_timer_raw_08de = -1;
}

/* `$86:C4FE-$C6AC` emits pending `$0964` codes 1/2/13 and preserves actor
 * IDs in `$492D/$492F`. `$86:C62A-$C63D` sends codes 2 and 13 directly to
 * personal bookkeeping; only defensive code 1 increments team context
 * `+$28` and tests the bonus. Persistent player `+$14` caps at six. If `$0948` says a
 * detached defensive shot, it defers the whistle through `$09BC`; a later
 * make ORs bit 15 in a separate scoring path. `$86:C65E-$C692` applies the
 * team-foul threshold only to a nondeferred defensive foul.
 * The caller supplies the team resolved through actor `+$70`; it is not
 * inferred from the actor slot.
 * Increment 5H connects this bookkeeping hook to the now-proven
 * `$86:CCFC-$D1CE` owned-ball pose-contact predicate. The subsequent whistle
 * scene/free-throw consumer remains a separate translation boundary. */
bool nba_gameplay_foul_record_contact(NbaGameplayFoulState *state,
                                      uint8_t event_code,
                                      uint8_t offender_actor,
                                      uint8_t victim_actor,
                                      uint8_t offender_team,
                                      bool shot_detached,
                                      uint16_t period_raw_0926) {
    if (!state || offender_actor >= 10u || victim_actor >= 10u ||
        offender_team >= 2u ||
        (event_code != NBA_GAMEPLAY_FOUL_DEFENSIVE &&
         event_code != NBA_GAMEPLAY_FOUL_CHARGING &&
         event_code != NBA_GAMEPLAY_FOUL_OFFENSIVE)) return false;
    state->offender_actor_raw = (int8_t)offender_actor;
    state->victim_actor_raw = (int8_t)victim_actor;
    if (state->personal_fouls[offender_actor] < 6u)
        ++state->personal_fouls[offender_actor];
    else
        state->personal_fouls[offender_actor] = 6u;
    if (event_code != NBA_GAMEPLAY_FOUL_DEFENSIVE) {
        state->foul_event_raw_0964 = event_code;
        return true;
    }
    unsigned team = offender_team;
    ++state->team_fouls[team];
    if (shot_detached) {
        state->foul_event_raw_0964 = 0u;
        state->shooting_foul_raw_09bc = 1u;
    } else {
        state->foul_event_raw_0964 = event_code;
        state->shooting_foul_raw_09bc = 0u;
        /* Literal `$86:C66E-$C684`: BCC selects CMP #$0005 when `$0926`
         * is below four; the fallthrough selects CMP #$0004. */
        uint16_t threshold = period_raw_0926 < 4u ? 5u : 4u;
        if (state->team_fouls[team] >= threshold) {
            state->free_throw_state_raw_0978 = 1u;
            state->free_throw_sequence_raw_097a = 2u;
        }
    }
    return true;
}

/* Literal portable decision tree from `$86:C4FE-$C6AC`. This routine is
 * intentionally fire-and-forget at the collision call sites: rejection or
 * acceptance never gates contact physics. It still masks/shifts the live
 * `$07F6` state after the early predicates, which is gameplay-significant
 * because later jitter/action rolls consume that mutated state. */
bool nba_gameplay_foul_classify_contact(
    NbaGameplayFoulState *state, NbaGameplayRng *rng,
    const NbaGameplayContactFoulInput *input,
    uint16_t *event_bits_raw_13e7) {
    if (!state || !rng || !input || input->offender_actor >= 10u ||
        input->victim_actor >= 10u || input->offender_team >= 2u ||
        input->live_state_raw_0936 >= 0x80u ||
        state->free_throw_state_raw_0978 != 0u ||
        state->shooting_foul_raw_09bc != 0u ||
        state->foul_event_raw_0964 != 0u ||
        state->whistle_active_raw_09b6 != 0u)
        return false;

    bool detached = input->ball_activity_raw_0948 != 0u;
    if (input->context_tag != 0x87u && !detached &&
        input->offender_movement_magnitude < 0x02F4u)
        return false;
    if (detached) {
        int active = input->ball_owner_actor >= 0 ?
                     input->ball_owner_actor : input->last_shooter_actor;
        if (active < 0 || active >= 10 ||
            (input->victim_actor != (uint8_t)active &&
             input->offender_actor != (uint8_t)active))
            return false;
    }

    uint16_t roll = (uint16_t)(rng->state & 0x00FFu);
    rng->state = roll;
    bool owner_is_offender = input->ball_owner_actor >= 0 &&
        input->offender_actor == (uint8_t)input->ball_owner_actor;
    if (input->offender_boost_raw_72 != 0u &&
        (((roll & 1u) == 0u) || input->offender_mode != 9u)) {
        roll >>= 1;
        rng->state = roll;
    }
    if (input->context_tag == 0x87u) {
        roll >>= 1;
        rng->state = roll;
        if (!owner_is_offender && roll < 5u) return false;
    }

    uint16_t rule;
    uint8_t event_code;
    if (owner_is_offender) {
        rule = input->offensive_rule_raw_17d3;
        event_code = NBA_GAMEPLAY_FOUL_CHARGING;
    } else if (input->ball_owner_actor >= 0 &&
               input->offender_group == input->offense_group_raw_093a) {
        rule = input->offensive_rule_raw_17d3;
        event_code = NBA_GAMEPLAY_FOUL_OFFENSIVE;
    } else {
        rule = input->defensive_rule_raw_17d1;
        event_code = NBA_GAMEPLAY_FOUL_DEFENSIVE;
    }
    uint16_t rule_delta = (uint16_t)((uint16_t)(rule << 2) - roll);
    if (rule == 0u || (int16_t)rule_delta < 0) return false;
    if (!nba_gameplay_foul_record_contact(
            state, event_code, input->offender_actor, input->victim_actor,
            input->offender_team, detached, input->period_raw_0926))
        return false;
    if (detached && event_code == NBA_GAMEPLAY_FOUL_DEFENSIVE &&
        event_bits_raw_13e7)
        *event_bits_raw_13e7 |= 0x2000u;
    return true;
}

bool nba_gameplay_foul_record_made_basket(NbaGameplayFoulState *state) {
    if (!state || state->shooting_foul_raw_09bc == 0u) return false;
    state->shooting_foul_raw_09bc |= 0x8000u;
    return true;
}

/* `$86:CE24-$CE65` emits code 6 without team/personal foul bookkeeping. */
bool nba_gameplay_foul_record_violation(NbaGameplayFoulState *state,
                                        uint8_t event_code,
                                        uint8_t actor,
                                        uint8_t shooter) {
    if (!state || event_code != NBA_GAMEPLAY_VIOLATION_INTERFERENCE ||
        actor >= 10u || shooter >= 10u ||
        state->foul_event_raw_0964 != 0u) return false;
    state->foul_event_raw_0964 = event_code;
    state->offender_actor_raw = (int8_t)actor;
    state->victim_actor_raw = (int8_t)shooter;
    return true;
}

/* Exact gameplay-visible WRAM writes from `$85:93F5-$945E`. `$87:BACB`
 * queues the whistle presentation only while the old signed `$08DE` is
 * negative; otherwise `$83:EBD8` is the single `STZ $08E2` instruction. */
bool nba_gameplay_foul_consume_pending(NbaGameplayFoulState *state,
                                       uint8_t camera_side_group,
                                       uint16_t *event_bits_raw_13e7,
                                       uint16_t *inbound_ready_raw_09ba,
                                       bool short_timer) {
    if (!state || state->whistle_active_raw_09b6 != 0u ||
        state->foul_event_raw_0964 == 0u) return false;
    state->latched_event_raw_08f0 = state->foul_event_raw_0964;
    state->foul_event_raw_0964 = 0u;
    if (inbound_ready_raw_09ba) *inbound_ready_raw_09ba = 0u;
    state->contact_context_raw_497f = 0u;
    state->presentation_pending_raw_4937 = 1u;
    if (state->shooting_foul_raw_09bc == 0u && event_bits_raw_13e7)
        *event_bits_raw_13e7 |= 0x2000u;
    state->shooting_foul_raw_09bc = 0u;
    if (camera_side_group == 0u)
        state->side_event_bits_raw_13e9 |= 1u;
    state->whistle_active_raw_09b6 = 1u;
    state->whistle_presentation_queued_raw = 0u;
    if (state->whistle_timer_raw_08de < 0)
        state->whistle_presentation_queued_raw = 1u;
    else
        state->presentation_gate_raw_08e2 = 0u;
    state->whistle_timer_raw_08de = short_timer ? 120 : 300;
    state->whistle_state_raw_08e6 = 17u;
    state->whistle_state_mirror_raw_08e8 = 17u;
    return true;
}

bool nba_gameplay_foul_self_test(void) {
    NbaGameplayFoulState state;
    nba_gameplay_foul_init(&state);
    if (state.offender_actor_raw != -1 || state.victim_actor_raw != -1 ||
        nba_gameplay_foul_record_contact(&state, 7u, 0u, 5u,
                                         0u, false, 0u) ||
        nba_gameplay_foul_record_made_basket(&state)) return false;
    for (unsigned i = 0; i < 5u; ++i)
        if (!nba_gameplay_foul_record_contact(
                &state, NBA_GAMEPLAY_FOUL_DEFENSIVE, 2u, 7u,
                0u, false, 3u)) return false;
    if (state.team_fouls[0] != 5u || state.personal_fouls[2] != 5u ||
        state.foul_event_raw_0964 != NBA_GAMEPLAY_FOUL_DEFENSIVE ||
        state.free_throw_state_raw_0978 != 1u ||
        state.free_throw_sequence_raw_097a != 2u) return false;
    NbaGameplayFoulState nondefensive;
    nba_gameplay_foul_init(&nondefensive);
    if (!nba_gameplay_foul_record_contact(
            &nondefensive, NBA_GAMEPLAY_FOUL_OFFENSIVE, 7u, 2u,
            1u, true, 6u) || nondefensive.team_fouls[1] != 0u ||
        nondefensive.personal_fouls[7] != 1u ||
        nondefensive.foul_event_raw_0964 != NBA_GAMEPLAY_FOUL_OFFENSIVE ||
        nondefensive.shooting_foul_raw_09bc != 0u ||
        nondefensive.free_throw_state_raw_0978 != 0u ||
        nba_gameplay_foul_record_made_basket(&nondefensive)) return false;
    NbaGameplayFoulState charging;
    nba_gameplay_foul_init(&charging);
    charging.personal_fouls[2] = 9u;
    for (unsigned i = 0; i < 4u; ++i)
        if (!nba_gameplay_foul_record_contact(
                &charging, NBA_GAMEPLAY_FOUL_CHARGING, 2u, 7u,
                0u, false, 6u)) return false;
    NbaGameplayFoulState classifier;
    NbaGameplayRng classifier_rng;
    NbaGameplayContactFoulInput contact = {
        7u, 2u, 1u, 5u, 1u, 0x02F3u, 0u, 2, 2, 0u,
        0u, 0u, 3u, 0u, 45u, 25u
    };
    nba_gameplay_foul_init(&classifier);
    classifier_rng.state = 0xAB20u;
    if (nba_gameplay_foul_classify_contact(
            &classifier, &classifier_rng, &contact, NULL) ||
        classifier_rng.state != 0xAB20u) return false;
    contact.offender_movement_magnitude = 0x0300u;
    classifier_rng.state = 0x91A0u;
    if (!nba_gameplay_foul_classify_contact(
            &classifier, &classifier_rng, &contact, NULL) ||
        classifier_rng.state != 0x00A0u ||
        classifier.foul_event_raw_0964 != NBA_GAMEPLAY_FOUL_DEFENSIVE ||
        classifier.offender_actor_raw != 7 ||
        classifier.victim_actor_raw != 2) return false;

    nba_gameplay_foul_init(&classifier);
    contact.offender_actor = 2u; contact.victim_actor = 7u;
    contact.offender_team = 0u; contact.offender_group = 0u;
    contact.offender_mode = 11u; contact.offender_boost_raw_72 = 1u;
    contact.ball_owner_actor = 2; contact.offense_group_raw_093a = 0u;
    contact.defensive_rule_raw_17d1 = 45u;
    contact.offensive_rule_raw_17d3 = 25u;
    classifier_rng.state = 0x91C8u;
    if (!nba_gameplay_foul_classify_contact(
            &classifier, &classifier_rng, &contact, NULL) ||
        classifier_rng.state != 0x0064u ||
        classifier.foul_event_raw_0964 != NBA_GAMEPLAY_FOUL_CHARGING)
        return false;

    nba_gameplay_foul_init(&classifier);
    contact.offender_actor = 3u; contact.victim_actor = 7u;
    contact.offender_boost_raw_72 = 0u; contact.ball_owner_actor = 2;
    classifier_rng.state = 0x0064u;
    if (!nba_gameplay_foul_classify_contact(
            &classifier, &classifier_rng, &contact, NULL) ||
        classifier.foul_event_raw_0964 != NBA_GAMEPLAY_FOUL_OFFENSIVE)
        return false;

    nba_gameplay_foul_init(&classifier);
    contact.offender_actor = 7u; contact.victim_actor = 2u;
    contact.offender_team = 1u; contact.offender_group = 5u;
    contact.ball_owner_actor = 2; contact.offense_group_raw_093a = 0u;
    contact.context_tag = 0x87u; contact.offender_movement_magnitude = 0u;
    classifier_rng.state = 9u;
    if (nba_gameplay_foul_classify_contact(
            &classifier, &classifier_rng, &contact, NULL) ||
        classifier_rng.state != 4u) return false;

    nba_gameplay_foul_init(&classifier);
    contact.context_tag = 0u; contact.ball_activity_raw_0948 = 1u;
    contact.ball_owner_actor = -1; contact.last_shooter_actor = 2;
    contact.defensive_rule_raw_17d1 = 40u;
    classifier_rng.state = 0x00A0u;
    uint16_t detached_bits = 0x0080u;
    if (!nba_gameplay_foul_classify_contact(
            &classifier, &classifier_rng, &contact, &detached_bits) ||
        classifier.foul_event_raw_0964 != 0u ||
        classifier.shooting_foul_raw_09bc != 1u ||
        detached_bits != 0x2080u) return false;
    nba_gameplay_foul_init(&classifier);
    contact.last_shooter_actor = 3;
    classifier_rng.state = 0xCA55u;
    if (nba_gameplay_foul_classify_contact(
            &classifier, &classifier_rng, &contact, NULL) ||
        classifier_rng.state != 0xCA55u) return false;

    NbaGameplayFoulState violation;
    nba_gameplay_foul_init(&violation);
    uint16_t event_bits = 0x0010u;
    uint16_t inbound_ready = 9u;
    violation.contact_context_raw_497f = 7u;
    violation.presentation_gate_raw_08e2 = 9u;
    bool violation_ok = nba_gameplay_foul_record_violation(
            &violation, NBA_GAMEPLAY_VIOLATION_INTERFERENCE, 7u, 2u) &&
        violation.team_fouls[0] == 0u && violation.team_fouls[1] == 0u &&
        nba_gameplay_foul_consume_pending(
            &violation, 0u, &event_bits, &inbound_ready, false) &&
        violation.latched_event_raw_08f0 ==
            NBA_GAMEPLAY_VIOLATION_INTERFERENCE &&
        violation.foul_event_raw_0964 == 0u &&
        violation.whistle_active_raw_09b6 == 1u &&
        violation.presentation_pending_raw_4937 == 1u &&
        violation.contact_context_raw_497f == 0u && inbound_ready == 0u &&
        violation.whistle_timer_raw_08de == 300 &&
        violation.whistle_presentation_queued_raw == 1u &&
        violation.presentation_gate_raw_08e2 == 9u &&
        violation.whistle_state_raw_08e6 == 17u &&
        violation.whistle_state_mirror_raw_08e8 == 17u &&
        violation.side_event_bits_raw_13e9 == 1u &&
        event_bits == 0x2010u &&
        !nba_gameplay_foul_consume_pending(
            &violation, 0u, &event_bits, &inbound_ready, false);
    NbaGameplayFoulState busy_presentation;
    nba_gameplay_foul_init(&busy_presentation);
    busy_presentation.foul_event_raw_0964 = NBA_GAMEPLAY_VIOLATION_INTERFERENCE;
    busy_presentation.whistle_timer_raw_08de = 12;
    busy_presentation.presentation_gate_raw_08e2 = 7u;
    bool busy_ok = nba_gameplay_foul_consume_pending(
            &busy_presentation, 5u, NULL, NULL, true) &&
        busy_presentation.whistle_presentation_queued_raw == 0u &&
        busy_presentation.presentation_gate_raw_08e2 == 0u &&
        busy_presentation.whistle_timer_raw_08de == 120;
    return charging.team_fouls[0] == 0u &&
           charging.personal_fouls[2] == 6u &&
           charging.foul_event_raw_0964 == NBA_GAMEPLAY_FOUL_CHARGING &&
           charging.shooting_foul_raw_09bc == 0u &&
           charging.free_throw_state_raw_0978 == 0u &&
           charging.free_throw_sequence_raw_097a == 0u &&
           violation_ok && busy_ok;
}
