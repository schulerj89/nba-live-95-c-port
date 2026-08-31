#ifndef NBA_ASSETS_H
#define NBA_ASSETS_H

#include "nba_types.h"

/* SNES ROM Addresses & Offsets for Assets */
#define SNES_ROM_OFF_NINTENDO_LICENSE   0x007D9E  /* $80:FD9E - license/legal sequence routine */
#define SNES_ROM_OFF_NBA_LEGAL_NOTICE   0x007EE6  /* $80:FEE6 - NBA legal timing routine */
#define SNES_ROM_OFF_EA_INTRO_CODE      0x01715C  /* $82:F15C - EA Sports intro routines */

/* Authentic Digitized Voice Sample Offsets (BRR compressed bitstreams in ROM) */
#define SNES_ROM_OFF_AUDIO_VOICE_E      0x12D9C5  /* $A5:D9C5 - 'E' sample (size: 0x0DA4, 3492 bytes BRR, 0.39s) */
#define SNES_ROM_OFF_AUDIO_VOICE_A      0x12801C  /* $A5:801C - 'A' sample (size: 0x15CC, 5580 bytes BRR, 0.62s) */
#define SNES_ROM_OFF_AUDIO_VOICE_SPORTS 0x11E03D  /* $A3:E03D - 'SPORTS' sample (size: 0x1710, 5904 bytes BRR, 0.66s) */
#define SNES_ROM_OFF_AUDIO_VOICE_GAME   0x11249B  /* $A2:A49B - 'It\'s in the game' (size: 0x234C, 9036 bytes BRR, 1.00s) */

