#include "nba_game.h"
#include "nba_player_lab.h"
#include <stdio.h>
#include <string.h>

static uint8_t direct_wram[NBA_GRAPHICS_WRAM_BYTES];
static uint8_t reject_wram[NBA_GRAPHICS_WRAM_BYTES];
static NbaGame game;

/* Host-only probe helper; no direct native address. Emit the appearance words
 * and canonical jersey/cache bytes owned by `$87:AFA2-$B058`. */
static void emit_result(const NbaPlayerAppearanceSetup *setup,
                        const uint8_t *wram) {
    printf("APPEARANCE %06x", (unsigned)setup->upload_address);
    for (unsigned actor = 0; actor < NBA_PLAYER_APPEARANCE_COUNT; ++actor) {
        const NbaPlayerAppearance *appearance = &setup->players[actor];
        printf(" %04x %04x %04x %04x %04x",
               appearance->palette_offset, appearance->alternate_lower,
               appearance->upper_variant, appearance->head_resource,
               appearance->dirty);
    }
    puts("");
    fputs("JERSEYS ", stdout);
    for (unsigned offset = 0; offset < NBA_PLAYER_JERSEY_WRAM_BYTES; ++offset)
        printf("%02x", wram[NBA_PLAYER_JERSEY_WRAM_BASE + offset]);
    puts("");
    fputs("CACHE ", stdout);
    for (unsigned offset = 0; offset < NBA_PLAYER_JERSEY_CACHE_BYTES; ++offset)
        printf("%02x", wram[NBA_PLAYER_JERSEY_CACHE_WRAM + offset]);
    puts("");
    printf("UPLOAD %02x%02x%02x\n",
           wram[NBA_PLAYER_APPEARANCE_UPLOAD_WRAM],
           wram[NBA_PLAYER_APPEARANCE_UPLOAD_WRAM + 1u],
           wram[NBA_PLAYER_APPEARANCE_UPLOAD_WRAM + 2u]);
}

/* Host-only probe helper; no direct native address. Rejecting an invalid
 * active roster must leave both the caller output and canonical WRAM intact. */
static int reject_invalid(const NbaAssetPack *assets,
                          const uint8_t teams[NBA_PLAYER_APPEARANCE_COUNT],
                          const uint8_t rosters[NBA_PLAYER_APPEARANCE_COUNT]) {
    uint8_t invalid_roster[NBA_PLAYER_APPEARANCE_COUNT];
    memcpy(invalid_roster, rosters, sizeof(invalid_roster));
    invalid_roster[9] = NBA_PLAYER_ROSTER_SIZE;
    memset(reject_wram, 0x5A, sizeof(reject_wram));
    NbaGraphicsBus bus = {reject_wram, sizeof(reject_wram)};
    NbaPlayerAppearanceSetup output;
    memset(&output, 0xA5, sizeof(output));
    NbaPlayerAppearanceSetup before = output;
    if (nba_player_publish_active_appearance(
            assets, &bus, teams, invalid_roster, &output) ||
        memcmp(&output, &before, sizeof(output))) return 20;
    for (size_t offset = 0; offset < sizeof(reject_wram); ++offset)
        if (reject_wram[offset] != 0x5Au) return 21;
    bus.size = NBA_GRAPHICS_WRAM_BYTES - 1u;
    if (nba_player_publish_active_appearance(
            assets, &bus, teams, rosters, &output) ||
        memcmp(&output, &before, sizeof(output))) return 22;
    for (size_t offset = 0; offset < sizeof(reject_wram); ++offset)
        if (reject_wram[offset] != 0x5Au) return 23;
    return 0;
}

/* Host-only direct probe; no direct native address. Replay the production
 * implementation of `$87:AFA2-$B058` over poisoned canonical WRAM. */
