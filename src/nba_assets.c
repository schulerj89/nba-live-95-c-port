#include "nba_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NBA_ASSET_MAGIC "NBA95PAK"

#define NBA_ASSET_PACK_VERSION 29u
#define NBA_ASSET_HEADER_SIZE 16u
#define NBA_ASSET_ENTRY_SIZE 24u

static uint32_t asset_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t asset_u16(const uint8_t *p) {
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}

static bool formation_payload_valid(const uint8_t *data, size_t size) {
    static const uint8_t counts[61] = {
        3,3,3,3,3,3,5,5,5,5,5,4,4,3,4,4,4,4,5,5,5,5,5,5,5,6,6,8,7,5,
        6,6,4,4,8,5,5,5,5,8,7,8,8,7,6,6,7,7,6,9,8,5,4,6,5,5,4,4,5,5,4
    };
    static const uint16_t roots[5] = {
        0xC745u, 0xC7BFu, 0xC839u, 0xC8B3u, 0xC92Du
    };
    if (!data || size != 8868u || memcmp(data, "NBFORM1", 8) ||
        asset_u32(data + 8) != 1u || asset_u32(data + 12) != 61u ||
        asset_u32(data + 16) != 5u || asset_u32(data + 20) != 1595u ||
        asset_u32(data + 24) != 48u || asset_u32(data + 28) != 2488u ||
        asset_u32(data + 32) != 6380u) return false;
    for (unsigned role = 0; role < 5u; ++role)
        if (asset_u16(data + 36u + role * 2u) != roots[role]) return false;
    size_t expected_offset = 2488u;
    for (unsigned play = 0; play < 61u; ++play) {
        uint16_t previous_pointer = 0u;
        for (unsigned role = 0; role < 5u; ++role) {
            const uint8_t *entry = data + 48u + (play * 5u + role) * 8u;
            uint16_t pointer = asset_u16(entry);
            if (asset_u16(entry + 2u) != counts[play] ||
                asset_u32(entry + 4u) != expected_offset) return false;
            if (role > 0u) {
                unsigned span = pointer > previous_pointer ?
                    pointer - previous_pointer : previous_pointer - pointer;
                if (span != (unsigned)counts[play] * 4u) return false;
            }
            previous_pointer = pointer;
            expected_offset += (size_t)counts[play] * 4u;
        }
    }
    return expected_offset == size;
}

static bool play_control_payload_valid(const uint8_t *data, size_t size) {
    static const uint8_t counts[61] = {
        3,3,3,3,3,3,5,5,5,5,5,4,4,3,4,4,4,4,5,5,5,5,5,5,5,6,6,8,8,5,
        6,6,4,4,8,5,5,5,5,8,7,8,8,7,6,6,7,7,6,9,8,5,4,4,4,6,5,5,5,5,4
    };
    if (!data || size != 3084u || memcmp(data, "NBPLAY1", 8) ||
        asset_u32(data + 8) != 1u || asset_u32(data + 12) != 61u ||
        asset_u32(data + 16) != 320u || asset_u32(data + 20) != 36u ||
        asset_u32(data + 24) != 524u || asset_u32(data + 28) != 2560u ||
        asset_u16(data + 32) != 0xC6AFu) return false;
    size_t expected_offset = 524u;
    for (unsigned play = 0; play < 61u; ++play) {
        const uint8_t *entry = data + 36u + play * 8u;
        uint16_t pointer = asset_u16(entry);
        uint16_t count = asset_u16(entry + 2u);
        if (pointer < 0xC9A7u || count != counts[play] ||
            asset_u32(entry + 4u) != expected_offset) return false;
        expected_offset += (size_t)count * 8u;
    }
    return expected_offset == size;
}

