#include "nba_game.h"
#include "nba_player_graphics_map.h"
#include "nba_player_lab.h"
#include <stdio.h>
#include <string.h>

static NbaGame game;

/* Host-only gameplay-graphics test for `$86:D7B8-$D85D`; no direct native
 * address. Emit the portable outputs at the exact replay boundary. */
static void emit_map(const NbaPlayerGraphicsMap *map) {
    fputs("ACTIVE", stdout);
    for (unsigned i = 0; i < NBA_PLAYER_GRAPHICS_MAP_ACTORS; ++i)
        printf(" %06x", (unsigned)map->active_roster_address[i]);
    fputs("\nSTATS", stdout);
    for (unsigned i = 0; i < NBA_PLAYER_GRAPHICS_MAP_ACTORS; ++i)
        printf(" %04x", map->statistics_address[i]);
    putchar('\n');
}

/* Host-only gameplay-graphics test for `$86:D7B8-$D85D`; no direct native
 * address. Replay one captured entry through the production implementation. */
static int replay_stdin(void) {
    NbaPlayerGraphicsMapInput input = {0};
    NbaPlayerGraphicsMap output;
    unsigned value;
    for (unsigned side = 0; side < NBA_PLAYER_GRAPHICS_MAP_SIDES; ++side)
        for (unsigned i = 0; i < NBA_PLAYER_GRAPHICS_MAP_ACTIVE; ++i) {
            if (scanf_s("%x", &value) != 1 || value > 0xFFFFu) return 2;
            input.lineup[side][i] = (uint16_t)value;
        }
    for (unsigned side = 0; side < NBA_PLAYER_GRAPHICS_MAP_SIDES; ++side)
        for (unsigned i = 0; i < NBA_PLAYER_GRAPHICS_MAP_ROSTER; ++i) {
            if (scanf_s("%x", &value) != 1 || value > 0xFFFFFFu) return 2;
            input.roster_address[side][i] = value;
        }
    if (!nba_player_graphics_map(&input, &output)) return 3;
    emit_map(&output);
    return 0;
}

/* Host-only gameplay-graphics test for `$86:D7B8-$D85D`; no direct native
 * address. Exercise both side offsets, nontrivial permutations, roster slot
 * eleven and atomic rejection. */
static int self_test(void) {
    NbaPlayerGraphicsMapInput input = {0};
    const uint16_t lineup[2][5] = {{11, 2, 8, 0, 5}, {7, 10, 1, 9, 3}};
    for (unsigned side = 0; side < 2; ++side) {
        memcpy(input.lineup[side], lineup[side], sizeof(lineup[side]));
        for (unsigned roster = 0; roster < 12; ++roster)
            input.roster_address[side][roster] =
                0xA00000u + side * 0x10000u + roster * 0x101u;
    }
    NbaPlayerGraphicsMap output;
    memset(&output, 0xA5, sizeof(output));
    if (!nba_player_graphics_map(&input, &output)) return 10;
    for (unsigned side = 0; side < 2; ++side)
        for (unsigned active = 0; active < 5; ++active) {
            unsigned actor = side * 5 + active;
            unsigned roster = lineup[side][active];
            if (output.active_roster_address[actor] !=
                    input.roster_address[side][roster] ||
                output.statistics_address[actor] !=
                    0x40EBu + 0x40u * (roster + side * 12u))
                return 11;
        }
    NbaPlayerGraphicsMap before = output;
    input.lineup[1][4] = 12;
    if (nba_player_graphics_map(&input, &output) ||
        memcmp(&output, &before, sizeof(output))) return 12;
    if (nba_player_graphics_map(NULL, &output) ||
        nba_player_graphics_map(&input, NULL)) return 13;
    puts("SELFTEST ok");
    return 0;
}

/* Host-only gameplay-graphics test for `$86:D7B8-$D85D`; no direct native
 * address. Check all ten initialization consumers of the selected records. */
