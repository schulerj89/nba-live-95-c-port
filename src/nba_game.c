#include "nba_game.h"
#include "nba_font.h"
#include "nba_audio.h"
#include <stdio.h>
#include <string.h>

/**
 * Offset/Address/Size: 0x000020 | $80:8020 | size: 0x80
 * Purpose: Game engine cold-boot initialization (loads ROM, assets, video/audio subsystems, enters initial state).
 */
bool nba_game_init(NbaGame *game, const char *rom_path, const char *assets_path) {
    if (!game) return false;
    memset(game, 0, sizeof(NbaGame));

    printf("[GAME] Initializing NBA Live '95 Native C Port...\n");

    nba_renderer_init(&game->renderer);
    nba_font_init();
    nba_audio_init();

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
    nba_audio_debugger_init(&game->audio_debugger);
    game->is_initialized = true;

    printf("[GAME] Initialization complete. Entering state NBA_STATE_NINTENDO_LICENSE.\n");
    return true;
}

/**
 * Offset/Address/Size: N/A | Host Game Shutdown | size: N/A
 * Purpose: Gracefully releases audio, asset pack, and ROM cartridge buffers.
 */
void nba_game_shutdown(NbaGame *game) {
    if (!game) return;
    nba_audio_shutdown();
    if (game->assets.is_loaded) {
        nba_assets_free(&game->assets);
    }
    if (game->rom.is_loaded) {
        nba_rom_free(&game->rom);
    }
    game->is_initialized = false;
    printf("[GAME] Shutdown complete.\n");
}

/**
 * Offset/Address/Size: 0x00059A | $00:059A | size: 0x10
 * Purpose: Polls controller joypad button edge states (pressed, held, released) per frame.
 */
void nba_game_input_update(NbaInput *input, uint16_t raw_buttons) {
    if (!input) return;
    input->pressed = (uint16_t)(raw_buttons & ~input->held);
    input->released = (uint16_t)(~raw_buttons & input->held);
    input->held = raw_buttons;
}

/**
 * Offset/Address/Size: 0x005A91 | $80:DA91 | size: 0xC8
 * Purpose: Main game loop dispatcher and scene timer state machine.
 */
void nba_game_tick(NbaGame *game, float delta_time) {
    /* Handle F10 Timing Debug overlay toggle */
    if (game->input.pressed & NBA_BTN_DEBUG_F10) {
        game->show_timing_debug = !game->show_timing_debug;
        printf("[DEBUG] Timing HUD overlay %s\n", game->show_timing_debug ? "ENABLED" : "DISABLED");
    }

    /* Handle F11 Audio Debugger toggle */
    if (game->input.pressed & NBA_BTN_DEBUG_F11) {
        nba_audio_debugger_toggle(&game->audio_debugger);
    }

    /* Update audio debugger navigation / playback */
    nba_audio_debugger_update(&game->audio_debugger, &game->assets, &game->input);

    /* If audio debugger is active, freeze game state progression */
    if (game->audio_debugger.is_active) {
        return;
    }

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
            if ((game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) || game->state_timer >= 3.0f) {
                game->state = NBA_STATE_EA_INTRO;
                game->state_timer = 0.0f;
            }
            break;

        case NBA_STATE_EA_INTRO:
            /* Trigger EA intro voice/audio if present in asset pack and not yet played */
            if (game->ea_voice_stage == 0) {
                const NbaAssetItem *audio_item = nba_assets_get(&game->assets, NBA_ASSET_AUDIO_EA_INTRO);
                if (audio_item && audio_item->data && audio_item->size > 0) {
                    printf("[AUDIO] Playing EA Sports intro voice sequence (%u bytes, 5.05s total)...\n", audio_item->size);
                    nba_audio_play_wav(audio_item->data, (size_t)audio_item->size);
                    game->ea_voice_stage = 1;
                }
            }

            /* Step through the authentic 4 assembly stages (5.05s total hold) or advance on button press */
            if (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) {
                nba_audio_stop();
                game->state = NBA_STATE_MAIN_MENU;
                game->state_timer = 0.0f;
            } else if (game->state_timer >= 5.05f) {
                game->state = NBA_STATE_MAIN_MENU;
                game->state_timer = 0.0f;
            }
            break;

        case NBA_STATE_MAIN_MENU:
        default:
            break;
    }
}

