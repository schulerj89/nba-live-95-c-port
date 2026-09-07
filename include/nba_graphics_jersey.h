#ifndef NBA_GRAPHICS_JERSEY_H
#define NBA_GRAPHICS_JERSEY_H

#include "nba_assets.h"
#include "nba_graphics_bus.h"
#include <stdbool.h>
#include <stdint.h>

enum {
    NBA_GRAPHICS_JERSEY_CACHE_BASE = 0x8e10,
    NBA_GRAPHICS_JERSEY_ACTOR_COUNT = 10,
    NBA_GRAPHICS_JERSEY_ACTOR_SOURCE_BYTES = 0x00c0,
    NBA_GRAPHICS_JERSEY_DESCRIPTOR_BYTES = 8
};

typedef struct {
    uint16_t destination_index;
    uint16_t descriptor_vram_destination;
    uint16_t descriptor_source_address;
    bool appended;
} NbaGraphicsJerseyAppendResult;

bool nba_graphics_jersey_append(
    const NbaAssetPack *assets, NbaGraphicsBus *bus,
    uint16_t actor_offset_be, uint16_t display_direction_c2,
    NbaGraphicsJerseyAppendResult *result);

#endif
