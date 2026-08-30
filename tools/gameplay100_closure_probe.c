#include "nba_assets.h"
#include "nba_game.h"
#include "nba_player_intro.h"
#include "nba_player_setup.h"
#include "nba_setup_screen.h"
#include "nba_team_select.h"
#include "nba_tipoff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint64_t digest;
    unsigned transitions;
    unsigned render_changes;
    unsigned actor_motion_frames;
    unsigned resource_changes;
    unsigned possession_changes;
} ClosureResult;

static void mix16(uint64_t *hash, uint16_t value) {
    *hash ^= (uint8_t)value; *hash *= 1099511628211ull;
    *hash ^= (uint8_t)(value >> 8); *hash *= 1099511628211ull;
}

static void mix32(uint64_t *hash, uint32_t value) {
    mix16(hash, (uint16_t)value);
    mix16(hash, (uint16_t)(value >> 16));
}

static uint64_t frame_hash(const NbaRenderer *renderer) {
    const uint8_t *bytes = (const uint8_t *)renderer->pixels;
    uint64_t hash = 1469598103934665603ull;
    for (size_t i = 0; i < sizeof(renderer->pixels); ++i) {
        hash ^= bytes[i]; hash *= 1099511628211ull;
    }
    return hash;
}

static void capture_frame(ClosureResult *result, NbaRenderer *renderer,
                          uint64_t *previous) {
    uint64_t rendered = frame_hash(renderer);
    mix32(&result->digest, (uint32_t)rendered);
    mix32(&result->digest, (uint32_t)(rendered >> 32));
    if (*previous && *previous != rendered) ++result->render_changes;
    *previous = rendered;
}

static NbaInput button(uint16_t pressed) {
    NbaInput input;
    memset(&input, 0, sizeof(input));
    input.held = input.pressed = pressed;
    return input;
}

static int settle_setup_transition(NbaSetupScreen *setup) {
    NbaInput idle = {0};
    for (unsigned frame = 0; frame < 600u; ++frame) {
        (void)nba_setup_screen_update(setup, &idle);
        if (setup->transition == NBA_SETUP_TRANSITION_NONE &&
            !setup->transition_release_pending) return 0;
    }
    return 1;
}

