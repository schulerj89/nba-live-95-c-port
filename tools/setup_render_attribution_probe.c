/* Controlled C-only attribution adapter. This intentionally injects a declared
 * initial configuration before scene entry; it is not natural UI input proof. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 6 || (strcmp(argv[3], "fresh") && strcmp(argv[3], "legacy"))) return 2;
    unsigned frames = (unsigned)strtoul(argv[4], NULL, 10);
    if (!frames || frames > 1000u) return 2;
    NbaGame *game = calloc(1, sizeof(*game));
    if (!game) return 3;
    if (!nba_game_init(game, argv[1], argv[2])) { free(game); return 4; }
    nba_audio_set_host_playback_enabled(&game->audio, false);
    if (!strcmp(argv[3], "legacy")) {
        game->session.config.main_values[1] = 1u;
        game->session.config.main_values[3] = 0u;
        nba_config_apply_style(&game->session.config, 1u);
    }
    int result = 0;
    if (!nba_game_enter_state(game, NBA_STATE_GAME_SETUP)) { result = 5; goto done; }
    for (unsigned i = 0; i < frames; ++i) {
        nba_game_input_update(&game->input, 0u);
        nba_game_tick(game, 1.0f / 60.0f);
    }
    nba_game_render(game);
    printf("ATTRIBUTION bg3_vscroll=%d row=%d\n", game->scene.setup.bg3_vscroll,
           (int)game->scene.setup.row);
    if (!nba_renderer_save_bmp(&game->renderer, argv[5])) result = 6;
done:
    nba_game_shutdown(game);
    free(game);
    return result;
}
