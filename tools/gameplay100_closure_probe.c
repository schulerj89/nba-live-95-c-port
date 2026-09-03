#include "nba_assets.h"
#include "nba_game.h"
#include "nba_player_intro.h"
#include "nba_player_setup.h"
#include "nba_setup_screen.h"
#include "nba_team_select.h"
#include "nba_tipoff.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint64_t digest;
    uint64_t gameplay_digest;
    uint64_t render_digest;
    uint64_t session_digest;
    unsigned transitions;
    unsigned render_changes;
    unsigned actor_motion_frames;
    unsigned resource_changes;
    unsigned possession_changes;
} ClosureResult;

/* Optional C-regression diagnostics. This deliberately accepts no expected
 * state/images and changes neither the legacy digest nor its acceptance gate.
 * Raw owned-state bytes require matching compiler/layout and are not a native
 * ROM protocol; the semantic telemetry accompanies them for inspection. */
typedef struct {
    const char *directory;
    FILE *samples, *state, *telemetry, *legacy, *sessions, *inputs;
    unsigned run, sample;
    int failed;
} ClosureDiagnostics;
static ClosureDiagnostics diagnostics;

static FILE *diagnostic_open(const char *name, const char *mode) {
    char path[2048];
    if (snprintf(path,sizeof(path),"%s/run%u-%s",diagnostics.directory,
                 diagnostics.run,name)<0) return NULL;
    return fopen(path,mode);
}

static int diagnostic_begin(unsigned run) {
    if (!diagnostics.directory) return 0;
    diagnostics.run=run;diagnostics.sample=0;
    diagnostics.samples=diagnostic_open("samples.jsonl","wb");
    diagnostics.state=diagnostic_open("owned-state.bin","wb");
    diagnostics.telemetry=diagnostic_open("telemetry.jsonl","wb");
    diagnostics.legacy=diagnostic_open("legacy-state.jsonl","wb");
    diagnostics.sessions=diagnostic_open("session-state.bin","wb");
    diagnostics.inputs=diagnostic_open("ui-inputs.jsonl","wb");
    if (!diagnostics.samples || !diagnostics.state || !diagnostics.telemetry ||
        !diagnostics.legacy || !diagnostics.sessions || !diagnostics.inputs) return 90;
    fprintf(diagnostics.legacy,"{\"schema\":\"closure-diagnostic-v2\","
        "\"tipoff_offset\":%zu,\"tipoff_bytes\":%zu,\"session_bytes\":%zu,"
        "\"session_style_offset\":%zu}\n",
        offsetof(NbaTipoff,frame),sizeof(NbaTipoff)-offsetof(NbaTipoff,frame),
        sizeof(NbaSession),offsetof(NbaSession,config)+
            offsetof(NbaGameConfig,main_values)+sizeof(uint16_t));
    return 0;
}

static int diagnostic_end(void) {
    if (!diagnostics.directory) return 0;
    FILE **streams[]={&diagnostics.samples,&diagnostics.state,&diagnostics.telemetry,
                      &diagnostics.legacy,&diagnostics.sessions,&diagnostics.inputs};
    for (unsigned i=0;i<sizeof(streams)/sizeof(streams[0]);++i) {
        if (*streams[i]) {
            if (ferror(*streams[i])) diagnostics.failed=1;
            if (fclose(*streams[i])) diagnostics.failed=1;
            *streams[i]=NULL;
        }
    }
    return diagnostics.failed ? 91 : 0;
}

