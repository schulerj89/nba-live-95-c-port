#include "nba_ea_intro.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Captured-bank closure: `$82:8000-$82:FFFF`; only retained executed
 * positions inside this window are claimed. Its `$82:F000-$FFFF` tail
 * combines the EA Mode-7 scene, transient
 * gameplay graphics scratch and APU/resource handoff helpers. This module
 * owns the visible `$82:F4C4/$82:F56D/$82:F64A` branch results; nba_jump_reach
 * owns the independently replayed three-slot scratch producer and nba_audio
 * owns stamped SPC/DSP publication. Exact EA frame hashes, scratch witnesses,
 * audio PCM fingerprints and the gameplay100 journey protect those portable
 * boundaries without treating hardware waits as additional game behavior. */

/*
 * $82:F56D does not interpolate a conventional sprite scale.  It writes the
 * Mode 7 A/D matrix to $0001, waits a frame, and then adds $000C per frame
 * until the value reaches $0100.  The apparent scale is therefore 256/A.
 */
static int nba_ea_intro_local_frame(float local_t) {
    int frame = (int)floorf(local_t * 60.0f + 0.0001f);
    return frame < 0 ? 0 : frame;
}

static int nba_ea_intro_mode7_matrix(int frame) {
    if (frame <= 1) return NBA_INTRO_MODE7_START;
    if (frame >= NBA_INTRO_ZOOM_FRAMES + 1) return NBA_INTRO_MODE7_UNIT;
    return NBA_INTRO_MODE7_START + NBA_INTRO_MODE7_STEP * (frame - 1);
}

static int nba_ea_intro_mode7_source(int screen_coordinate, int matrix,
                                     int scroll, int center) {
    /* The recomp PPU's Mode 7 path preserves the SNES's low-six-bit
     * truncation before the final 8.8 shift.  Callers provide the hardware
     * scroll; the decoded canvas separately maps source (382,402) to (0,0). */
    int clipped = scroll - center;
    int start = ((matrix * clipped) & ~63) + (center << 8);
    return (start + matrix * screen_coordinate) >> 8;
}