static bool initialized_appearance_matches_map(
    const NbaTipoff *tipoff, const NbaPlayerGraphicsMap *map) {
    NbaPlayerAppearanceSetup expected;
    if (!nba_player_appearance_setup_from_addresses(
            tipoff->assets, map->active_roster_address, &expected))
        return false;
    for (unsigned actor = 0; actor < 10; ++actor) {
        const NbaTipoffActor *actual = &tipoff->actors[actor];
        const NbaPlayerAppearance *want = &expected.players[actor];
        unsigned persistent = (map->statistics_address[actor] - 0x40EBu) / 0x40u;
        if (tipoff->fatigue.active_roster[actor] != persistent ||
            actual->free_throw_launch_half_raw_a8 != want->alternate_lower ||
            actual->animation_variant_raw_6c != want->upper_variant ||
            actual->head_resource_base_raw_2e != want->head_resource ||
            actual->player_palette_offset_raw_ac != want->palette_offset)
            return false;
    }
    return true;
}

/* Host-only gameplay-graphics test for `$86:D7B8-$D85D`; no direct native
 * address. Check every substitution consumer while intentionally excluding
 * head/palette/jersey fields that this caller does not republish. */
static bool substitution_matches_map(
    const NbaTipoff *tipoff, const NbaPlayerGraphicsMap *map,
    const uint16_t expected_stats[24][5],
    const uint8_t expected_fouls[24], const uint16_t old_status[10]) {
    NbaPlayerAppearanceSetup appearance;
    if (!nba_player_appearance_setup_from_addresses(
            tipoff->assets, map->active_roster_address, &appearance))
        return false;
    for (unsigned actor = 0; actor < 10; ++actor) {
        const NbaTipoffActor *actual = &tipoff->actors[actor];
        unsigned side = actor / 5u;
        unsigned active = actor % 5u;
        unsigned roster = tipoff->session->match.active_lineup[side][active];
        unsigned persistent = roster + side * 12u;
        uint32_t address;
        uint16_t upper, lower;
        uint16_t body = (uint16_t)(old_status[actor] & 0x7FFFu);
        if (actual->direction < 3u) body |= 0x8000u;
        if (!nba_player_gameplay_roster_address(
                tipoff->assets,
                (uint8_t)tipoff->team_context[side].strategy_team_raw_00,
                (uint8_t)roster, &address) ||
            map->active_roster_address[actor] != address ||
            map->statistics_address[actor] != 0x40EBu + 0x40u * persistent ||
            actual->roster_slot != roster ||
            tipoff->fatigue.active_roster[actor] != persistent ||
            memcmp(actual->shot_statistics, expected_stats[persistent],
                   sizeof(actual->shot_statistics)) ||
            tipoff->fouls.personal_fouls[actor] != expected_fouls[persistent] ||
            actual->shot_stamina_raw_18 != tipoff->fatigue.stamina[persistent] ||
            actual->free_throw_launch_half_raw_a8 !=
                appearance.players[actor].alternate_lower ||
            actual->animation_variant_raw_6c !=
                appearance.players[actor].upper_variant ||
            !nba_player_animation_resources_for_appearance(
                tipoff->assets, actual->animation_state,
                actual->lower_animation_state, actual->direction,
                actual->upper_animation_tick, actual->lower_animation_tick,
                actual->free_throw_launch_half_raw_a8 != 0u,
                actual->animation_variant_raw_6c, &upper, &lower) ||
            !actual->animation_resources_valid ||
            actual->upper_animation_resource_raw_2a != upper ||
            actual->lower_animation_resource_raw_2c != lower ||
            actual->actor_status_raw_28 != body)
            return false;
    }
    return true;
}

/* Host-only gameplay-graphics test for `$86:D7B8-$D85D`; no direct native
 * address. Exercise the normal AF9E-equivalent graphics entry and a committed
 * foul-out lineup swap. */
