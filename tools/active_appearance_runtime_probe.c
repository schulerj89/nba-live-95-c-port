#include "nba_tipoff.h"
#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "active appearance runtime line %d\n", __LINE__); \
        return 10; \
    } \
} while (0)

int main(int argc, char **argv) {
    NbaAssetPack pack = {0};
    if (argc != 2 || !nba_assets_load(&pack, argv[1])) return 2;
    static const uint8_t lineup[5] = {2u, 0u, 1u, 3u, 4u};
    NbaSession session;
    NbaTipoff game;
    nba_session_init(&session);
    CHECK(nba_tipoff_init(&game, &pack, &session));
    for (unsigned actor = 0; actor < 10u; ++actor) {
        const NbaTipoffActor *state = &game.actors[actor];
        unsigned logical = actor % 5u;
        unsigned paired = actor < 5u ? 5u + logical : logical;
        /* Native first-court context order is home/right then visitor/left;
         * the independent64-field witness separately verifies publication. */
        uint8_t team = actor < 5u ? session.right_team : session.left_team;
        uint16_t variant = 0u;
        CHECK(state->roster_slot == lineup[logical]);
        CHECK(state->assignment_base_raw == paired * 2u);
        CHECK(state->assignment_current_raw == paired * 2u);
        CHECK(state->assignment_alternate_raw == paired * 2u);
        CHECK(state->assignment_actor == paired);
        CHECK(state->help_request_raw_80 == 0u);
        CHECK(nba_player_gameplay_animation_variant(
            &pack, team, lineup[logical], &variant));
        CHECK(state->animation_variant_raw_6c == variant);
    }
    /* Exhaust every team through the same two helpers used by the adapter;
     * the live binding above stays on the configured matchup so unrelated
     * team-specific pass self-tests do not constrain this appearance test. */
    unsigned checked = 10u;
    for (unsigned team = 0; team < NBA_TEAM_COUNT; ++team) {
        uint8_t teams[10], rosters[10];
        for (unsigned actor = 0; actor < 10u; ++actor) {
            teams[actor] = (uint8_t)team;
            rosters[actor] = lineup[actor % 5u];
        }
        NbaPlayerAppearanceSetup appearance;
        CHECK(nba_player_appearance_setup(&pack, teams, rosters, &appearance));
        for (unsigned actor = 0; actor < 10u; ++actor)
            CHECK(appearance.players[actor].dirty == 0xFFFFu);
        checked += 10u;
    }
    nba_assets_free(&pack);
    printf("active appearance runtime binding passed: %u actor records, all 29 teams\n",
           checked);
    return 0;
}
