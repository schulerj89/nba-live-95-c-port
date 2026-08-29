#include "nba_player_lab.h"
#include <stdio.h>

/* Asset/render closure for every descriptor state, direction, appearance
 * variant and normal/tall lower-body table. This does not claim every state
 * is naturally reachable in CPU play; it proves that any state selected by
 * the native animation machinery resolves to packed ROM art and can enter
 * the production four-layer compositor without a missing attachment. */

#define PAIR_SLOTS 16384u
static uint32_t pairs[PAIR_SLOTS];
static unsigned pair_count;
static unsigned upper_resources[0x10000];
static unsigned lower_resources[0x10000];

static int check_pair(const NbaAssetPack *pack, uint16_t upper,
                      uint16_t lower, uint8_t direction) {
    uint32_t key = ((uint32_t)upper << 16) | lower;
    uint32_t stored = key + 1u;
    unsigned slot = (unsigned)((key * 2654435761u) & (PAIR_SLOTS - 1u));
    while (pairs[slot] && pairs[slot] != stored)
        slot = (slot + 1u) & (PAIR_SLOTS - 1u);
    if (pairs[slot] == stored) return 1;
    pairs[slot] = stored;
    ++pair_count;
    upper_resources[upper] = 1u;
    lower_resources[lower] = 1u;

    for (uint8_t side = 0u; side < 2u; ++side) {
        NbaPlayerSpriteDiagnostics diagnostic = {0};
        NbaPlayerSpriteComposition composition = {0};
        if (!nba_player_sprite_diagnose_resources(
                pack, 3u, 0u, side, direction, upper, lower, &diagnostic) ||
            !nba_player_compose_sprite_parts(
                pack, 3u, 0u, side, direction, upper, lower,
                100, 150, &composition) || composition.count < 3u ||
            composition.count > 4u) {
            fprintf(stderr,
                "animation render closure failed u%04X l%04X d%u s%u\n",
                upper, lower, direction, side);
            return 0;
        }
    }
    return 1;
}

static unsigned count_set(const unsigned *set, unsigned count) {
    unsigned result = 0u;
    for (unsigned index = 0u; index < count; ++index)
        result += set[index] != 0u;
    return result;
}

int main(int argc, char **argv) {
    NbaAssetPack pack = {0};
    if (argc != 2 || !nba_assets_load(&pack, argv[1])) return 2;
    unsigned upper_states = 0u;
    unsigned lower_states[2] = {0u, 0u};
    bool upper_seen[NBA_PLAYER_ANIMATION_STATES] = {0};

    for (uint8_t alternate = 0u; alternate < 2u; ++alternate) {
        uint16_t baseline_count = 0u;
        if (!nba_player_animation_frame_count(
                &pack, false, 0u, alternate != 0u, &baseline_count) ||
            baseline_count == 0u) return 3;
        for (uint8_t state = 0u; state < NBA_PLAYER_ANIMATION_STATES; ++state) {
            uint16_t count = 0u;
            if (nba_player_animation_frame_count(
                    &pack, true, state, alternate != 0u, &count) && count) {
                if (!upper_seen[state]) {
                    upper_seen[state] = true;
                    ++upper_states;
                }
                for (uint8_t variant = 0u; variant < 2u; ++variant)
                    for (uint8_t direction = 0u; direction < 8u; ++direction)
                        for (uint32_t tick = 0u; tick < 1024u; ++tick) {
                            uint16_t upper = 0u, lower = 0u;
                            if (!nba_player_animation_resources_for_appearance(
                                    &pack, state, 0u, direction, tick, tick,
                                    alternate != 0u, variant, &upper, &lower) ||
                                !check_pair(&pack, upper, lower, direction))
                                return 10;
                        }
            }
            if (nba_player_animation_frame_count(
                    &pack, false, state, alternate != 0u, &count) && count) {
                ++lower_states[alternate];
                for (uint8_t variant = 0u; variant < 2u; ++variant)
                    for (uint8_t direction = 0u; direction < 8u; ++direction)
                        for (uint32_t tick = 0u; tick < 1024u; ++tick) {
                            uint16_t upper = 0u, lower = 0u;
                            if (!nba_player_animation_resources_for_appearance(
                                    &pack, 0u, state, direction, tick, tick,
                                    alternate != 0u, variant, &upper, &lower) ||
                                !check_pair(&pack, upper, lower, direction))
                                return 11;
                        }
            }
        }
    }

    unsigned upper_count = count_set(upper_resources, 0x10000u);
    unsigned lower_count = count_set(lower_resources, 0x10000u);
    /* Five upper and eighteen lower table slots are native null entries,
     * not missing packed data. Lock the exact non-null descriptor/resource
     * closure so extractor regressions cannot silently reduce it. */
    if (upper_states != 52u || lower_states[0] != 39u ||
        lower_states[1] != 39u || pair_count != 2610u ||
        upper_count != 955u || lower_count != 710u) {
        fprintf(stderr,
            "unexpected closure ustate%u lstate%u/%u pairs%u resources%u/%u\n",
            upper_states, lower_states[0], lower_states[1], pair_count,
            upper_count, lower_count);
        return 12;
    }
    nba_assets_free(&pack);
    printf("ANIMATION RENDER CLOSURE PASS: states upper=%u lower=%u/%u pairs=%u resources=%u/%u\n",
        upper_states, lower_states[0], lower_states[1], pair_count,
        upper_count, lower_count);
    return 0;
}