static int exercise_setup(const NbaAssetPack *assets, NbaSession *session,
                          NbaRenderer *renderer, ClosureResult *result,
                          uint64_t *previous_render) {
    NbaSetupScreen *setup = (NbaSetupScreen *)calloc(1, sizeof(*setup));
    if (!setup) return 10;
    nba_setup_screen_init(setup, assets, &session->config);
    for (unsigned frame = 0; frame < 180u; ++frame)
        (void)nba_setup_screen_update(setup, NULL);
    if (!setup->is_initialized || setup->frame < NBA_SETUP_ENTER_FRAMES ||
        setup->brightness != 15) { free(setup); return 11; }
    nba_setup_screen_render(setup, renderer);
    capture_frame(result, renderer, previous_render);

    /* `$81:D000-$DFFF`: open Rules, alter a bounded meter, and commit through
     * the real transition path. B never commits in the original. */
    NbaInput input = button(NBA_BTN_DOWN);
    for (unsigned row = 0; row < 4u; ++row)
        (void)nba_setup_screen_update(setup, &input);
    if (setup->row != NBA_SETUP_ROW_RULES) { free(setup); return 12; }
    input = button(NBA_BTN_A);
    NbaSetupUpdateResult update = nba_setup_screen_update(setup, &input);
    if (update.action != NBA_SETUP_ACTION_OPEN_RULES ||
        settle_setup_transition(setup)) { free(setup); return 13; }
    ++result->transitions;
    uint16_t old_rule = setup->working_rules[0];
    input = button(old_rule ? NBA_BTN_LEFT : NBA_BTN_RIGHT);
    update = nba_setup_screen_update(setup, &input);
    if (update.sound != NBA_SETUP_SOUND_ADJUST ||
        setup->working_rules[0] == old_rule) { free(setup); return 14; }
    uint16_t committed_rule = setup->working_rules[0];
    nba_setup_screen_render(setup, renderer);
    capture_frame(result, renderer, previous_render);
    input = button(NBA_BTN_START);
    update = nba_setup_screen_update(setup, &input);
    if (update.action != NBA_SETUP_ACTION_RETURN_MAIN ||
        settle_setup_transition(setup) ||
        session->config.rules[0] != committed_rule) { free(setup); return 15; }
    ++result->transitions;

    /* `$82:8CD9-$8EA5`: the adjacent Options page owns a separate seven-word
     * commit buffer and redraws its ROM-authored value canvas. */
    input = button(NBA_BTN_DOWN);
    (void)nba_setup_screen_update(setup, &input);
    if (setup->row != NBA_SETUP_ROW_OPTIONS) { free(setup); return 16; }
    input = button(NBA_BTN_A);
    update = nba_setup_screen_update(setup, &input);
    if (update.action != NBA_SETUP_ACTION_OPEN_OPTIONS ||
        settle_setup_transition(setup)) { free(setup); return 17; }
    ++result->transitions;
    uint16_t old_option = setup->working_options[0];
    input = button(old_option ? NBA_BTN_LEFT : NBA_BTN_RIGHT);
    update = nba_setup_screen_update(setup, &input);
    if (update.sound != NBA_SETUP_SOUND_ADJUST ||
        setup->working_options[0] == old_option) { free(setup); return 18; }
    uint16_t committed_option = setup->working_options[0];
    nba_setup_screen_render(setup, renderer);
    capture_frame(result, renderer, previous_render);
    input = button(NBA_BTN_START);
    update = nba_setup_screen_update(setup, &input);
    if (update.action != NBA_SETUP_ACTION_RETURN_MAIN ||
        settle_setup_transition(setup) ||
        session->config.options[0] != committed_option) { free(setup); return 19; }
    ++result->transitions;

    /* Return to Exhibition and allow all 52 native exit frames to complete. */
    setup->row = NBA_SETUP_ROW_MODE;
    input = button(NBA_BTN_START);
    update = nba_setup_screen_update(setup, &input);
    if (update.action != NBA_SETUP_ACTION_NONE ||
        !setup->team_select_exit_active) { free(setup); return 20; }
    for (unsigned frame = 0; frame < 80u; ++frame) {
        update = nba_setup_screen_update(setup, NULL);
        if (update.action == NBA_SETUP_ACTION_CONFIRM_MODE) break;
    }
    if (update.action != NBA_SETUP_ACTION_CONFIRM_MODE) { free(setup); return 21; }
    ++result->transitions;
    free(setup);
    return 0;
}

