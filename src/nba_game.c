#include "nba_game.h"
#include "nba_font.h"
#include <stdio.h>
#include <string.h>

bool nba_game_init(NbaGame *game, const char *rom_path) {
    if (!game) return false;
    memset(game, 0, sizeof(NbaGame));

    printf("[GAME] Initializing NBA Live '95 Native C Port...\n");

    nba_renderer_init(&game->renderer);
    nba_font_init();

    if (rom_path && rom_path[0] != '\0') {
        if (!nba_rom_load_file(&game->rom, rom_path)) {
            printf("[GAME] Warning: Continuing with emulated SNES boot environment without ROM file.\n");
        }
    }

    /* Run the authentic SNES hardware reset routine */
    nba_snes_init(&game->snes);

    /* Setup initial game state */
    game->state = NBA_STATE_NINTENDO_LICENSE;
    game->state_timer = 0.0f;
    game->frame_count = 0;
    game->is_initialized = true;

    printf("[GAME] Initialization complete. Entering state NBA_STATE_NINTENDO_LICENSE.\n");
    return true;
}

void nba_game_shutdown(NbaGame *game) {
    if (!game) return;
    if (game->rom.is_loaded) {
        nba_rom_free(&game->rom);
    }
    game->is_initialized = false;
    printf("[GAME] Shutdown complete.\n");
}

void nba_game_input_update(NbaInput *input, uint16_t raw_buttons) {
    if (!input) return;
    input->pressed = (uint16_t)(raw_buttons & ~input->held);
    input->released = (uint16_t)(~raw_buttons & input->held);
    input->held = raw_buttons;
}

void nba_game_tick(NbaGame *game, float delta_time) {
    if (!game || !game->is_initialized) return;

    game->state_timer += delta_time;
    game->frame_count++;

    /* Update SNES joypad register */
    nba_snes_set_joypad(&game->snes, game->input.held);

    switch (game->state) {
        case NBA_STATE_BOOT_RESET:
            if (game->state_timer >= 0.1f) {
                game->state = NBA_STATE_NINTENDO_LICENSE;
                game->state_timer = 0.0f;
            }
            break;

        case NBA_STATE_NINTENDO_LICENSE:
            /* Progress to next screen after 3.5 seconds or if Start / A is pressed */
            if (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) {
                printf("[GAME] Start pressed during license screen -> advancing state\n");
                game->state = NBA_STATE_EA_INTRO;
                game->state_timer = 0.0f;
            } else if (game->state_timer >= 3.5f) {
                game->state = NBA_STATE_EA_INTRO;
                game->state_timer = 0.0f;
            }
            break;

        case NBA_STATE_EA_INTRO:
            if (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) {
                game->state = NBA_STATE_MAIN_MENU;
                game->state_timer = 0.0f;
            }
            break;

        case NBA_STATE_MAIN_MENU:
        default:
            break;
    }
}

void nba_game_render(NbaGame *game) {
    if (!game || !game->is_initialized) return;

    NbaRenderer *ren = &game->renderer;

    /* Palette colors */
    uint32_t col_black      = 0xFF000000;
    uint32_t col_white      = 0xFFFFFFFF;
    uint32_t col_shadow     = 0xFF202020;
    uint32_t col_red        = 0xFFD82800; /* Authentic SNES Nintendo Red */
    uint32_t col_gold       = 0xFFF8B800;
    uint32_t col_cyan       = 0xFF00E0E0;
    uint32_t col_gray       = 0xFFA0A0A0;

    switch (game->state) {
        case NBA_STATE_BOOT_RESET:
            nba_renderer_clear(ren, col_black);
            break;

        case NBA_STATE_NINTENDO_LICENSE: {
            nba_renderer_clear(ren, col_black);

            /* Center 128x11 bitmap: X = (256 - 128) / 2 = 64, Y = (224 - 11) / 2 = 106 */
            int start_x = (NBA_SNES_WIDTH - 128) / 2;
            int start_y = (NBA_SNES_HEIGHT - 11) / 2;

            nba_font_render_licensed_by_nintendo(
                ren->pixels, NBA_SNES_WIDTH,
                start_x, start_y,
                col_white,
                1
            );
            break;
        }

        case NBA_STATE_EA_INTRO: {
            nba_renderer_clear(ren, col_black);

            nba_font_render_text_centered(
                ren->pixels, NBA_SNES_WIDTH,
                50,
                "EA SPORTS",
                col_gold, col_shadow,
                2
            );

            nba_font_render_text_centered(
                ren->pixels, NBA_SNES_WIDTH,
                80,
                "IF IT'S IN THE GAME,",
                col_white, col_shadow,
                1
            );
            nba_font_render_text_centered(
                ren->pixels, NBA_SNES_WIDTH,
                96,
                "IT'S IN THE GAME.",
                col_cyan, col_shadow,
                1
            );

            nba_font_render_text_centered(
                ren->pixels, NBA_SNES_WIDTH,
                140,
                "NBA LIVE '95",
                col_gold, col_shadow,
                2
            );

            nba_font_render_text_centered(
                ren->pixels, NBA_SNES_WIDTH,
                180,
                "PRESS START TO PLAY",
                col_white, col_shadow,
                1
            );
            break;
        }

        case NBA_STATE_MAIN_MENU: {
            nba_renderer_clear(ren, 0xFF001830); /* SNES deep blue menu background */

            nba_font_render_text_centered(
                ren->pixels, NBA_SNES_WIDTH,
                30,
                "NBA LIVE '95",
                col_gold, col_shadow,
                2
            );

            const char *menu_items[] = {
                "EXHIBITION",
                "SEASON",
                "PLAYOFFS",
                "SET RULES",
                "SET OPTIONS"
            };

            for (int i = 0; i < 5; i++) {
                uint32_t col = (i == 0) ? col_gold : col_white;
                nba_font_render_text_centered(
                    ren->pixels, NBA_SNES_WIDTH,
                    80 + i * 20,
                    menu_items[i],
                    col, col_shadow,
                    1
                );
            }
            break;
        }
    }
}
