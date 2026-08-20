#include "nba_game.h"
#include "nba_font.h"
#include "nba_audio.h"
#include <stdio.h>
#include <string.h>

static void nba_game_enter_state(NbaGame *game, NbaGameState state) {
    game->state = state;
    game->state_frame = 0;
    game->state_timer = 0.0f;
}

/* SNES INIDISP master-brightness levels are linear values from 0 through 15. */
static uint32_t nba_game_master_brightness_color(uint32_t color, int brightness) {
    if (brightness >= 15) return color;
    if (brightness <= 0) return color & 0xFF000000u;

    uint32_t r = ((color >> 16) & 0xFFu) * (uint32_t)brightness / 15u;
    uint32_t g = ((color >> 8) & 0xFFu) * (uint32_t)brightness / 15u;
    uint32_t b = (color & 0xFFu) * (uint32_t)brightness / 15u;
    return (color & 0xFF000000u) | (r << 16) | (g << 8) | b;
}

static int nba_game_license_brightness(uint32_t state_frame) {
    int fade_frame = (int)state_frame - NBA_LICENSE_FRAMES;
    return fade_frame <= 0 ? 15 : 15 - fade_frame;
}

static int nba_game_legal_brightness(uint32_t state_frame) {
    int frame = (int)state_frame;
    int fade_out_start = NBA_SCREEN_FADE_FRAMES + NBA_LEGAL_FRAMES;
    if (frame < NBA_SCREEN_FADE_FRAMES) return frame;
    if (frame <= fade_out_start) return 15;
    return 15 - (frame - fade_out_start);
}

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
    if (assets_path && assets_path[0] != '\0' &&
        !nba_assets_load(&game->assets, assets_path)) {
        nba_audio_shutdown();
        return false;
    }

    if (rom_path && rom_path[0] != '\0') {
        if (!nba_rom_load_file(&game->rom, rom_path)) {
            nba_assets_free(&game->assets);
            nba_audio_shutdown();
            return false;
        }
    }

    /* Setup initial game state */
    nba_game_enter_state(game, NBA_STATE_NINTENDO_LICENSE);
    game->frame_count = 0;
    nba_audio_debugger_init(&game->audio_debugger);
    nba_title_sequence_init(&game->title_sequence);
    nba_setup_screen_init(&game->setup, &game->assets);
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
    game->state_frame++;
    game->frame_count++;

    switch (game->state) {
        case NBA_STATE_BOOT_RESET:
            if (game->state_frame >= 6) {
                nba_game_enter_state(game, NBA_STATE_NINTENDO_LICENSE);
            }
            break;

        case NBA_STATE_NINTENDO_LICENSE:
            /* $80:FD9E holds for $78 frames, then $80:CF1B fades brightness to zero. */
            if (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) {
                nba_game_enter_state(game, NBA_STATE_NBA_LEGAL_NOTICE);
            } else if (game->state_frame >=
                       NBA_LICENSE_FRAMES + NBA_SCREEN_FADE_FRAMES) {
                nba_game_enter_state(game, NBA_STATE_NBA_LEGAL_NOTICE);
            }
            break;

        case NBA_STATE_NBA_LEGAL_NOTICE:
            /* $80:CF3B fades in, $80:FEE6 holds for $B4 frames, then $80:CF1B fades out. */
            if ((game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) ||
                game->state_frame >=
                    NBA_SCREEN_FADE_FRAMES + NBA_LEGAL_FRAMES + NBA_SCREEN_FADE_FRAMES) {
                nba_game_enter_state(game, NBA_STATE_EA_INTRO);
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
                nba_game_enter_state(game, NBA_STATE_TITLE_SEQUENCE);
                nba_title_sequence_init(&game->title_sequence);
            } else if (game->state_frame >= NBA_INTRO_TOTAL_FRAMES) {
                nba_game_enter_state(game, NBA_STATE_TITLE_SEQUENCE);
                nba_title_sequence_init(&game->title_sequence);
            }
            break;

        case NBA_STATE_TITLE_SEQUENCE:
            /* $80:E01E enters the NBA shield/title scene immediately after $82:AC0E. */
            if (!game->title_sequence.audio_started) {
                nba_audio_play_title_spc(&game->assets);
                game->title_sequence.audio_started = true;
            }

            /* $80:E5C7 - dismissing the title. Bit 7 of $0A4C selects the path:
             * if the build is still running the ROM snaps it complete via
             * $80:F07E and holds 120 frames ($80:E5D3 #$0078); if it had already
             * finished it holds 40 ($80:E5D9 #$0028). Pressing again during the
             * hold does nothing - the count is fixed. */
            if (game->title_sequence.phase == NBA_TITLE_PHASE_BUILD) {
                int title_frame = (int)game->state_frame;
                bool build_complete = title_frame >= NBA_TITLE_BUILD_COMPLETE_FRAMES;
                bool dismissed =
                    (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) != 0;

                if (dismissed || title_frame >= NBA_TITLE_SEQUENCE_FRAMES) {
                    if (!build_complete) {
                        nba_title_sequence_snap_complete(&game->title_sequence);
                        /* $80:E5F9 DEC A / BPL runs the wait count+1 times. */
                        game->title_sequence.hold_frames_left = NBA_TITLE_SNAP_HOLD_FRAMES + 1;
                        printf("[TITLE] Start during build: snapped complete ($80:F07E), "
                               "holding %d frames.\n", NBA_TITLE_SNAP_HOLD_FRAMES);
                    } else {
                        /* $80:E5F9 DEC A / BPL runs the wait count+1 times. */
                        game->title_sequence.hold_frames_left = NBA_TITLE_COMPLETE_HOLD_FRAMES + 1;
                        printf("[TITLE] Start after build: holding %d frames.\n",
                               NBA_TITLE_COMPLETE_HOLD_FRAMES);
                    }
                    game->title_sequence.phase = NBA_TITLE_PHASE_HOLD;
                }
            } else if (nba_title_sequence_advance(&game->title_sequence)) {
                /* $80:CF1B finished ramping INIDISP to zero. */
                nba_audio_stop();
                nba_game_enter_state(game, NBA_STATE_GAME_SETUP);
                nba_setup_screen_init(&game->setup, &game->assets);
                game->setup.bgm_started =
                    nba_audio_play_setup_spc(&game->assets);
            }
            break;

        case NBA_STATE_GAME_SETUP:
            /* $80:A3B8 - per-frame Game Setup update: slide-in, backdrop
             * scroll and row cursor. $80:A9E3/$80:AA7B/$80:AACD feed the
             * cycle-timed SPC command path started at the title handoff. */
            nba_setup_screen_update(&game->setup, &game->input, delta_time);
            break;

        default:
            break;
    }
}

