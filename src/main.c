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
    bool is_headless = false;
    bool audio_debug_test = false;
    bool start_at_title = false;
    bool start_at_setup = false;
    bool spc_self_test = false;
    int step_frames = 30;
    double tick_rate = 60.0;

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
        } else if (strcmp(argv[i], "--title-only") == 0) {
            start_at_title = true;
        } else if (strcmp(argv[i], "--setup-only") == 0) {
            start_at_setup = true;
        } else if (strcmp(argv[i], "--spc-self-test") == 0) {
            spc_self_test = true;
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
            printf("  --title-only          Start at $80:E01E title state (headless tests)\n");
            printf("  --setup-only          Start at the $80:E600 -> $80:A2BF handoff\n");
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
        printf("[HEADLESS] Starting headless verification (ROM: %s, Assets: %s, frames: %d)\n",
               rom_path ? rom_path : "(none)", assets_path ? assets_path : "(none)", step_frames);
        NbaGame game;
        if (!nba_game_init(&game, rom_path, assets_path)) {
            fprintf(stderr, "[HEADLESS] Error: Failed to initialize game\n");
            return 1;
        }

        bool timing_debug_test = false;
        bool enter_setup = false;
        int title_press_frame = -1;
        int setup_down_count = 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--timing-debug") == 0) timing_debug_test = true;
            if (strcmp(argv[i], "--enter-setup") == 0) enter_setup = true;
            if (strcmp(argv[i], "--title-press") == 0 && i + 1 < argc) title_press_frame = atoi(argv[++i]);
            if (strcmp(argv[i], "--setup-down") == 0 && i + 1 < argc) setup_down_count = atoi(argv[++i]);
        }

        if (timing_debug_test) {
            game.show_timing_debug = true;
        }

        if (start_at_title) {
            game.state = NBA_STATE_TITLE_SEQUENCE;
            game.state_frame = 0;
            game.state_timer = 0.0f;
            nba_title_sequence_init(&game.title_sequence);
        }

        if (start_at_setup) {
            game.state = NBA_STATE_GAME_SETUP;
            game.state_frame = 0;
            game.state_timer = 0.0f;
            nba_setup_screen_init(&game.setup_screen, &game.assets);
            nba_audio_play_setup_dsp(&game.audio, &game.assets);
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
                game.setup_screen.frame > NBA_SETUP_ENTER_FRAMES) {
                int since = game.setup_screen.frame - NBA_SETUP_ENTER_FRAMES;
                if (since % 8 == 1 && (since / 8) < setup_down_count) {
                    game.input.pressed = NBA_BTN_DOWN;
                }
            }
            nba_game_tick(&game, (float)(1.0 / tick_rate));
        }

        nba_game_render(&game);

        if (dump_frame_path) {
            printf("[HEADLESS] Saving frame capture to: %s\n", dump_frame_path);
            if (nba_renderer_save_bmp(&game.renderer, dump_frame_path)) {
                printf("[HEADLESS] BMP frame written successfully (%dx%d).\n",
                       game.renderer.width, game.renderer.height);
            } else {
                fprintf(stderr, "[HEADLESS] Failed to write BMP file: %s\n", dump_frame_path);
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