typedef enum {
    NBA_ASSET_NONE             = 0,
    NBA_ASSET_NINTENDO_LICENSE = 1,
    NBA_ASSET_NBA_LEGAL_NOTICE = 2,
    NBA_ASSET_EA_LOGO_STAGE1   = 3,
    NBA_ASSET_EA_LOGO_STAGE2   = 4,
    NBA_ASSET_EA_LOGO_STAGE3   = 5,
    NBA_ASSET_EA_LOGO_STAGE4   = 6,
    NBA_ASSET_AUDIO_EA_INTRO   = 7,
    NBA_ASSET_AUDIO_EA_E       = 8,
    NBA_ASSET_AUDIO_EA_A       = 9,
    NBA_ASSET_AUDIO_EA_SPORTS  = 10,
    NBA_ASSET_AUDIO_EA_GAME    = 11,
    /* IDs 12-15 were the removed screenshot/WAV title shortcut. */
    NBA_ASSET_SETUP_VRAM            = 16,
    NBA_ASSET_SETUP_CGRAM           = 17,
    NBA_ASSET_DEBUG_AUDIO_BASE      = 18,
    /* $82:F4F6/$82:F512 independent indexed Mode 7 letter tilegroups. */
    NBA_ASSET_EA_A_LAYER            = 70,
    NBA_ASSET_EA_E_LAYER            = 71,
    NBA_ASSET_EA_LOGO_FINAL         = 72,
    NBA_ASSET_EA_A_FIXED_SEQUENCE   = 73,
    /* $82:F52E's two $80:8FA3 draws of the $82:F6D8 Mode 7 tilegroup. */
    NBA_ASSET_EA_SPORTS_LAYER       = 74,
    /* Indexed ROM resources; supersede legacy bitmap IDs 1-6 and 70-74. */
    NBA_ASSET_EA_INDEXED           = 75,
    NBA_ASSET_INTRO_TEXT           = 76,
    NBA_ASSET_TITLE_VRAM            = 80,
    NBA_ASSET_TITLE_CGRAM           = 81,
    NBA_ASSET_TITLE_PPU_TRACE       = 82,
    NBA_ASSET_TITLE_SPC_RAM         = 83,
    NBA_ASSET_TITLE_SPC_DSP         = 84,
    NBA_ASSET_TITLE_SPC_STATE       = 85,
    NBA_ASSET_TITLE_APU_TRACE       = 86,
    NBA_ASSET_TITLE_CUE_TRACE       = 87,
    NBA_ASSET_SETUP_SPC_RAM         = 88,
    NBA_ASSET_SETUP_SPC_DSP         = 89,
    NBA_ASSET_SETUP_SPC_STATE       = 90,
    NBA_ASSET_SETUP_APU_TRACE       = 91,
    NBA_ASSET_SETUP_PPU_TRACE       = 92,
    NBA_ASSET_SETUP_DSP_TRACE       = 93,
    /* 30 BRR sources from Setup's streamed SPC directory, exposed by F11. */
    NBA_ASSET_SETUP_SAMPLE_BASE     = 94,
    NBA_ASSET_SETUP_SAMPLE_LAST     = 123,
    NBA_ASSET_SET_RULES_VRAM        = 124,
    NBA_ASSET_SET_RULES_CGRAM       = 125,
    NBA_ASSET_SET_OPTIONS_VRAM      = 126,
    NBA_ASSET_SET_OPTIONS_CGRAM     = 127,
    NBA_ASSET_SET_RULES_OAM         = 128,
    NBA_ASSET_SET_OPTIONS_OAM       = 129,
    NBA_ASSET_OPTIONS_OFF_VRAM      = 130,
    NBA_ASSET_OPTIONS_MONO_VRAM     = 131,
    NBA_ASSET_OPTIONS_CPU_VRAM      = 132,
    NBA_ASSET_SETUP_MODE_SEASON_VRAM      = 133,
    NBA_ASSET_SETUP_MODE_PLAYOFFS_VRAM    = 134,
    NBA_ASSET_SETUP_MODE_LOAD_SERIES_VRAM = 135,
    NBA_ASSET_SETUP_STYLE_CUSTOM_VRAM     = 136,
    NBA_ASSET_SETUP_STYLE_ARCADE_VRAM     = 137,
    NBA_ASSET_SETUP_LEVEL_STARTER_VRAM    = 138,
    NBA_ASSET_SETUP_LEVEL_ALL_STAR_VRAM   = 139,
    NBA_ASSET_SETUP_QUARTER_5_VRAM        = 140,
    NBA_ASSET_SETUP_QUARTER_8_VRAM        = 141,
    NBA_ASSET_SETUP_QUARTER_12_VRAM       = 142,
    NBA_ASSET_RULES_OPEN_VRAM              = 143,
    NBA_ASSET_RULES_OPEN_CGRAM             = 144,
    NBA_ASSET_RULES_OPEN_PPU_TRACE         = 145,
    NBA_ASSET_OPTIONS_OPEN_VRAM            = 146,
    NBA_ASSET_OPTIONS_OPEN_CGRAM           = 147,
    NBA_ASSET_OPTIONS_OPEN_PPU_TRACE       = 148,
    NBA_ASSET_SETUP_RETURN_VRAM             = 149,
    NBA_ASSET_SETUP_RETURN_CGRAM            = 150,
    NBA_ASSET_SETUP_RETURN_PPU_TRACE        = 151,
    NBA_ASSET_OPTIONS_CROWD_OFF_VRAM        = 152,
    NBA_ASSET_RULES_RETURN_VRAM             = 153,
    NBA_ASSET_RULES_RETURN_CGRAM            = 154,
    NBA_ASSET_RULES_RETURN_PPU_TRACE        = 155,
    NBA_ASSET_OPTIONS_SLOW_ON_VRAM          = 156,
    NBA_ASSET_OPTIONS_ASSISTANCE_ON_VRAM    = 157,
    NBA_ASSET_TEAM_LOGO_BASE                = 160,
    NBA_ASSET_TEAM_LOGO_LAST                = 188,
    NBA_ASSET_TEAM_SELECT_OAM               = 189,
    NBA_ASSET_TEAM_VRAM_BASE                = 192,
    NBA_ASSET_TEAM_VRAM_LAST                = 220,
    NBA_ASSET_TEAM_CGRAM_BASE               = 221,
    NBA_ASSET_TEAM_CGRAM_LAST               = 249,
    NBA_ASSET_TEAM_SELECTED_PALETTE_CYCLE   = 250,
    NBA_ASSET_PLAYER_ROSTERS                = 251,
    NBA_ASSET_PLAYER_DEFAULT_POSE           = 252,
    NBA_ASSET_PLAYER_TILE_SOURCES           = 253,
    NBA_ASSET_PLAYER_PALETTE_TABLES         = 254,
    NBA_ASSET_PLAYER_POSE_LAYOUT            = 255,
    NBA_ASSET_PLAYER_ANIMATIONS              = 256,
    NBA_ASSET_PLAYER_SETUP_VRAM              = 257,
    NBA_ASSET_PLAYER_SETUP_CGRAM             = 258,
    NBA_ASSET_PLAYER_SETUP_OAM               = 259,
    NBA_ASSET_PLAYER_INTRO_COURT              = 260,
    NBA_ASSET_PLAYER_INTRO_PORTRAITS          = 261,
    NBA_ASSET_TIPOFF_BALL                      = 262,
    NBA_ASSET_GAMEPLAY_COURT                   = 263,
    /* Six ROM OBJ poses used by $83:F901's team-rating basketball rows. */
    NBA_ASSET_PLAYER_INTRO_RATING_BALLS        = 264,
    NBA_ASSET_PLAYER_INTRO_SPC_RAM              = 265,
    NBA_ASSET_PLAYER_INTRO_SPC_DSP              = 266,
    NBA_ASSET_PLAYER_INTRO_SPC_STATE            = 267,
    NBA_ASSET_PLAYER_INTRO_DSP_TRACE            = 268,
    NBA_ASSET_PLAYER_INTRO_FONT                 = 269,
    NBA_ASSET_STARTING_LINEUP_FONT               = 270,
    /* NBCOURT1: 29 ROM-decoded home presentation courts in team-ID order. */
    NBA_ASSET_HOME_COURTS                        = 271,
    /* NBCOURT1: gameplay-bright variants selected by $84:E55D-$E57A. */
    NBA_ASSET_GAMEPLAY_HOME_COURTS               = 272,
    /* NBCOURT2 v2: complete ROM $A0:8006 148x52-tile panoramas. */
    NBA_ASSET_GAMEPLAY_COURT_PANORAMAS            = 273,
    /* NBFORM1: exact `$85:AD6B` 61-play x five-role coordinate graph. */
    NBA_ASSET_GAMEPLAY_FORMATIONS                  = 274,
    /* NBPLAY1: `$85:B377/$B2DC` 61-play control stream graph. */
    NBA_ASSET_GAMEPLAY_PLAY_CONTROL                 = 275,
    /* NBCAI1: ROM CPU strategy, pass-flight, and release-threshold tables. */
    NBA_ASSET_GAMEPLAY_CPU_TABLES                   = 276,
    /* NBSHOT1: ROM shot probability, timing and free-throw launch tables. */
    NBA_ASSET_GAMEPLAY_SHOT_TABLES                  = 277,
    NBA_ASSET_GAMEPLAY_FATIGUE_TABLES              = 278,
    NBA_ASSET_GAMEPLAY_COURT_MAP                 = 279,
    NBA_ASSET_GAMEPLAY_JUMP_TABLES              = 280,
    NBA_ASSET_GAMEPLAY_GRAPHICS_SCRATCH         = 281,
    NBA_ASSET_GAMEPLAY_GOAL_LAYER               = 282,
    NBA_ASSET_GAMEPLAY_CROWD_TILES              = 283,
    /* NBPPUIN1: 29 raw gameplay VRAM/CGRAM states. Runtime samples original
     * indexed tiles instead of treating a flattened panorama as opaque. */
    NBA_ASSET_GAMEPLAY_PPU_INPUTS                = 284,
    /* NBGAUD1: ROM-decoded gameplay BRR sources used by the live mixer. */
    NBA_ASSET_GAMEPLAY_AUDIO_BANK                = 285,
    NBA_ASSET_GAMEPLAY_HUD          = 286, /* WIP resource; production caller is unwired. */
    NBA_ASSET_MAX                   = 287
} NbaAssetId;

