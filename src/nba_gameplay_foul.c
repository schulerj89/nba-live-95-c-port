#include "nba_gameplay_foul.h"
#include <string.h>

void nba_gameplay_foul_init(NbaGameplayFoulState *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->offender_actor_raw = -1;
    state->victim_actor_raw = -1;
}

/* `$86:C500-$C6A9` emits pending `$0964` codes 1/2/13 and preserves actor
 * IDs in `$492D/$492F`. The bookkeeping consumer increments team context
 * `+$28` and persistent player `+$14` (capped at six). If `$0948` says a
 * shot is detached, it defers the whistle through `$09BC`; a later make ORs
 * bit 15. Only the defensive event enters the `$0978/$097A` bonus path.
 * This pure hook is intentionally not called by host collision code until a
 * deterministic Mesen detector trace establishes the contact predicate. */
bool nba_gameplay_foul_record_contact(NbaGameplayFoulState *state,
                                      uint8_t event_code,
                                      uint8_t offender_actor,
                                      uint8_t victim_actor,
                                      bool shot_detached,
                                      bool shot_made,
                                      uint16_t period_raw_0926) {
    if (!state || offender_actor >= 10u || victim_actor >= 10u ||
        (event_code != NBA_GAMEPLAY_FOUL_DEFENSIVE &&
         event_code != NBA_GAMEPLAY_FOUL_CHARGING &&
         event_code != NBA_GAMEPLAY_FOUL_OFFENSIVE)) return false;
    state->offender_actor_raw = (int8_t)offender_actor;
    state->victim_actor_raw = (int8_t)victim_actor;
    unsigned team = offender_actor / 5u;
    ++state->team_fouls[team];
    if (state->personal_fouls[offender_actor] < 6u)
        ++state->personal_fouls[offender_actor];
    if (shot_detached) {
        state->foul_event_raw_0964 = 0u;
        state->shooting_foul_raw_09bc =
            (uint16_t)(1u | (shot_made ? 0x8000u : 0u));
    } else {
        state->foul_event_raw_0964 = event_code;
        state->shooting_foul_raw_09bc = 0u;
    }
    if (event_code == NBA_GAMEPLAY_FOUL_DEFENSIVE) {
        uint16_t threshold = period_raw_0926 < 4u ? 4u : 5u;
        if (state->team_fouls[team] >= threshold) {
            state->free_throw_state_raw_0978 = 1u;
            state->free_throw_sequence_raw_097a = 2u;
        }
    }
    return true;
}

bool nba_gameplay_foul_self_test(void) {
    NbaGameplayFoulState state;
    nba_gameplay_foul_init(&state);
    if (state.offender_actor_raw != -1 || state.victim_actor_raw != -1 ||
        nba_gameplay_foul_record_contact(&state, 7u, 0u, 5u,
                                         false, false, 0u)) return false;
    for (unsigned i = 0; i < 4u; ++i)
        if (!nba_gameplay_foul_record_contact(
                &state, NBA_GAMEPLAY_FOUL_DEFENSIVE, 2u, 7u,
                false, false, 3u)) return false;
    if (state.team_fouls[0] != 4u || state.personal_fouls[2] != 4u ||
        state.foul_event_raw_0964 != NBA_GAMEPLAY_FOUL_DEFENSIVE ||
        state.free_throw_state_raw_0978 != 1u ||
        state.free_throw_sequence_raw_097a != 2u) return false;
    for (unsigned i = 0; i < 4u; ++i)
        if (!nba_gameplay_foul_record_contact(
                &state, NBA_GAMEPLAY_FOUL_OFFENSIVE, 2u, 7u,
                true, i == 3u, 6u)) return false;
    return state.team_fouls[0] == 8u && state.personal_fouls[2] == 6u &&
           state.foul_event_raw_0964 == 0u &&
           state.shooting_foul_raw_09bc == 0x8001u;
}
