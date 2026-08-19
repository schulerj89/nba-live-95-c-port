#include "nba_ea_intro.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/**
 * Offset/Address/Size: 0x0172EA | $82:F2EA | size: 0x80 (128 bytes)
 * Subroutines: $82:94DF (Mode 7 scale init), $82:F4F6 (E tilegroup draw), $82:F56D (22-frame zoom loop)
 * Purpose: Renders Stage 1 "E" zooming in from the foreground (3.5x -> 1.0x) to its exact emblem position.
 */
void nba_ea_intro_render_stage1(const NbaAssetPack *assets, NbaRenderer *ren, float local_t,
                               int start_x, int start_y, uint32_t width, uint32_t height) {
    if (!assets || !ren) return;
    const NbaAssetItem *item1 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE1);
    if (!item1 || !item1->data) return;

    const uint32_t *p1 = (const uint32_t *)item1->data;
    float scale = 1.0f;
    int flash_boost = 0;

    if (local_t < NBA_INTRO_ANIM_DURATION_SEC) {
        float p = local_t / NBA_INTRO_ANIM_DURATION_SEC;
        float ease = 1.0f - (1.0f - p) * (1.0f - p);
        scale = NBA_INTRO_MAX_ZOOM_FACTOR - (NBA_INTRO_MAX_ZOOM_FACTOR - 1.0f) * ease;
    } else {
        int flash_frame = (int)((local_t - NBA_INTRO_ANIM_DURATION_SEC) * 60.0f);
        if (flash_frame < 8) flash_boost = (8 - flash_frame) * 14;
    }

    /* Exact geometric center of 'E' piece on screen */
    float pcx = (float)start_x + 46.0f;
    float pcy = (float)start_y + 44.5f;
    float inv_scale = 1.0f / scale;

    for (int py = 0; py < NBA_SNES_HEIGHT; py++) {
        int ty = (int)floorf(pcy + (float)(py - pcy) * inv_scale - (float)start_y + 0.5f);
        if (ty < 0 || ty >= (int)height) continue;

        for (int px = 0; px < NBA_SNES_WIDTH; px++) {
            int tx = (int)floorf(pcx + (float)(px - pcx) * inv_scale - (float)start_x + 0.5f);
            if (tx < 0 || tx >= (int)width) continue;

            uint32_t color = p1[ty * width + tx];
            if (color != 0) {
                if (flash_boost > 0) {
                    uint32_t a = (color >> 24) & 0xFF;
                    uint32_t red   = ((color >> 16) & 0xFF) + (uint32_t)flash_boost;
                    uint32_t green = ((color >> 8) & 0xFF) + (uint32_t)flash_boost;
                    uint32_t blue  = (color & 0xFF) + (uint32_t)flash_boost;
                    if (red > 255) red = 255;
                    if (green > 255) green = 255;
                    if (blue > 255) blue = 255;
                    color = (a << 24) | (red << 16) | (green << 8) | blue;
                }
                ren->pixels[py * NBA_SNES_WIDTH + px] = color;
            }
        }
    }
}

/**
 * Offset/Address/Size: 0x01736A | $82:F36A | size: 0x90 (144 bytes)
 * Subroutines: $82:F512 (A tilegroup draw), $82:F56D (22-frame zoom loop), $82:F4C4 (8-frame flash)
 * Purpose: Renders Stage 2 stationary "E" while "A" zooms in from foreground to meet "E".
 */