typedef struct {
    uint32_t id;
    uint32_t offset;
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t flags;
    const void *data;
} NbaAssetItem;

typedef struct {
    uint8_t *raw_data;
    size_t raw_size;
    NbaAssetItem items[NBA_ASSET_MAX];
    uint32_t item_count;
    bool is_loaded;
} NbaAssetPack;

bool nba_assets_load(NbaAssetPack *pack, const char *asset_path);
void nba_assets_free(NbaAssetPack *pack);
const NbaAssetItem *nba_assets_get(const NbaAssetPack *pack, NbaAssetId id);
const uint32_t *nba_assets_home_court(const NbaAssetPack *pack, uint8_t home_team);
const uint32_t *nba_assets_gameplay_home_court(const NbaAssetPack *pack,
                                                uint8_t home_team);
const uint32_t *nba_assets_gameplay_court_panorama(const NbaAssetPack *pack,
                                                    uint8_t home_team);
bool nba_assets_gameplay_ppu_input(const NbaAssetPack *pack, uint8_t home_team,
                                   const uint8_t **vram,
                                   const uint8_t **cgram);
bool nba_assets_gameplay_formation_offset(const NbaAssetPack *pack,
                                          uint8_t play, uint8_t role,
                                          uint8_t index, bool mirror_y,
                                          int16_t side_anchor_x, int16_t *x,
                                          int16_t *y);

typedef struct {
    int16_t countdown;
    int16_t selector_a;
    int16_t selector_b;
    int16_t selector_c;
} NbaGameplayPlayControlRecord;

bool nba_assets_gameplay_play_control(const NbaAssetPack *pack, uint8_t play,
                                      uint8_t index,
                                      NbaGameplayPlayControlRecord *record,
                                      uint8_t *count);
bool nba_assets_gameplay_cpu_strategy(const NbaAssetPack *pack, uint8_t team,
                                      uint8_t coin, uint8_t *strategy,
                                      uint8_t *play_base, uint8_t *play_count,
                                      bool *hold_final);
bool nba_assets_gameplay_pass_launch(const NbaAssetPack *pack, uint8_t family,
                                     uint8_t band, int16_t *flight_scalar,
                                     int16_t *vertical,
                                     int16_t *opaque_raw_2);
bool nba_assets_gameplay_pass_release_threshold(const NbaAssetPack *pack,
                                                uint8_t upper_state,
                                                uint8_t *threshold);

#endif /* NBA_ASSETS_H */
