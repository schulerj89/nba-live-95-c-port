#include "nba_tipoff.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "foul-out substitution runtime line %d\n", __LINE__); \
    return false; \
} } while (0)

static bool expected_selection(const NbaTipoff *game, unsigned actor,
                               NbaGameplaySubstitutionResult *result) {
    unsigned side = actor / NBA_MATCH_LINEUP_SIZE;
    NbaGameplaySubstitutionInput input = {0};
    input.outgoing_lineup_index = (uint8_t)(actor % NBA_MATCH_LINEUP_SIZE);
    memcpy(input.roster_order, game->session->match.roster_order[side],
           sizeof(input.roster_order));
    memcpy(input.eligible, game->session->match.roster_available[side],
           sizeof(input.eligible));
    input.eligible[input.roster_order[input.outgoing_lineup_index]] = false;
    uint8_t team = side ? game->session->left_team : game->session->right_team;
    for (uint8_t roster = 0; roster < NBA_MATCH_ROSTER_SIZE; ++roster)
        CHECK(nba_player_gameplay_position(
            game->assets, team, roster, &input.position[roster]));
    return nba_gameplay_select_foul_out_replacement(&input, result);
}

static bool selection_vectors(void) {
    NbaGameplaySubstitutionInput input = {0};
    static const uint8_t order[12] = {2u,0u,1u,3u,4u,5u,6u,7u,8u,9u,10u,11u};
    memcpy(input.roster_order, order, sizeof(order));
    input.outgoing_lineup_index = 0u;
    for (unsigned i = 0; i < 12u; ++i) input.eligible[i] = true;
    input.position[2] = 1u;
    input.position[5] = 2u;
    input.position[6] = 1u;
    NbaGameplaySubstitutionResult result;
    CHECK(nba_gameplay_select_foul_out_replacement(&input, &result));
    CHECK(result.replacement_order_index == 6u &&
          result.replacement_roster == 6u && result.roster_order[0] == 6u &&
          result.roster_order[6] == 2u);
    input.position[6] = 2u;
    CHECK(nba_gameplay_select_foul_out_replacement(&input, &result));
    CHECK(result.replacement_order_index == 5u &&
          result.replacement_roster == 5u);
    NbaGameplaySubstitutionResult sentinel;
    memset(&sentinel, 0xA5, sizeof(sentinel));
    NbaGameplaySubstitutionResult before = sentinel;
    input.roster_order[11] = input.roster_order[10];
    CHECK(!nba_gameplay_select_foul_out_replacement(&input, &sentinel) &&
          memcmp(&sentinel, &before, sizeof(sentinel)) == 0);
    return true;
}

static bool run_case(const NbaAssetPack *pack, uint8_t left, uint8_t right,
                     uint8_t actor) {
    NbaSession session;
    NbaTipoff game;
    nba_session_init(&session);
    session.left_team = left; session.right_team = right;
    CHECK(nba_tipoff_init(&game, pack, &session));
    NbaGameplaySubstitutionResult expected;
    CHECK(expected_selection(&game, actor, &expected));
    unsigned side = actor / NBA_MATCH_LINEUP_SIZE;
    unsigned persistent_out = side * NBA_MATCH_ROSTER_SIZE +
        expected.outgoing_roster;
    unsigned persistent_in = side * NBA_MATCH_ROSTER_SIZE +
        expected.replacement_roster;
    for (unsigned stat = 0; stat < 5u; ++stat) {
        game.actors[actor].shot_statistics[stat] = (uint16_t)(10u + stat);
        game.roster_shot_statistics[persistent_in][stat] =
            (uint16_t)(70u + stat);
    }
    game.fatigue.stamina[persistent_in] = 0x4321u;
    game.fouls.personal_fouls[actor] = 5u;
    CHECK(nba_gameplay_foul_record_bookkeeping(
        &game.fouls, actor, (uint8_t)side, -1, true));
    CHECK(game.fouls.substitution_request_raw_0a08 == 1u &&
          game.fouls.foul_out_state_raw_09ca == 8u &&
          game.fouls.substitution_actor_raw_492d == (int8_t)actor);
    CHECK(nba_tipoff_apply_foul_out_substitution(&game));
    CHECK(game.fouls.substitution_request_raw_0a08 == 0u &&
          game.fouls.foul_out_state_raw_09ca == 0u &&
          game.fouls.substitution_actor_raw_492d == -1);
    CHECK(!session.match.roster_available[side][expected.outgoing_roster]);
    CHECK(memcmp(session.match.roster_order[side], expected.roster_order,
                 sizeof(expected.roster_order)) == 0);
    CHECK(session.match.active_lineup[side][actor % 5u] ==
              expected.replacement_roster &&
          game.actors[actor].roster_slot == expected.replacement_roster);
    CHECK(game.fatigue.active_roster[actor] == persistent_in &&
          game.actors[actor].shot_stamina_raw_18 == 0x4321u);
    CHECK(game.roster_personal_fouls[persistent_out] == 6u &&
          game.fouls.personal_fouls[actor] ==
              game.roster_personal_fouls[persistent_in]);
    for (unsigned stat = 0; stat < 5u; ++stat) {
        CHECK(game.roster_shot_statistics[persistent_out][stat] ==
              (uint16_t)(10u + stat));
        CHECK(game.actors[actor].shot_statistics[stat] ==
              (uint16_t)(70u + stat));
    }
    uint16_t variant = 0u;
    uint8_t team = side ? left : right;
    CHECK(nba_player_gameplay_animation_variant(
        pack, team, expected.replacement_roster, &variant));
    CHECK(game.actors[actor].animation_variant_raw_6c == variant &&
          game.actors[actor].animation_resources_valid);
    for (unsigned current = 0; current < NBA_GAMEPLAY_ACTOR_COUNT; ++current) {
        unsigned paired = game.actors[current].assignment_base_raw >> 1;
        CHECK(paired < NBA_GAMEPLAY_ACTOR_COUNT &&
              game.actors[current].assignment_actor == paired &&
              game.actors[current].assignment_current_raw ==
                  game.actors[current].assignment_base_raw);
    }
    return true;
}

