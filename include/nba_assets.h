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
    NBA_ASSET_MAX                   = 128
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

#endif /* NBA_ASSETS_H */
