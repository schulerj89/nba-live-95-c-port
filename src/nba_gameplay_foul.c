#include "nba_gameplay_foul.h"
#include <string.h>

void nba_gameplay_foul_init(NbaGameplayFoulState *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->offender_actor_raw = -1;
    state->victim_actor_raw = -1;
}

/* `$86:C4FE-$C6AC` emits pending `$0964` codes 1/2/13 and preserves actor
 * IDs in `$492D/$492F`. The bookkeeping consumer increments team context
 * `+$28` and persistent player `+$14` (capped at six). If `$0948` says a
 * shot is detached, it defers the whistle through `$09BC`; a later make ORs
 * bit 15 in a separate scoring path. `$86:C65E-$C692` applies the team-foul
 * threshold whenever `$09BC` is clear, independent of which valid code won.
 * The caller supplies the team resolved through actor `+$70`; it is not
 * inferred from the actor slot.
 * This pure hook is intentionally not called by host collision code until a
 * deterministic Mesen detector trace establishes the contact predicate. */
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
    return offensive_bonus.team_fouls[0] == 4u &&
           offensive_bonus.personal_fouls[2] == 4u &&
           offensive_bonus.foul_event_raw_0964 ==
               NBA_GAMEPLAY_FOUL_OFFENSIVE &&
           offensive_bonus.shooting_foul_raw_09bc == 0u &&
           offensive_bonus.free_throw_state_raw_0978 == 1u &&
           offensive_bonus.free_throw_sequence_raw_097a == 2u;
}