static bool no_bench_is_atomic(const NbaAssetPack *pack) {
    NbaSession session;
    NbaTipoff game;
    nba_session_init(&session);
    CHECK(nba_tipoff_init(&game, pack, &session));
    const unsigned actor = 0u, side = 0u;
    for (unsigned i = NBA_MATCH_LINEUP_SIZE;
         i < NBA_MATCH_ROSTER_SIZE; ++i)
        session.match.roster_available[side]
            [session.match.roster_order[side][i]] = false;
    game.fouls.personal_fouls[actor] = 5u;
    CHECK(nba_gameplay_foul_record_bookkeeping(
        &game.fouls, (uint8_t)actor, (uint8_t)side, -1, true));
    NbaMatchLifecycle match_before = session.match;
    NbaTipoffActor actors_before[NBA_GAMEPLAY_ACTOR_COUNT];
    NbaShotFatigue fatigue_before = game.fatigue;
    uint16_t stats_before[24][5];
    uint8_t personal_before[24];
    memcpy(actors_before, game.actors, sizeof(actors_before));
    memcpy(stats_before, game.roster_shot_statistics, sizeof(stats_before));
    memcpy(personal_before, game.roster_personal_fouls,
           sizeof(personal_before));
    CHECK(!nba_tipoff_apply_foul_out_substitution(&game));
    CHECK(memcmp(&session.match, &match_before, sizeof(match_before)) == 0 &&
          memcmp(game.actors, actors_before, sizeof(actors_before)) == 0 &&
          memcmp(&game.fatigue, &fatigue_before, sizeof(fatigue_before)) == 0 &&
          memcmp(game.roster_shot_statistics, stats_before,
                 sizeof(stats_before)) == 0 &&
          memcmp(game.roster_personal_fouls, personal_before,
                 sizeof(personal_before)) == 0);
    CHECK(game.fouls.substitution_request_raw_0a08 == 1u &&
          game.fouls.foul_out_state_raw_09ca == 8u &&
          game.fouls.substitution_actor_raw_492d == 0);
    return true;
}

int main(int argc, char **argv) {
    NbaAssetPack pack = {0};
    if (argc != 2 || !nba_assets_load(&pack, argv[1])) return 2;
    bool ok = selection_vectors() &&
              run_case(&pack, 3u, 18u, 0u) &&
              run_case(&pack, 12u, 10u, 7u) &&
              no_bench_is_atomic(&pack);
    nba_assets_free(&pack);
    if (!ok) return 10;
    puts("FOUL_OUT_SUBSTITUTION PASS teams=4 sides=2 appearance=2 no_bench=atomic");
    return 0;
}
