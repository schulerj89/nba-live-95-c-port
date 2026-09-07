#include "nba_graphics_jersey.h"
#include "nba_graphics_allocator.h"

/* Host-only guard; no direct native address. It protects the canonical WRAM
 * view required by jersey cache/appender `$80:AD2B-$AD88`. */
static bool jersey_bus_valid(const NbaGraphicsBus *bus) {
    return bus && bus->wram && bus->size >= NBA_GRAPHICS_WRAM_BYTES;
}

/* Host-only DBR=$7E helper for `$80:AD2B-$AD88`; native absolute word reads
 * wrap their second byte inside the same 64 KiB bank. */
static uint16_t jersey_read16(const NbaGraphicsBus *bus, uint16_t address) {
    return (uint16_t)(bus->wram[address] |
                      ((uint16_t)bus->wram[(uint16_t)(address + 1u)] << 8));
}

/* Host-only DBR=$7E helper for `$80:AD2B-$AD88`; native overlapping word
 * stores build the eight-byte publication descriptor in exact write order. */
static void jersey_write16(NbaGraphicsBus *bus, uint16_t address,
                           uint16_t value) {
    bus->wram[address] = (uint8_t)value;
    bus->wram[(uint16_t)(address + 1u)] = (uint8_t)(value >> 8);
}

/* `$80:AD2B-$AD88`, rendering: cache the `$87:A99E` direction source and
 * append one 32-byte jersey upload to the canonical 512-byte graphics ring.
 * BE is twice actor identity and C2 is the actor's +$52 display direction. */
bool nba_graphics_jersey_append(
    const NbaAssetPack *assets, NbaGraphicsBus *bus,
    uint16_t actor_offset_be, uint16_t display_direction_c2,
    NbaGraphicsJerseyAppendResult *result) {
    uint16_t source_base;
    if (!result || !jersey_bus_valid(bus) ||
        actor_offset_be >= NBA_GRAPHICS_JERSEY_ACTOR_COUNT * 2u ||
        (actor_offset_be & 1u) != 0u || display_direction_c2 >= 8u ||
        !nba_assets_player_jersey_source(
            assets, display_direction_c2, &source_base) ||
        source_base == 0xffffu) return false;

    uint16_t actor = (uint16_t)(actor_offset_be >> 1);
    uint16_t destination_index = (uint16_t)(
        actor + jersey_read16(bus, NBA_GRAPHICS_ALLOCATOR_EXTENT));
    uint16_t cache_address = (uint16_t)(
        NBA_GRAPHICS_JERSEY_CACHE_BASE + actor_offset_be);
    bool cache_hit = jersey_read16(bus, cache_address) == source_base;
    uint16_t tail = 0u;
    if (!cache_hit) {
        tail = jersey_read16(bus, NBA_GRAPHICS_QUEUE_TAIL);
        if (tail > NBA_GRAPHICS_QUEUE_BYTES - NBA_GRAPHICS_JERSEY_DESCRIPTOR_BYTES ||
            (tail & (NBA_GRAPHICS_JERSEY_DESCRIPTOR_BYTES - 1u)) != 0u)
            return false;
    }
    NbaGraphicsJerseyAppendResult next = {destination_index, 0u, 0u, false};

    jersey_write16(bus, 0x0004u, destination_index);
    if (!cache_hit) {
        uint16_t descriptor_source = (uint16_t)(
            source_base + actor * NBA_GRAPHICS_JERSEY_ACTOR_SOURCE_BYTES);
        uint16_t descriptor_destination = (uint16_t)(
            (uint16_t)(destination_index << 4) +
            jersey_read16(bus, NBA_GRAPHICS_ALLOCATOR_VRAM_BASE) +
            ((destination_index & 0x1000u) != 0u));
        jersey_write16(bus, cache_address, source_base);
        jersey_write16(bus, 0x0000u, source_base);
        jersey_write16(bus, 0x0000u, descriptor_source);
        uint16_t record = (uint16_t)(NBA_GRAPHICS_QUEUE_RECORDS + tail);
        jersey_write16(bus, record, 0x0001u);
        jersey_write16(bus, (uint16_t)(record + 1u), descriptor_source);
        jersey_write16(bus, (uint16_t)(record + 3u), 0x007eu);
        jersey_write16(bus, (uint16_t)(record + 4u), 0x0020u);
        jersey_write16(bus, (uint16_t)(record + 6u), descriptor_destination);
        jersey_write16(bus, NBA_GRAPHICS_QUEUE_TAIL,
            (uint16_t)((tail + NBA_GRAPHICS_JERSEY_DESCRIPTOR_BYTES) & 0x01ffu));
        next.descriptor_vram_destination = descriptor_destination;
        next.descriptor_source_address = descriptor_source;
        next.appended = true;
    }
    *result = next;
    return true;
}