static int run_direct(const NbaAssetPack *assets,
                      const uint8_t teams[NBA_PLAYER_APPEARANCE_COUNT],
                      const uint8_t rosters[NBA_PLAYER_APPEARANCE_COUNT]) {
    memset(direct_wram, 0xA5, sizeof(direct_wram));
    NbaGraphicsBus bus = {direct_wram, sizeof(direct_wram)};
    NbaPlayerAppearanceSetup setup;
    if (!nba_player_publish_active_appearance(
            assets, &bus, teams, rosters, &setup)) return 10;
    for (size_t offset = 0; offset < sizeof(direct_wram); ++offset) {
        bool owned = offset >= NBA_PLAYER_JERSEY_WRAM_BASE &&
                     offset < NBA_PLAYER_JERSEY_CACHE_WRAM +
                                  NBA_PLAYER_JERSEY_CACHE_BYTES;
        owned = owned || (offset >= NBA_PLAYER_APPEARANCE_UPLOAD_WRAM &&
                          offset < NBA_PLAYER_APPEARANCE_UPLOAD_WRAM + 3u);
        if (!owned && direct_wram[offset] != 0xA5u) return 11;
    }
    int rejected = reject_invalid(assets, teams, rosters);
    if (rejected) return rejected;
    emit_result(&setup, direct_wram);
    return 0;
}

/* Host-only caller probe; no direct native address. Exercise the normal game
 * Tipoff entry that binds WRAM before invoking `$87:AFA2-$B058`. */
static int run_caller(const char *pack_path,
                      const uint8_t teams[NBA_PLAYER_APPEARANCE_COUNT],
                      const uint8_t rosters[NBA_PLAYER_APPEARANCE_COUNT]) {
    for (unsigned actor = 1; actor < 5u; ++actor)
        if (teams[actor] != teams[0]) return 30;
    for (unsigned actor = 6; actor < 10u; ++actor)
        if (teams[actor] != teams[5]) return 31;
    if (!nba_game_init(&game, NULL, pack_path)) return 32;
    game.session.right_team = teams[0];
    game.session.left_team = teams[5];
    for (unsigned actor = 0; actor < NBA_PLAYER_APPEARANCE_COUNT; ++actor)
        game.session.match.active_lineup[actor / 5u][actor % 5u] = rosters[actor];
    if (!nba_game_enter_state(&game, NBA_STATE_TIPOFF)) {
        nba_game_shutdown(&game);
        return 33;
    }
    NbaPlayerAppearanceSetup setup = {0};
    setup.upload_address = 0x80800Cu;
    for (unsigned actor = 0; actor < NBA_PLAYER_APPEARANCE_COUNT; ++actor) {
        const NbaTipoffActor *state = &game.scene.tipoff.actors[actor];
        NbaPlayerAppearance *appearance = &setup.players[actor];
        if (state->roster_slot != rosters[actor]) {
            nba_game_shutdown(&game);
            return 34;
        }
        appearance->palette_offset = state->player_palette_offset_raw_ac;
        appearance->alternate_lower = state->free_throw_launch_half_raw_a8;
        appearance->upper_variant = state->animation_variant_raw_6c;
        appearance->head_resource = state->head_resource_base_raw_2e;
        appearance->dirty = (uint16_t)(
            game.graphics_wram[NBA_PLAYER_JERSEY_CACHE_WRAM + actor * 2u] |
            ((uint16_t)game.graphics_wram[
                NBA_PLAYER_JERSEY_CACHE_WRAM + actor * 2u + 1u] << 8));
    }
    if (game.scene.tipoff.free_throw_upload_raw_180b != 0x800Cu ||
        game.scene.tipoff.free_throw_upload_raw_180c != 0x8080u) {
        nba_game_shutdown(&game);
        return 35;
    }
    emit_result(&setup, game.graphics_wram);
    nba_game_shutdown(&game);
    return 0;
}

/* Host-only publication probe entry; no direct native address. */
int main(int argc, char **argv) {
    if (argc != 3 || (strcmp(argv[2], "--direct") &&
                      strcmp(argv[2], "--caller"))) return 2;
    uint8_t teams[NBA_PLAYER_APPEARANCE_COUNT];
    uint8_t rosters[NBA_PLAYER_APPEARANCE_COUNT];
    for (unsigned actor = 0; actor < NBA_PLAYER_APPEARANCE_COUNT; ++actor) {
        unsigned team, roster;
        if (scanf_s("%x %x", &team, &roster) != 2 || team > 0xFFu ||
            roster > 0xFFu) return 3;
        teams[actor] = (uint8_t)team;
        rosters[actor] = (uint8_t)roster;
    }
    if (!strcmp(argv[2], "--caller"))
        return run_caller(argv[1], teams, rosters);
    NbaAssetPack assets = {0};
    if (!nba_assets_load(&assets, argv[1])) return 4;
    int result = run_direct(&assets, teams, rosters);
    nba_assets_free(&assets);
    return result;
}
