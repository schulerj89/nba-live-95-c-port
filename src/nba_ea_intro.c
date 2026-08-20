#include "nba_ea_intro.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/*
 * $82:F56D does not interpolate a conventional sprite scale.  It writes the
 * Mode 7 A/D matrix to $0001, waits a frame, and then adds $000C per frame
 * until the value reaches $0100.  The apparent scale is therefore 256/A.
 */
static int nba_ea_intro_local_frame(float local_t) {
    int frame = (int)floorf(local_t * 60.0f + 0.0001f);
    return frame < 0 ? 0 : frame;
}

static float nba_ea_intro_mode7_scale(int frame) {
    /* $82:F56D presents $0001 for two end-of-frame captures before the
     * first $000C increment.  Mesen then records $000D..$00FD and finally
     * $0100.  Keeping that initial duplicate is important: without it every
     * incoming letter is one zoom step too small. */
    if (frame <= 1) return (float)NBA_INTRO_MODE7_UNIT /
                           (float)NBA_INTRO_MODE7_START;
    if (frame >= NBA_INTRO_ZOOM_FRAMES + 1) return 1.0f;
    int matrix = NBA_INTRO_MODE7_START + NBA_INTRO_MODE7_STEP * (frame - 1);
    return (float)NBA_INTRO_MODE7_UNIT / (float)matrix;
}