/**
 * Offset/Address/Size: 0x007EE6 | $80:FEE6 | size: N/A (timing routine)
 * Purpose: Renders the extracted NBA / NBPA legal bitmap with the ROM's master-brightness ramp.
 */
void nba_game_render_nba_legal_notice(NbaGame *game) {
    if (!game) return;
    NbaRenderer *ren = &game->renderer;

    nba_renderer_clear(ren, 0xFF000000); /* Solid Black */

    const NbaAssetItem *item = nba_assets_get(&game->assets, NBA_ASSET_NBA_LEGAL_NOTICE);
    if (item && item->data) {
        uint32_t col_white = nba_game_master_brightness_color(
            0xFFFFFFFF, nba_game_legal_brightness(game->state_frame));
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
 * Subroutines: $82:94DF (Mode 7 scale init), $82:962D (Matrix A/D update), $82:F56D (22-frame zoom loop), $82:F4C4 (8-frame flash)
 * Purpose: Multi-stage EA Sports intro animation wrapper delegating to dedicated nba_ea_intro module.
 */
void nba_game_render_ea_intro(NbaGame *game) {
    if (!game) return;
    nba_ea_intro_render(&game->assets, &game->renderer,
                        (float)game->state_frame / 60.0f);
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

    switch (game->state) {
        case NBA_STATE_BOOT_RESET:
            nba_renderer_clear(ren, col_black);
            break;

        case NBA_STATE_NINTENDO_LICENSE: {
            nba_renderer_clear(ren, col_black);

            uint32_t license_white = nba_game_master_brightness_color(
                col_white, nba_game_license_brightness(game->state_frame));

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
                                    ren->pixels[py * NBA_SNES_WIDTH + px] = license_white;
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
                    license_white,
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

        case NBA_STATE_TITLE_SEQUENCE:
            nba_title_sequence_render(&game->title_sequence, &game->assets, ren,
                                      (int)game->state_frame);
            break;

        case NBA_STATE_GAME_SETUP:
            nba_setup_screen_render(&game->setup, ren);
            break;
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
                if (game->state_frame < 32) stage_str = "STG 1: 'E'";
                else if (game->state_frame < 63) stage_str = "STG 2: 'A'";
                else if (game->state_frame < 123) stage_str = "STG 3: 'SPORTS'";
                else stage_str = "STG 4: 'GAME' (HOLD)";
                break;
            }
            case NBA_STATE_TITLE_SEQUENCE: state_str = "TITLE SEQUENCE"; break;
            case NBA_STATE_GAME_SETUP: state_str = "GAME SETUP"; break;
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
