#include "nba_game.h"
#include "nba_font.h"
#include <stdio.h>
#include <string.h>

bool nba_game_init(NbaGame *game, const char *rom_path, const char *assets_path) {
    if (!game) return false;
    memset(game, 0, sizeof(NbaGame));

    printf("[GAME] Initializing NBA Live '95 Native C Port...\n");

    nba_renderer_init(&game->renderer);
    nba_font_init();

    /* Load asset pack if provided via parameter */
    if (assets_path && assets_path[0] != '\0') {
        nba_assets_load(&game->assets, assets_path);
    }

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
    if (game->assets.is_loaded) {
        nba_assets_free(&game->assets);
    }
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

void nba_game_render_nba_legal_notice(NbaGame *game) {
    if (!game) return;
    NbaRenderer *ren = &game->renderer;

    nba_renderer_clear(ren, 0xFF000000); /* Solid Black */

    const NbaAssetItem *item = nba_assets_get(&game->assets, NBA_ASSET_NBA_LEGAL_NOTICE);
    if (item && item->data) {
        uint32_t col_white = 0xFFFFFFFF;
        const uint8_t *bitmap = (const uint8_t *)item->data;
        int start_y = (int)item->flags; /* Stored start_y in flags */

        for (uint32_t r = 0; r < item->height; r++) {
            int py = start_y + r;
            if (py < 0 || py >= NBA_SNES_HEIGHT) continue;

            for (int b = 0; b < 32; b++) {
                uint8_t byte_val = bitmap[r * 32 + b];
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
}

void nba_game_render_ea_intro(NbaGame *game) {
    if (!game) return;
    NbaRenderer *ren = &game->renderer;
    float timer = game->state_timer;

    nba_renderer_clear(ren, 0xFF000000); /* Solid Black */

    NbaAssetId stage_id;
    if (timer < 0.4f) {
        stage_id = NBA_ASSET_EA_LOGO_STAGE1;
    } else if (timer < 0.8f) {
        stage_id = NBA_ASSET_EA_LOGO_STAGE2;
    } else if (timer < 1.2f) {
        stage_id = NBA_ASSET_EA_LOGO_STAGE3;
    } else {
        stage_id = NBA_ASSET_EA_LOGO_STAGE4;
    }

    const NbaAssetItem *item = nba_assets_get(&game->assets, stage_id);
    if (item && item->data) {
        const uint32_t *stage_pixels = (const uint32_t *)item->data;
        int start_x = (NBA_SNES_WIDTH - (int)item->width) / 2;
        int start_y = (NBA_SNES_HEIGHT - (int)item->height) / 2 - 1;

        for (uint32_t r = 0; r < item->height; r++) {
            int py = start_y + r;
            if (py < 0 || py >= NBA_SNES_HEIGHT) continue;

            for (uint32_t c = 0; c < item->width; c++) {
                uint32_t color = stage_pixels[r * item->width + c];
                if (color != 0) {
                    int px = start_x + c;
                    if (px >= 0 && px < NBA_SNES_WIDTH) {
                        ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                    }
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

            const NbaAssetItem *item = nba_assets_get(&game->assets, NBA_ASSET_NINTENDO_LICENSE);
            if (item && item->data) {
                const uint8_t *bitmap = (const uint8_t *)item->data;
                int start_x = (NBA_SNES_WIDTH - (int)item->width) / 2;
                int start_y = (NBA_SNES_HEIGHT - (int)item->height) / 2;

                for (uint32_t r = 0; r < item->height; r++) {
                    int py = start_y + r;
                    if (py < 0 || py >= NBA_SNES_HEIGHT) continue;

                    for (int b = 0; b < 16; b++) {
                        uint8_t byte_val = bitmap[r * 16 + b];
                        if (!byte_val) continue;

                        for (int bit = 0; bit < 8; bit++) {
                            if (byte_val & (0x80 >> bit)) {
                                int px = start_x + (b * 8 + bit);
                                if (px >= 0 && px < NBA_SNES_WIDTH) {
                                    ren->pixels[py * NBA_SNES_WIDTH + px] = col_white;
                                }
                            }
                        }
                    }
                }
            } else {
                /* Fallback to embedded font helper */
                int start_x = (NBA_SNES_WIDTH - 128) / 2;
                int start_y = (NBA_SNES_HEIGHT - 11) / 2;
                nba_font_render_licensed_by_nintendo(
                    ren->pixels, NBA_SNES_WIDTH,
                    start_x, start_y,
                    col_white,
                    1
                );
            }
            break;
        }

        case NBA_STATE_NBA_LEGAL_NOTICE:
            nba_game_render_nba_legal_notice(game);
            break;

        case NBA_STATE_EA_INTRO:
            nba_game_render_ea_intro(game);
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
