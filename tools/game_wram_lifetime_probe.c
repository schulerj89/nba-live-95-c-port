#include "nba_game.h"
#include "nba_graphics_allocator.h"
#include <stdio.h>
#include <string.h>

static NbaGame game;
static NbaGame failed_game;
static NbaTipoff standalone_tipoff;

/* Host-only lifecycle probe; no direct native address. It exercises the real
 * game initializer and scene-entry path around the canonical WRAM owner. */
static int require_condition(int condition, const char *label, int code) {
    if (condition) return 0;
    fprintf(stderr, "game WRAM lifetime check failed: %s\n", label);
    return code;
}

/* Host-only alias check; no direct native address. Views must borrow exactly
 * the allocation owned by NbaGame and retain the full SNES WRAM span. */
static int require_game_views(const NbaGame *value, int tipoff_bound) {
    int code;
    code = require_condition(value->graphics_wram != NULL,
                             "owner allocation", 20);
    if (code) return code;
    code = require_condition(
        value->graphics_bus.wram == value->graphics_wram &&
        value->graphics_bus.size == NBA_GRAPHICS_WRAM_BYTES,
        "game view identity", 21);
    if (code) return code;
    code = require_condition(
        value->renderer.graphics_bus.wram == value->graphics_wram &&
        value->renderer.graphics_bus.size == NBA_GRAPHICS_WRAM_BYTES,
        "renderer view identity", 22);
    if (code) return code;
    if (tipoff_bound)
        return require_condition(
            value->scene.tipoff.graphics_bus.wram == value->graphics_wram &&
            value->scene.tipoff.graphics_bus.size == NBA_GRAPHICS_WRAM_BYTES,
            "Tipoff view identity", 23);
    return 0;
}

/* Host-only persistence check; no direct native address. These addresses span
 * queue cursors/records, `$012C`, and upper WRAM without asserting behavior
 * for any untranslated native producer. */
static int require_sentinels(const NbaGame *value) {
    static const size_t address[] = {
        NBA_GRAPHICS_QUEUE_HEAD, NBA_GRAPHICS_QUEUE_TAIL,
        NBA_GRAPHICS_QUEUE_RECORDS, NBA_GRAPHICS_RECORD5_WORD,
        NBA_GRAPHICS_RECORD5_WORD + 1u, NBA_GRAPHICS_WRAM_BYTES - 2u,
        NBA_GRAPHICS_WRAM_BYTES - 1u
    };
    static const uint8_t expected[] = {
        0x18u, 0x20u, 0x5au, 0x34u, 0x12u, 0xa5u, 0x6cu
    };
    for (size_t index = 0; index < sizeof(address) / sizeof(address[0]); ++index)
        if (value->graphics_wram[address[index]] != expected[index]) return 30;
    return 0;
}

/* Host-only production caller check; no direct native address. It verifies
 * the bounded `$85:8B6C-$8B75` allocator state after real Tipoff entry while
 * retaining bytes outside `$80:AB7E-$ACC1` functional footprints. */
static int require_allocator_state(const NbaGame *value) {
    const uint8_t *wram = value->graphics_wram;
    uint16_t word = 0u;
#define REQUIRE_WORD(address, expected) \
    do { \
        if (!nba_graphics_bus_read16(&value->graphics_bus, (address), &word) || \
            word != (expected)) return 60; \
    } while (0)
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_VRAM_BASE, 0x6000u);
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_X, 0u);
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_EXTENT, 0x01e0u);
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_COUNT, 0u);
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_STRIDE, 2u);
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_BLOCKS, 0x0080u);
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_FILL_HEAD, 0x2000u);
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_FILL_TAIL, 0x2420u);
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_SAVED_TAIL, 0x2000u);
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_ACTIVE_BASE, 0x2220u);
    REQUIRE_WORD(NBA_GRAPHICS_ALLOCATOR_PENDING_BASE, 0x2000u);