/**
 * Offset/Address/Size: 0x007EE6 | $00:FEE6 | size: 0x1E0 (480 bytes)
 * Purpose: Renders the 256x15 1bpp NBA / NBPA legal copyright notice bitmap onto screen.
 */
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

/**
 * Offset/Address/Size: 0x01715C | $82:F15C | size: 0x4E0 (1248 bytes)
 * Purpose: Multi-stage EA Sports intro animation renderer with Mode 7 sliding/dropping transitions & specular flash.
 */
void nba_game_render_ea_intro(NbaGame *game) {
    if (!game) return;
    NbaRenderer *ren = &game->renderer;
    float timer = game->state_timer;

    nba_renderer_clear(ren, 0xFF000000); /* Solid Black */

    const NbaAssetItem *item1 = nba_assets_get(&game->assets, NBA_ASSET_EA_LOGO_STAGE1);
    const NbaAssetItem *item2 = nba_assets_get(&game->assets, NBA_ASSET_EA_LOGO_STAGE2);
    const NbaAssetItem *item3 = nba_assets_get(&game->assets, NBA_ASSET_EA_LOGO_STAGE3);
    const NbaAssetItem *item4 = nba_assets_get(&game->assets, NBA_ASSET_EA_LOGO_STAGE4);

    const NbaAssetItem *base_item = item4 ? item4 : (item1 ? item1 : NULL);
    if (!base_item || !base_item->data) return;

    int start_x, start_y;
    if (base_item->flags != 0) {
        start_x = (int)((base_item->flags >> 16) & 0xFFFF);
        start_y = (int)(base_item->flags & 0xFFFF);
    } else {
        start_x = (NBA_SNES_WIDTH - (int)base_item->width) / 2;
        start_y = (NBA_SNES_HEIGHT - (int)base_item->height) / 2;
    }

    uint32_t width = base_item->width;
    uint32_t height = base_item->height;

    const uint32_t *p1 = (item1 && item1->data) ? (const uint32_t *)item1->data : NULL;
    const uint32_t *p2 = (item2 && item2->data) ? (const uint32_t *)item2->data : NULL;
    const uint32_t *p3 = (item3 && item3->data) ? (const uint32_t *)item3->data : NULL;
    const uint32_t *p4 = (item4 && item4->data) ? (const uint32_t *)item4->data : NULL;

    int flash_boost = 0;

    if (timer < 0.533f) {
        /* Stage 1: "E" piece slides from left horizontally */
        float local_t = timer;
        int off_x = 0;
        if (local_t < 0.366f) {
            float p = local_t / 0.366f;
            float ease = 1.0f - (1.0f - p) * (1.0f - p);
            off_x = (int)((1.0f - ease) * -80.0f);
        } else {
            int flash_frame = (int)((local_t - 0.366f) * 60.0f);
            if (flash_frame < 8) flash_boost = (8 - flash_frame) * 14;
        }

        if (p1) {
            for (uint32_t r = 0; r < height; r++) {
                int py = start_y + (int)r;
                if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
                for (uint32_t c = 0; c < width; c++) {
                    uint32_t color = p1[r * width + c];
                    if (color != 0) {
                        if (flash_boost > 0) {
                            uint32_t a = (color >> 24) & 0xFF;
                            uint32_t red   = ((color >> 16) & 0xFF) + (uint32_t)flash_boost;
                            uint32_t green = ((color >> 8) & 0xFF) + (uint32_t)flash_boost;
                            uint32_t blue  = (color & 0xFF) + (uint32_t)flash_boost;
                            if (red > 255) red = 255;
                            if (green > 255) green = 255;
                            if (blue > 255) blue = 255;
                            color = (a << 24) | (red << 16) | (green << 8) | blue;
                        }
                        int px = start_x + off_x + (int)c;
                        if (px >= 0 && px < NBA_SNES_WIDTH) {
                            ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                        }
                    }
                }
            }
        }
    }
    else if (timer < 1.050f) {
        /* Stage 2: "E" is stationary, "A" slides from right horizontally to join "E" */
        float local_t = timer - 0.533f;
        int off_x_a = 0;
        if (local_t < 0.366f) {
            float p = local_t / 0.366f;
            float ease = 1.0f - (1.0f - p) * (1.0f - p);
            off_x_a = (int)((1.0f - ease) * 80.0f);
        } else {
            int flash_frame = (int)((local_t - 0.366f) * 60.0f);
            if (flash_frame < 8) flash_boost = (8 - flash_frame) * 14;
        }

        if (p2) {
            for (uint32_t r = 0; r < height; r++) {
                int py = start_y + (int)r;
                if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
                for (uint32_t c = 0; c < width; c++) {
                    uint32_t col1 = p1 ? p1[r * width + c] : 0;
                    uint32_t col2 = p2[r * width + c];

                    if (col1 != 0) {
                        /* Stationary E piece */
                        uint32_t color = col1;
                        if (flash_boost > 0) {
                            uint32_t a = (color >> 24) & 0xFF;
                            uint32_t red   = ((color >> 16) & 0xFF) + (uint32_t)flash_boost;
                            uint32_t green = ((color >> 8) & 0xFF) + (uint32_t)flash_boost;
                            uint32_t blue  = (color & 0xFF) + (uint32_t)flash_boost;
                            if (red > 255) red = 255;
                            if (green > 255) green = 255;
                            if (blue > 255) blue = 255;
                            color = (a << 24) | (red << 16) | (green << 8) | blue;
                        }
                        int px = start_x + (int)c;
                        if (px >= 0 && px < NBA_SNES_WIDTH) {
                            ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                        }
                    } else if (col2 != 0) {
                        /* Moving A piece */
                        uint32_t color = col2;
                        if (flash_boost > 0) {
                            uint32_t a = (color >> 24) & 0xFF;
                            uint32_t red   = ((color >> 16) & 0xFF) + (uint32_t)flash_boost;
                            uint32_t green = ((color >> 8) & 0xFF) + (uint32_t)flash_boost;
                            uint32_t blue  = (color & 0xFF) + (uint32_t)flash_boost;
                            if (red > 255) red = 255;
                            if (green > 255) green = 255;
                            if (blue > 255) blue = 255;
                            color = (a << 24) | (red << 16) | (green << 8) | blue;
                        }
                        int px = start_x + off_x_a + (int)c;
                        if (px >= 0 && px < NBA_SNES_WIDTH) {
                            ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                        }
                    }
                }
            }
        }
    }
    else if (timer < 2.050f) {
        /* Stage 3: "EA" emblem is stationary, "SPORTS" ribbon drops down from above */
        float local_t = timer - 1.050f;
        int off_y_s = 0;
        if (local_t < 0.366f) {
            float p = local_t / 0.366f;
            float ease = 1.0f - (1.0f - p) * (1.0f - p);
            off_y_s = (int)((1.0f - ease) * -40.0f);
        } else {
            int flash_frame = (int)((local_t - 0.366f) * 60.0f);
            if (flash_frame < 8) flash_boost = (8 - flash_frame) * 14;
        }

        if (p3) {
            for (uint32_t r = 0; r < height; r++) {
                for (uint32_t c = 0; c < width; c++) {
                    uint32_t col2 = p2 ? p2[r * width + c] : 0;
                    uint32_t col3 = p3[r * width + c];

                    if (col2 != 0) {
                        /* Stationary EA emblem */
                        int py = start_y + (int)r;
                        if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
                        uint32_t color = col2;
                        if (flash_boost > 0) {
                            uint32_t a = (color >> 24) & 0xFF;
                            uint32_t red   = ((color >> 16) & 0xFF) + (uint32_t)flash_boost;
                            uint32_t green = ((color >> 8) & 0xFF) + (uint32_t)flash_boost;
                            uint32_t blue  = (color & 0xFF) + (uint32_t)flash_boost;
                            if (red > 255) red = 255;
                            if (green > 255) green = 255;
                            if (blue > 255) blue = 255;
                            color = (a << 24) | (red << 16) | (green << 8) | blue;
                        }
                        int px = start_x + (int)c;
                        if (px >= 0 && px < NBA_SNES_WIDTH) {
                            ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                        }
                    } else if (col3 != 0) {
                        /* Dropping SPORTS ribbon */
                        int py = start_y + off_y_s + (int)r;
                        if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
                        uint32_t color = col3;
                        if (flash_boost > 0) {
                            uint32_t a = (color >> 24) & 0xFF;
                            uint32_t red   = ((color >> 16) & 0xFF) + (uint32_t)flash_boost;
                            uint32_t green = ((color >> 8) & 0xFF) + (uint32_t)flash_boost;
                            uint32_t blue  = (color & 0xFF) + (uint32_t)flash_boost;
                            if (red > 255) red = 255;
                            if (green > 255) green = 255;
                            if (blue > 255) blue = 255;
                            color = (a << 24) | (red << 16) | (green << 8) | blue;
                        }
                        int px = start_x + (int)c;
                        if (px >= 0 && px < NBA_SNES_WIDTH) {
                            ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                        }
                    }
                }
            }
        }
    }
    else {
        /* Stage 4: Full completed logo and bottom "ELECTRONIC ARTS" typography banner held steadily */
        if (p4) {
            for (uint32_t r = 0; r < height; r++) {
                int py = start_y + (int)r;
                if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
                for (uint32_t c = 0; c < width; c++) {
                    uint32_t color = p4[r * width + c];
                    if (color != 0) {
                        int px = start_x + (int)c;
                        if (px >= 0 && px < NBA_SNES_WIDTH) {
                            ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                        }
                    }
                }
            }
        }
    }
}