/* $82:F64A advances every BGR555 channel six levels before each flash frame. */
static uint32_t nba_ea_intro_flash_color(uint32_t color, int flash_frame) {
    if (flash_frame < 0) return color;

    /* F4C4 calls F64A twice per wait and F64A advances at most three
     * channel steps.  The observed palette cadence is base,+6,+12,+18,+24,
     * +30,base,+6,+12: saturation reloads the source palette and starts the
     * next highlight sweep instead of remaining solid white. */
    int steps = (flash_frame % 6) * 6;
    if (steps == 0) return color;
    uint32_t a = (color >> 24) & 0xFF;
    int r5 = (int)(((color >> 16) & 0xFF) * 31u / 255u);
    int g5 = (int)(((color >> 8) & 0xFF) * 31u / 255u);
    int b5 = (int)((color & 0xFF) * 31u / 255u);
    r5 = r5 + steps > 31 ? 31 : r5 + steps;
    g5 = g5 + steps > 31 ? 31 : g5 + steps;
    b5 = b5 + steps > 31 ? 31 : b5 + steps;

    uint32_t r = (uint32_t)((r5 << 3) | (r5 >> 2));
    uint32_t g = (uint32_t)((g5 << 3) | (g5 >> 2));
    uint32_t b = (uint32_t)((b5 << 3) | (b5 >> 2));
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static int nba_ea_intro_pixel_visible(uint32_t color) {
    /* Asset extraction marks transparency explicitly.  Authentic dark
     * BGR555 pixels are artwork and must not be discarded by a brightness
     * heuristic; doing so punches holes in E/A edge shading. */
    return (color >> 24) != 0u;
}

static void nba_ea_intro_render_captured_stage(const NbaAssetItem *item,
                                               NbaRenderer *ren,
                                               int start_x, int start_y) {
    if (!item || !item->data || !ren) return;
    const uint32_t *pixels = (const uint32_t *)item->data;
    for (uint32_t r = 0; r < item->height; ++r) {
        int py = start_y + (int)r;
        if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
        for (uint32_t c = 0; c < item->width; ++c) {
            int px = start_x + (int)c;
            uint32_t color = pixels[r * item->width + c];
            if (px >= 0 && px < NBA_SNES_WIDTH &&
                nba_ea_intro_pixel_visible(color)) {
                ren->pixels[py * NBA_SNES_WIDTH + px] = color;
            }
        }
    }
}

/**
 * Offset/Address/Size: 0x0172EA | $82:F2EA | size: 0x80 (128 bytes)
 * Subroutines: $82:94DF (Mode 7 scale init), $82:F4F6 (E tilegroup draw), $82:F56D (22-frame zoom loop)
 * Purpose: Renders Stage 1 "E" zooming in from the foreground (3.5x -> 1.0x) to its exact emblem position.
 */
void nba_ea_intro_render_stage1(const NbaAssetPack *assets, NbaRenderer *ren, float local_t,
                               int start_x, int start_y, uint32_t width, uint32_t height) {
    if (!assets || !ren) return;
    const NbaAssetItem *e_item = nba_assets_get(assets, NBA_ASSET_EA_E_LAYER);
    if (!e_item || !e_item->data ||
        e_item->size < width * height * 4u) return;

    const uint32_t *e_layer = (const uint32_t *)e_item->data;
    int frame = nba_ea_intro_local_frame(local_t);
    /* The PPU register callback observes the next F56D matrix before that
     * matrix appears in the completed picture. Preserve the one-frame
     * presentation delay: motion 3 remains offscreen and motion 4 is the
     * first visible E slice. */
    int matrix_frame = frame > 0 ? frame - 1 : 0;
    int matrix = nba_ea_intro_mode7_matrix(matrix_frame);
    int flash_frame = frame - (NBA_INTRO_ZOOM_FRAMES + 2);

    for (int py = 0; py < NBA_SNES_HEIGHT; py++) {
        int source_y = nba_ea_intro_mode7_source(py, matrix, 401, 512);
        int ty = source_y - 402 - start_y;
        if (ty < 0 || ty >= (int)height) continue;

        for (int px = 0; px < NBA_SNES_WIDTH; px++) {
            int source_x = nba_ea_intro_mode7_source(px, matrix, 384, 512);
            int tx = source_x - 382 - start_x;
            if (tx < 0 || tx >= (int)width) continue;

            uint32_t index = (uint32_t)ty * width + (uint32_t)tx;
            uint32_t source_color = e_layer[index];
            if (nba_ea_intro_pixel_visible(source_color)) {
                uint32_t color = source_color;
                color = nba_ea_intro_flash_color(color, flash_frame);
                ren->pixels[py * NBA_SNES_WIDTH + px] = color;
            }
        }
    }
}

/**
 * Offset/Address/Size: 0x01736A | $82:F36A | size: 0x90 (144 bytes)
 * Subroutines: $82:F512 (A tilegroup draw), $82:F56D (22-frame zoom loop), $82:F4C4 (8-frame flash)
 * Purpose: Keeps the completed E fixed while the new A tilegroup zooms over it.
 */
void nba_ea_intro_render_stage2(const NbaAssetPack *assets, NbaRenderer *ren, float local_t,
                               int start_x, int start_y, uint32_t width, uint32_t height) {
    if (!assets || !ren) return;
    const NbaAssetItem *item2 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE2);
    const NbaAssetItem *e_item = nba_assets_get(assets, NBA_ASSET_EA_E_LAYER);
    const NbaAssetItem *a_item = nba_assets_get(assets, NBA_ASSET_EA_A_LAYER);
    const NbaAssetItem *fixed = nba_assets_get(assets, NBA_ASSET_EA_A_FIXED_SEQUENCE);
    if (!e_item || !e_item->data || !item2 || !item2->data ||
        !a_item || !a_item->data || a_item->size < width * height * 4u ||
        !fixed || !fixed->data || fixed->size < width * height * 4u * 11u) return;

    const uint32_t *e_layer = (const uint32_t *)e_item->data;
    const uint32_t *a_layer = (const uint32_t *)a_item->data;
    int frame = nba_ea_intro_local_frame(local_t);
    if (frame >= NBA_INTRO_ZOOM_FRAMES + 1) {
        /* $82:F4C4 has switched ownership to fixed OAM here. Frames 56-66
         * carry two identity waits, all eight palette writes, and settle. */
        int fixed_frame = frame - (NBA_INTRO_ZOOM_FRAMES + 1);
        if (fixed_frame > 10) fixed_frame = 10;
        NbaAssetItem view = *fixed;
        size_t frame_pixels = (size_t)width * height;
        view.data = (const uint32_t *)fixed->data + frame_pixels * fixed_frame;
        view.size = (uint32_t)(frame_pixels * sizeof(uint32_t));
        nba_ea_intro_render_captured_stage(&view, ren, start_x, start_y);
        return;
    }
    /* Motion 33 installs A's matrix/tiles but its completed picture still
     * contains only the settled E. The A presentation begins on motion 34. */
    int a_matrix_frame = frame > 0 ? frame - 1 : 0;
    int a_matrix = nba_ea_intro_mode7_matrix(a_matrix_frame);

    for (int py = 0; py < NBA_SNES_HEIGHT; py++) {
        for (int px = 0; px < NBA_SNES_WIDTH; px++) {
            /* The settled E remains at the identity transform throughout A's
             * entrance; $82:F512 never repositions or rebuilds it. */
            int e_source_y = nba_ea_intro_mode7_source(py, NBA_INTRO_MODE7_UNIT, 401, 512);
            int e_source_x = nba_ea_intro_mode7_source(px, NBA_INTRO_MODE7_UNIT, 384, 512);
            int e_ty = e_source_y - 402 - start_y;
            int e_tx = e_source_x - 382 - start_x;
            if (e_tx >= 0 && e_tx < (int)width && e_ty >= 0 && e_ty < (int)height) {
                uint32_t e_color = e_layer[(uint32_t)e_ty * width + (uint32_t)e_tx];
                if (nba_ea_intro_pixel_visible(e_color)) {
                    ren->pixels[py * NBA_SNES_WIDTH + px] = e_color;
                }
            }

            if (frame == 0) continue;
            int source_y = nba_ea_intro_mode7_source(py, a_matrix, 401, 512);
            int source_x = nba_ea_intro_mode7_source(px, a_matrix, 384, 512);
            int ty = source_y - 402 - start_y;
            int tx = source_x - 382 - start_x;
            if (tx < 0 || tx >= (int)width || ty < 0 || ty >= (int)height) continue;

            uint32_t index = (uint32_t)ty * width + (uint32_t)tx;
            uint32_t a_color = a_layer[index];
            if (nba_ea_intro_pixel_visible(a_color)) {
                ren->pixels[py * NBA_SNES_WIDTH + px] = a_color;
            }
        }
    }
}

