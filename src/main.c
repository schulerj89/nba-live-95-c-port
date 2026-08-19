#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "nba_game.h"

extern int win32_run_game(const char *rom_path, const char *assets_path);

/**
 * Offset/Address/Size: N/A | Application Entry Point / CLI Dispatcher | size: N/A
 * Purpose: Parses CLI flags, executes headless frame verifications, or launches Win32 desktop application.
 */
int main(int argc, char *argv[]) {
    const char *rom_path = NULL;
    const char *assets_path = NULL;
    const char *dump_frame_path = NULL;
    bool is_headless = false;
    bool audio_debug_test = false;
    int step_frames = 30;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--rom") == 0 && i + 1 < argc) {
            rom_path = argv[++i];
        } else if (strcmp(argv[i], "--assets") == 0 && i + 1 < argc) {
            assets_path = argv[++i];
        } else if (strcmp(argv[i], "--headless") == 0) {
            is_headless = true;
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            step_frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--dump-frame") == 0 && i + 1 < argc) {
            dump_frame_path = argv[++i];
        } else if (strcmp(argv[i], "--audio-debug") == 0) {
            audio_debug_test = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("NBA Live '95 Native C Port\n");
            printf("Usage: nba95_port.exe [options]\n\n");
            printf("Options:\n");
            printf("  --rom <path>          Path to SNES ROM file\n");
            printf("  --assets <path>       Path to extracted asset pack (.pak)\n");
            printf("  --headless            Run without opening GUI window\n");
            printf("  --frames <N>          Number of frames to step in headless mode (default: 30)\n");
            printf("  --audio-debug         Activate audio sample debugger in headless render\n");
            printf("  --dump-frame <file>   Save rendered frame to 24-bit BMP image\n");
            printf("  --help, -h            Show this help text\n");
            return 0;
        }
    }

    if (is_headless) {
        printf("[HEADLESS] Starting headless verification (ROM: %s, Assets: %s, frames: %d)\n",
               rom_path ? rom_path : "(none)", assets_path ? assets_path : "(none)", step_frames);
        NbaGame game;
        if (!nba_game_init(&game, rom_path, assets_path)) {
            fprintf(stderr, "[HEADLESS] Error: Failed to initialize game\n");
            return 1;
        }

        bool timing_debug_test = false;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--timing-debug") == 0) timing_debug_test = true;
        }

        if (timing_debug_test) {
            game.show_timing_debug = true;
        }

        if (audio_debug_test) {
            game.audio_debugger.is_active = true;
            nba_audio_debugger_update(&game.audio_debugger, &game.assets, &game.input);
        }

        /* Step frames to reach desired screen */
        for (int frame = 0; frame < step_frames; frame++) {
            nba_game_tick(&game, 1.0f / 60.0f);
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

        nba_game_shutdown(&game);
        printf("[HEADLESS] Headless execution completed successfully.\n");
        return 0;
    }

    /* Normal Win32 graphical execution */
    return win32_run_game(rom_path, assets_path);
}
