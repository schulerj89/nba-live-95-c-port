#ifndef NBA_GAME_H
#define NBA_GAME_H

#include "nba_types.h"
#include "nba_rom.h"
#include "nba_assets.h"
#include "nba_renderer.h"
#include "nba_audio_debugger.h"
#include "nba_ea_intro.h"
#include "nba_title_sequence.h"
#include "nba_setup_screen.h"

/* SNES Subroutine Addresses (LoROM Bank $80 and Bank $82) */
#define SNES_ADDR_RESET_BOOT            0x808020  /* $80:8020 - Cold boot reset handler */
#define SNES_ADDR_GAME_LOOP             0x80DA91  /* $80:DA91 - Main scene dispatcher loop */
#define SNES_ADDR_SETUP_INTRO           0x82F15C  /* $82:F15C - EA Sports intro setup entry point */
#define SNES_ADDR_DRAW_STAGE1_E         0x82F4F6  /* $82:F4F6 - Assemble 'E' emblem tilegroup */
#define SNES_ADDR_DRAW_STAGE2_A         0x82F512  /* $82:F512 - Assemble 'A' emblem tilegroup */
#define SNES_ADDR_DRAW_STAGE3_SPORTS    0x82F52E  /* $82:F52E - Assemble 'SPORTS' banner */
#define SNES_ADDR_DRAW_STAGE4_BANNER    0x808FA3  /* $80:8FA3 - Assemble 'ELECTRONIC ARTS' banner */
#define SNES_ADDR_FLASH_SUBROUTINE      0x82F4C4  /* $82:F4C4 - 8-frame specular highlight flash loop */
#define SNES_ADDR_COLOR_STEP            0x82F5E7  /* $82:F5E7 - Palette RGB channel stepping algorithm */
#define SNES_ADDR_STAGE_STEP_LOOP       0x82F56D  /* $82:F56D - 22-frame entrance animation step loop */
#define SNES_ADDR_SCROLL_MATRIX_UPDATE  0x82962D  /* $82:962D - Mode 7 matrix & scroll register dispatcher */

/* SNES Frame Timing Counts (at 60 FPS) */
#define NBA_LICENSE_FRAMES              120       /* 2.000s ($80:FD9E) */
#define NBA_LEGAL_FRAMES                180       /* 3.000s ($80:FEE6) */
#define NBA_SCREEN_FADE_FRAMES          15        /* $80:CF1B/$80:CF3B - INIDISP levels 0..15 */
#define NBA_INTRO_STAGE1_FRAMES         32        /* 0.533s ($82:F2EA) */
#define NBA_INTRO_STAGE2_FRAMES         31        /* 0.517s ($82:F36A) */
#define NBA_INTRO_STAGE3_FRAMES         60        /* 1.000s ($82:F408) */
#define NBA_INTRO_STAGE4_FRAMES         180       /* 3.000s ($82:F469) */
#define NBA_INTRO_TOTAL_FRAMES          303       /* 5.050s total */

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
    NbaRenderer renderer;
    NbaInput input;
    NbaGameState state;
    uint32_t frame_count;
    float state_timer;
    uint8_t ea_voice_stage;
    NbaAudioDebugger audio_debugger;
    NbaTitleSequence title_sequence;
    NbaSetupScreen setup;
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
