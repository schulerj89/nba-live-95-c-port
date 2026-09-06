#ifndef NBA_GRAPHICS_ALLOCATOR_H
#define NBA_GRAPHICS_ALLOCATOR_H

#include "nba_graphics_bus.h"
#include <stdbool.h>
#include <stdint.h>

enum {
    NBA_GRAPHICS_ALLOCATOR_VRAM_BASE = 0x05eb,
    NBA_GRAPHICS_ALLOCATOR_X = 0x05ed,
    NBA_GRAPHICS_ALLOCATOR_EXTENT = 0x05ef,
    NBA_GRAPHICS_ALLOCATOR_COUNT = 0x05f1,
    NBA_GRAPHICS_ALLOCATOR_ACTIVE_BASE = 0x05e1,
    NBA_GRAPHICS_ALLOCATOR_PENDING_BASE = 0x05e3,
    NBA_GRAPHICS_ALLOCATOR_BLOCKS = 0x05e5,
    NBA_GRAPHICS_ALLOCATOR_FILL_HEAD = 0x05f3,
    NBA_GRAPHICS_ALLOCATOR_FILL_TAIL = 0x05f5,
    NBA_GRAPHICS_ALLOCATOR_SAVED_TAIL = 0x05df,
    NBA_GRAPHICS_ALLOCATOR_STRIDE = 0x05f9,
    NBA_GRAPHICS_ALLOCATOR_READY = 0x0566
};

bool nba_graphics_allocator_initialize(NbaGraphicsBus *bus,
                                       uint16_t vram_base,
                                       uint16_t allocator_x,
                                       uint16_t extent_y);
bool nba_graphics_allocator_clear_cache(NbaGraphicsBus *bus);
bool nba_graphics_allocator_fill_and_swap(NbaGraphicsBus *bus);

#endif