#undef REQUIRE_WORD
    if (wram[NBA_GRAPHICS_ALLOCATOR_READY] != 1u ||
        wram[NBA_GRAPHICS_ALLOCATOR_READY + 1u] != 0x45u ||
        wram[0x263fu] != 0x31u || wram[0x2e72u] != 0x32u ||
        wram[0x2002u] != 0x41u || wram[0x2003u] != 0x42u ||
        wram[0x21feu] != 0x43u || wram[0x21ffu] != 0x44u ||
        wram[0x3271u] != 8u || wram[0x32e9u] != 0u ||
        wram[0x32eau] != 0xffu || wram[0x32f2u] != 0u ||
        wram[0x3362u] != 0x70u || wram[0x3363u] != 0x20u ||
        wram[0x3383u] != 0u || wram[0x33c4u] != 0xffu ||
        wram[0x33e4u] != 0u) return 61;
    for (size_t address = 0x2640u; address <= 0x2e71u; ++address)
        if (wram[address] != 0xffu) return 62;
    for (size_t address = 0x2000u; address < 0x2200u; address += 4u)
        if (wram[address] != 0u || wram[address + 1u] != 0xe1u) return 63;
    return 0;
}

/* Host-only transition driver; no direct native address. It uses the real
 * scene entry and checks that clearing the scene union cannot clear WRAM. */
static int enter_and_check(NbaGame *value, NbaGameState state) {
    uint8_t *owner_before = value->graphics_wram;
    if (!nba_game_enter_state(value, state)) return 40 + (int)state;
    if (value->graphics_wram != owner_before) return 39;
    int code = require_game_views(value, state == NBA_STATE_TIPOFF);
    return code ? code : require_sentinels(value);
}