/**
 * Offset/Address/Size: 0x005A91 | $80:DA91 | size: 0x140
 * Purpose: Top-level scene rendering multiplexer and debug overlay compositor.
 */
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

    /* Render Timing Debug Overlay [F10] if enabled */
    if (game->show_timing_debug && !game->audio_debugger.is_active) {
        /* Draw top debug banner */
        uint32_t hud_bg = 0xCC001020;
        uint32_t hud_border = 0xFF4A90E2;
        uint32_t col_cyan = 0xFF40C0FF;
        uint32_t col_yellow = 0xFFFFD700;

        int bx = 4, by = 4, bw = 248, bh = 38;
        for (int y = by; y < by + bh; y++) {
            for (int x = bx; x < bx + bw; x++) {
                if (x == bx || x == bx + bw - 1 || y == by || y == by + bh - 1) {
                    ren->pixels[y * NBA_SNES_WIDTH + x] = hud_border;
                } else {
                    ren->pixels[y * NBA_SNES_WIDTH + x] = hud_bg;
                }
            }
        }

        char l1[40], l2[40], l3[40];
        const char *state_str = "UNKNOWN";
        const char *stage_str = "";

        switch (game->state) {
            case NBA_STATE_NINTENDO_LICENSE: state_str = "NINTENDO LICENSE"; break;
            case NBA_STATE_NBA_LEGAL_NOTICE: state_str = "NBA LEGAL NOTICE"; break;
            case NBA_STATE_EA_INTRO: {
                state_str = "EA INTRO";
                if (game->state_timer < 0.533f) stage_str = "STG 1: 'E'";
                else if (game->state_timer < 1.050f) stage_str = "STG 2: 'A'";
                else if (game->state_timer < 2.050f) stage_str = "STG 3: 'SPORTS'";
                else stage_str = "STG 4: 'GAME' (HOLD)";
                break;
            }
            case NBA_STATE_MAIN_MENU: state_str = "MAIN MENU"; break;
            default: break;
        }

        snprintf(l1, sizeof(l1), "TIMING DEBUG [F10]");
        snprintf(l2, sizeof(l2), "ST: %-10s %s", state_str, stage_str);
        snprintf(l3, sizeof(l3), "F:%04u  T:%4.2fs  VOICE:%u", game->frame_count, game->state_timer, game->ea_voice_stage);

        nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, bx + 6, by + 4, l1, col_yellow, col_shadow, 1);
        nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, bx + 6, by + 14, l2, col_white, col_shadow, 1);
        nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, bx + 6, by + 24, l3, col_cyan, col_shadow, 1);
    }

    /* Render Audio Debugger Overlay on top of any game screen if active */
    if (game->audio_debugger.is_active) {
        nba_audio_debugger_render(&game->audio_debugger, &game->assets, ren);
    }
}
