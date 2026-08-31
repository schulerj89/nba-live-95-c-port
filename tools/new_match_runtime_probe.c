/* C integration regression, not a native timing/whole-state oracle.
 * The external Python verifier compares its reset projection to retained
 * native first-court state; this probe never supplies those expectations. */
#include "nba_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void previous_final(NbaSession *s) {
    s->score[0] = 103u; s->score[1] = 99u;
    s->game_clock_ticks = 32000u;
    s->match.period_raw_0926 = 5u;
    s->match.flow_state = NBA_MATCH_FLOW_FINAL;
    s->match.final_marker = NBA_MATCH_FINAL_CONFIRMED;
    s->match.presentation_ticks_remaining = 7u;
    s->match.pause.state = NBA_MATCH_PAUSE_MENU_AFTER_TIMEOUT;
    s->match.pause.selection = NBA_MATCH_PAUSE_SELECT_RESUME;
    s->match.pause.saved_live_state_raw_4988 = 0x82u;
    s->match.pause.transition_ticks_remaining = 9u;
    for (unsigned side = 0; side < 2u; ++side) {
        s->match.timeouts_remaining[side] = (uint16_t)side;
        for (unsigned i = 0; i < 12u; ++i) {
            s->match.roster_order[side][i] = (uint8_t)(11u - i);
            s->match.roster_available[side][i] = false;
        }
        for (unsigned i = 0; i < 5u; ++i)
            s->match.active_lineup[side][i] = (uint8_t)(11u - i);
    }
}

static void print_projection(const NbaSession *s, unsigned side) {
    printf("NEW_MATCH_PROJECTION {\"side\":%u,\"period\":%u,"
           "\"scores\":[%u,%u],\"timeouts\":[%u,%u],\"roster_order\":[",
           side, s->match.period_raw_0926, s->score[0], s->score[1],
           s->match.timeouts_remaining[0], s->match.timeouts_remaining[1]);
    for (unsigned team = 0; team < 2u; ++team) {
        if (team) putchar(',');
        putchar('[');
        for (unsigned i = 0; i < 12u; ++i)
            printf("%s%u", i ? "," : "", s->match.roster_order[team][i]);
        putchar(']');
    }
    puts("]}");
}

static bool clean_host_match(const NbaSession *s) {
    if (s->match.flow_state != NBA_MATCH_FLOW_LIVE ||
        s->match.final_marker != NBA_MATCH_FINAL_ACTIVE ||
        s->match.presentation_ticks_remaining != 0u ||
        s->match.pause.state != NBA_MATCH_PAUSE_INACTIVE ||
        s->match.pause.saved_live_state_raw_4988 != 0u ||
        s->match.pause.transition_ticks_remaining != 0u ||
        /* Pause holds native home0/visitor1, opposite the legacy UI's
         * left0/right1. The new-match initializer already translates it. */
        s->match.pause.selected_side != (s->player_one_side ? 0u : 1u)) return false;
    for (unsigned side = 0; side < 2u; ++side) {
        if (memcmp(s->match.active_lineup[side],
                   s->match.roster_order[side], 5u) != 0) return false;
        for (unsigned i = 0; i < 12u; ++i)
            if (!s->match.roster_available[side][i]) return false;
    }
    return true;
}

static void step(NbaGame *g, uint32_t held) {
    nba_game_input_update(&g->input, held);
    nba_game_tick(g, 1.0f / 60.0f);
}

static bool run_return_journey(NbaGame *g, unsigned side) {
    g->session.player_one_side = (uint8_t)side;
    g->session.left_team = (uint8_t)(side ? 0u : 3u);
    g->session.right_team = (uint8_t)(side ? 14u : 18u);
    g->session.config.main_values[0] = 0u;
    g->session.config.main_values[1] = (uint16_t)side;
    g->session.config.main_values[2] = 1u;
    g->session.config.main_values[3] = (uint16_t)(side + 1u);
    g->session.config.rules[0] = 23u;
    g->session.config.rules[8] = 0u;
    g->session.config.options[0] = 17u;
    g->session.config.options[1] = 9u;
    NbaGameConfig config = g->session.config;
    uint8_t left = g->session.left_team, right = g->session.right_team;
    previous_final(&g->session);
    /* Controlled seed of an already completed C match. Everything after
     * this entry follows the real postgame/menu/update route. */
    if (!nba_game_enter_state(g, NBA_STATE_POSTGAME)) return false;
    step(g, NBA_BTN_A);
    if (g->state != NBA_STATE_GAME_SETUP ||
        g->session.match.flow_state != NBA_MATCH_FLOW_FINAL) return false;
    for (unsigned i = 0; i < 220u; ++i) step(g, 0u);
    /* Browsing Setup must not erase final scores or match state. */
    if (g->session.score[0] != 103u ||
        g->session.match.flow_state != NBA_MATCH_FLOW_FINAL) return false;
    unsigned seen = 0u;
    bool projected = false;
    for (unsigned i = 0; i < 6000u && g->state != NBA_STATE_TIPOFF; ++i) {
        /* A bounded liveness ceiling, not a native timing tolerance. */
        step(g, i % 20u == 0u ? NBA_BTN_START : 0u);
        seen |= 1u << (unsigned)g->state;
        if (!projected && g->state == NBA_STATE_TEAM_SELECT) {
            print_projection(&g->session, side);
            if (!clean_host_match(&g->session) ||
                g->session.game_clock_ticks != 0u) return false;
            projected = true;
        }
    }
    unsigned required = (1u << NBA_STATE_TEAM_SELECT) |
        (1u << NBA_STATE_PLAYER_SETUP) | (1u << NBA_STATE_PLAYER_INTRO) |
        (1u << NBA_STATE_TIPOFF);
    if (!projected || (seen & required) != required ||
        g->state != NBA_STATE_TIPOFF ||
        memcmp(&g->session.config, &config, sizeof(config)) != 0 ||
        g->session.left_team != left || g->session.right_team != right ||
        g->session.player_one_side != side || !clean_host_match(&g->session) ||
        g->scene.tipoff.period_raw_0926 != 0u ||
        g->scene.tipoff.match_clock_raw_0928 !=
            nba_match_regulation_clock(config.main_values[3])) return false;
    int before = g->scene.tipoff.frame;
    for (unsigned i = 0; i < 60u; ++i) step(g, 0u);
    return g->scene.tipoff.frame == before + 60 &&
           g->session.match.flow_state == NBA_MATCH_FLOW_LIVE;
}

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    NbaGame *g = (NbaGame *)calloc(1, sizeof(*g));
    if (!g) return 3;
    if (!nba_game_init(g, argv[2], argv[1])) { free(g); return 4; }
    nba_audio_set_host_playback_enabled(&g->audio, false);
    bool ok = run_return_journey(g, 0u) && run_return_journey(g, 1u);
    nba_game_shutdown(g);
    free(g);
    puts(ok ? "new-match production return: PASS (C integration only)" :
              "new-match production return: FAIL");
    return ok ? 0 : 1;
}