void nba_ea_intro_render_stage2(const NbaAssetPack *assets, NbaRenderer *ren, float local_t,
                               int start_x, int start_y, uint32_t width, uint32_t height) {
    if (!assets || !ren) return;
    const NbaAssetItem *item1 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE1);
    const NbaAssetItem *item2 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE2);
    if (!item2 || !item2->data) return;

    const uint32_t *p1 = (item1 && item1->data) ? (const uint32_t *)item1->data : NULL;
    const uint32_t *p2 = (const uint32_t *)item2->data;
    float scale = 1.0f;
    int flash_boost = 0;

    if (local_t < NBA_INTRO_ANIM_DURATION_SEC) {
        float p = local_t / NBA_INTRO_ANIM_DURATION_SEC;
        float ease = 1.0f - (1.0f - p) * (1.0f - p);
        scale = NBA_INTRO_MAX_ZOOM_FACTOR - (NBA_INTRO_MAX_ZOOM_FACTOR - 1.0f) * ease;
    } else {
        int flash_frame = (int)((local_t - NBA_INTRO_ANIM_DURATION_SEC) * 60.0f);
        if (flash_frame < 8) flash_boost = (8 - flash_frame) * 14;
    }

    /* 1. Zooming "A" piece from foreground (rendered behind stationary E) */
    /* Exact geometric center of 'A' piece */
    float pcx = (float)start_x + 69.5f;
    float pcy = (float)start_y + 37.5f;
    float inv_scale = 1.0f / scale;

    for (int py = 0; py < NBA_SNES_HEIGHT; py++) {
        int ty = (int)floorf(pcy + (float)(py - pcy) * inv_scale - (float)start_y + 0.5f);
        if (ty < 0 || ty >= (int)height) continue;

        for (int px = 0; px < NBA_SNES_WIDTH; px++) {
            int tx = (int)floorf(pcx + (float)(px - pcx) * inv_scale - (float)start_x + 0.5f);
            if (tx < 0 || tx >= (int)width) continue;

            if (tx >= 4) {
                uint32_t col1 = p1 ? p1[ty * width + tx] : 0;
                uint32_t col2 = p2[ty * width + tx];
                if (col2 != 0 && col1 == 0) {
                    uint32_t color = col2;
                    if (flash_boost > 0) {
                        uint32_t a = (color >> 24) & 0xFF;
                        uint32_t red   = ((color >> 16) & 0xFF) + (uint32_t)flash_boost;
                        uint32_t green = ((color >> 8) & 0xFF) + (uint32_t)flash_boost;
                        uint32_t blue  = (color & 0xFF) + (uint32_t)flash_boost;
                        if (red > 255) red = 255;
                        if (green > 255) green = 255;
                        if (blue > 255) blue = 255;
                        color = (a << 24) | (red << 16) | (green << 8) | blue;
                    }
                    ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                }
            }
        }
    }

    /* 2. Stationary "E" piece (rendered ON TOP so E remains 100% intact and crisp) */
    if (p1) {
        for (uint32_t r = 0; r < height; r++) {
            int py = start_y + (int)r;
            if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
            for (uint32_t c = 0; c < width; c++) {
                uint32_t color = p1[r * width + c];
                if (color != 0) {
                    if (flash_boost > 0) {
                        uint32_t a = (color >> 24) & 0xFF;
                        uint32_t red   = ((color >> 16) & 0xFF) + (uint32_t)flash_boost;
                        uint32_t green = ((color >> 8) & 0xFF) + (uint32_t)flash_boost;
                        uint32_t blue  = (color & 0xFF) + (uint32_t)flash_boost;
                        if (red > 255) red = 255;
                        if (green > 255) green = 255;
                        if (blue > 255) blue = 255;
                        color = (a << 24) | (red << 16) | (green << 8) | blue;
                    }
                    int px = start_x + (int)c;
                    if (px >= 0 && px < NBA_SNES_WIDTH) {
                        ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                    }
                }
            }
        }
    }
}

/**
 * Offset/Address/Size: 0x017408 | $82:F408 | size: 0x90 (144 bytes)
 * Subroutines: $82:F52E (SPORTS banner draw), $82:F56D (22-frame zoom loop), $82:F4C4 (8-frame flash)
 * Purpose: Renders Stage 3 stationary "EA" emblem while "SPORTS" ribbon zooms in from foreground.
 */
