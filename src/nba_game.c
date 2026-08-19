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
            printf("[GAME] Warning: Continuing without ROM file.\n");
        }
    }

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

    switch (game->state) {
        case NBA_STATE_BOOT_RESET:
            if (game->state_timer >= 0.1f) {
                game->state = NBA_STATE_NINTENDO_LICENSE;
                game->state_timer = 0.0f;
            }
            break;

        case NBA_STATE_NINTENDO_LICENSE:
            /* Exact SNES timing from ROM $00:FD9E: 120 frames (2.0s) or button press */
            if (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) {
                game->state = NBA_STATE_NBA_LEGAL_NOTICE;
                game->state_timer = 0.0f;
            } else if (game->state_timer >= 2.0f) {
                game->state = NBA_STATE_NBA_LEGAL_NOTICE;
                game->state_timer = 0.0f;
            }
            break;

        case NBA_STATE_NBA_LEGAL_NOTICE:
            /* Exact SNES timing from ROM $00:FEE6: 180 frames (3.0s) or button press */
            if (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) {
                game->state = NBA_STATE_EA_INTRO;
                game->state_timer = 0.0f;
            } else if (game->state_timer >= 3.0f) {
                game->state = NBA_STATE_EA_INTRO;
                game->state_timer = 0.0f;
            }
            break;

        case NBA_STATE_EA_INTRO:
            /* Step through the 4 assembly stages (2.8s total) or advance on button press */
            if (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) {
                game->state = NBA_STATE_MAIN_MENU;
                game->state_timer = 0.0f;
            } else if (game->state_timer >= 2.8f) {
                game->state = NBA_STATE_MAIN_MENU;
                game->state_timer = 0.0f;
            }
            break;

        case NBA_STATE_MAIN_MENU:
        default:
            break;
    }
}

#include "nba_legal_bitmap.h"
#include "nba_ea_logo.h"

void nba_game_render_nba_legal_notice(NbaRenderer *ren) {
    if (!ren) return;

    nba_renderer_clear(ren, 0xFF000000); /* Solid Black */

    uint32_t col_white = 0xFFFFFFFF;
    int start_y = NBA_LEGAL_START_Y;

    for (int r = 0; r < NBA_LEGAL_HEIGHT; r++) {
        int py = start_y + r;
        if (py < 0 || py >= NBA_SNES_HEIGHT) continue;

        for (int b = 0; b < 32; b++) {
            uint8_t byte_val = g_nba_legal_notice_bitmap[r][b];
            if (!byte_val) continue;

            for (int bit = 0; bit < 8; bit++) {
                if (byte_val & (0x80 >> bit)) {
                    int px = b * 8 + bit;
                    if (px >= 0 && px < NBA_SNES_WIDTH) {
                        ren->pixels[py * NBA_SNES_WIDTH + px] = col_white;
                    }
                }
            }
        }
    }
}

void nba_game_render_ea_intro(NbaRenderer *ren, float timer) {
    if (!ren) return;

    nba_renderer_clear(ren, 0xFF000000); /* Solid Black */

    /* Determine animation stage based on authentic timings:
     * Stage 1 (0.0s - 0.4s): "E" piece slams in
     * Stage 2 (0.4s - 0.8s): "A" piece joins with bright peach flash
     * Stage 3 (0.8s - 1.2s): "SPORTS" blue banner drops in and flashes
     * Stage 4 (1.2s - 2.8s): Complete logo with "ELECTRONIC ARTS" banner
     */
    const uint32_t (*stage_pixels)[NBA_EA_LOGO_WIDTH] = NULL;

    if (timer < 0.4f) {
        stage_pixels = g_ea_logo_stage1;
    } else if (timer < 0.8f) {
        stage_pixels = g_ea_logo_stage2;
    } else if (timer < 1.2f) {
        stage_pixels = g_ea_logo_stage3;
    } else {
        stage_pixels = g_ea_logo_stage4;
    }

    /* Render the selected assembly stage */
    int start_x = NBA_EA_LOGO_X;
    int start_y = NBA_EA_LOGO_Y;

    for (int r = 0; r < NBA_EA_LOGO_HEIGHT; r++) {
        int py = start_y + r;
        if (py < 0 || py >= NBA_SNES_HEIGHT) continue;

        for (int c = 0; c < NBA_EA_LOGO_WIDTH; c++) {
            uint32_t color = stage_pixels[r][c];
            if (color != 0) {
                int px = start_x + c;
                if (px >= 0 && px < NBA_SNES_WIDTH) {
                    ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                }
            }
        }
    }
}

void nba_game_render(NbaGame *game) {
    if (!game || !game->is_initialized) return;

    NbaRenderer *ren = &game->renderer;

    /* Palette colors */
    uint32_t col_black      = 0xFF000000;
    uint32_t col_white      = 0xFFFFFFFF;
    uint32_t col_shadow     = 0xFF202020;
    uint32_t col_gold       = 0xFFF8B800;

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

        case NBA_STATE_NBA_LEGAL_NOTICE:
            nba_game_render_nba_legal_notice(ren);
            break;

        case NBA_STATE_EA_INTRO:
            nba_game_render_ea_intro(ren, game->state_timer);
            break;

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