/**
 * Offset/Address/Size: 0x017408 | $82:F408 | size: 0x90 (144 bytes)
 * Subroutines: $82:F52E (SPORTS banner draw), $82:F56D (22-frame zoom loop), $82:F4C4 (8-frame flash)
 * Purpose: Keeps the completed EA fixed while the SPORTS tilegroup zooms over it.
 */
void nba_ea_intro_render_stage3(const NbaAssetPack *assets, NbaRenderer *ren, float local_t,
                               int start_x, int start_y, uint32_t width, uint32_t height) {
    if (!assets || !ren) return;
    const NbaAssetItem *item3 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE3);
    const NbaAssetItem *sports_item = nba_assets_get(assets, NBA_ASSET_EA_SPORTS_LAYER);
    const NbaAssetItem *fixed = nba_assets_get(assets, NBA_ASSET_EA_A_FIXED_SEQUENCE);
    size_t frame_pixels = (size_t)width * height;
    if (!item3 || !item3->data || !sports_item || !sports_item->data ||
        sports_item->size < frame_pixels * sizeof(uint32_t) ||
        !fixed || !fixed->data ||
        fixed->size < frame_pixels * sizeof(uint32_t) * 11u) return;

    /* F408/F52E do not reconstruct EA. The fixed OAM group produced by the
     * preceding F4C4 path remains resident, so carry its final captured frame
     * forward verbatim. NBA_ASSET_EA_LOGO_STAGE2 predates that ownership
     * switch and causes a visible palette/art jump at the stage boundary. */
    const uint32_t *settled_ea = (const uint32_t *)fixed->data + frame_pixels * 10u;
    const uint32_t *sports_layer = (const uint32_t *)sports_item->data;
    int frame = nba_ea_intro_local_frame(local_t);
    if (frame >= NBA_INTRO_STAGE3_FRAMES) {
        nba_ea_intro_render_captured_stage(item3, ren, start_x, start_y);
        return;
    }
    /* SPORTS has one preparation frame at motion 67 before F56D begins.
     * The register trace observes the next matrix during the frame, while the
     * completed picture presents that update on the following frame.  Delay
     * the rendered matrix once more so motion 73 remains offscreen and motion
     * 74 is the first visible SPORTS slice, matching the Mesen frame oracle. */
    int matrix_frame = frame > 1 ? frame - 2 : 0;
    int matrix = nba_ea_intro_mode7_matrix(matrix_frame);
    int flash_frame = frame - (NBA_INTRO_ZOOM_FRAMES + 2);

    /* Draw the settled EA first; the incoming SPORTS layer is composited over it. */
    for (uint32_t r = 0; r < height; r++) {
        int py = start_y + (int)r;
        if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
        for (uint32_t c = 0; c < width; c++) {
            uint32_t index = r * width + c;
            uint32_t source_color = settled_ea[index];
            if (nba_ea_intro_pixel_visible(source_color)) {
                int px = start_x + (int)c;
                if (px >= 0 && px < NBA_SNES_WIDTH) {
                    /* $82:F43A selects SPORTS' palette block for F4C4, not settled EA. */
                    ren->pixels[py * NBA_SNES_WIDTH + px] = source_color;
                }
            }
        }
    }

    /* $82:F408 prepares the SPORTS group before $82:F56D's first visible
     * matrix update; the entry frame still presents the settled EA image. */
    if (frame == 0) return;

    /* `$82:F52E` draws the `$82:F6D8` ROM tilegroup twice, at tile rows $38
     * and $3D.  This indexed asset is that combined hardware layer.  SPORTS
     * is independent of the settled OAM EA; transparent cells never erase or
     * manufacture EA pixels. */
    for (int py = 0; py < NBA_SNES_HEIGHT; py++) {
        int source_y = nba_ea_intro_mode7_source(py, matrix, 401, 512);
        int ty = source_y - 402 - start_y;
        if (ty < 0 || ty >= (int)height) continue;

        for (int px = 0; px < NBA_SNES_WIDTH; px++) {
            int source_x = nba_ea_intro_mode7_source(px, matrix, 384, 512);
            int tx = source_x - 382 - start_x;
            if (tx < 0 || tx >= (int)width) continue;

            uint32_t index = (uint32_t)ty * width + (uint32_t)tx;
            uint32_t source_color = sports_layer[index];
            if (nba_ea_intro_pixel_visible(source_color)) {
                uint32_t color = nba_ea_intro_flash_color(source_color, flash_frame);
                ren->pixels[py * NBA_SNES_WIDTH + px] = color;
            }
        }
    }
}

