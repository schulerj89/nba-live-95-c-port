#include "nba_tipoff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    uint16_t period, left, right, expected_period, expected_clock;
    uint16_t expected_stamina;
    bool final;
} Case;

static bool fixture_contains_cases(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return false; }
    long size = ftell(file);
    if (size <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file); return false;
    }
    char *text = (char *)malloc((size_t)size + 1u);
    if (!text) { fclose(file); return false; }
    bool ok = fread(text, 1u, (size_t)size, file) == (size_t)size;
    fclose(file);
    text[size] = '\0';
    static const char *required[] = {
        "\"name\": \"q1\"", "\"name\": \"halftime\"",
        "\"name\": \"regulation_tie\"",
        "\"name\": \"regulation_final\"", "\"$87:8EB2\"",
        "\"$87:95E9\"", "\"$87:976E\"", "\"$87:979D\""
    };
    for (unsigned i = 0; ok && i < sizeof(required) / sizeof(required[0]); ++i)
        ok = strstr(text, required[i]) != NULL;
    free(text);
    return ok;
}

static void seed(NbaTipoff *game, NbaSession *session, const Case *c) {
    memset(game, 0, sizeof(*game));
    nba_session_init(session);
    session->config.main_values[3] = 3u;
    session->score[0] = c->left;
    session->score[1] = c->right;
    session->match.period_raw_0926 = c->period;
    game->session = session;
    game->period_raw_0926 = c->period;
    game->match_clock_raw_0928 = 0u;
    game->rim_raw_092c = 77u;
    game->shot_clock_mirror_raw_09c6 = 77u;
    game->possession_actor = 4;
    game->ball.owner_actor = 4;
    game->pass_actor_raw = -1;
    game->pass_receiver_raw = -1;
    game->team_context[0].anchor_x_raw_0a = -336;
    game->team_context[1].anchor_x_raw_0a = 336;
    game->tip_winner_group_raw_0932 = 0u;
    for (unsigned i = 0; i < 24u; ++i) game->fatigue.stamina[i] = 0u;
}

static bool drive_case(const Case *c) {
    NbaSession session;
    NbaTipoff game;
    seed(&game, &session, c);
    if (!nba_tipoff_step_match_lifecycle(&game)) return false;
    if (game.period_raw_0926 != c->expected_period ||
        session.match.period_raw_0926 != c->expected_period ||
        game.fatigue.stamina[0] != c->expected_stamina ||
        (game.rim_raw_13e7 & 0x0800u) == 0u ||
        game.dead_ball_dispatch_busy_raw_09b4 != 1u) return false;
    if (c->final) {
        if (session.match.final_marker != NBA_MATCH_FINAL_CONFIRMED ||
            session.match.flow_state !=
                NBA_MATCH_FLOW_POSTGAME_PRESENTATION_PENDING ||
            game.match_clock_raw_0928 != 0u) return false;
        session.match.presentation_ticks_remaining = 1u;
        if (!nba_tipoff_step_match_lifecycle(&game) ||
            session.match.flow_state != NBA_MATCH_FLOW_FINAL) return false;
        return true;
    }
    if (session.match.final_marker != NBA_MATCH_FINAL_ACTIVE ||
        session.match.flow_state != NBA_MATCH_FLOW_PERIOD_PRESENTATION_PENDING)
        return false;
    session.match.presentation_ticks_remaining = 1u;
    if (!nba_tipoff_step_match_lifecycle(&game) ||
        session.match.flow_state != NBA_MATCH_FLOW_PERIOD_RESTART_PENDING)
        return false;
    if (!nba_tipoff_step_match_lifecycle(&game) ||
        session.match.flow_state != NBA_MATCH_FLOW_LIVE ||
        game.match_clock_raw_0928 != c->expected_clock ||
        game.rim_raw_092c != 0x05A0u ||
        game.shot_clock_mirror_raw_09c6 != 0x05A0u ||
        game.dead_ball_dispatch_busy_raw_09b4 != 0u ||
        (game.rim_raw_13e7 & 0x0800u) != 0u) return false;
    if (c->period == 1u &&
        (game.team_context[0].anchor_x_raw_0a != 336 ||
         game.team_context[1].anchor_x_raw_0a != -336)) return false;
    return true;
}

static bool horn_gate_cases(void) {
    NbaSession session;
    NbaTipoff game;
    const Case base = {"horn", 0u, 4u, 4u, 1u, 43200u, 0x4000u, false};
    seed(&game, &session, &base);
    game.possession_actor = -1;
    game.ball.owner_actor = -1;
    game.ball.z_fp = 8 * 256;
    game.pass_receiver_raw = -1;
    if (nba_tipoff_step_match_lifecycle(&game) ||
        session.match.flow_state != NBA_MATCH_FLOW_HORN_BALL_LIVE ||
        game.period_raw_0926 != 0u) return false;
    /* `$0946 >= 0` resolves the same high ownerless ball on the next gate. */
    game.pass_receiver_raw = 0;
    if (!nba_tipoff_step_match_lifecycle(&game) ||
        game.period_raw_0926 != 1u) return false;

    seed(&game, &session, &base);
    game.match_clock_raw_0928 = 5u;
    if (nba_tipoff_step_match_lifecycle(&game) ||
        session.match.flow_state != NBA_MATCH_FLOW_LIVE) return false;
    game.match_clock_raw_0928 = 0xF000u;
    game.ball.z_fp = 7 * 256;
    game.possession_actor = -1;
    game.ball.owner_actor = -1;
    return nba_tipoff_step_match_lifecycle(&game) &&
           game.period_raw_0926 == 1u;
}

int main(int argc, char **argv) {
    if (argc != 2 || !fixture_contains_cases(argv[1])) {
        fprintf(stderr, "usage: match_lifecycle_expiry_probe <fixture.json>\n");
        return 2;
    }
    static const Case cases[] = {
        {"q1", 0u, 10u, 8u, 1u, 43200u, 0x1000u, false},
        {"halftime", 1u, 10u, 8u, 2u, 43200u, 0x7000u, false},
        {"regulation_tie", 3u, 10u, 10u, 4u, 18000u, 0x4000u, false},
        {"regulation_final", 3u, 10u, 8u, 5u, 0u, 0x1000u, true}
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (!drive_case(&cases[i])) {
            fprintf(stderr, "lifecycle mismatch: %s\n", cases[i].name);
            return 3;
        }
    }
    if (!horn_gate_cases()) {
        fprintf(stderr, "shot-at-horn gate mismatch\n");
        return 4;
    }
    puts("match lifecycle expiry: 4 native outcomes + horn gate PASS");
    return 0;
}