/* Host-only production-lifecycle probe entry; no direct native address. */
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    if (!nba_game_init(&game, NULL, argv[1])) return 3;
    int code = require_game_views(&game, 0);
    if (code) return code;
    if (!nba_renderer_bind_graphics_bus(&game.renderer,
                                        &game.renderer.graphics_bus)) return 13;
    if ((code = require_game_views(&game, 0)) != 0) return code;
    for (size_t address = 0; address < NBA_GRAPHICS_WRAM_BYTES; ++address)
        if (game.graphics_wram[address] != 0u) return 4;

    memset(&standalone_tipoff, 0xa5, sizeof(standalone_tipoff));
    if (!nba_tipoff_init(&standalone_tipoff, &game.assets, &game.session))
        return 5;
    if (standalone_tipoff.graphics_bus.wram != NULL ||
        standalone_tipoff.graphics_bus.size != 0u) return 6;

    game.graphics_wram[NBA_GRAPHICS_ALLOCATOR_VRAM_BASE] = 0x9au;
    if (!nba_tipoff_bind_graphics_bus(&standalone_tipoff,
                                      &game.graphics_bus) ||
        game.graphics_wram[NBA_GRAPHICS_ALLOCATOR_VRAM_BASE] != 0x9au)
        return 16;
    (void)nba_tipoff_bind_graphics_bus(&standalone_tipoff, NULL);

    game.graphics_bus.wram[NBA_GRAPHICS_QUEUE_HEAD] = 0x18u;
    game.graphics_wram[NBA_GRAPHICS_QUEUE_TAIL] = 0x20u;
    game.renderer.graphics_bus.wram[NBA_GRAPHICS_QUEUE_RECORDS] = 0x5au;
    game.graphics_bus.wram[NBA_GRAPHICS_RECORD5_WORD] = 0x34u;
    game.renderer.graphics_bus.wram[NBA_GRAPHICS_RECORD5_WORD + 1u] = 0x12u;
    game.graphics_wram[NBA_GRAPHICS_WRAM_BYTES - 2u] = 0xa5u;
    game.renderer.graphics_bus.wram[NBA_GRAPHICS_WRAM_BYTES - 1u] = 0x6cu;
    game.graphics_wram[NBA_GRAPHICS_ALLOCATOR_READY + 1u] = 0x45u;
    game.graphics_wram[0x263fu] = 0x31u;
    game.graphics_wram[0x2e72u] = 0x32u;
    game.graphics_wram[0x2002u] = 0x41u;
    game.graphics_wram[0x2003u] = 0x42u;
    game.graphics_wram[0x21feu] = 0x43u;
    game.graphics_wram[0x21ffu] = 0x44u;
    uint16_t receiver_word = 0u;
    if (!nba_graphics_bus_receiver_word(&game.renderer.graphics_bus,
                                        &receiver_word) ||
        receiver_word != 0x1234u) return 7;

    static const NbaGameState route[] = {
        NBA_STATE_NBA_LEGAL_NOTICE, NBA_STATE_EA_INTRO,
        NBA_STATE_TITLE_SEQUENCE, NBA_STATE_GAME_SETUP
    };
    for (size_t index = 0; index < sizeof(route) / sizeof(route[0]); ++index) {
        code = enter_and_check(&game, route[index]);
        if (code) return code;
    }
    nba_session_begin_match(&game.session);
    if ((code = require_sentinels(&game)) != 0) return code;
    static const NbaGameState match_route[] = {
        NBA_STATE_TEAM_SELECT, NBA_STATE_PLAYER_SETUP,
        NBA_STATE_PLAYER_INTRO, NBA_STATE_TIPOFF,
        NBA_STATE_POSTGAME, NBA_STATE_TIPOFF
    };
    unsigned tipoff_count = 0u;
    for (size_t index = 0;
         index < sizeof(match_route) / sizeof(match_route[0]); ++index) {
        code = enter_and_check(&game, match_route[index]);
        if (code) return code;
        if (match_route[index] == NBA_STATE_TIPOFF) {
            ++tipoff_count;
            if ((code = require_allocator_state(&game)) != 0) return code;
            if (!nba_tipoff_bind_graphics_bus(
                    &game.scene.tipoff,
                    &game.scene.tipoff.graphics_bus)) return 14;
            if ((code = require_game_views(&game, 1)) != 0) return code;
            uint16_t upper_word = 0u;
            if (!nba_graphics_bus_read16(&game.scene.tipoff.graphics_bus,
                    NBA_GRAPHICS_WRAM_BYTES - 2u, &upper_word) ||
                upper_word != 0x6ca5u ||
                nba_graphics_bus_read16(&game.scene.tipoff.graphics_bus,
                    NBA_GRAPHICS_WRAM_BYTES - 1u, &upper_word)) return 15;
            if (tipoff_count == 1u) {
                game.graphics_wram[NBA_GRAPHICS_ALLOCATOR_VRAM_BASE] = 0x7cu;
                game.graphics_wram[NBA_GRAPHICS_ALLOCATOR_VRAM_BASE + 1u] = 0x6du;
                nba_game_tick(&game, 1.0f / 60.0f);
                if (game.graphics_wram[NBA_GRAPHICS_ALLOCATOR_VRAM_BASE] != 0x7cu ||
                    game.graphics_wram[NBA_GRAPHICS_ALLOCATOR_VRAM_BASE + 1u] != 0x6du)
                    return 17;
            }
        } else if (match_route[index] == NBA_STATE_POSTGAME) {
            if (game.graphics_wram[NBA_GRAPHICS_ALLOCATOR_VRAM_BASE] != 0x7cu ||
                game.graphics_wram[NBA_GRAPHICS_ALLOCATOR_VRAM_BASE + 1u] != 0x6du ||
                game.graphics_wram[0x2640u] != 0xffu ||
                game.graphics_wram[0x2000u] != 0u ||
                game.graphics_wram[0x2001u] != 0xe1u) return 18;
            game.graphics_wram[0x2640u] = 0x6du;
            game.graphics_wram[0x2000u] = 0x2cu;
        }
    }

    nba_game_shutdown(&game);
    if (game.graphics_wram != NULL || game.graphics_bus.wram != NULL ||
        game.renderer.graphics_bus.wram != NULL ||
        game.scene.tipoff.graphics_bus.wram != NULL) return 8;

    if (!nba_game_init(&game, NULL, argv[1])) return 9;
    if ((code = require_game_views(&game, 0)) != 0) return code;
    if (game.graphics_wram[NBA_GRAPHICS_QUEUE_HEAD] != 0u ||
        game.graphics_wram[NBA_GRAPHICS_QUEUE_TAIL] != 0u ||
        game.graphics_wram[NBA_GRAPHICS_QUEUE_RECORDS] != 0u ||
        game.graphics_wram[NBA_GRAPHICS_RECORD5_WORD] != 0u ||
        game.graphics_wram[NBA_GRAPHICS_WRAM_BYTES - 1u] != 0u) return 10;
    nba_game_shutdown(&game);

    if (nba_game_init(&failed_game, NULL,
                      "missing-game-wram-lifetime-pack.pak")) return 11;
    if (failed_game.graphics_wram != NULL ||
        failed_game.graphics_bus.wram != NULL ||
        failed_game.renderer.graphics_bus.wram != NULL) return 12;
    nba_game_shutdown(&failed_game);
    puts("game-wram-lifetime-ok");
    return 0;
}