void nba_ea_intro_render_stage3(const NbaAssetPack *assets, NbaRenderer *ren, float local_t,
                               int start_x, int start_y, uint32_t width, uint32_t height) {
    if (!assets || !ren) return;
    const NbaAssetItem *item2 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE2);
    const NbaAssetItem *item3 = nba_assets_get(assets, NBA_ASSET_EA_LOGO_STAGE3);
    if (!item3 || !item3->data) return;

    const uint32_t *p2 = (item2 && item2->data) ? (const uint32_t *)item2->data : NULL;
    const uint32_t *p3 = (const uint32_t *)item3->data;
    float scale = 1.0f;
    int flash_boost = 0;

    if (local_t < NBA_INTRO_ANIM_DURATION_SEC) {
        float p = local_t / NBA_INTRO_ANIM_DURATION_SEC;
        float ease = 1.0f - (1.0f - p) * (1.0f - p);
        scale = NBA_INTRO_MAX_ZOOM_FACTOR - (NBA_INTRO_MAX_ZOOM_FACTOR - 1.0f) * ease;
    } else {
        int flash_frame = (int)((local_t - NBA_INTRO_ANIM_DURATION_SEC) * 60.0f);
        if (flash_frame < 8) flash_boost = (8 - flash_frame) * 14;
    }

    /* 1. Zooming "SPORTS" ribbon from foreground (rendered behind stationary EA emblem) */
    /* Exact geometric center of 'SPORTS' ribbon */
    float pcx = (float)start_x + 75.5f;
    float pcy = (float)start_y + 95.5f;
    float inv_scale = 1.0f / scale;

    for (int py = 0; py < NBA_SNES_HEIGHT; py++) {
        int ty = (int)floorf(pcy + (float)(py - pcy) * inv_scale - (float)start_y + 0.5f);
        if (ty < 0 || ty >= (int)height) continue;

        for (int px = 0; px < NBA_SNES_WIDTH; px++) {
            int tx = (int)floorf(pcx + (float)(px - pcx) * inv_scale - (float)start_x + 0.5f);
            if (tx < 0 || tx >= (int)width) continue;

            uint32_t col2 = p2 ? p2[ty * width + tx] : 0;
            uint32_t col3 = p3[ty * width + tx];
            if (col3 != 0 && col2 == 0) {
                uint32_t color = col3;
                if (flash_boost > 0) {
                    uint32_t a = (color >> 24) & 0xFF;
                    uint32_t red   = ((color >> 16) & 0xFF) + (uint32_t)flash_boost;
                    uint32_t green = ((color >> 8) & 0xFF) + (uint32_t)flash_boost;
                    uint32_t blue  = (color & 0xFF) + (uint32_t)flash_boost;
                    if (red > 255) red = 255;
                    if (green > 255) green = 255;
                    if (blue > 255) blue = 255;
                    color = (a << 24) | (red << 16) | (green << 8) | blue;
                }
                ren->pixels[py * NBA_SNES_WIDTH + px] = color;
            }
        }
    }

    /* 2. Stationary "EA" emblem (rendered ON TOP so emblem remains 100% intact and crisp) */
    if (p2) {
        for (uint32_t r = 0; r < height; r++) {
            int py = start_y + (int)r;
            if (py < 0 || py >= NBA_SNES_HEIGHT) continue;
            for (uint32_t c = 0; c < width; c++) {
                uint32_t color = p2[r * width + c];
                if (color != 0) {
                    if (flash_boost > 0) {
                        uint32_t a = (color >> 24) & 0xFF;
                        uint32_t red   = ((color >> 16) & 0xFF) + (uint32_t)flash_boost;
                        uint32_t green = ((color >> 8) & 0xFF) + (uint32_t)flash_boost;
                        uint32_t blue  = (color & 0xFF) + (uint32_t)flash_boost;
                        if (red > 255) red = 255;
                        if (green > 255) green = 255;
                        if (blue > 255) blue = 255;
                        color = (a << 24) | (red << 16) | (green << 8) | blue;
                    }
                    int px = start_x + (int)c;
                    if (px >= 0 && px < NBA_SNES_WIDTH) {
                        ren->pixels[py * NBA_SNES_WIDTH + px] = color;
                    }
                }
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
            if (color != 0) {
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

    if (timer < 0.533f) {
        /* Stage 1: E (0.00s - 0.533s, 32 frames) */
        nba_ea_intro_render_stage1(assets, ren, timer, start_x, start_y, width, height);
    } else if (timer < 1.050f) {
        /* Stage 2: A (0.533s - 1.050s, 31 frames) */
        nba_ea_intro_render_stage2(assets, ren, timer - 0.533f, start_x, start_y, width, height);
    } else if (timer < 2.050f) {
        /* Stage 3: SPORTS (1.050s - 2.050s, 60 frames) */
        nba_ea_intro_render_stage3(assets, ren, timer - 1.050f, start_x, start_y, width, height);
    } else {
        /* Stage 4: ELECTRONIC ARTS hold (2.050s - 5.050s, 180 frames) */
        nba_ea_intro_render_stage4(assets, ren, start_x, start_y, width, height);
    }
}
