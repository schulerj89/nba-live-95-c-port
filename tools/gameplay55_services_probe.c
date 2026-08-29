#include "nba_assets.h"
#include "nba_player_lab.h"
#include "nba_renderer.h"
#include "nba_snes_ppu.h"
#include "nba_title_sequence.h"
#include <stdio.h>
#include <string.h>

static uint64_t fnv1a64(const void *bytes, size_t size) {
    const uint8_t *p = (const uint8_t *)bytes;
    uint64_t hash = 1469598103934665603ull;
    for (size_t i = 0; i < size; ++i) {
        hash ^= p[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

static int sprite_resource_probe(const NbaAssetPack *assets) {
    /* `$80:8C00-$A7C5` ultimately publishes indexed tilegroups and resource
     * records as ordered, clipped OAM. Exercise the portable production
     * boundary with a real gameplay torso, including the mirror equation used
     * by `$80:A75E/$80:B452` and both off-screen rejection sides. */
    NbaRomSpriteOamComposition normal = {0}, flipped = {0}, clipped = {0};
    if (!nba_rom_sprite_resource_compose(
            assets, 0x00F0u, 0u, 100, 100, &normal) || normal.count == 0u ||
        !nba_rom_sprite_resource_compose(
            assets, 0x00F0u, 0x4000u, 100, 100, &flipped) ||
        normal.count != flipped.count) return 1;
    for (unsigned i = 0; i < normal.count; ++i) {
        int extent = normal.entries[i].large ? 16 : 8;
        if (normal.entries[i].x + flipped.entries[i].x !=
                200 - (extent - 1) ||
            normal.entries[i].y != flipped.entries[i].y ||
            normal.entries[i].tile != flipped.entries[i].tile ||
            (uint8_t)(normal.entries[i].attribute ^ 0x40u) !=
                flipped.entries[i].attribute) return 2;
    }
    if (!nba_rom_sprite_resource_compose(
            assets, 0x00F0u, 0u, -300, 100, &clipped) || clipped.count != 0u)
        return 3;
    if (!nba_rom_sprite_resource_compose(
            assets, 0x00F0u, 0u, 300, 100, &clipped) || clipped.count != 0u)
        return 4;
    if (!nba_player_animation_self_test(assets)) return 5;
    return 0;
}

static int resource_publication_probe(const NbaAssetPack *assets) {
    uint64_t hashes[29] = {0};
    unsigned distinct = 0u;
    for (unsigned team = 0; team < 29u; ++team) {
        const uint8_t *vram = NULL, *cgram = NULL;
        if (!nba_assets_gameplay_ppu_input(
                assets, (uint8_t)team, &vram, &cgram) || !vram || !cgram)
            return 10 + (int)team;
        hashes[team] = fnv1a64(vram, 0x10000u) ^
                       (fnv1a64(cgram, 0x200u) << 1u);
        bool seen = false;
        for (unsigned prior = 0; prior < team; ++prior)
            if (hashes[prior] == hashes[team]) seen = true;
        if (!seen) ++distinct;
    }
    /* Courts may share generic tiles, but the home palette/logo publication
     * must retain substantially different raw PPU states. */
    if (distinct < 24u) return 40;
    const NbaAssetItem *scratch = nba_assets_get(
        assets, NBA_ASSET_GAMEPLAY_GRAPHICS_SCRATCH);
    const NbaAssetItem *goal = nba_assets_get(
        assets, NBA_ASSET_GAMEPLAY_GOAL_LAYER);
    if (!scratch || !goal || scratch->size != 788u || goal->size != 35352u ||
        !scratch->data || !goal->data ||
        fnv1a64(scratch->data, scratch->size) == 0u ||
        fnv1a64(goal->data, goal->size) == 0u) return 41;
    return 0;
}

static int scene_timing_probe(const NbaAssetPack *assets) {
    NbaRenderer renderer;
    nba_renderer_init(&renderer);
    NbaTitleSequence sequence;
    nba_title_sequence_init(&sequence);
    nba_title_sequence_render(&sequence, assets, &renderer, 0);
    uint64_t frame0 = fnv1a64(renderer.pixels, sizeof(renderer.pixels));
    nba_title_sequence_render(&sequence, assets, &renderer, 820);
    uint64_t frame820 = fnv1a64(renderer.pixels, sizeof(renderer.pixels));
    nba_title_sequence_render(&sequence, assets, &renderer, 965);
    uint64_t frame965 = fnv1a64(renderer.pixels, sizeof(renderer.pixels));
    nba_title_sequence_render(&sequence, assets, &renderer, 0);
    if (frame0 != fnv1a64(renderer.pixels, sizeof(renderer.pixels)) ||
        frame0 == frame820 || frame820 == frame965 || frame0 == frame965)
        return 50;

    nba_title_sequence_init(&sequence);
    nba_title_sequence_snap_complete(&sequence);
    if (sequence.snap_frame != NBA_TITLE_BUILD_COMPLETE_FRAMES) return 51;
    sequence.phase = NBA_TITLE_PHASE_HOLD;
    sequence.hold_frames_left = NBA_TITLE_SNAP_HOLD_FRAMES;
    for (int i = 0; i < NBA_TITLE_SNAP_HOLD_FRAMES; ++i)
        if (nba_title_sequence_advance(&sequence) ||
            sequence.phase != NBA_TITLE_PHASE_HOLD) return 52;
    if (nba_title_sequence_advance(&sequence) ||
        sequence.phase != NBA_TITLE_PHASE_FADE_OUT ||
        sequence.fade_level != 15) return 53;
    for (int level = 14; level > 0; --level) {
        if (nba_title_sequence_advance(&sequence) ||
            sequence.fade_level != level) return 54;
    }
    if (!nba_title_sequence_advance(&sequence) || sequence.fade_level != 0)
        return 55;

    nba_title_sequence_init(&sequence);
    sequence.phase = NBA_TITLE_PHASE_HOLD;
    sequence.hold_frames_left = NBA_TITLE_COMPLETE_HOLD_FRAMES;
    for (int i = 0; i < NBA_TITLE_COMPLETE_HOLD_FRAMES; ++i)
        if (nba_title_sequence_advance(&sequence)) return 56;
    if (nba_title_sequence_advance(&sequence) ||
        sequence.phase != NBA_TITLE_PHASE_FADE_OUT) return 57;
    return 0;
}

int main(int argc, char **argv) {
    NbaAssetPack assets = {0};
    if (argc != 2 || !nba_assets_load(&assets, argv[1])) return 2;
    int sprite = sprite_resource_probe(&assets);
    int resources = resource_publication_probe(&assets);
    int timing = scene_timing_probe(&assets);
    nba_assets_free(&assets);
    printf("GAMEPLAY55_SERVICES sprite=%s resources=%s timing=%s\n",
           sprite == 0 ? "PASS" : "FAIL",
           resources == 0 ? "PASS" : "FAIL",
           timing == 0 ? "PASS" : "FAIL");
    if (sprite || resources || timing)
        fprintf(stderr, "codes sprite=%d resources=%d timing=%d\n",
                sprite, resources, timing);
    return sprite == 0 && resources == 0 && timing == 0 ? 0 : 1;
}