static int caller_test(const char *pack_path) {
    static const uint8_t initial[2][5] = {
        {2, 0, 1, 3, 4}, {2, 0, 1, 3, 4}
    };
    if (!nba_game_init(&game, NULL, pack_path)) return 20;
    game.session.right_team = 7;
    game.session.left_team = 18;
    for (unsigned side = 0; side < 2; ++side) {
        bool used[12] = {0};
        memcpy(game.session.match.active_lineup[side], initial[side], 5);
        memcpy(game.session.match.roster_order[side], initial[side], 5);
        for (unsigned i = 0; i < 5; ++i) used[initial[side][i]] = true;
        unsigned at = 5;
        for (unsigned roster = 0; roster < 12; ++roster)
            if (!used[roster])
                game.session.match.roster_order[side][at++] = (uint8_t)roster;
    }
    if (!nba_game_enter_state(&game, NBA_STATE_TIPOFF)) {
        nba_game_shutdown(&game);
        return 21;
    }
    NbaTipoff *tipoff = &game.scene.tipoff;
    uint8_t teams[2] = {game.session.right_team, game.session.left_team};
    NbaPlayerGraphicsMap map;
    if (!nba_player_graphics_map_from_assets(
            &game.assets, teams, game.session.match.active_lineup, &map) ||
        !initialized_appearance_matches_map(tipoff, &map)) {
        nba_game_shutdown(&game);
        return 22;
    }
    uint16_t expected_stats[24][5];
    uint8_t expected_fouls[24];
    uint16_t old_status[10];
    for (unsigned persistent = 0; persistent < 24; ++persistent) {
        for (unsigned stat = 0; stat < 5; ++stat)
            tipoff->roster_shot_statistics[persistent][stat] =
                (uint16_t)(0x2000u + persistent * 0x10u + stat);
        tipoff->roster_personal_fouls[persistent] =
            (uint8_t)(0x40u + persistent);
        tipoff->fatigue.stamina[persistent] =
            (uint16_t)(0x3000u + persistent * 0x20u);
    }
    memcpy(expected_stats, tipoff->roster_shot_statistics,
           sizeof(expected_stats));
    memcpy(expected_fouls, tipoff->roster_personal_fouls,
           sizeof(expected_fouls));
    for (unsigned actor = 0; actor < 10; ++actor) {
        unsigned side = actor / 5u;
        unsigned persistent = side * 12u + tipoff->actors[actor].roster_slot;
        for (unsigned stat = 0; stat < 5; ++stat)
            tipoff->actors[actor].shot_statistics[stat] =
                (uint16_t)(0x5000u + actor * 0x10u + stat);
        tipoff->fouls.personal_fouls[actor] = (uint8_t)(actor + 1u);
        memcpy(expected_stats[persistent], tipoff->actors[actor].shot_statistics,
               sizeof(expected_stats[persistent]));
        expected_fouls[persistent] = tipoff->fouls.personal_fouls[actor];
        tipoff->actors[actor].upper_animation_resource_raw_2a = 0xDEADu;
        tipoff->actors[actor].lower_animation_resource_raw_2c = 0xBEEFu;
        tipoff->actors[actor].animation_resources_valid = false;
        tipoff->actors[actor].actor_status_raw_28 =
            (uint16_t)(0x8100u + actor);
        old_status[actor] = tipoff->actors[actor].actor_status_raw_28;
    }
    uint8_t old_roster = tipoff->actors[0].roster_slot;
    tipoff->fouls.substitution_request_raw_0a08 = 1;
    tipoff->fouls.foul_out_state_raw_09ca = 8;
    tipoff->fouls.substitution_actor_raw_492d = 0;
    if (!nba_tipoff_apply_foul_out_substitution(tipoff) ||
        tipoff->actors[0].roster_slot == old_roster ||
        !nba_player_graphics_map_from_assets(
            &game.assets, teams, game.session.match.active_lineup, &map) ||
        !substitution_matches_map(
            tipoff, &map, expected_stats, expected_fouls, old_status)) {
        nba_game_shutdown(&game);
        return 23;
    }
    printf("CALLER init=%u substitution=%u\n", old_roster,
           tipoff->actors[0].roster_slot);
    nba_game_shutdown(&game);
    return 0;
}

/* Host-only gameplay-graphics test for `$86:D7B8-$D85D`; no direct native
 * address. Dispatch its replay, asset-free, and production-caller cases. */
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "replay")) return replay_stdin();
    if (argc == 2 && !strcmp(argv[1], "selftest")) return self_test();
    if (argc == 3 && !strcmp(argv[1], "caller")) return caller_test(argv[2]);
    fprintf(stderr, "usage: player_graphics_map_probe replay|selftest|caller PACK\n");
    return 2;
}
