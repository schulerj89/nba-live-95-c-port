#ifndef NBA_GAME_H
#define NBA_GAME_H

#include "nba_types.h"
#include "nba_rom.h"
#include "nba_assets.h"
#include "nba_renderer.h"
#include "nba_audio.h"
#include "nba_audio_debugger.h"
#include "nba_asset_debugger.h"
#include "nba_ea_intro.h"
#include "nba_title_sequence.h"
#include "nba_setup_screen.h"

/* SNES Subroutine Addresses (LoROM Bank $80 and Bank $82) */
#define SNES_ADDR_RESET_BOOT            0x808020  /* $80:8020 - Cold boot reset handler */
#define SNES_ADDR_GAME_LOOP             0x80DA91  /* $80:DA91 - Main scene dispatcher loop */

/* SNES Frame Timing Counts (at 60 FPS) */
#define NBA_LICENSE_FRAMES              120       /* 2.000s ($80:FD9E) */
#define NBA_LEGAL_FRAMES                180       /* 3.000s ($80:FEE6) */
#define NBA_SCREEN_FADE_FRAMES          15        /* $80:CF1B/$80:CF3B - INIDISP levels 0..15 */

typedef enum {
    NBA_STATE_BOOT_RESET = 0,
    NBA_STATE_NINTENDO_LICENSE,
    NBA_STATE_NBA_LEGAL_NOTICE,
    NBA_STATE_EA_INTRO,
    NBA_STATE_TITLE_SEQUENCE,
    NBA_STATE_GAME_SETUP
} NbaGameState;

typedef struct {
    NbaRom rom;
    NbaAssetPack assets;
    NbaAudio audio;
    NbaRenderer renderer;
    NbaInput input;
    NbaGameState state;
    uint32_t frame_count;
    uint32_t state_frame;
    float state_timer;
    bool ea_intro_audio_started;
    NbaAudioDebugger audio_debugger;
    NbaAssetDebugger asset_debugger;
    NbaTitleSequence title_sequence;
    NbaSetupScreen setup_screen;
    bool show_timing_debug;
    bool is_initialized;
} NbaGame;

bool nba_game_init(NbaGame *game, const char *rom_path, const char *assets_path);
void nba_game_shutdown(NbaGame *game);
void nba_game_input_update(NbaInput *input, uint16_t raw_buttons);
void nba_game_tick(NbaGame *game, float delta_time);
void nba_game_render(NbaGame *game);
void nba_game_render_nba_legal_notice(NbaGame *game);
void nba_game_render_ea_intro(NbaGame *game);

#endif /* NBA_GAME_H */
