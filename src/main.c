#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "nba_game.h"

extern int win32_run_game(const char *rom_path);

int main(int argc, char *argv[]) {
    const char *rom_path = "F:\\Games\\SNES\\NBA Live 95 (USA).sfc";
    const char *dump_frame_path = NULL;
    bool is_headless = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--rom") == 0 && i + 1 < argc) {
            rom_path = argv[++i];
        } else if (strcmp(argv[i], "--headless") == 0) {
            is_headless = true;
        } else if (strcmp(argv[i], "--dump-frame") == 0 && i + 1 < argc) {
            dump_frame_path = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("NBA Live '95 Native C Port\n");
            printf("Usage: nba95_port.exe [options]\n\n");
            printf("Options:\n");
            printf("  --rom <path>          Path to SNES ROM file\n");
            printf("  --headless            Run without opening GUI window\n");
            printf("  --dump-frame <file>   Save rendered frame to 24-bit BMP image\n");
            printf("  --help, -h            Show this help text\n");
            return 0;
        }
    }

    if (is_headless) {
        printf("[HEADLESS] Starting headless verification with ROM: %s\n", rom_path);
        NbaGame game;
        if (!nba_game_init(&game, rom_path)) {
            fprintf(stderr, "[HEADLESS] Error: Failed to initialize game\n");
            return 1;
        }

        /* Step frames to reach licensing screen */
        for (int frame = 0; frame < 30; frame++) {
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
    return win32_run_game(rom_path);
}