static void diagnostic_gameplay(const NbaTipoff *game,unsigned frame) {
    if (!diagnostics.directory) return;
    NbaGameplayTelemetry telemetry;
    nba_tipoff_capture_telemetry(game,NULL,&telemetry);
    nba_gameplay_telemetry_write_jsonl(diagnostics.telemetry,&telemetry);
    /* The four preceding members are host addresses/callbacks. Every member
     * from frame onward is owned value state in the audited layout. init
     * zeroes the structure, including padding. No differing byte is omitted. */
    const size_t offset=offsetof(NbaTipoff,frame);
    if (fwrite((const unsigned char *)game+offset,1,sizeof(*game)-offset,
               diagnostics.state)!=sizeof(*game)-offset ||
        fwrite(game->session,1,sizeof(*game->session),diagnostics.sessions)!=
               sizeof(*game->session)) diagnostics.failed=1;
    fprintf(diagnostics.legacy,"{\"frame\":%u,\"ball_x\":%ld,\"ball_y\":%ld,"
        "\"owner\":%d,\"clock\":%u,\"play\":%u}\n",frame,
        (long)game->ball.x_fp,(long)game->ball.y_fp,(int)game->possession_actor,
        game->match_clock_raw_0928,game->play_code);
}

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
                          uint64_t *previous,const char *label,int frame,
                          const NbaSetupScreen *setup) {
    uint64_t rendered = frame_hash(renderer);
    mix32(&result->digest, (uint32_t)rendered);
    mix32(&result->digest, (uint32_t)(rendered >> 32));
    mix32(&result->render_digest, (uint32_t)rendered);
    mix32(&result->render_digest, (uint32_t)(rendered >> 32));
    if (*previous && *previous != rendered) ++result->render_changes;
    *previous = rendered;
    if (diagnostics.directory) {
        char name[160],path[2048];
        snprintf(name,sizeof(name),"sample%02u-%s.bmp",diagnostics.sample,label);
        snprintf(path,sizeof(path),"%s/run%u-%s",diagnostics.directory,
                 diagnostics.run,name);
        if (!nba_renderer_save_bmp(renderer,path)) diagnostics.failed=1;
        snprintf(name,sizeof(name),"sample%02u-%s.pixels",diagnostics.sample,label);
        FILE *pixels=diagnostic_open(name,"wb");
        if (!pixels) diagnostics.failed=1;
        else {
            if (fwrite(renderer->pixels,1,sizeof(renderer->pixels),pixels)!=
                    sizeof(renderer->pixels)) diagnostics.failed=1;
            if (fclose(pixels)) diagnostics.failed=1;
        }
        fprintf(diagnostics.samples,"{\"sample\":%u,\"label\":\"%s\","
            "\"frame\":%d,\"pixel_fnv\":\"%016llx\",\"setup\":",
            diagnostics.sample,label,frame,(unsigned long long)rendered);
        if (!setup) fprintf(diagnostics.samples,"null");
        else fprintf(diagnostics.samples,"{\"page\":%d,\"row\":%d,\"menu_row\":%d,"
            "\"menu_scroll\":%d,\"brightness\":%d,\"main_screen\":%d,"
            "\"sub_screen\":%d,\"bg2_scroll\":%d,\"bg3_scroll\":%d,"
            "\"rule0\":%u,\"option0\":%u,\"main_style\":%u}",
            (int)setup->page,(int)setup->row,setup->menu_row,setup->menu_scroll,
            setup->brightness,setup->main_screen,setup->sub_screen,
            setup->bg2_vscroll,setup->bg3_vscroll,
            setup->working_rules[0],setup->working_options[0],
            setup->config->main_values[1]);
        fputs("}\n",diagnostics.samples);++diagnostics.sample;
    }
}

static NbaInput button(uint16_t pressed) {
    NbaInput input;
    memset(&input, 0, sizeof(input));
    input.held = input.pressed = pressed;
    return input;
}

static NbaSetupUpdateResult closure_update_setup(NbaSetupScreen *setup,
                                                 const NbaInput *input) {
    int frame=setup->frame,page=(int)setup->page,row=(int)setup->row;
    NbaSetupUpdateResult result=nba_setup_screen_update(setup,input);
    if (diagnostics.directory && input && input->pressed)
        fprintf(diagnostics.inputs,"{\"frame_before\":%d,\"pressed\":%u,"
            "\"page_before\":%d,\"row_before\":%d,\"page_after\":%d,"
            "\"row_after\":%d,\"action\":%d}\n",frame,input->pressed,
            page,row,(int)setup->page,(int)setup->row,(int)result.action);
    return result;
}

static int settle_setup_transition(NbaSetupScreen *setup) {
    NbaInput idle = {0};
    for (unsigned frame = 0; frame < 600u; ++frame) {
        (void)closure_update_setup(setup, &idle);
        if (setup->transition == NBA_SETUP_TRANSITION_NONE &&
            !setup->transition_release_pending) return 0;
    }
    return 1;
}

static NbaSetupUpdateResult closure_press_setup(NbaSetupScreen *setup,
                                               const NbaInput *input) {
    /* The native menu producer consumes held words. Separate each requested
     * press with a released frame; repeated pressed flags on a held button
     * do not constitute separate native navigation commands. */
    NbaInput released = {0};
    (void)closure_update_setup(setup, &released);
    return closure_update_setup(setup, input);
}

