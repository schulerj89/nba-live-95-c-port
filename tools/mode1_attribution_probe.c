/* Controlled C-only attribution harness. This is not a native parity gate.
 * It differs from --tipoff-only only by an explicit pre-init configuration
 * switch and extra state snapshots. Never use its output as a native oracle. */
#include "nba_game.h"
#include "nba_snes_ppu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *output_file(const char *prefix, const char *suffix) {
    char path[2048];
    FILE *file = NULL;
    if (snprintf(path, sizeof(path), "%s%s", prefix, suffix) >= sizeof(path))
        return NULL;
    if (fopen_s(&file, path, "wb") != 0) return NULL;
    return file;
}

static void state(NbaGame *game, FILE *stream) {
    nba_tipoff_capture_telemetry(&game->scene.tipoff, &game->input,
                                 &game->gameplay_telemetry);
    game->gameplay_telemetry.global_frame = game->frame_count;
    nba_gameplay_telemetry_write_jsonl(stream, &game->gameplay_telemetry);
}

int main(int argc, char **argv) {
    if ((argc != 5 && argc != 6) ||
        (argc == 6 && strcmp(argv[5], "--legacy-role-injection")) ||
        (strcmp(argv[3], "factory") && strcmp(argv[3], "legacy")))
        return 2;
    static NbaGame game;
    if (!nba_game_init(&game, argv[1], argv[2])) return 3;
    nba_audio_set_host_playback_enabled(&game.audio, false);
    if (!strcmp(argv[3], "legacy")) {
        /* Controlled historical C prestate, NOT factory native defaults and
         * NOT a substitute for configuring a normal journey through menus. */
        game.session.config.main_values[1] = 1u;
        game.session.config.main_values[3] = 0u;
        game.session.config.rules[0] = game.session.config.rules[1] = 45u;
        for (unsigned i = 2; i < NBA_SETUP_RULE_COUNT; ++i)
            game.session.config.rules[i] = 1u;
    }
    if (!nba_game_enter_state(&game, NBA_STATE_TIPOFF)) return 4;
    if (argc == 6) {
        /* Explicit controlled intervention: restore the former roster-position
         * category in actor+$92, keeping current team IDs/ratings/appearance.
         * This is intentionally not native behavior; it isolates causality. */
        for (unsigned actor = 0; actor < 10; ++actor) {
            uint8_t team = (uint8_t)game.scene.tipoff.team_context[actor / 5u].strategy_team_raw_00;
            if (!nba_player_gameplay_position(&game.assets, team,
                    game.scene.tipoff.actors[actor].roster_slot,
                    &game.scene.tipoff.actors[actor].assignment_role_raw_92)) return 10;
        }
    }
    FILE *logical = output_file(argv[4], ".state.jsonl");
    if (!logical) return 5;
    printf("ATTRIBUTION profile=%s main=%u/%u/%u/%u contexts=%u/%u ranks=",
           argv[3], game.session.config.main_values[0],
           game.session.config.main_values[1], game.session.config.main_values[2],
           game.session.config.main_values[3],
           game.scene.tipoff.team_context[0].strategy_team_raw_00,
           game.scene.tipoff.team_context[1].strategy_team_raw_00);
    for (unsigned i = 0; i < 10; ++i)
        printf("%s%u", i ? "/" : "", game.scene.tipoff.actors[i].assignment_role_raw_92);
    printf("\n");
    state(&game, logical);
    for (unsigned frame = 1; frame <= 1000u; ++frame) {
        nba_game_input_update(&game.input, 0u);
        nba_game_tick(&game, 1.0f / 60.0f);
        if (frame == 1u || frame == 65u || frame == 1000u) state(&game, logical);
    }
    if (fclose(logical) || game.state != NBA_STATE_TIPOFF || game.state_frame != 1000u)
        return 6;
    nba_game_render(&game);
    FILE *pixels = output_file(argv[4], ".ppu.jsonl");
    if (!pixels) return 7;
    bool written = nba_snes_mode1_write_jsonl(pixels, &game.renderer,
                                             game.frame_count, game.state_frame);
    if (fclose(pixels) || !written) return 8;
    char image_path[2048];
    if (snprintf(image_path, sizeof(image_path), "%s.bmp", argv[4]) >= sizeof(image_path) ||
        !nba_renderer_save_bmp(&game.renderer, image_path)) return 9;
    nba_game_shutdown(&game);
    puts("PASS controlled C-only Mode1 attribution probe");
    return 0;
}
