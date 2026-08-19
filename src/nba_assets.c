#include "nba_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NBA_ASSET_MAGIC "NBA95PAK"

typedef struct {
    char magic[8];
    uint32_t version;
    uint32_t asset_count;
} PackHeader;

typedef struct {
    uint32_t id;
    uint32_t offset;
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t flags;
} PackEntry;

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

    if (file_size < (long)(sizeof(PackHeader))) {
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

    PackHeader *hdr = (PackHeader *)pack->raw_data;
    if (memcmp(hdr->magic, NBA_ASSET_MAGIC, 8) != 0) {
        fprintf(stderr, "[ASSETS] Error: Invalid asset pack magic: %.8s\n", hdr->magic);
        free(pack->raw_data);
        pack->raw_data = NULL;
        return false;
    }

    pack->item_count = hdr->asset_count;
    if (pack->item_count > NBA_ASSET_MAX) {
        pack->item_count = NBA_ASSET_MAX;
    }

    PackEntry *entries = (PackEntry *)(pack->raw_data + sizeof(PackHeader));
    for (uint32_t i = 0; i < pack->item_count; i++) {
        PackEntry *e = &entries[i];
        if (e->offset + e->size <= pack->raw_size) {
            pack->items[i].id     = e->id;
            pack->items[i].offset = e->offset;
            pack->items[i].size   = e->size;
            pack->items[i].width  = e->width;
            pack->items[i].height = e->height;
            pack->items[i].flags  = e->flags;
            pack->items[i].data   = pack->raw_data + e->offset;
        }
    }

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
    if (pack->raw_data) {
        free(pack->raw_data);
        pack->raw_data = NULL;
    }
    pack->raw_size = 0;
    pack->item_count = 0;
    pack->is_loaded = false;
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