static bool cpu_table_payload_valid(const uint8_t *data, size_t size) {
    static const uint16_t ranges[14] = {
        0x1Du,6u, 0x18u,5u, 0x12u,6u, 0x2Cu,7u,
        0x27u,5u, 0x23u,4u, 0x33u,5u
    };
    static const uint8_t thresholds[8] = {2u,1u,2u,1u,1u,1u,1u,1u};
    if (!data || size != 246u || memcmp(data, "NBCAI1\0\0", 8) ||
        asset_u32(data + 8) != 1u || asset_u32(data + 12) != 29u ||
        asset_u32(data + 16) != 7u || asset_u32(data + 20) != 3u ||
        asset_u32(data + 24) != 6u || asset_u32(data + 28) != 44u ||
        asset_u32(data + 32) != 102u || asset_u32(data + 36) != 130u ||
        asset_u32(data + 40) != 238u) return false;
    for (unsigned i = 0; i < 58u; ++i)
        if (data[44u + i] >= 7u) return false;
    for (unsigned i = 0; i < 14u; ++i)
        if (asset_u16(data + 102u + i * 2u) != ranges[i]) return false;
    return memcmp(data + 238u, thresholds, sizeof(thresholds)) == 0;
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
        return size == 24u + 29u * 1184u * 416u * sizeof(uint32_t) &&
               width == 1184u && height == 416u && flags == 29u;
    if (id == NBA_ASSET_GAMEPLAY_COURT_MAP)
        return size == 6u + 148u * 52u * 2u && width == 148u &&
               height == 52u && flags == 0xA08000u;
    if (id == NBA_ASSET_GAMEPLAY_FORMATIONS)
        return size == 8868u && width == 61u && height == 5u && flags == 1595u;
    if (id == NBA_ASSET_GAMEPLAY_CPU_TABLES)
        return size == 246u && width == 29u && height == 7u && flags == 0x85C661u;
    if (id == NBA_ASSET_GAMEPLAY_SHOT_TABLES)
        return size == 528u && width == 5u && height == 0u && flags == 0x869EB2u;
    if (id == NBA_ASSET_GAMEPLAY_FATIGUE_TABLES)
        return size == 88u && width == 4u && height == 8u && flags == 0x8798DAu;
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
    const NbaAssetItem *formations = nba_assets_get(
        pack, NBA_ASSET_GAMEPLAY_FORMATIONS);
    if (formations && !formation_payload_valid(
            (const uint8_t *)formations->data, formations->size))
        return asset_load_error(pack, "Gameplay formation graph is invalid");
    if (formations) {
        int16_t x = 0, y = 0;
        if (!nba_assets_gameplay_formation_offset(
                pack, 0x01u, 0u, 0u, false, 0, &x, &y) || x != 80 || y != 0 ||
            !nba_assets_gameplay_formation_offset(
                pack, 0x10u, 0u, 0u, true, -1, &x, &y) ||
            x != -320 || y != -120)
            return asset_load_error(pack,
                "Gameplay formation accessor vectors are invalid");
    }
    const NbaAssetItem *play_control = nba_assets_get(
        pack, NBA_ASSET_GAMEPLAY_PLAY_CONTROL);
    if (play_control && !play_control_payload_valid(
            (const uint8_t *)play_control->data, play_control->size))
        return asset_load_error(pack, "Gameplay play-control graph is invalid");
    if (play_control) {
        NbaGameplayPlayControlRecord record;
        uint8_t count = 0u;
        if (!nba_assets_gameplay_play_control(
                pack, 0x35u, 0u, &record, &count) || count != 4u ||
            record.countdown != -1 || record.selector_a != 4 ||
            record.selector_b != 2 || record.selector_c != -1)
            return asset_load_error(pack,
                "Gameplay play-control accessor vectors are invalid");
    }
    const NbaAssetItem *cpu_tables = nba_assets_get(
        pack, NBA_ASSET_GAMEPLAY_CPU_TABLES);
    if (cpu_tables && !cpu_table_payload_valid(
            (const uint8_t *)cpu_tables->data, cpu_tables->size))
        return asset_load_error(pack, "Gameplay CPU tables are invalid");
    if (cpu_tables) {
        uint8_t strategy, base, count;
        bool hold;
        int16_t scalar, vertical, opaque;
        if (!nba_assets_gameplay_cpu_strategy(
                pack, 0u, 0u, &strategy, &base, &count, &hold) ||
            strategy != 1u || base != 0x18u || count != 5u || hold ||
            !nba_assets_gameplay_pass_launch(
                pack, 2u, 5u, &scalar, &vertical, &opaque) ||
            scalar != 40 || vertical != 0 || opaque != 32)
            return asset_load_error(pack,
                "Gameplay CPU table accessor vectors are invalid");
    }
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
    const size_t frame_size = 1184u * 416u * sizeof(uint32_t);
    if (!item || !item->data || home_team >= 29u ||
        item->size != 24u + 29u * frame_size)
        return NULL;
    const uint8_t *data = (const uint8_t *)item->data;
    if (memcmp(data, "NBCOURT2", 8) || asset_u32(data + 8) != 2u ||
        asset_u32(data + 12) != 29u || asset_u32(data + 16) != 1184u ||
        asset_u32(data + 20) != 416u)
        return NULL;
    return (const uint32_t *)(data + 24u + (size_t)home_team * frame_size);
}

