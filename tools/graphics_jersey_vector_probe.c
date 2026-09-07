/* Production-linked replay probe for jersey cache/appender `$80:AD2B-$AD88`. */
#include "nba_graphics_jersey.h"
#include "nba_graphics_allocator.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t wram[NBA_GRAPHICS_WRAM_BYTES];
static uint8_t before[NBA_GRAPHICS_WRAM_BYTES];

/* Host-only probe helper; no native address. Read a wrapping DBR=$7E word. */
static uint16_t probe_read16(uint16_t address) {
    return (uint16_t)(wram[address] |
                      ((uint16_t)wram[(uint16_t)(address + 1u)] << 8));
}

/* Host-only probe helper; no native address. Seed a wrapping DBR=$7E word. */
static void probe_write16(uint16_t address, uint16_t value) {
    wram[address] = (uint8_t)value;
    wram[(uint16_t)(address + 1u)] = (uint8_t)(value >> 8);
}

/* Host-only probe digest; no native address. FNV-1a retains full post-state. */
static uint64_t probe_hash(const uint8_t *data, size_t size) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t index = 0; index < size; ++index) {
        hash ^= data[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

/* Host-only probe footprint digest; no native address. Hash every changed
 * byte address in increasing order. */
static uint64_t probe_changed_hash(size_t *count) {
    uint64_t hash = UINT64_C(14695981039346656037);
    *count = 0u;
    for (uint32_t address = 0; address < NBA_GRAPHICS_WRAM_BYTES; ++address) {
        if (wram[address] == before[address]) continue;
        ++*count;
        for (unsigned shift = 0; shift < 24; shift += 8) {
            hash ^= (uint8_t)(address >> shift);
            hash *= UINT64_C(1099511628211);
        }
    }
    return hash;
}

/* Host-only pack-negative helper; no native address. Locate the mutable item
 * metadata owned by the loaded test pack. */
static NbaAssetItem *probe_draw_item(NbaAssetPack *assets) {
    for (uint32_t index = 0; index < assets->item_count; ++index)
        if (assets->items[index].id == NBA_ASSET_PLAYER_DRAW_INPUTS)
            return &assets->items[index];
    return NULL;
}

/* Host-only jersey vector entry; no direct native address. Each row invokes
 * the production implementation and reports complete WRAM/result state. */
int main(int argc, char **argv) {
    NbaAssetPack assets = {0};
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;

    unsigned mode, fill, be, c2, extent, vram, tail, cache;
    while (scanf_s("%x %x %x %x %x %x %x %x", &mode, &fill, &be, &c2,
                   &extent, &vram, &tail, &cache) == 8) {
        memset(wram, (uint8_t)fill, sizeof(wram));
        probe_write16(0x00beu, (uint16_t)be);
        probe_write16(0x00c2u, (uint16_t)c2);
        probe_write16(NBA_GRAPHICS_ALLOCATOR_EXTENT, (uint16_t)extent);
        probe_write16(NBA_GRAPHICS_ALLOCATOR_VRAM_BASE, (uint16_t)vram);
        probe_write16(NBA_GRAPHICS_QUEUE_TAIL, (uint16_t)tail);
        if (be < NBA_GRAPHICS_JERSEY_ACTOR_COUNT * 2u && (be & 1u) == 0u)
            probe_write16((uint16_t)(NBA_GRAPHICS_JERSEY_CACHE_BASE + be),
                          (uint16_t)cache);
        memcpy(before, wram, sizeof(before));

        NbaGraphicsBus bus = {wram, sizeof(wram)};
        NbaGraphicsJerseyAppendResult result = {
            0xa5a5u, 0xa5a5u, 0xa5a5u, true
        };
        const NbaAssetPack *asset_argument = &assets;
        NbaGraphicsBus *bus_argument = &bus;
        NbaGraphicsJerseyAppendResult *result_argument = &result;
        NbaAssetItem *draw = probe_draw_item(&assets);
        uint32_t saved_size = draw ? draw->size : 0u;
        uint8_t *draw_data = draw ? (uint8_t *)draw->data : NULL;
        uint8_t saved_magic = draw_data ? draw_data[0] : 0u;
        if (mode == 1u) asset_argument = NULL;
        if (mode == 2u) bus_argument = NULL;
        if (mode == 3u) bus.wram = NULL;
        if (mode == 4u) bus.size = NBA_GRAPHICS_WRAM_BYTES - 1u;
        if (mode == 5u) result_argument = NULL;
        if (mode == 6u && draw) --draw->size;
        if (mode == 7u && draw_data) draw_data[0] ^= 0xFFu;
        bool ok = nba_graphics_jersey_append(
            asset_argument, bus_argument, (uint16_t)be, (uint16_t)c2,
            result_argument);
        if (draw) draw->size = saved_size;
        if (draw_data) draw_data[0] = saved_magic;

        size_t changed = 0u;
        uint64_t changed_hash = probe_changed_hash(&changed);
        uint16_t record = (uint16_t)(NBA_GRAPHICS_QUEUE_RECORDS + (uint16_t)tail);
        printf("%u %04x %04x %04x %u %016llx %016llx %zu "
               "%04x %04x %04x %04x %04x %04x "
               "%02x %02x %02x %02x %02x %02x %02x %02x "
               "%02x %02x %02x %02x %02x %02x\n",
               ok ? 1u : 0u, result.destination_index,
               result.descriptor_vram_destination,
               result.descriptor_source_address, result.appended ? 1u : 0u,
               (unsigned long long)probe_hash(wram, sizeof(wram)),
               (unsigned long long)changed_hash, changed,
               probe_read16(0x0000u), probe_read16(0x0004u),
               probe_read16(NBA_GRAPHICS_QUEUE_TAIL),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_VRAM_BASE),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_EXTENT),
               be < NBA_GRAPHICS_JERSEY_ACTOR_COUNT * 2u && (be & 1u) == 0u
                   ? probe_read16((uint16_t)(NBA_GRAPHICS_JERSEY_CACHE_BASE + be))
                   : 0xa5a5u,
               wram[record], wram[(uint16_t)(record + 1u)],
               wram[(uint16_t)(record + 2u)], wram[(uint16_t)(record + 3u)],
               wram[(uint16_t)(record + 4u)], wram[(uint16_t)(record + 5u)],
               wram[(uint16_t)(record + 6u)], wram[(uint16_t)(record + 7u)],
               wram[0x0035u], wram[0x0039u], wram[0x012cu],
               wram[0x0566u], wram[0x8e24u], wram[0x1ffffu]);
    }
    nba_assets_free(&assets);
    return ferror(stdin) ? 1 : 0;
}