static int exercise_flow_and_gameplay(const NbaAssetPack *assets,
                                      NbaSession *session,
                                      NbaRenderer *renderer,
                                      ClosureResult *result,
                                      uint64_t *previous_render) {
    NbaInput input = {0};
    NbaTeamSelect select;
    if (!nba_team_select_init(&select, assets, session)) return 30;
    for (int frame = 0; frame < NBA_TEAM_TRANSITION_FRAMES; ++frame)
        (void)nba_team_select_update(&select, NULL);
    input = button(NBA_BTN_L);
    if (nba_team_select_update(&select, &input) != NBA_TEAM_SOUND_SIDE) return 31;
    input = button(NBA_BTN_LEFT);
    if (nba_team_select_update(&select, &input) != NBA_TEAM_SOUND_CHANGE) return 32;
    input = button(NBA_BTN_L);
    if (nba_team_select_update(&select, &input) != NBA_TEAM_SOUND_SIDE) return 33;
    input = button(NBA_BTN_RIGHT);
    if (nba_team_select_update(&select, &input) != NBA_TEAM_SOUND_CHANGE) return 34;
    nba_team_select_render(&select, renderer);
    capture_frame(result, renderer, previous_render);
    input = button(NBA_BTN_START);
    if (nba_team_select_update(&select, &input) != NBA_TEAM_SOUND_CONFIRM ||
        !select.confirm_requested) return 35;
    ++result->transitions;
    nba_team_select_shutdown(&select);

    NbaPlayerSetup setup;
    if (!nba_player_setup_init(&setup, assets, session, renderer->pixels)) return 40;
    for (int frame = 0; frame < NBA_PLAYER_SETUP_TRANSITION_FRAMES; ++frame)
        (void)nba_player_setup_update(&setup, NULL);
    input = button(NBA_BTN_LEFT);
    if (nba_player_setup_update(&setup, &input) != NBA_PLAYER_SETUP_SOUND_MOVE)
        return 41;
    input = button(NBA_BTN_RIGHT);
    if (nba_player_setup_update(&setup, &input) != NBA_PLAYER_SETUP_SOUND_MOVE)
        return 42;
    nba_player_setup_render(&setup, renderer);
    capture_frame(result, renderer, previous_render);
    input = button(NBA_BTN_START);
    if (nba_player_setup_update(&setup, &input) !=
        NBA_PLAYER_SETUP_SOUND_CONFIRM) return 43;
    ++result->transitions;

    NbaPlayerIntro intro;
    if (!nba_player_intro_init(&intro, assets, session, renderer->pixels)) return 50;
    nba_player_setup_shutdown(&setup);
    for (int frame = 0; frame < NBA_PLAYER_INTRO_TRANSITION_FRAMES; ++frame)
        nba_player_intro_update(&intro, NULL);
    if (intro.phase != NBA_PLAYER_INTRO_MATCHUP) return 51;
    nba_player_intro_render(&intro, renderer);
    capture_frame(result, renderer, previous_render);
    for (int frame = 0; frame < NBA_PLAYER_INTRO_MATCHUP_FRAMES; ++frame)
        nba_player_intro_update(&intro, NULL);
    for (int frame = 0; frame < NBA_PLAYER_INTRO_RATINGS_FRAMES; ++frame)
        nba_player_intro_update(&intro, NULL);
    if (intro.phase != NBA_PLAYER_INTRO_LINEUPS) return 52;
    for (unsigned card = 0; card < NBA_PLAYER_INTRO_CARD_COUNT; ++card) {
        nba_player_intro_render(&intro, renderer);
        capture_frame(result, renderer, previous_render);
        input = button(NBA_BTN_A);
        nba_player_intro_update(&intro, &input);
    }
    input = button(NBA_BTN_START);
    nba_player_intro_update(&intro, &input);
    if (intro.phase != NBA_PLAYER_INTRO_COMPLETE) return 53;
    ++result->transitions;
    nba_player_intro_shutdown(&intro);

    NbaTipoff game;
    if (!nba_tipoff_init(&game, assets, session)) return 60;
    /* Controlled pre-expiry seed for the historical gameplay-core digest. */
    game.match_clock_raw_0928 = 43200u;
    game.rng.state = 0x5A17u;
    int32_t previous_x[NBA_GAMEPLAY_ACTOR_COUNT];
    int32_t previous_y[NBA_GAMEPLAY_ACTOR_COUNT];
    uint16_t previous_upper[NBA_GAMEPLAY_ACTOR_COUNT];
    int8_t previous_owner = game.possession_actor;
    for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
        previous_x[actor] = game.actors[actor].x_fp;
        previous_y[actor] = game.actors[actor].y_fp;
        previous_upper[actor] = game.actors[actor].upper_animation_resource_raw_2a;
    }
    for (unsigned frame = 0; frame < 6000u; ++frame) {
        nba_tipoff_update(&game, NULL);
        bool moved = false;
        for (unsigned actor = 0; actor < NBA_GAMEPLAY_ACTOR_COUNT; ++actor) {
            if (previous_x[actor] != game.actors[actor].x_fp ||
                previous_y[actor] != game.actors[actor].y_fp) moved = true;
            previous_x[actor] = game.actors[actor].x_fp;
            previous_y[actor] = game.actors[actor].y_fp;
            if (previous_upper[actor] !=
                game.actors[actor].upper_animation_resource_raw_2a)
                ++result->resource_changes;
            previous_upper[actor] =
                game.actors[actor].upper_animation_resource_raw_2a;
        }
        if (moved) ++result->actor_motion_frames;
        if (previous_owner != game.possession_actor) ++result->possession_changes;
        previous_owner = game.possession_actor;
        mix32(&result->digest, (uint32_t)game.ball.x_fp);
        mix32(&result->digest, (uint32_t)game.ball.y_fp);
        mix16(&result->digest, (uint16_t)game.possession_actor);
        mix16(&result->digest, game.match_clock_raw_0928);
        mix16(&result->digest, game.play_code);
        if ((frame % 120u) == 0u) {
            nba_tipoff_render(&game, renderer);
            capture_frame(result, renderer, previous_render);
        }
    }
    if (result->actor_motion_frames < 2500u ||
        result->resource_changes < 50u || result->possession_changes < 2u)
        return 61;
    return 0;
}

