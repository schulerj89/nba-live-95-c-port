#include <stdint.h>
#include <stdio.h>
#include "nba_tipoff.h"

static int fail(const char *message) {
    fprintf(stderr, "[MATCHUP RUNTIME] %s\n", message);
    return 1;
}

static void clear_assignments(NbaTipoff *state) {
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        state->actors[i].assignment_current_raw = NBA_GAMEPLAY_UNKNOWN_WORD;
        state->actors[i].assignment_actor = 0xFFu;
        state->actors[i].control_mode = 0u;
    }
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    NbaAssetPack assets = {0};
    NbaSession session;
    NbaTipoff state;
    if (!nba_assets_load(&assets, argv[1])) return 3;
    nba_session_init(&session);
    if (!nba_tipoff_init(&state, &assets, &session)) return 4;

    clear_assignments(&state);
    state.team_context[1].anchor_x_raw_0a = -32768;
    state.actors[0].x_fp = 80 << 8;
    state.actors[0].anchor_distance_raw = 0x80u;
    state.actors[0].assignment_alternate_raw = 10u; /* actor five */
    state.actors[5].anchor_distance_raw = 0x50u;
    state.actors[5].x_fp = 64 << 8;
    uint8_t selected = 0xFFu;
    if (!nba_tipoff_replay_matchup_helper(&state, 0xBAE4u, 5u, 0u, 1u,
                                           &selected) || selected != 5u ||
        state.actors[5].assignment_current_raw != 0u ||
        state.actors[0].assignment_current_raw != NBA_GAMEPLAY_UNKNOWN_WORD)
        return fail("far opposite-half assignment is no longer one-way");

    clear_assignments(&state);
    state.team_context[1].anchor_x_raw_0a = 0x1000;
    state.actors[0].x_fp = 80 << 8;
    state.actors[0].assignment_alternate_raw = 10u;
    if (!nba_tipoff_replay_matchup_helper(&state, 0xBAE4u, 5u, 0u, 1u,
                                           &selected) ||
        state.actors[5].assignment_current_raw != 0u ||
        state.actors[0].assignment_current_raw != 10u)
        return fail("same-half assignment is no longer symmetric");

    clear_assignments(&state);
    state.actors[5].anchor_distance_raw = 4u;
    state.actors[6].anchor_distance_raw = 4u;
    if (!nba_tipoff_replay_matchup_helper(&state, 0xB9D2u, 5u, 0u, 1u,
                                           &selected) || selected != 5u)
        return fail("nearest-primary first-tie ordering changed");

    nba_assets_free(&assets);
    puts("[MATCHUP RUNTIME] PASS: one-way, symmetric, first-tie");
    return 0;
}