/**
 * Offset/Address/Size: 0x017469 | $82:F469 | size: 0x40 (64 bytes)
 * Purpose: Renders Stage 4 full completed logo and bottom "ELECTRONIC ARTS" typography banner.
 */
void nba_ea_intro_render_stage4(const NbaAssetPack *assets, NbaRenderer *ren,
                               int local_frame, int start_x, int start_y,
                               uint32_t width, uint32_t height) {
    if (!assets || !ren) return;
    const NbaAssetItem *item4 = nba_assets_get(
        assets, local_frame >= 31 ? NBA_ASSET_EA_LOGO_FINAL :
                                    NBA_ASSET_EA_LOGO_STAGE4);
    if (!item4 || !item4->data) return;

    const uint32_t *p4 = (const uint32_t *)item4->data;
    for (uint32_t r = 0; r < height; r++) {
        int py = start_y + (int)r;
        if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
        for (uint32_t c = 0; c < width; c++) {
            uint32_t color = p4[r * width + c];
            if (nba_ea_intro_pixel_visible(color)) {
                int px = start_x + (int)c;
                if (px >= 0 && px < NBA_SNES_WIDTH) {
                    ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                }
            }
        }
    }
}

/**
 * Offset/Address/Size: 0x01715C | $82:F15C | size: 0x4E0 (1248 bytes)
 * Subroutines: $82:94DF (Mode 7 scale init), $82:962D (Matrix A/D update), $82:F56D (22-frame zoom loop), $82:F4C4 (8-frame flash)
 * Purpose: Top-level multi-stage EA Sports intro animation dispatcher across Stages 1-4 with specular flash.
 */