static int run_closure(const NbaAssetPack *assets, ClosureResult *result) {
    memset(result, 0, sizeof(*result));
    result->digest = 1469598103934665603ull;
    NbaSession session;
    nba_session_init(&session);
    session.left_team = 0;
    session.right_team = 18;
    NbaRenderer renderer;
    nba_renderer_init(&renderer);
    uint64_t previous_render = 0;

    /* `$00:8156/$00:8600-$861C` and `$80:CE33-$CEFD` terminate at the
     * portable frame/input publication boundary. Exercise that edge contract
     * before entering the scene services. */
    NbaInput edge = {0};
    nba_game_input_update(&edge, NBA_BTN_A | NBA_BTN_RIGHT);
    if (edge.pressed != (NBA_BTN_A | NBA_BTN_RIGHT)) return 2;
    nba_game_input_update(&edge, NBA_BTN_RIGHT);
    if (edge.released != NBA_BTN_A || edge.pressed != 0u) return 3;
    nba_game_input_update(&edge, 0u);
    if (edge.released != NBA_BTN_RIGHT) return 4;

    int code = exercise_setup(assets, &session, &renderer, result,
                              &previous_render);
    if (code) return code;
    code = exercise_flow_and_gameplay(assets, &session, &renderer, result,
                                      &previous_render);
    if (code) return code;
    mix16(&result->digest, session.left_team);
    mix16(&result->digest, session.right_team);
    mix16(&result->digest, session.player_one_side);
    mix16(&result->digest, session.config.rules[0]);
    mix16(&result->digest, session.config.options[0]);
    return result->transitions == 8u && result->render_changes >= 12u ? 0 : 70;
}

int main(int argc, char **argv) {
    /* Re-reviewed after the gameplay ball presentation was latched to the
     * same OAM frame as its owning player (fa6fd63). Simulation counters are
     * unchanged; the closure digest intentionally includes rendered pixels. */
    /* Re-reviewed after cached action body art adopted `$87:A52C-$A5FA`'s
     * presentation direction. All transition/motion/resource/possession
     * counters remain unchanged; sampled gameplay pixels intentionally do. */
    /* C-only digest re-reviewed 2026-08-29 after native ownership/substeps,
     * actor edges, OOB and dynamic formation fixes. Both journey runs match:
     * eight scene transitions, 65 render changes, 2,910 motion frames,
     * 13,122 resource changes and 72 possession changes. This is repeatable
     * integration coverage, not evidence of whole-frame ROM equivalence. */
    static const uint64_t expected_digest = 0x773c1df2a9820701ull;
    if (argc != 2) return 2;
    NbaAssetPack assets = {0};
    if (!nba_assets_load(&assets, argv[1])) return 3;
    ClosureResult first = {0}, second = {0};
    int code = run_closure(&assets, &first);
    if (!code) code = run_closure(&assets, &second);
    if (!code && (first.digest != second.digest ||
        memcmp(&first, &second, sizeof(first)) != 0)) code = 80;
    if (!code && first.digest != expected_digest) code = 81;
    nba_assets_free(&assets);
    printf("GAMEPLAY100_CLOSURE %s digest=%016llx transitions=%u "
           "renders=%u motion=%u resources=%u possessions=%u code=%d\n",
           code == 0 ? "PASS" : "FAIL", (unsigned long long)first.digest,
           first.transitions, first.render_changes, first.actor_motion_frames,
           first.resource_changes, first.possession_changes, code);
    return code == 0 ? 0 : 1;
}