/* $82:F64A advances every BGR555 channel six levels before each flash frame. */
static uint32_t nba_ea_intro_flash_color(uint32_t color, int flash_frame) {
    if (flash_frame < 0 || flash_frame >= NBA_INTRO_FLASH_FRAMES) return color;

    int steps = (flash_frame + 1) * 6;
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

/* Captured stage images contain a few near-black antialias/background pixels. */
static int nba_ea_intro_pixel_visible(uint32_t color) {
    uint32_t r = (color >> 16) & 0xFF;
    uint32_t g = (color >> 8) & 0xFF;
    uint32_t b = color & 0xFF;
    return r > 24 || g > 24 || b > 24;
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
    const NbaAssetItem *item1 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE1);
    const NbaAssetItem *e_item = nba_assets_get(assets, NBA_ASSET_EA_E_LAYER);
    if (!item1 || !item1->data || !e_item || !e_item->data ||
        e_item->size < width * height * 4u) return;

    const uint32_t *e_layer = (const uint32_t *)e_item->data;
    int frame = nba_ea_intro_local_frame(local_t);
    if (frame >= NBA_INTRO_ZOOM_FRAMES + 1 + NBA_INTRO_FLASH_FRAMES) {
        nba_ea_intro_render_captured_stage(item1, ren, start_x, start_y);
        return;
    }
    float scale = nba_ea_intro_mode7_scale(frame);
    int flash_frame = frame - (NBA_INTRO_ZOOM_FRAMES + 1);

    /* M7X/M7Y are fixed at $0200: every component shares the screen center. */
    float pcx = (float)NBA_SNES_WIDTH * 0.5f;
    float pcy = (float)NBA_SNES_HEIGHT * 0.5f;
    float inv_scale = 1.0f / scale;

    for (int py = 0; py < NBA_SNES_HEIGHT; py++) {
        int ty = (int)floorf(pcy - 2.0f + (float)(py - pcy) * inv_scale -
                             (float)start_y + 0.5f);
        if (ty < 0 || ty >= (int)height) continue;

        for (int px = 0; px < NBA_SNES_WIDTH; px++) {
            int tx = (int)floorf(pcx + 2.0f + (float)(px - pcx) * inv_scale -
                                 (float)start_x + 0.5f);
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
    const NbaAssetItem *item1 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE1);
    const NbaAssetItem *a_item = nba_assets_get(assets, NBA_ASSET_EA_A_LAYER);
    if (!item1 || !item1->data || !item2 || !item2->data ||
        !a_item || !a_item->data || a_item->size < width * height * 4u) return;

    const uint32_t *p1 = (const uint32_t *)item1->data;
    const uint32_t *a_layer = (const uint32_t *)a_item->data;
    int frame = nba_ea_intro_local_frame(local_t);
    if (frame >= NBA_INTRO_ZOOM_FRAMES + 1 + NBA_INTRO_FLASH_FRAMES) {
        nba_ea_intro_render_captured_stage(item2, ren, start_x, start_y);
        return;
    }
    float scale = nba_ea_intro_mode7_scale(frame);
    int flash_frame = frame - (NBA_INTRO_ZOOM_FRAMES + 1);

    /* The previous E has already settled. Draw it first so the incoming A can occlude it. */
    for (uint32_t r = 0; r < height; r++) {
        int py = start_y + (int)r;
        if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
        for (uint32_t c = 0; c < width; c++) {
            uint32_t index = r * width + c;
            uint32_t source_color = p1[index];
            if (nba_ea_intro_pixel_visible(source_color)) {
                int px = start_x + (int)c;
                if (px >= 0 && px < NBA_SNES_WIDTH) {
                    /* $82:F3E9 selects A's palette block for F4C4, not this settled E. */
                    ren->pixels[py * NBA_SNES_WIDTH + px] = source_color;
                }
            }
        }
    }

    /* F512 draws A as an independent indexed Mode 7 tilegroup over the
     * settled E. Transparent A cells do not erase E. */
    float pcx = (float)NBA_SNES_WIDTH * 0.5f;
    float pcy = (float)NBA_SNES_HEIGHT * 0.5f;
    float inv_scale = 1.0f / scale;

    for (int py = 0; py < NBA_SNES_HEIGHT; py++) {
        int ty = (int)floorf(pcy - 2.0f + (float)(py - pcy) * inv_scale -
                             (float)start_y + 0.5f);
        if (ty < 0 || ty >= (int)height) continue;

        for (int px = 0; px < NBA_SNES_WIDTH; px++) {
            int tx = (int)floorf(pcx + 2.0f + (float)(px - pcx) * inv_scale -
                                 (float)start_x + 0.5f);
            if (tx < 0 || tx >= (int)width) continue;

            uint32_t index = (uint32_t)ty * width + (uint32_t)tx;
            uint32_t a_color = a_layer[index];
            if (nba_ea_intro_pixel_visible(a_color)) {
                uint32_t color = nba_ea_intro_flash_color(a_color, flash_frame);
                ren->pixels[py * NBA_SNES_WIDTH + px] = color;
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
    const NbaAssetItem *item2 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE2);
    const NbaAssetItem *item3 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE3);
    if (!item3 || !item3->data) return;

    const uint32_t *p2 = (item2 && item2->data) ? (const uint32_t *)item2->data : NULL;
    const uint32_t *p3 = (const uint32_t *)item3->data;
    int frame = nba_ea_intro_local_frame(local_t);
    if (frame >= NBA_INTRO_ZOOM_FRAMES + 1 + NBA_INTRO_FLASH_FRAMES) {
        nba_ea_intro_render_captured_stage(item3, ren, start_x, start_y);
        return;
    }
    float scale = nba_ea_intro_mode7_scale(frame);
    int flash_frame = frame - (NBA_INTRO_ZOOM_FRAMES + 1);

    /* Draw the settled EA first; the incoming SPORTS layer is composited over it. */
    if (p2) {
        for (uint32_t r = 0; r < height; r++) {
            int py = start_y + (int)r;
            if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
            for (uint32_t c = 0; c < width; c++) {
                uint32_t index = r * width + c;
                uint32_t source_color = p2[index];
                if (nba_ea_intro_pixel_visible(source_color)) {
                    int px = start_x + (int)c;
                    if (px >= 0 && px < NBA_SNES_WIDTH) {
                        /* $82:F43A selects SPORTS' palette block for F4C4, not settled EA. */
                        ren->pixels[py * NBA_SNES_WIDTH + px] = source_color;
                    }
                }
            }
        }
    }

    /* $82:F408 prepares the SPORTS group before $82:F56D's first visible
     * matrix update; the entry frame still presents the settled EA image. */
    if (frame == 0) return;

    /* F52E likewise overwrites its whole changed tilegroup, including cells
     * which replace or clear pixels from the settled EA artwork. */
    float pcx = (float)NBA_SNES_WIDTH * 0.5f;
    float pcy = (float)NBA_SNES_HEIGHT * 0.5f;
    float inv_scale = 1.0f / scale;

    for (int py = 0; py < NBA_SNES_HEIGHT; py++) {
        int ty = (int)floorf(pcy + (float)(py - pcy) * inv_scale - (float)start_y + 0.5f);
        if (ty < 0 || ty >= (int)height) continue;

        for (int px = 0; px < NBA_SNES_WIDTH; px++) {
            int tx = (int)floorf(pcx + (float)(px - pcx) * inv_scale - (float)start_x + 0.5f);
            if (tx < 0 || tx >= (int)width) continue;

            uint32_t index = (uint32_t)ty * width + (uint32_t)tx;
            uint32_t col3 = p3[index];
            uint32_t col2 = p2 ? p2[index] : 0;
            if (col3 != col2) {
                uint32_t color = nba_ea_intro_pixel_visible(col3) ?
                    nba_ea_intro_flash_color(col3, flash_frame) : 0xFF000000u;
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
                               int start_x, int start_y, uint32_t width, uint32_t height) {
    if (!assets || !ren) return;
    const NbaAssetItem *item4 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE4);
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
        nba_ea_intro_render_stage4(assets, ren, start_x, start_y, width, height);
    }
}
