#include "nba_assets.h"
#include "nba_player_intro.h"
#include "nba_player_setup.h"
#include "nba_renderer.h"
#include "nba_session.h"
#include "nba_team_select.h"
#include <stdio.h>
#include <string.h>

static uint64_t frame_hash(const NbaRenderer *renderer) {
    const uint8_t *p = (const uint8_t *)renderer->pixels;
    uint64_t hash = 1469598103934665603ull;
    for (size_t i = 0; i < sizeof(renderer->pixels); ++i) {
        hash ^= p[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

static void press(NbaInput *input, uint16_t button) {
    memset(input, 0, sizeof(*input));
    input->held = input->pressed = button;
}

static int run_pair(const NbaAssetPack *assets, uint8_t away, uint8_t home,
                    uint64_t hashes[5]) {
    NbaSession session;
    nba_session_init(&session);
    session.left_team = away;
    session.right_team = home;
    session.player_one_side = NBA_TEAM_SIDE_RIGHT;
    NbaRenderer renderer;
    nba_renderer_init(&renderer);
    NbaInput input = {0};

    /* `$82:809A-$91FF`: run the actual transition cadence, exercise side and
     * category navigation, and retain the selected pair through confirmation. */
    NbaTeamSelect select;
    if (!nba_team_select_init(&select, assets, &session)) return 10;
    for (int frame = 0; frame < NBA_TEAM_TRANSITION_FRAMES; ++frame)
        nba_team_select_update(&select, NULL);
    press(&input, NBA_BTN_L);
    if (nba_team_select_update(&select, &input) != NBA_TEAM_SOUND_SIDE ||
        select.active_side != NBA_TEAM_SIDE_LEFT) return 11;
    press(&input, NBA_BTN_DOWN);
    if (nba_team_select_update(&select, &input) != NBA_TEAM_SOUND_CATEGORY ||
        select.selector != NBA_TEAM_SELECT_SCORING) return 12;
    press(&input, NBA_BTN_UP);
    if (nba_team_select_update(&select, &input) != NBA_TEAM_SOUND_CATEGORY ||
        select.selector != NBA_TEAM_SELECT_LEFT_NAME) return 13;
    press(&input, NBA_BTN_L);
    if (nba_team_select_update(&select, &input) != NBA_TEAM_SOUND_SIDE ||
        select.active_side != NBA_TEAM_SIDE_RIGHT) return 14;
    nba_team_select_render(&select, &renderer);
    hashes[0] = frame_hash(&renderer);
    press(&input, NBA_BTN_START);
    if (nba_team_select_update(&select, &input) != NBA_TEAM_SOUND_CONFIRM ||
        !select.confirm_requested || session.left_team != away ||
        session.right_team != home) return 15;
    nba_team_select_shutdown(&select);

    /* `$82:8553/$82:863C` and the Player Setup transition preserve the home
     * wallpaper and controller ownership selected by the previous scene. */
    NbaPlayerSetup setup;
    if (!nba_player_setup_init(&setup, assets, &session, renderer.pixels))
        return 20;
    for (int frame = 0; frame < NBA_PLAYER_SETUP_TRANSITION_FRAMES; ++frame)
        nba_player_setup_update(&setup, NULL);
    nba_player_setup_render(&setup, &renderer);
    hashes[1] = frame_hash(&renderer);
    press(&input, NBA_BTN_LEFT);
    if (nba_player_setup_update(&setup, &input) != NBA_PLAYER_SETUP_SOUND_MOVE ||
        session.controller_selection[0] != 1) return 21;
    /* Native $81:A7D0-$A843 moves one slot per press: right -> neutral ->
     * left. The legacy UI side has no neutral value; assert canonical state. */
    nba_player_setup_update(&setup, NULL);
    press(&input, NBA_BTN_LEFT);
    if (nba_player_setup_update(&setup, &input) != NBA_PLAYER_SETUP_SOUND_MOVE ||
        session.controller_selection[0] != 0 ||
        session.player_one_side != NBA_TEAM_SIDE_LEFT) return 24;
    nba_player_setup_update(&setup, NULL);
    press(&input, NBA_BTN_RIGHT);
    if (nba_player_setup_update(&setup, &input) != NBA_PLAYER_SETUP_SOUND_MOVE ||
        session.controller_selection[0] != 1) return 22;
    nba_player_setup_update(&setup, NULL);
    press(&input, NBA_BTN_RIGHT);
    if (nba_player_setup_update(&setup, &input) != NBA_PLAYER_SETUP_SOUND_MOVE ||
        session.controller_selection[0] != 2 ||
        session.player_one_side != NBA_TEAM_SIDE_RIGHT) return 25;
    nba_player_setup_update(&setup, NULL);
    press(&input, NBA_BTN_START);
    if (nba_player_setup_update(&setup, &input) !=
            NBA_PLAYER_SETUP_SOUND_CONFIRM || !setup.confirm_requested)
        return 23;

    /* `$83:F000-$FA90`: advance the production matchup, five-row ratings and
     * ten-card lineup state machine without bypassing its frame counters. */
    NbaPlayerIntro intro;
    if (!nba_player_intro_init(&intro, assets, &session, renderer.pixels))
        return 30;
    nba_player_setup_shutdown(&setup);
    for (int frame = 0; frame < NBA_PLAYER_INTRO_TRANSITION_FRAMES; ++frame)
        nba_player_intro_update(&intro, NULL);
    if (intro.phase != NBA_PLAYER_INTRO_MATCHUP) return 31;
    nba_player_intro_render(&intro, &renderer);
    hashes[2] = frame_hash(&renderer);
    for (int frame = 0; frame < NBA_PLAYER_INTRO_MATCHUP_FRAMES; ++frame)
        nba_player_intro_update(&intro, NULL);
    if (intro.phase != NBA_PLAYER_INTRO_RATINGS) return 32;
    nba_player_intro_render(&intro, &renderer);
    hashes[3] = frame_hash(&renderer);
    for (int frame = 0; frame < NBA_PLAYER_INTRO_RATINGS_FRAMES; ++frame)
        nba_player_intro_update(&intro, NULL);
    if (intro.phase != NBA_PLAYER_INTRO_LINEUPS || intro.lineup_card != 0)
        return 33;
    for (int card = 0; card < NBA_PLAYER_INTRO_CARD_COUNT; ++card) {
        nba_player_intro_render(&intro, &renderer);
        uint64_t card_hash = frame_hash(&renderer);
        if (card == 0) hashes[4] = card_hash;
        if (card > 0 && card_hash == hashes[4]) return 34;
        if (card + 1 < NBA_PLAYER_INTRO_CARD_COUNT) {
            for (int frame = 0; frame < NBA_PLAYER_INTRO_CARD_FRAMES; ++frame)
                nba_player_intro_update(&intro, NULL);
        }
    }
    press(&input, NBA_BTN_START);
    nba_player_intro_update(&intro, &input);
    if (intro.phase != NBA_PLAYER_INTRO_COMPLETE ||
        session.left_team != away || session.right_team != home) return 35;
    nba_player_intro_shutdown(&intro);
    return 0;
}

int main(int argc, char **argv) {
    NbaAssetPack assets = {0};
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    const uint8_t pairs[][2] = {{0, 18}, {18, 3}, {26, 8}};
    uint64_t hashes[3][5] = {{0}};
    int result = 0;
    for (unsigned pair = 0; pair < 3u; ++pair) {
        result = run_pair(&assets, pairs[pair][0], pairs[pair][1], hashes[pair]);
        if (result != 0) break;
        for (unsigned phase = 0; phase < 5u; ++phase)
            if (hashes[pair][phase] == 0u) result = 40;
    }
    if (result == 0) {
        for (unsigned phase = 0; phase < 5u; ++phase)
            if (hashes[0][phase] == hashes[1][phase] ||
                hashes[1][phase] == hashes[2][phase]) result = 41;
    }
    nba_assets_free(&assets);
    printf("GAMEPLAY65_FLOW %s pairs=3 phases=5 lineup_cards=30\n",
           result == 0 ? "PASS" : "FAIL");
    if (result) fprintf(stderr, "gameplay65 flow code=%d\n", result);
    return result == 0 ? 0 : 1;
}