bool nba_assets_gameplay_formation_offset(const NbaAssetPack *pack,
                                          uint8_t play, uint8_t role,
                                          uint8_t index, bool mirror_y,
                                          int16_t side_anchor_x, int16_t *x,
                                          int16_t *y) {
    const NbaAssetItem *item = nba_assets_get(pack,
        NBA_ASSET_GAMEPLAY_FORMATIONS);
    if (!item || !item->data || !x || !y || play >= 61u || role >= 5u ||
        item->size != 8868u) return false;
    const uint8_t *data = (const uint8_t *)item->data;
    if (memcmp(data, "NBFORM1", 8) || asset_u32(data + 8) != 1u ||
        asset_u32(data + 24) != 48u || asset_u32(data + 28) != 2488u)
        return false;
    const uint8_t *entry = data + 48u + ((size_t)play * 5u + role) * 8u;
    uint16_t count = asset_u16(entry + 2u);
    uint32_t offset = asset_u32(entry + 4u);
    if (index >= count || offset < 2488u ||
        (size_t)offset + (size_t)count * 4u > item->size) return false;
    const uint8_t *pair = data + offset + (size_t)index * 4u;
    uint16_t raw_x = asset_u16(pair);
    uint16_t raw_y = asset_u16(pair + 2u);
    if (mirror_y) raw_y = (uint16_t)(0u - raw_y);
    /* `$85:ADF5-$AE03`: mirror from DP `$9E` team context `+$0A`
     * (-336 left/+336 right), never from the moving ball X coordinate. */
    if (side_anchor_x < 0) {
        raw_x = (uint16_t)(0u - raw_x);
        if (play >= 0x0Eu) raw_y = (uint16_t)(0u - raw_y);
    }
    *x = (int16_t)raw_x;
    *y = (int16_t)raw_y;
    return true;
}

bool nba_assets_gameplay_play_control(const NbaAssetPack *pack, uint8_t play,
                                      uint8_t index,
                                      NbaGameplayPlayControlRecord *record,
                                      uint8_t *count) {
    const NbaAssetItem *item = nba_assets_get(
        pack, NBA_ASSET_GAMEPLAY_PLAY_CONTROL);
    if (!item || !item->data || play >= 61u || item->size != 3084u)
        return false;
    const uint8_t *data = (const uint8_t *)item->data;
    if (memcmp(data, "NBPLAY1", 8) || asset_u32(data + 8) != 1u ||
        asset_u32(data + 20) != 36u || asset_u32(data + 24) != 524u)
        return false;
    const uint8_t *entry = data + 36u + (size_t)play * 8u;
    uint16_t record_count = asset_u16(entry + 2u);
    uint32_t offset = asset_u32(entry + 4u);
    if (count) *count = (uint8_t)record_count;
    if (!record || index >= record_count || offset < 524u ||
        (size_t)offset + (size_t)record_count * 8u > item->size)
        return false;
    const uint8_t *source = data + offset + (size_t)index * 8u;
    record->countdown = (int16_t)asset_u16(source);
    record->selector_a = (int16_t)asset_u16(source + 2u);
    record->selector_b = (int16_t)asset_u16(source + 4u);
    record->selector_c = (int16_t)asset_u16(source + 6u);
    return true;
}

bool nba_assets_gameplay_cpu_strategy(const NbaAssetPack *pack, uint8_t team,
                                      uint8_t coin, uint8_t *strategy,
                                      uint8_t *play_base, uint8_t *play_count,
                                      bool *hold_final) {
    const NbaAssetItem *item = nba_assets_get(pack,
        NBA_ASSET_GAMEPLAY_CPU_TABLES);
    if (!item || !cpu_table_payload_valid((const uint8_t *)item->data,
            item->size) || team >= 29u || coin >= 2u || !strategy ||
        !play_base || !play_count || !hold_final) return false;
    const uint8_t *data = (const uint8_t *)item->data;
    uint8_t selected = data[44u + team * 2u + coin];
    const uint8_t *range = data + 102u + selected * 4u;
    *strategy = selected;
    *play_base = (uint8_t)asset_u16(range);
    *play_count = (uint8_t)asset_u16(range + 2u);
    *hold_final = selected == 5u;
    return true;
}

bool nba_assets_gameplay_pass_launch(const NbaAssetPack *pack, uint8_t family,
                                     uint8_t band, int16_t *flight_scalar,
                                     int16_t *vertical,
                                     int16_t *opaque_raw_2) {
    const NbaAssetItem *item = nba_assets_get(pack,
        NBA_ASSET_GAMEPLAY_CPU_TABLES);
    if (!item || !cpu_table_payload_valid((const uint8_t *)item->data,
            item->size) || family >= 3u || band >= 6u || !flight_scalar ||
        !vertical || !opaque_raw_2) return false;
    const uint8_t *record = (const uint8_t *)item->data + 130u +
                            ((size_t)family * 6u + band) * 6u;
    *flight_scalar = (int16_t)asset_u16(record);
    *vertical = (int16_t)asset_u16(record + 2u);
    *opaque_raw_2 = (int16_t)asset_u16(record + 4u);
    return true;
}

bool nba_assets_gameplay_pass_release_threshold(const NbaAssetPack *pack,
                                                uint8_t upper_state,
                                                uint8_t *threshold) {
    const NbaAssetItem *item = nba_assets_get(pack,
        NBA_ASSET_GAMEPLAY_CPU_TABLES);
    if (!item || !cpu_table_payload_valid((const uint8_t *)item->data,
            item->size) || upper_state < 0x2Au || upper_state > 0x31u ||
        !threshold) return false;
    *threshold = ((const uint8_t *)item->data)[238u + upper_state - 0x2Au];
    return true;
}