void nba_ea_intro_render(const NbaAssetPack *assets, NbaRenderer *ren, float timer) {
    if (!assets || !ren) return;
    nba_renderer_clear(ren, 0xFF000000); /* Solid Black */

    const NbaAssetItem *item4 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE4);
    const NbaAssetItem *item1 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE1);
    const NbaAssetItem *base_item = item4 ? item4 : (item1 ? item1 : NULL);
    if (!base_item || !base_item->data) return;

    int start_x, start_y;
    if (base_item->flags != 0) {
        start_x = (int)((base_item->flags >> 16) & 0xFFFF);
        start_y = (int)(base_item->flags & 0xFFFF);
    } else {
        start_x = (NBA_SNES_WIDTH - (int)base_item->width) / 2;
        start_y = (NBA_SNES_HEIGHT - (int)base_item->height) / 2;
    }

    uint32_t width = base_item->width;
    uint32_t height = base_item->height;

    int intro_frame = nba_ea_intro_local_frame(timer);
    if (intro_frame < NBA_INTRO_STAGE1_FRAMES) {
        /* Stage 1: E, Mesen motion frames 0-32. */
        nba_ea_intro_render_stage1(assets, ren, (float)intro_frame / 60.0f,
                                   start_x, start_y, width, height);
    } else if (intro_frame < NBA_INTRO_STAGE1_FRAMES + NBA_INTRO_STAGE2_FRAMES) {
        /* Stage 2: A, Mesen motion frames 33-66. */
        int local_frame = intro_frame - NBA_INTRO_STAGE1_FRAMES;
        nba_ea_intro_render_stage2(assets, ren, (float)local_frame / 60.0f,
                                   start_x, start_y, width, height);
    } else if (intro_frame < NBA_INTRO_STAGE1_FRAMES + NBA_INTRO_STAGE2_FRAMES +
                             NBA_INTRO_STAGE3_FRAMES) {
        /* Stage 3: SPORTS, Mesen motion frames 67-99. */
        int local_frame = intro_frame - NBA_INTRO_STAGE1_FRAMES - NBA_INTRO_STAGE2_FRAMES;
        nba_ea_intro_render_stage3(assets, ren, (float)local_frame / 60.0f,
                                   start_x, start_y, width, height);
    } else {
        /* Stage 4: completed logo hold, Mesen motion frame 100 onward. */
        int local_frame = intro_frame - NBA_INTRO_STAGE1_FRAMES -
                          NBA_INTRO_STAGE2_FRAMES - NBA_INTRO_STAGE3_FRAMES;
        nba_ea_intro_render_stage4(assets, ren, local_frame,
                                   start_x, start_y, width, height);
    }
}
