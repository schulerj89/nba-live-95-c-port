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
 * IDs in `$492D/$492F`. The bookkeeping consumer increments team context
 * `+$28` and persistent player `+$14` (capped at six). If `$0948` says a
 * shot is detached, it defers the whistle through `$09BC`; a later make ORs
 * bit 15 in a separate scoring path. `$86:C65E-$C692` applies the team-foul
 * threshold whenever `$09BC` is clear, independent of which valid code won.
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
    unsigned team = offender_team;
    ++state->team_fouls[team];
    if (state->personal_fouls[offender_actor] < 6u)
        ++state->personal_fouls[offender_actor];
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
    state->whistle_audio_queued_raw = 0u;
    if (state->whistle_timer_raw_08de < 0)
        state->whistle_audio_queued_raw = 1u;
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
    NbaGameplayFoulState deferred;
    nba_gameplay_foul_init(&deferred);
    if (!nba_gameplay_foul_record_contact(
            &deferred, NBA_GAMEPLAY_FOUL_OFFENSIVE, 7u, 2u,
            1u, true, 6u) || deferred.team_fouls[1] != 1u ||
        deferred.personal_fouls[7] != 1u ||
        deferred.foul_event_raw_0964 != 0u ||
        deferred.shooting_foul_raw_09bc != 1u ||
        deferred.free_throw_state_raw_0978 != 0u ||
        !nba_gameplay_foul_record_made_basket(&deferred) ||
        deferred.shooting_foul_raw_09bc != 0x8001u) return false;
    NbaGameplayFoulState offensive_bonus;
    nba_gameplay_foul_init(&offensive_bonus);
    for (unsigned i = 0; i < 4u; ++i)
        if (!nba_gameplay_foul_record_contact(
                &offensive_bonus, NBA_GAMEPLAY_FOUL_OFFENSIVE, 2u, 7u,
                0u, false, 6u)) return false;
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
        violation.whistle_audio_queued_raw == 1u &&
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
        busy_presentation.whistle_audio_queued_raw == 0u &&
        busy_presentation.presentation_gate_raw_08e2 == 0u &&
        busy_presentation.whistle_timer_raw_08de == 120;
    return offensive_bonus.team_fouls[0] == 4u &&
           offensive_bonus.personal_fouls[2] == 4u &&
           offensive_bonus.foul_event_raw_0964 ==
               NBA_GAMEPLAY_FOUL_OFFENSIVE &&
           offensive_bonus.shooting_foul_raw_09bc == 0u &&
           offensive_bonus.free_throw_state_raw_0978 == 1u &&
           offensive_bonus.free_throw_sequence_raw_097a == 2u &&
           violation_ok && busy_ok;
}
