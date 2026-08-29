#include <stdio.h>
#include "nba_gameplay_foul.h"

int main(void) {
    unsigned v[9];
    for (;;) {
        for (unsigned i = 0; i < 9u; ++i)
            if (scanf("%x", &v[i]) != 1) return i ? 2 : 0;
        NbaGameplayFoulState state;
        nba_gameplay_foul_init(&state);
        int8_t stat_slot = (int8_t)(int16_t)(uint16_t)v[2];
        state.personal_fouls[v[0]] = (uint8_t)v[3];
        state.team_active_roster_count[v[1]] = (uint16_t)v[4];
        if (stat_slot >= 0 && stat_slot < 5)
            state.game_foul_stats[(uint8_t)stat_slot] = (uint16_t)v[5];
        state.foul_out_state_raw_09ca = (uint16_t)v[7];
        state.substitution_request_raw_0a08 = (uint16_t)v[8];
        if (!nba_gameplay_foul_record_bookkeeping(
                &state, (uint8_t)v[0], (uint8_t)v[1], stat_slot,
                v[6] != 0u)) return 3;
        unsigned stat = stat_slot >= 0 && stat_slot < 5 ?
            state.game_foul_stats[(uint8_t)stat_slot] : 0u;
        printf("%x %x %x %x %x\n", state.personal_fouls[v[0]],
               state.team_active_roster_count[v[1]], stat,
               state.foul_out_state_raw_09ca,
               state.substitution_request_raw_0a08);
    }
}
