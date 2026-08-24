#include "nba_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NBA_ASSET_MAGIC "NBA95PAK"

#define NBA_ASSET_PACK_VERSION 23u
#define NBA_ASSET_HEADER_SIZE 16u
#define NBA_ASSET_ENTRY_SIZE 24u

static uint32_t asset_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool asset_load_error(NbaAssetPack *pack, const char *message) {
    fprintf(stderr, "[ASSETS] Error: %s\n", message);
    free(pack->raw_data);
    memset(pack, 0, sizeof(*pack));
    return false;
}

static bool asset_metadata_valid(uint32_t id, uint32_t size, uint32_t width,
                                 uint32_t height, uint32_t flags) {
    uint64_t required;
    if (id >= NBA_ASSET_TEAM_LOGO_BASE && id <= NBA_ASSET_TEAM_LOGO_LAST) {
        return width == 48u && height == 56u && flags == id - NBA_ASSET_TEAM_LOGO_BASE &&
               size == width * height * sizeof(uint32_t);
    }
    if (id >= NBA_ASSET_TEAM_VRAM_BASE && id <= NBA_ASSET_TEAM_VRAM_LAST)
        return size == 0x10000u && width == 0u && height == 0u &&
               flags == id - NBA_ASSET_TEAM_VRAM_BASE;
    if (id >= NBA_ASSET_TEAM_CGRAM_BASE && id <= NBA_ASSET_TEAM_CGRAM_LAST)
        return size == 0x200u && width == 0u && height == 0u &&
               flags == id - NBA_ASSET_TEAM_CGRAM_BASE;
    if (id == NBA_ASSET_TEAM_SELECT_OAM)
        return size == 0x220u && width == 0u && height == 0u && flags == 0u;
    if (id == NBA_ASSET_TEAM_SELECTED_PALETTE_CYCLE)
        return size == 26u && width == 7u && height == 7u && flags == 8u;
    if (id == NBA_ASSET_PLAYER_SETUP_VRAM)
        return size == 0x10000u && width == 0u && height == 0u && flags == 0u;
    if (id == NBA_ASSET_PLAYER_SETUP_CGRAM)
        return size == 0x200u && width == 0u && height == 0u && flags == 0u;
    if (id == NBA_ASSET_PLAYER_SETUP_OAM)
        return size == 0x220u && width == 0u && height == 0u && flags == 0u;
    if (id == NBA_ASSET_PLAYER_INTRO_COURT)
        return size == 256u * 224u * sizeof(uint32_t) &&
               width == 256u && height == 224u && flags == 0u;
    if (id == NBA_ASSET_PLAYER_INTRO_PORTRAITS)
        return size == 6015784u && width == 72u && height == 72u && flags == 290u;
    if (id == NBA_ASSET_TIPOFF_BALL)
        return size == 56u && width == 8u && height == 8u && flags == 0x0D9C27u;
    if (id == NBA_ASSET_GAMEPLAY_COURT)
        return size == 256u * 224u * sizeof(uint32_t) &&
               width == 256u && height == 224u && flags == 0u;
    if (id == NBA_ASSET_HOME_COURTS || id == NBA_ASSET_GAMEPLAY_HOME_COURTS)
        return size == 24u + 29u * 256u * 224u * sizeof(uint32_t) &&
               width == 256u && height == 224u && flags == 29u;
    if (id == NBA_ASSET_GAMEPLAY_COURT_PANORAMAS)
        return size == 24u + 29u * 912u * 416u * sizeof(uint32_t) &&
               width == 912u && height == 416u && flags == 29u;
    if (id == NBA_ASSET_EA_A_FIXED_SEQUENCE) {
        uint32_t x = flags >> 16;
        uint32_t y = flags & 0xFFFFu;
        required = (uint64_t)width * (uint64_t)height * sizeof(uint32_t) * 11u;
        return width > 0u && height > 0u && width <= NBA_SNES_WIDTH &&
               height <= NBA_SNES_HEIGHT && x <= NBA_SNES_WIDTH - width &&
               y <= NBA_SNES_HEIGHT - height && required == size;
    }
    switch ((NbaAssetId)id) {
        case NBA_ASSET_NINTENDO_LICENSE:
            return width == 128u && height == 11u && size == 176u && flags == 0u;

        case NBA_ASSET_NBA_LEGAL_NOTICE:
            return width == NBA_SNES_WIDTH && height > 0u &&
                   height <= NBA_SNES_HEIGHT && flags <= NBA_SNES_HEIGHT - height &&
                   size == 32u * height;

        case NBA_ASSET_SETUP_VRAM:
        case NBA_ASSET_SET_RULES_VRAM:
        case NBA_ASSET_SET_OPTIONS_VRAM:
        case NBA_ASSET_OPTIONS_OFF_VRAM:
        case NBA_ASSET_OPTIONS_MONO_VRAM:
        case NBA_ASSET_OPTIONS_CPU_VRAM:
        case NBA_ASSET_OPTIONS_CROWD_OFF_VRAM:
        case NBA_ASSET_OPTIONS_SLOW_ON_VRAM:
        case NBA_ASSET_OPTIONS_ASSISTANCE_ON_VRAM:
        case NBA_ASSET_SETUP_MODE_SEASON_VRAM:
        case NBA_ASSET_SETUP_MODE_PLAYOFFS_VRAM:
        case NBA_ASSET_SETUP_MODE_LOAD_SERIES_VRAM:
        case NBA_ASSET_SETUP_STYLE_CUSTOM_VRAM:
        case NBA_ASSET_SETUP_STYLE_ARCADE_VRAM:
        case NBA_ASSET_SETUP_LEVEL_STARTER_VRAM:
        case NBA_ASSET_SETUP_LEVEL_ALL_STAR_VRAM:
        case NBA_ASSET_SETUP_QUARTER_5_VRAM:
        case NBA_ASSET_SETUP_QUARTER_8_VRAM:
        case NBA_ASSET_SETUP_QUARTER_12_VRAM:
        case NBA_ASSET_RULES_OPEN_VRAM:
        case NBA_ASSET_OPTIONS_OPEN_VRAM:
        case NBA_ASSET_SETUP_RETURN_VRAM:
        case NBA_ASSET_RULES_RETURN_VRAM:
            return size == 0x10000u && width == 0u && height == 0u && flags == 0u;

        case NBA_ASSET_SETUP_CGRAM:
        case NBA_ASSET_SET_RULES_CGRAM:
        case NBA_ASSET_SET_OPTIONS_CGRAM:
        case NBA_ASSET_RULES_OPEN_CGRAM:
        case NBA_ASSET_OPTIONS_OPEN_CGRAM:
        case NBA_ASSET_SETUP_RETURN_CGRAM:
        case NBA_ASSET_RULES_RETURN_CGRAM:
            return size == 0x200u && width == 0u && height == 0u && flags == 0u;

        case NBA_ASSET_SET_RULES_OAM:
        case NBA_ASSET_SET_OPTIONS_OAM:
            return size == 0x220u && width == 0x6000u && height == 0u && flags == 0u;

        case NBA_ASSET_EA_LOGO_STAGE1:
        case NBA_ASSET_EA_LOGO_STAGE2:
        case NBA_ASSET_EA_LOGO_STAGE3:
        case NBA_ASSET_EA_LOGO_STAGE4:
        case NBA_ASSET_EA_A_LAYER:
        case NBA_ASSET_EA_E_LAYER:
        case NBA_ASSET_EA_SPORTS_LAYER:
        case NBA_ASSET_EA_LOGO_FINAL: {
            uint32_t x = flags >> 16;
            uint32_t y = flags & 0xFFFFu;
            required = (uint64_t)width * (uint64_t)height * sizeof(uint32_t);
            return width > 0u && height > 0u && width <= NBA_SNES_WIDTH &&
                   height <= NBA_SNES_HEIGHT && x <= NBA_SNES_WIDTH - width &&
                   y <= NBA_SNES_HEIGHT - height && required == size;
        }

        default:
            return true;
    }
}

