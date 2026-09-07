/* Production caller probe for `$87:A64D-$A656` -> `$80:AD2B-$AD88`. */
#include "nba_game.h"
#include "nba_graphics_allocator.h"
#include <stdio.h>
#include <string.h>

static NbaGame game;
static uint8_t jersey_before[NBA_PLAYER_JERSEY_WRAM_BYTES];

/* Host-only caller-probe helper; no native address. Read canonical WRAM. */
static uint16_t probe_read16(uint16_t address) {
    return (uint16_t)(game.graphics_wram[address] |
        ((uint16_t)game.graphics_wram[(uint16_t)(address + 1u)] << 8));
}

/* Host-only caller-probe helper; no native address. Seed caller-controlled
 * cache/ring words without fabricating publisher-owned source buffers. */
static void probe_write16(uint16_t address, uint16_t value) {
    game.graphics_wram[address] = (uint8_t)value;
    game.graphics_wram[(uint16_t)(address + 1u)] = (uint8_t)(value >> 8);
}

/* Host-only caller-probe helper; no native address. Validate one overlapping
 * eight-byte descriptor emitted by `$80:AD2B-$AD88`. */
static int require_descriptor(uint16_t tail, uint16_t source,
                              uint16_t destination) {
    const uint8_t *record = game.graphics_wram + NBA_GRAPHICS_QUEUE_RECORDS + tail;
    return record[0] == 1u && record[1] == (uint8_t)source &&
        record[2] == (uint8_t)(source >> 8) && record[3] == 0x7Eu &&
        record[4] == 0x20u && record[5] == 0u &&
        record[6] == (uint8_t)destination &&
        record[7] == (uint8_t)(destination >> 8);
}

/* Host-only caller-probe helper; no native address. Validate the source base
 * published by the production caller, then apply the native actor stride. */
static int require_actor_descriptor(unsigned actor, uint8_t direction,
                                    uint16_t tail) {
    (void)direction;
    uint16_t base = probe_read16(
        (uint16_t)(NBA_PLAYER_JERSEY_CACHE_WRAM + actor * 2u));
    if (base == 0xFFFFu) return 0;
    uint16_t source = (uint16_t)(
        base + actor * NBA_PLAYER_JERSEY_ACTOR_BYTES);
    uint16_t index = (uint16_t)(
        actor + probe_read16(NBA_GRAPHICS_ALLOCATOR_EXTENT));
    uint16_t destination = (uint16_t)(
        (uint16_t)(index << 4) +
        probe_read16(NBA_GRAPHICS_ALLOCATOR_VRAM_BASE) +
        ((index & 0x1000u) != 0u));
    return require_descriptor(tail, source, destination);
}

/* Host-only real renderer-caller probe; no direct native address. It enters
 * Tipoff through NbaGame, consumes AFA2-published buffers, and verifies miss,
 * hit, direction-change, rollover, and directions 1/5's ACC2 guard. */
int main(int argc, char **argv) {
    if (argc != 2 || !nba_game_init(&game, NULL, argv[1])) return 2;
    if (!nba_game_enter_state(&game, NBA_STATE_TIPOFF)) {
        nba_game_shutdown(&game);
        return 3;
    }
    NbaTipoff *tipoff = &game.scene.tipoff;
    if (tipoff->graphics_bus.wram != game.graphics_wram ||
        tipoff->graphics_bus.size != NBA_GRAPHICS_WRAM_BYTES) return 4;
    memcpy(jersey_before, game.graphics_wram + NBA_PLAYER_JERSEY_WRAM_BASE,
           sizeof(jersey_before));
    unsigned nonzero = 0u;
    for (unsigned index = 0; index < sizeof(jersey_before); ++index)
        nonzero += jersey_before[index] != 0u;
    if (nonzero == 0u) return 5;

    static const uint8_t direction[NBA_PLAYER_APPEARANCE_COUNT] = {
        0, 1, 2, 3, 4, 5, 6, 7, 0, 2
    };
    for (unsigned actor = 0; actor < NBA_PLAYER_APPEARANCE_COUNT; ++actor) {
        tipoff->actors[actor].direction = direction[actor];
        tipoff->player_screen_visible[actor] = true;
    }
    tipoff->tip_contact_actor = 0;
    probe_write16(NBA_GRAPHICS_QUEUE_TAIL, 0u);
    memset(game.graphics_wram + NBA_GRAPHICS_QUEUE_RECORDS, 0xA5,
           NBA_GRAPHICS_QUEUE_BYTES);
    memset(game.graphics_wram + NBA_PLAYER_JERSEY_CACHE_WRAM, 0xFF,
           NBA_PLAYER_JERSEY_CACHE_BYTES);

    nba_game_render(&game);
    static const uint8_t appended_actor[8] = {0, 2, 3, 4, 6, 7, 8, 9};
    if (probe_read16(NBA_GRAPHICS_QUEUE_TAIL) != 0x40u) return 6;
    for (unsigned index = 0; index < 8u; ++index)
        if (!require_actor_descriptor(appended_actor[index],
                direction[appended_actor[index]], (uint16_t)(index * 8u)))
            return 7;
    if (probe_read16(NBA_PLAYER_JERSEY_CACHE_WRAM + 2u) != 0xFFFFu ||
        probe_read16(NBA_PLAYER_JERSEY_CACHE_WRAM + 10u) != 0xFFFFu)
        return 8;
    if (memcmp(jersey_before,
            game.graphics_wram + NBA_PLAYER_JERSEY_WRAM_BASE,
            sizeof(jersey_before))) return 9;

    probe_write16(0x0004u, 0xA5A5u);
    nba_game_render(&game);
    if (probe_read16(NBA_GRAPHICS_QUEUE_TAIL) != 0x40u ||
        probe_read16(0x0004u) != 0x01E9u) return 10;

    tipoff->actors[0].direction = 2u;
    nba_game_render(&game);
    if (probe_read16(NBA_GRAPHICS_QUEUE_TAIL) != 0x48u ||
        !require_actor_descriptor(0u, 2u, 0x40u)) return 11;

    probe_write16(NBA_GRAPHICS_QUEUE_TAIL, 0x01F8u);
    probe_write16(NBA_PLAYER_JERSEY_CACHE_WRAM, 0xFFFFu);
    nba_game_render(&game);
    if (probe_read16(NBA_GRAPHICS_QUEUE_TAIL) != 0u ||
        !require_actor_descriptor(0u, 2u, 0x01F8u) ||
        memcmp(jersey_before,
            game.graphics_wram + NBA_PLAYER_JERSEY_WRAM_BASE,
            sizeof(jersey_before))) return 12;

    nba_game_shutdown(&game);
    puts("graphics-jersey-caller-ok");
    return 0;
}
