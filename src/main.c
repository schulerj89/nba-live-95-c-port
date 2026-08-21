#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "nba_game.h"
#include "nba_audio.h"
#include "nba_spc.h"

extern int win32_run_game(const char *rom_path, const char *assets_path,
                          bool title_only, bool setup_only);

/**
 * Offset/Address/Size: N/A | Application Entry Point / CLI Dispatcher | size: N/A
 * Purpose: Parses CLI flags, executes headless frame verifications, or launches Win32 desktop application.
 */
int main(int argc, char *argv[]) {
    const char *rom_path = NULL;
    const char *assets_path = NULL;
    const char *dump_frame_path = NULL;
    const char *dump_audio_path = NULL;
    const char *dump_menu_sfx_path = NULL;
    int menu_sfx_srcn = 0x1B;
    bool is_headless = false;
    bool audio_debug_test = false;
    int asset_debug_id = -1;
    bool start_at_title = false;
    bool start_at_setup = false;
    bool spc_self_test = false;
    int step_frames = 30;
    double tick_rate = 60.0;
    const char *setup_menu = NULL;
    int setup_menu_row = 0;
    int setup_menu_right = 0;
    int setup_main_row = -1;
    int setup_main_right = 0;
    int setup_main_left = 0;
    bool setup_main_confirm = false;
    bool setup_reenter = false;
    bool setup_menu_confirm = false;
    bool setup_menu_b = false;
    bool timing_debug = false;
    bool debug_state = false;
    int debug_every = 0;
    int debug_hud_page = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--rom") == 0 && i + 1 < argc) {
            rom_path = argv[++i];
        } else if (strcmp(argv[i], "--assets") == 0 && i + 1 < argc) {
            assets_path = argv[++i];
        } else if (strcmp(argv[i], "--headless") == 0) {
            is_headless = true;
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            step_frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--tick-rate") == 0 && i + 1 < argc) {
            tick_rate = strtod(argv[++i], NULL);
        } else if (strcmp(argv[i], "--dump-frame") == 0 && i + 1 < argc) {
            dump_frame_path = argv[++i];
        } else if (strcmp(argv[i], "--dump-audio") == 0 && i + 1 < argc) {
            dump_audio_path = argv[++i];
        } else if (strcmp(argv[i], "--audio-debug") == 0) {
            audio_debug_test = true;
        } else if (strcmp(argv[i], "--asset-debug") == 0 && i + 1 < argc) {
            char *end = NULL;
            long value = strtol(argv[++i], &end, 10);
            if (!end || *end != '\0' || value <= 0 || value >= NBA_ASSET_MAX) {
                fprintf(stderr, "[HEADLESS] Invalid ROM asset ID: %s\n", argv[i]);
                return 1;
            }
            asset_debug_id = (int)value;
        } else if (strcmp(argv[i], "--dump-menu-sfx") == 0 && i + 1 < argc) {
            dump_menu_sfx_path = argv[++i];
        } else if (strcmp(argv[i], "--menu-sfx-srcn") == 0 && i + 1 < argc) {
            char *end = NULL;
            long value = strtol(argv[++i], &end, 0);
            if (!end || *end != '\0' || value < 0x1A || value > 0x1C) {
                fprintf(stderr, "[HEADLESS] Menu SFX SRCN must be 0x1A..0x1C.\n");
                return 1;
            }
            menu_sfx_srcn = (int)value;
        } else if (strcmp(argv[i], "--title-only") == 0) {
            start_at_title = true;
        } else if (strcmp(argv[i], "--setup-only") == 0) {
            start_at_setup = true;
        } else if (strcmp(argv[i], "--spc-self-test") == 0) {
            spc_self_test = true;
        } else if (strcmp(argv[i], "--setup-menu") == 0 && i + 1 < argc) {
            setup_menu = argv[++i];
        } else if (strcmp(argv[i], "--setup-menu-row") == 0 && i + 1 < argc) {
            setup_menu_row = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--setup-menu-right") == 0 && i + 1 < argc) {
            setup_menu_right = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--setup-main-row") == 0 && i + 1 < argc) {
            setup_main_row = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--setup-main-right") == 0 && i + 1 < argc) {
            setup_main_right = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--setup-main-left") == 0 && i + 1 < argc) {
            setup_main_left = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--setup-main-confirm") == 0) {
            setup_main_confirm = true;
        } else if (strcmp(argv[i], "--setup-reenter") == 0) {
            setup_reenter = true;
        } else if (strcmp(argv[i], "--setup-menu-confirm") == 0) {
            setup_menu_confirm = true;
        } else if (strcmp(argv[i], "--setup-menu-b") == 0) {
            setup_menu_b = true;
        } else if (strcmp(argv[i], "--timing-debug") == 0) {
            timing_debug = true;
        } else if (strcmp(argv[i], "--debug-hud-page") == 0 && i + 1 < argc) {
            char *end = NULL;
            long value = strtol(argv[++i], &end, 10);
            if (!end || *end != '\0' || value < 1 || value > 2) {
                fprintf(stderr, "[HEADLESS] --debug-hud-page must be 1 or 2.\n");
                return 1;
            }
            debug_hud_page = (int)value;
        } else if (strcmp(argv[i], "--debug-state") == 0) {
            debug_state = true;
        } else if (strcmp(argv[i], "--debug-every") == 0 && i + 1 < argc) {
            char *end = NULL;
            long value = strtol(argv[++i], &end, 10);
            if (!end || *end != '\0' || value <= 0 || value > 1000000) {
                fprintf(stderr, "[HEADLESS] --debug-every must be 1..1000000.\n");
                return 1;
            }
            debug_every = (int)value;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("NBA Live '95 Native C Port\n");
            printf("Usage: nba95_port.exe [options]\n\n");
            printf("Options:\n");
            printf("  --rom <path>          Path to SNES ROM file\n");
            printf("  --assets <path>       Path to extracted asset pack (.pak)\n");
            printf("  --headless            Run without opening GUI window\n");
            printf("  --frames <N>          Number of frames to step in headless mode (default: 30)\n");
            printf("  --tick-rate <Hz>      Headless host tick rate (default: 60.0)\n");
            printf("  --audio-debug         Activate audio sample debugger in headless render\n");
            printf("  --asset-debug <ID>    Render the F12 ROM asset browser at asset ID\n");
            printf("  --dump-menu-sfx FILE  Save a deterministic packed-SPC menu sound\n");
            printf("  --menu-sfx-srcn N     Select menu SRCN 0x1A..0x1C (default 0x1B)\n");
            printf("  --title-only          Start at $80:E01E title state (headless tests)\n");
            printf("  --setup-only          Start at the $80:E600 -> $80:A2BF handoff\n");
            printf("  --setup-menu <name>   Open Rules or Options in headless mode\n");
            printf("  --setup-menu-row <N>  Move to submenu row N\n");
            printf("  --setup-menu-right N  Apply N right-value adjustments\n");
            printf("  --setup-menu-confirm  Press Start to commit submenu values\n");
            printf("  --setup-menu-b        Press ignored B after scripted edits\n");
            printf("  --setup-main-row N    Select main Setup row 0..3\n");
            printf("  --setup-main-right N  Apply N right adjustments on the main row\n");
            printf("  --setup-main-left N   Apply N left adjustments on the main row\n");
            printf("  --setup-main-confirm  Press A and report the requested scene action\n");
            printf("  --setup-reenter       Reinitialize Setup to verify session persistence\n");
            printf("  --timing-debug        Draw compact F10 overview page in a frame dump\n");
            printf("  --debug-hud-page N    Draw compact F10 page 1 or 2 in a frame dump\n");
            printf("  --debug-state         Print one expanded state snapshot after stepping\n");
            printf("  --debug-every N       Print an expanded state snapshot every N frames\n");
            printf("  --spc-self-test       Run deterministic SPC700/S-DSP core vectors\n");
            printf("  --dump-frame <file>   Save rendered frame to 24-bit BMP image\n");
            printf("  --dump-audio <file>   Save the active runtime-synthesized WAV\n");
            printf("  --help, -h            Show this help text\n");
            return 0;
        }
    }

    if (spc_self_test) {
        if (!nba_spc_self_test()) {
            fprintf(stderr, "[SPC TEST] FAIL\n");
            return 1;
        }
        printf("[SPC TEST] PASS: opcodes, timers, ports, BRR, and envelopes\n");
        return 0;
    }

    if (is_headless) {
        if (step_frames < 0 || tick_rate <= 0.0) {
            fprintf(stderr, "[HEADLESS] --frames must be non-negative and --tick-rate must be positive.\n");
            return 1;
        }
        if (setup_menu && strcmp(setup_menu, "rules") != 0 &&
            strcmp(setup_menu, "options") != 0) {
            fprintf(stderr, "[HEADLESS] --setup-menu must be rules or options.\n");
            return 1;
        }
        if (setup_menu_row < 0 || setup_menu_row > 1000 ||
            setup_menu_right < 0 || setup_menu_right > 1000 ||
            setup_main_row < -1 || setup_main_row > 3 ||
            setup_main_right < 0 || setup_main_right > 1000 ||
            setup_main_left < 0 || setup_main_left > 1000) {
            fprintf(stderr, "[HEADLESS] Invalid Setup menu row or adjustment count.\n");
            return 1;
        }
        printf("[HEADLESS] Starting headless verification (ROM: %s, Assets: %s, frames: %d)\n",
               rom_path ? rom_path : "(none)", assets_path ? assets_path : "(none)", step_frames);
        static NbaGame game; /* large renderer/active-scene buffers live off-stack */
        if (!nba_game_init(&game, rom_path, assets_path)) {
            fprintf(stderr, "[HEADLESS] Error: Failed to initialize game\n");
            return 1;
        }
        /* Synthesis and capture must not depend on a Windows output device. */
        nba_audio_set_host_playback_enabled(&game.audio, false);

        bool enter_setup = false;
        int title_press_frame = -1;
        int setup_down_count = 0;
        bool setup_menu_opened = false;
        bool setup_menu_done = false;
        int setup_menu_moves_done = 0;
        int setup_menu_right_done = 0;
        int setup_main_right_done = 0;
        int setup_main_left_done = 0;
        bool setup_main_confirm_done = false;
        bool setup_main_done = setup_main_row < 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--enter-setup") == 0) enter_setup = true;
            if (strcmp(argv[i], "--title-press") == 0 && i + 1 < argc) title_press_frame = atoi(argv[++i]);
            if (strcmp(argv[i], "--setup-down") == 0 && i + 1 < argc) setup_down_count = atoi(argv[++i]);
        }

        if (timing_debug && debug_hud_page == 0) debug_hud_page = 1;
        game.debug_hud_page = (uint8_t)debug_hud_page;

        if (start_at_title) {
            if (!nba_game_enter_state(&game, NBA_STATE_TITLE_SEQUENCE)) {
                nba_game_shutdown(&game);
                return 1;
            }
        }

        if (start_at_setup) {
            if (!nba_game_enter_state(&game, NBA_STATE_GAME_SETUP)) {
                nba_game_shutdown(&game);
                return 1;
            }
        }

        if (audio_debug_test) {
            game.audio_debugger.is_active = true;
            nba_audio_debugger_update(&game.audio_debugger, &game.audio,
                                      &game.assets, &game.input);
        }

        /* Step frames to reach desired screen */
        for (int frame = 0; frame < step_frames; frame++) {
            game.input.pressed = 0;

            if (enter_setup) {
                if (game.state == NBA_STATE_TITLE_SEQUENCE) {
                    game.input.pressed = NBA_BTN_START; /* skip the title video */
                }
            }

            /* --title-press N: press Start once on frame N, mirroring the
             * Mesen experiment used to time $80:E5C7's hold and fade. */
            if (title_press_frame >= 0 && frame == title_press_frame) {
                game.input.pressed = NBA_BTN_START;
            }

            /* --setup-down N: step the Game Setup cursor down N rows, one press
             * every 8 frames once the screen has settled. */
            if (setup_down_count > 0 && game.state == NBA_STATE_GAME_SETUP &&
                game.scene.setup.frame > NBA_SETUP_ENTER_FRAMES) {
                int since = game.scene.setup.frame - NBA_SETUP_ENTER_FRAMES;
                if (since % 8 == 1 && (since / 8) < setup_down_count) {
                    game.input.pressed = NBA_BTN_DOWN;
                }
            }

            /* Deterministic controller script for Rules/Options regressions.
             * One new press is issued per frame, after $80:A3B8 has settled. */
            if (setup_menu && !setup_menu_done &&
                game.state == NBA_STATE_GAME_SETUP &&
                game.scene.setup.frame >= NBA_SETUP_BG3_SETTLE_FRAME) {
                NbaSetupRow target = strcmp(setup_menu, "rules") == 0 ?
                                     NBA_SETUP_ROW_RULES : NBA_SETUP_ROW_OPTIONS;
                if (!setup_menu_opened) {
                    if (game.scene.setup.row != target) {
                        game.input.pressed = NBA_BTN_DOWN;
                    } else {
                        game.input.pressed = NBA_BTN_A;
                        setup_menu_opened = true;
                    }
                } else if (game.scene.setup.page != NBA_SETUP_PAGE_MAIN &&
                           game.scene.setup.transition == NBA_SETUP_TRANSITION_NONE) {
                    if (setup_menu_moves_done < setup_menu_row) {
                        game.input.pressed = NBA_BTN_DOWN;
                        setup_menu_moves_done++;
                    } else if (setup_menu_right_done < setup_menu_right) {
                        game.input.pressed = NBA_BTN_RIGHT;
                        setup_menu_right_done++;
                    } else if (setup_menu_b) {
                        game.input.pressed = NBA_BTN_B;
                        setup_menu_done = true;
                    } else if (setup_menu_confirm) {
                        game.input.pressed = NBA_BTN_START;
                        setup_menu_done = true;
                    } else {
                        setup_menu_done = true;
                    }
                }
            }
            if (!setup_main_done && game.state == NBA_STATE_GAME_SETUP &&
                game.scene.setup.page == NBA_SETUP_PAGE_MAIN &&
                game.scene.setup.frame >= NBA_SETUP_BG3_SETTLE_FRAME) {
                if ((int)game.scene.setup.row != setup_main_row) {
                    game.input.pressed = NBA_BTN_DOWN;
                } else if (setup_main_right_done < setup_main_right) {
                    game.input.pressed = NBA_BTN_RIGHT;
                    setup_main_right_done++;
                } else if (setup_main_left_done < setup_main_left) {
                    game.input.pressed = NBA_BTN_LEFT;
                    setup_main_left_done++;
                } else if (setup_main_confirm && !setup_main_confirm_done) {
                    game.input.pressed = NBA_BTN_A;
                    setup_main_confirm_done = true;
                } else {
                    setup_main_done = true;
                }
            }
            nba_game_tick(&game, (float)(1.0 / tick_rate));
            if (debug_every > 0 && (frame + 1) % debug_every == 0) {
                printf("[DEBUG SAMPLE] stepped=%d\n", frame + 1);
                nba_game_debug_print(&game);
            }
        }
        if (asset_debug_id >= 0) {
            game.asset_debugger.is_active = true;
            bool found = false;
            for (uint32_t index = 0; index < game.assets.item_count; ++index) {
                if (game.assets.items[index].id == (uint32_t)asset_debug_id) {
                    found = true;
                    game.asset_debugger.selected_index = (int)index;
                    if (game.assets.items[index].size == 0x10000u)
                        game.asset_debugger.tile_page = 16;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "[HEADLESS] ROM asset ID %d is not present in this pack.\n",
                        asset_debug_id);
                nba_game_shutdown(&game);
                return 1;
            }
        }

        if (dump_menu_sfx_path) {
            nba_audio_play_setup_sfx(&game.audio, &game.assets,
                                     (uint8_t)menu_sfx_srcn);
            if (!nba_audio_save_setup_sfx_wav(&game.audio, dump_menu_sfx_path)) {
                fprintf(stderr, "[HEADLESS] Failed to write menu SFX WAV.\n");
                nba_game_shutdown(&game);
                return 1;
            }
        }

        if (setup_menu) {
            const NbaSetupScreen *s = &game.scene.setup;
            int menu_count = strcmp(setup_menu, "rules") == 0 ?
                             NBA_SETUP_RULE_COUNT : NBA_SETUP_OPTION_COUNT;
            int report_row = setup_menu_row % menu_count;
            printf("[SETUP TEST] page=%d menu_row=%d transition=%d/%d blank=%d gfx=%d "
                   "rules0=%u/%u options0=%u/%u "
                   "option_row=%d working=%u committed=%u\n",
                   (int)s->page, s->menu_row, (int)s->transition,
                   s->transition_frame, s->transition_blank, s->has_gfx,
                   s->working_rules[0], s->config->rules[0],
                   s->working_options[0], s->config->options[0],
                   report_row,
                   strcmp(setup_menu, "rules") == 0 ?
                       s->working_rules[report_row] : s->working_options[report_row],
                   strcmp(setup_menu, "rules") == 0 ?
                       s->config->rules[report_row] : s->config->options[report_row]);
        }
        if (setup_main_row >= 0) {
            const NbaSetupScreen *s = &game.scene.setup;
            printf("[SETUP MAIN TEST] row=%d mode=%u style=%u level=%u quarter=%u action=%d\n",
                   (int)s->row, s->config->main_values[0], s->config->main_values[1],
                   s->config->main_values[2], s->config->main_values[3],
                   (int)game.last_setup_action);
        }
        if (setup_reenter) {
            if (!nba_game_enter_state(&game, NBA_STATE_GAME_SETUP)) {
                nba_game_shutdown(&game);
                return 1;
            }
            printf("[SETUP REENTER] mode=%u style=%u level=%u quarter=%u\n",
                   game.session.config.main_values[0],
                   game.session.config.main_values[1],
                   game.session.config.main_values[2],
                   game.session.config.main_values[3]);
        }
        if (debug_state) nba_game_debug_print(&game);

        nba_game_render(&game);

        if (dump_frame_path) {
            printf("[HEADLESS] Saving frame capture to: %s\n", dump_frame_path);
            if (nba_renderer_save_bmp(&game.renderer, dump_frame_path)) {
                printf("[HEADLESS] BMP frame written successfully (%dx%d).\n",
                       game.renderer.width, game.renderer.height);
            } else {
                fprintf(stderr, "[HEADLESS] Failed to write BMP file: %s\n", dump_frame_path);
                nba_game_shutdown(&game);
                return 1;
            }
        }

        if (dump_audio_path) {
            printf("[HEADLESS] Saving synthesized audio to: %s\n", dump_audio_path);
            if (!nba_audio_save_generated_wav(&game.audio, dump_audio_path)) {
                fprintf(stderr, "[HEADLESS] Failed to write synthesized audio.\n");
                nba_game_shutdown(&game);
                return 1;
            }
        }

        nba_game_shutdown(&game);
        printf("[HEADLESS] Headless execution completed successfully.\n");
        return 0;
    }

    /* Normal Win32 graphical execution */
    return win32_run_game(rom_path, assets_path, start_at_title, start_at_setup);
}