static int exercise_setup(const NbaAssetPack *assets, NbaSession *session,
                          NbaRenderer *renderer, ClosureResult *result,
                          uint64_t *previous_render) {
    NbaSetupScreen *setup = (NbaSetupScreen *)calloc(1, sizeof(*setup));
    if (!setup) return 10;
    nba_setup_screen_init(setup, assets, &session->config);
    for (unsigned frame = 0; frame < 180u; ++frame)
        (void)closure_update_setup(setup, NULL);
    if (!setup->is_initialized || setup->frame < NBA_SETUP_ENTER_FRAMES ||
        setup->brightness != 15) { free(setup); return 11; }
    nba_setup_screen_render(setup, renderer);
    capture_frame(result, renderer, previous_render,"setup-main",setup->frame,setup);

    /* `$81:D000-$DFFF`: open Rules, alter a bounded meter, and commit through
     * the real transition path. B never commits in the original. */
    NbaInput input = button(NBA_BTN_DOWN);
    for (unsigned row = 0; row < 4u; ++row)
        (void)closure_press_setup(setup, &input);
    if (setup->row != NBA_SETUP_ROW_RULES) { free(setup); return 12; }
    input = button(NBA_BTN_A);
    NbaSetupUpdateResult update = closure_press_setup(setup, &input);
    if (update.action != NBA_SETUP_ACTION_OPEN_RULES ||
        settle_setup_transition(setup)) { free(setup); return 13; }
    ++result->transitions;
    uint16_t old_rule = setup->working_rules[0];
    input = button(old_rule ? NBA_BTN_LEFT : NBA_BTN_RIGHT);
    update = closure_press_setup(setup, &input);
    if (update.sound != NBA_SETUP_SOUND_ADJUST ||
        setup->working_rules[0] == old_rule) { free(setup); return 14; }
    uint16_t committed_rule = setup->working_rules[0];
    nba_setup_screen_render(setup, renderer);
    capture_frame(result, renderer, previous_render,"rules-edited",setup->frame,setup);
    input = button(NBA_BTN_START);
    update = closure_press_setup(setup, &input);
    if (update.action != NBA_SETUP_ACTION_RETURN_MAIN ||
        settle_setup_transition(setup) ||
        session->config.rules[0] != committed_rule) { free(setup); return 15; }
    ++result->transitions;

    /* `$82:8CD9-$8EA5`: the adjacent Options page owns a separate seven-word
     * commit buffer and redraws its ROM-authored value canvas. */
    input = button(NBA_BTN_DOWN);
#ifdef NBA_CLOSURE_HISTORICAL_NAVIGATION
    /* Reproduce the old C-only probe while auditing its checked-in digest.
     * That version incorrectly retained row4 after Rules return. */
    (void)closure_press_setup(setup, &input);
#else
    /* Original $81:B901 rebuilds Main with row0. The corrected production
     * return requires all five real Down dispatches to reach Options. */
    if (setup->row != NBA_SETUP_ROW_MODE) { free(setup); return 16; }
    for (unsigned row = 0; row < 5u; ++row)
        (void)closure_press_setup(setup, &input);
#endif
    if (setup->row != NBA_SETUP_ROW_OPTIONS) { free(setup); return 16; }
    input = button(NBA_BTN_A);
    update = closure_press_setup(setup, &input);
    if (update.action != NBA_SETUP_ACTION_OPEN_OPTIONS ||
        settle_setup_transition(setup)) { free(setup); return 17; }
    ++result->transitions;
    uint16_t old_option = setup->working_options[0];
    input = button(old_option ? NBA_BTN_LEFT : NBA_BTN_RIGHT);
    update = closure_press_setup(setup, &input);
    if (update.sound != NBA_SETUP_SOUND_ADJUST ||
        setup->working_options[0] == old_option) { free(setup); return 18; }
    uint16_t committed_option = setup->working_options[0];
    nba_setup_screen_render(setup, renderer);
    capture_frame(result, renderer, previous_render,"options-edited",setup->frame,setup);
    input = button(NBA_BTN_START);
    update = closure_press_setup(setup, &input);
    if (update.action != NBA_SETUP_ACTION_RETURN_MAIN ||
        settle_setup_transition(setup) ||
        session->config.options[0] != committed_option) { free(setup); return 19; }
    ++result->transitions;

    /* Return to Exhibition and allow all 52 native exit frames to complete. */
    if (setup->row != NBA_SETUP_ROW_MODE) { free(setup); return 20; }
    input = button(NBA_BTN_START);
    update = closure_press_setup(setup, &input);
    if (update.action != NBA_SETUP_ACTION_NONE ||
        !setup->team_select_exit_active) { free(setup); return 20; }
    for (unsigned frame = 0; frame < 80u; ++frame) {
        update = closure_update_setup(setup, NULL);
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
    capture_frame(result, renderer, previous_render,"team-select",
                  select.transition_frame+select.steady_frame,NULL);
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
    capture_frame(result, renderer, previous_render,"player-setup",
                  setup.transition_frame+setup.steady_frame,NULL);
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
    capture_frame(result, renderer, previous_render,"matchup",intro.phase_frame,NULL);
    for (int frame = 0; frame < NBA_PLAYER_INTRO_MATCHUP_FRAMES; ++frame)
        nba_player_intro_update(&intro, NULL);
    for (int frame = 0; frame < NBA_PLAYER_INTRO_RATINGS_FRAMES; ++frame)
        nba_player_intro_update(&intro, NULL);
    if (intro.phase != NBA_PLAYER_INTRO_LINEUPS) return 52;
    for (unsigned card = 0; card < NBA_PLAYER_INTRO_CARD_COUNT; ++card) {
        nba_player_intro_render(&intro, renderer);
        capture_frame(result, renderer, previous_render,"lineup",intro.phase_frame,NULL);
        if (card + 1 < NBA_PLAYER_INTRO_CARD_COUNT) {
            for (int frame = 0; frame < NBA_PLAYER_INTRO_CARD_FRAMES; ++frame)
                nba_player_intro_update(&intro, NULL);
        }
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
        mix32(&result->gameplay_digest, (uint32_t)game.ball.x_fp);
        mix32(&result->gameplay_digest, (uint32_t)game.ball.y_fp);
        mix16(&result->gameplay_digest, (uint16_t)game.possession_actor);
        mix16(&result->gameplay_digest, game.match_clock_raw_0928);
        mix16(&result->gameplay_digest, game.play_code);
        diagnostic_gameplay(&game,frame);
        if ((frame % 120u) == 0u) {
            nba_tipoff_render(&game, renderer);
            capture_frame(result, renderer, previous_render,"gameplay",(int)frame,NULL);
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
    result->gameplay_digest = result->render_digest = result->session_digest =
        1469598103934665603ull;
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
    mix16(&result->session_digest, session.left_team);
    mix16(&result->session_digest, session.right_team);
    mix16(&result->session_digest, session.player_one_side);
    mix16(&result->session_digest, session.config.rules[0]);
    mix16(&result->session_digest, session.config.options[0]);
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
    /* 2026-08-30 bounded update, independently reviewed by the integrator:
     * old773c1df2a9820701 -> fdbdd69c21271f89 changes only Rules sample1.
     * All6000 owned C gameplay snapshots and other65 sampled images are
     * byte-identical. Final return replay permits only the observed native
     * session Style1-to2 change; every other session byte remains exact.
     * New sample1 exactly matches native Rules44 frame620
     * (57344 active pixels), under explicitly state-aligned input journeys.
     * Rules Custom marking is also checked through real C menu callers from
     * explicit native prestates; no whole-game/whole-state parity is implied.
     * Further transition changes require fresh attribution, never automatic
     * refresh. */
#ifndef NBA_CLOSURE_EXPECTED_DIGEST
/* Released menu presses, native factory configuration, canonical teams/ranks
 * and the C39C layout repair have separate before/after controls. The old
 * fdbdd69c21271f89 remains exactly reproducible by the historical
 * source/configuration control. This is a C regression, not native parity. */
#define NBA_CLOSURE_EXPECTED_DIGEST 0xd26e6deec1fdc18eull
#endif
    /* Private historical builds derive this override from that revision's
     * checked-in golden. It affects only final acceptance, never game input. */
    static const uint64_t expected_digest = NBA_CLOSURE_EXPECTED_DIGEST;
    if (argc != 2 && (argc != 4 || strcmp(argv[2],"--diagnostics"))) return 2;
    if (argc == 4) diagnostics.directory=argv[3];
    NbaAssetPack assets = {0};
    if (!nba_assets_load(&assets, argv[1])) return 3;
    ClosureResult first = {0}, second = {0};
    int code = diagnostic_begin(0);
    if (!code) code = run_closure(&assets, &first);
    int diagnostic_code=diagnostic_end();
    if (!code) code=diagnostic_code;
    if (!code) code=diagnostic_begin(1);
    if (!code) code = run_closure(&assets, &second);
    diagnostic_code=diagnostic_end();
    if (!code) code=diagnostic_code;
    if (!code && (first.digest != second.digest ||
        memcmp(&first, &second, sizeof(first)) != 0)) code = 80;
    if (!code && first.digest != expected_digest) code = 81;
    nba_assets_free(&assets);
    printf("GAMEPLAY100_CLOSURE %s digest=%016llx transitions=%u "
           "renders=%u motion=%u resources=%u possessions=%u code=%d\n",
           code == 0 ? "PASS" : "FAIL", (unsigned long long)first.digest,
           first.transitions, first.render_changes, first.actor_motion_frames,
           first.resource_changes, first.possession_changes, code);
    if (diagnostics.directory) printf("CLOSURE_COMPONENTS gameplay=%016llx "
        "render=%016llx session=%016llx samples=%u\n",
        (unsigned long long)first.gameplay_digest,
        (unsigned long long)first.render_digest,
        (unsigned long long)first.session_digest,diagnostics.sample);
    return code == 0 ? 0 : 1;
}
