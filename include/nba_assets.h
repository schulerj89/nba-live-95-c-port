#ifndef NBA_ASSETS_H
#define NBA_ASSETS_H

#include "nba_types.h"

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
    NBA_ASSET_AUDIO_EA_GAME    = 10,
    NBA_ASSET_MAX              = 16
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
