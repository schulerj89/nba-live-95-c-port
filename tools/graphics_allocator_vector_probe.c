/* Production-linked replay probe for graphics allocator `$80:AB7E-$AC0C`
 * and its direct `$80:AC0D`/`$80:AC89` children. */
#include "nba_graphics_allocator.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t wram[NBA_GRAPHICS_WRAM_BYTES];
static uint8_t before[NBA_GRAPHICS_WRAM_BYTES];

/* Host-only probe helper; no native address. Read a little-endian word from
 * the allocator's canonical WRAM after production code returns. */
static uint16_t probe_read16(uint16_t address) {
    return (uint16_t)(wram[address] |
                      ((uint16_t)wram[(uint16_t)(address + 1u)] << 8));
}

/* Host-only probe setup; no native address. Write direct-child input words
 * before taking the immutable comparison copy. */
static void probe_write16(uint16_t address, uint16_t value) {
    wram[address] = (uint8_t)value;
    wram[(uint16_t)(address + 1u)] = (uint8_t)(value >> 8);
}

/* Host-only probe digest; no native address. FNV-1a retains complete bounded
 * post-state without embedding a 128 KiB fixture in the repository. */
static uint64_t probe_hash(const uint8_t *data, size_t size) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t index = 0; index < size; ++index) {
        hash ^= data[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

/* Host-only probe footprint digest; no native address. Hash every byte address
 * whose final value differs from the fixed nonzero entry projection. */
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

/* Host-only graphics allocator vector entry; no direct native address. Each
 * row invokes one production implementation and reports its complete state
 * and exact changed-address fingerprint. */
int main(void) {
    unsigned operation, fill, v0, v1, v2, v3, v4;
    while (scanf_s("%x %x %x %x %x %x %x", &operation, &fill,
                   &v0, &v1, &v2, &v3, &v4) == 7) {
        memset(wram, (uint8_t)fill, sizeof(wram));
        NbaGraphicsBus bus = {wram, sizeof(wram)};
        if (operation == 2u || operation == 3u) {
            probe_write16(NBA_GRAPHICS_ALLOCATOR_FILL_HEAD, (uint16_t)v0);
            probe_write16(NBA_GRAPHICS_ALLOCATOR_FILL_TAIL, (uint16_t)v1);
            probe_write16(NBA_GRAPHICS_ALLOCATOR_SAVED_TAIL, (uint16_t)v2);
            probe_write16(NBA_GRAPHICS_ALLOCATOR_ACTIVE_BASE, (uint16_t)v3);
            probe_write16(NBA_GRAPHICS_ALLOCATOR_PENDING_BASE, (uint16_t)v4);
        }
        memcpy(before, wram, sizeof(before));

        bool ok;
        if (operation == 4u) bus.wram = NULL;
        if (operation == 5u) bus.size = NBA_GRAPHICS_WRAM_BYTES - 1u;
        if (operation == 0u)
            ok = nba_graphics_allocator_initialize(
                &bus, (uint16_t)v0, (uint16_t)v1, (uint16_t)v2);
        else if (operation == 1u)
            ok = nba_graphics_allocator_clear_cache(&bus);
        else if (operation == 2u)
            ok = nba_graphics_allocator_fill_and_swap(&bus);
        else if (operation == 2u || operation == 3u) {
            ok = nba_graphics_allocator_fill_and_swap(&bus);
        } else
            ok = nba_graphics_allocator_initialize(
                &bus, (uint16_t)v0, (uint16_t)v1, (uint16_t)v2);

        size_t changed = 0u;
        uint64_t changed_hash = probe_changed_hash(&changed);
        printf("%u %016llx %016llx %zu %016llx %016llx %016llx "
               "%04x %04x %04x %04x %04x %04x %04x %04x %04x "
               "%04x %04x %02x %02x %02x %02x %02x %02x\n",
               ok ? 1u : 0u,
               (unsigned long long)probe_hash(wram, sizeof(wram)),
               (unsigned long long)changed_hash, changed,
               (unsigned long long)probe_hash(wram + 0x2000u, 0x0200u),
               (unsigned long long)probe_hash(wram + 0x2640u, 0x0832u),
               (unsigned long long)probe_hash(wram + 0x3271u, 0x0179u),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_SAVED_TAIL),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_ACTIVE_BASE),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_PENDING_BASE),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_BLOCKS),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_VRAM_BASE),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_X),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_EXTENT),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_COUNT),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_FILL_HEAD),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_FILL_TAIL),
               probe_read16(NBA_GRAPHICS_ALLOCATOR_STRIDE),
               wram[NBA_GRAPHICS_ALLOCATOR_READY],
               wram[NBA_GRAPHICS_ALLOCATOR_READY + 1u],
               wram[NBA_GRAPHICS_QUEUE_HEAD],
               wram[NBA_GRAPHICS_QUEUE_TAIL],
               wram[NBA_GRAPHICS_RECORD5_WORD], wram[0x8e10u]);
    }
    return ferror(stdin) ? 1 : 0;
}
