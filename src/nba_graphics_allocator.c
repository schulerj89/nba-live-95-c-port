#include "nba_graphics_allocator.h"
#include <stdint.h>

/* Host-only guard; no direct native address. It protects the canonical WRAM
 * view required by `$80:AB7E-$ACC1` before any native-equivalent write. */
static bool graphics_bus_valid(const NbaGraphicsBus *bus) {
    return bus && bus->wram && bus->size >= NBA_GRAPHICS_WRAM_BYTES;
}

/* Host-only DBR=$7E helper for `$80:AB7E-$ACC1`; native absolute word reads
 * wrap their second byte inside the same 64 KiB bank. */
static uint16_t read16(const NbaGraphicsBus *bus, uint16_t address) {
    return (uint16_t)(bus->wram[address] |
                      ((uint16_t)bus->wram[(uint16_t)(address + 1u)] << 8));
}

/* Host-only DBR=$7E helper for `$80:AB7E-$ACC1`; native absolute word writes
 * wrap their second byte inside the same 64 KiB bank. */
static void write16(NbaGraphicsBus *bus, uint16_t address, uint16_t value) {
    bus->wram[address] = (uint8_t)value;
    bus->wram[(uint16_t)(address + 1u)] = (uint8_t)(value >> 8);
}

/* `$80:AC0D-$AC1A`, graphics allocator: mark all 1,049 cache words at
 * canonical WRAM `$2640-$2E71` unused without touching either neighbor. */
bool nba_graphics_allocator_clear_cache(NbaGraphicsBus *bus) {
    if (!graphics_bus_valid(bus)) return false;
    for (uint16_t offset = 0x0830u;; offset = (uint16_t)(offset - 2u)) {
        write16(bus, (uint16_t)(0x2640u + offset), 0xffffu);
        if (offset == 0u) break;
    }
    return true;
}

/* `$80:AC89-$ACC1`, graphics allocator: fill aligned four-byte records in
 * the pending buffer, rotate the three native buffer endpoints, and publish
 * the byte-ready flag while preserving `$0567` and each two-byte hole. */
bool nba_graphics_allocator_fill_and_swap(NbaGraphicsBus *bus) {
    if (!graphics_bus_valid(bus)) return false;
    uint16_t position = read16(bus, NBA_GRAPHICS_ALLOCATOR_FILL_HEAD);
    uint16_t tail = read16(bus, NBA_GRAPHICS_ALLOCATOR_FILL_TAIL);
    if (position < tail && (position & 3u) != 0u) return false;

    while (position < read16(bus, NBA_GRAPHICS_ALLOCATOR_FILL_TAIL)) {
        write16(bus, position, 0xe100u);
        position = (uint16_t)(position + 4u);
        if (position == 0u) break;
    }

    uint16_t saved_tail = read16(bus, NBA_GRAPHICS_ALLOCATOR_SAVED_TAIL);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_SAVED_TAIL,
            read16(bus, NBA_GRAPHICS_ALLOCATOR_FILL_HEAD));
    write16(bus, NBA_GRAPHICS_ALLOCATOR_FILL_TAIL, saved_tail);

    uint16_t active = read16(bus, NBA_GRAPHICS_ALLOCATOR_ACTIVE_BASE);
    uint16_t pending = read16(bus, NBA_GRAPHICS_ALLOCATOR_PENDING_BASE);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_PENDING_BASE, active);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_ACTIVE_BASE, pending);
    bus->wram[NBA_GRAPHICS_ALLOCATOR_READY] = 1u;
    return true;
}

/* `$80:AB7E-$AC0C`, graphics allocator: initialize its canonical WRAM
 * control words and signed byte tables, clear the cache through `$80:AC0D`,
 * then fill and rotate native buffers through `$80:AC89`. The production
 * `$85:8B6C-$8B75` caller supplies A=$6000, X=0 and Y=$01E0. */
bool nba_graphics_allocator_initialize(NbaGraphicsBus *bus,
                                       uint16_t vram_base,
                                       uint16_t allocator_x,
                                       uint16_t extent_y) {
    if (!graphics_bus_valid(bus)) return false;

    write16(bus, NBA_GRAPHICS_ALLOCATOR_VRAM_BASE, vram_base);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_X, allocator_x);
    uint16_t rounded = extent_y < 0x01c0u ? 0x01a0u : extent_y;
    rounded = (uint16_t)((uint16_t)(rounded + 0x001fu) & 0xffe0u);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_EXTENT, rounded);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_COUNT, 0u);

    uint8_t index = (uint8_t)(rounded >> 2);
    uint8_t table_value = 0u;
    do {
        bus->wram[0x3271u + index] = table_value;
        uint8_t next = (uint8_t)(index - 8u);
        bus->wram[0x32eau + index] = next;
        index = next;
        table_value = 8u;
    } while ((int8_t)index >= 0);
    bus->wram[0x32eau] = 0xffu;

    write16(bus, NBA_GRAPHICS_ALLOCATOR_STRIDE, 2u);
    uint8_t delta = (uint8_t)(0x0200u - rounded);
    bus->wram[0x3363u] = delta;
    bus->wram[0x3363u + delta] = 0u;
    bus->wram[0x33c4u + delta] = 0u;
    bus->wram[0x33c4u] = 0xffu;

    write16(bus, NBA_GRAPHICS_ALLOCATOR_BLOCKS, 0x0080u);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_FILL_HEAD, 0x2000u);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_FILL_TAIL, 0x2200u);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_SAVED_TAIL, 0x2420u);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_PENDING_BASE, 0x2220u);
    write16(bus, NBA_GRAPHICS_ALLOCATOR_ACTIVE_BASE, 0x2000u);

    return nba_graphics_allocator_clear_cache(bus) &&
           nba_graphics_allocator_fill_and_swap(bus);
}