/**
 * Offset/Address/Size: 0x000000 | Asset Pack Binary (NBA95PAK) | size: Dynamic (approx 1.45 MB)
 * Purpose: Loads pre-extracted authentic ROM bitmaps, palettes, and BRR->PCM audio assets.
 */
bool nba_assets_load(NbaAssetPack *pack, const char *asset_path) {
    if (!pack || !asset_path) return false;
    memset(pack, 0, sizeof(NbaAssetPack));

    FILE *f = fopen(asset_path, "rb");
    if (!f) {
        printf("[ASSETS] Note: Asset pack '%s' not found.\n", asset_path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < (long)NBA_ASSET_HEADER_SIZE) {
        fprintf(stderr, "[ASSETS] Error: Asset pack too small (%ld bytes)\n", file_size);
        fclose(f);
        return false;
    }

    pack->raw_data = (uint8_t *)malloc(file_size);
    if (!pack->raw_data) {
        fprintf(stderr, "[ASSETS] Error: Out of memory allocating %ld bytes for asset pack\n", file_size);
        fclose(f);
        return false;
    }

    if (fread(pack->raw_data, 1, file_size, f) != (size_t)file_size) {
        fprintf(stderr, "[ASSETS] Error: Failed to read asset pack data\n");
        free(pack->raw_data);
        pack->raw_data = NULL;
        fclose(f);
        return false;
    }
    fclose(f);
    pack->raw_size = (size_t)file_size;

    const uint8_t *header = pack->raw_data;
    if (memcmp(header, NBA_ASSET_MAGIC, 8) != 0) {
        return asset_load_error(pack, "Invalid asset pack magic");
    }

    uint32_t version = asset_u32(header + 8);
    uint32_t asset_count = asset_u32(header + 12);
    if (version != NBA_ASSET_PACK_VERSION) {
        return asset_load_error(pack, "Unsupported asset pack version");
    }
    if (asset_count == 0 || asset_count >= NBA_ASSET_MAX) {
        return asset_load_error(pack, "Invalid asset count");
    }
    if (asset_count > (pack->raw_size - NBA_ASSET_HEADER_SIZE) /
                      NBA_ASSET_ENTRY_SIZE) {
        return asset_load_error(pack, "Truncated asset directory");
    }

    size_t data_start = NBA_ASSET_HEADER_SIZE +
                        (size_t)asset_count * NBA_ASSET_ENTRY_SIZE;
    uint32_t ea_width = 0, ea_height = 0, ea_flags = 0;
    for (uint32_t i = 0; i < asset_count; ++i) {
        const uint8_t *entry = header + NBA_ASSET_HEADER_SIZE +
                               (size_t)i * NBA_ASSET_ENTRY_SIZE;
        uint32_t id = asset_u32(entry);
        uint32_t offset = asset_u32(entry + 4);
        uint32_t size = asset_u32(entry + 8);
        uint32_t width = asset_u32(entry + 12);
        uint32_t height = asset_u32(entry + 16);
        uint32_t flags = asset_u32(entry + 20);
        if (id == NBA_ASSET_NONE || id >= NBA_ASSET_MAX) {
            return asset_load_error(pack, "Asset ID is outside the supported range");
        }
        for (uint32_t previous = 0; previous < i; ++previous) {
            if (pack->items[previous].id == id) {
                return asset_load_error(pack, "Duplicate asset ID");
            }
        }
        if ((size_t)offset < data_start || (size_t)offset > pack->raw_size ||
            (size_t)size > pack->raw_size - (size_t)offset) {
            return asset_load_error(pack, "Asset payload is outside the pack");
        }
        if (!asset_metadata_valid(id, size, width, height, flags)) {
            return asset_load_error(pack, "Asset metadata does not match its payload type");
        }
        if (id >= NBA_ASSET_EA_LOGO_STAGE1 && id <= NBA_ASSET_EA_LOGO_STAGE4) {
            if (ea_width != 0u &&
                (width != ea_width || height != ea_height || flags != ea_flags)) {
                return asset_load_error(pack, "EA stage dimensions are inconsistent");
            }
            ea_width = width;
            ea_height = height;
            ea_flags = flags;
        }
        if ((id == NBA_ASSET_EA_A_LAYER || id == NBA_ASSET_EA_E_LAYER ||
             id == NBA_ASSET_EA_SPORTS_LAYER ||
             id == NBA_ASSET_EA_LOGO_FINAL ||
             id == NBA_ASSET_EA_A_FIXED_SEQUENCE) &&
            (width != ea_width || height != ea_height || flags != ea_flags)) {
            return asset_load_error(pack, "EA letter-layer dimensions are inconsistent");
        }

        pack->items[i].id = id;
        pack->items[i].offset = offset;
        pack->items[i].size = size;
        pack->items[i].width = width;
        pack->items[i].height = height;
        pack->items[i].flags = flags;
        pack->items[i].data = pack->raw_data + offset;
    }
    pack->item_count = asset_count;

    pack->is_loaded = true;
    printf("[ASSETS] Loaded asset pack: '%s' (%zu bytes, %u assets)\n",
           asset_path, pack->raw_size, pack->item_count);
    return true;
}

/**
 * Offset/Address/Size: N/A | Host Memory | size: N/A
 * Purpose: Releases asset pack buffer and invalidates item entries.
 */
void nba_assets_free(NbaAssetPack *pack) {
    if (!pack) return;
    free(pack->raw_data);
    memset(pack, 0, sizeof(*pack));
}

/**
 * Offset/Address/Size: N/A | Item Directory Lookup | size: N/A
 * Purpose: Queries a loaded asset item by its unique enumeration ID.
 */
const NbaAssetItem *nba_assets_get(const NbaAssetPack *pack, NbaAssetId id) {
    if (!pack || !pack->is_loaded) return NULL;
    for (uint32_t i = 0; i < pack->item_count; i++) {
        if (pack->items[i].id == (uint32_t)id) {
            return &pack->items[i];
        }
    }
    return NULL;
}

const uint32_t *nba_assets_home_court(const NbaAssetPack *pack, uint8_t home_team) {
    const NbaAssetItem *item = nba_assets_get(pack, NBA_ASSET_HOME_COURTS);
    const size_t frame_size = 256u * 224u * sizeof(uint32_t);
    if (!item || !item->data || home_team >= 29u || item->size != 24u + 29u * frame_size)
        return NULL;
    const uint8_t *data = (const uint8_t *)item->data;
    if (memcmp(data, "NBCOURT1", 8) || asset_u32(data + 8) != 1u ||
        asset_u32(data + 12) != 29u || asset_u32(data + 16) != 256u ||
        asset_u32(data + 20) != 224u)
        return NULL;
    return (const uint32_t *)(data + 24u + (size_t)home_team * frame_size);
}

const uint32_t *nba_assets_gameplay_home_court(const NbaAssetPack *pack,
                                                uint8_t home_team) {
    const NbaAssetItem *item = nba_assets_get(pack, NBA_ASSET_GAMEPLAY_HOME_COURTS);
    const size_t frame_size = 256u * 224u * sizeof(uint32_t);
    if (!item || !item->data || home_team >= 29u || item->size != 24u + 29u * frame_size)
        return NULL;
    const uint8_t *data = (const uint8_t *)item->data;
    if (memcmp(data, "NBCOURT1", 8) || asset_u32(data + 8) != 1u ||
        asset_u32(data + 12) != 29u || asset_u32(data + 16) != 256u ||
        asset_u32(data + 20) != 224u)
        return NULL;
    return (const uint32_t *)(data + 24u + (size_t)home_team * frame_size);
}

const uint32_t *nba_assets_gameplay_court_panorama(const NbaAssetPack *pack,
                                                    uint8_t home_team) {
    const NbaAssetItem *item = nba_assets_get(pack,
        NBA_ASSET_GAMEPLAY_COURT_PANORAMAS);
    const size_t frame_size = 912u * 416u * sizeof(uint32_t);
    if (!item || !item->data || home_team >= 29u ||
        item->size != 24u + 29u * frame_size)
        return NULL;
    const uint8_t *data = (const uint8_t *)item->data;
    if (memcmp(data, "NBCOURT2", 8) || asset_u32(data + 8) != 1u ||
        asset_u32(data + 12) != 29u || asset_u32(data + 16) != 912u ||
        asset_u32(data + 20) != 416u)
        return NULL;
    return (const uint32_t *)(data + 24u + (size_t)home_team * frame_size);
}
