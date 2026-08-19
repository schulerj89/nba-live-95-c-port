#ifndef NBA_EA_INTRO_H
#define NBA_EA_INTRO_H

#include "nba_types.h"
#include "nba_assets.h"
#include "nba_renderer.h"

/* SNES Subroutine Addresses (LoROM Bank $80 and Bank $82) */
#define SNES_ADDR_SETUP_INTRO           0x82F15C  /* $82:F15C - EA Sports intro setup entry point */
#define SNES_ADDR_DRAW_STAGE1_E         0x82F4F6  /* $82:F4F6 - Assemble 'E' emblem tilegroup */
#define SNES_ADDR_DRAW_STAGE2_A         0x82F512  /* $82:F512 - Assemble 'A' emblem tilegroup */
#define SNES_ADDR_DRAW_STAGE3_SPORTS    0x82F52E  /* $82:F52E - Assemble 'SPORTS' banner */
#define SNES_ADDR_DRAW_STAGE4_BANNER    0x808FA3  /* $80:8FA3 - Assemble 'ELECTRONIC ARTS' banner */
#define SNES_ADDR_FLASH_SUBROUTINE      0x82F4C4  /* $82:F4C4 - 8-frame specular highlight flash loop */
#define SNES_ADDR_COLOR_STEP            0x82F5E7  /* $82:F5E7 - Palette RGB channel stepping algorithm */
#define SNES_ADDR_STAGE_STEP_LOOP       0x82F56D  /* $82:F56D - 22-frame entrance animation step loop */
#define SNES_ADDR_SCROLL_MATRIX_UPDATE  0x82962D  /* $82:962D - Mode 7 matrix & scroll register dispatcher */

/* SNES Frame Timing Counts (at 60 FPS) */
#define NBA_INTRO_STAGE1_FRAMES         32        /* 0.533s ($82:F2EA) */
#define NBA_INTRO_STAGE2_FRAMES         31        /* 0.517s ($82:F36A) */
#define NBA_INTRO_STAGE3_FRAMES         60        /* 1.000s ($82:F408) */
#define NBA_INTRO_STAGE4_FRAMES         180       /* 3.000s ($82:F469) */
#define NBA_INTRO_TOTAL_FRAMES          303       /* 5.050s total */

#define NBA_INTRO_ANIM_DURATION_SEC     0.366f    /* 22 frames in SNES $82:F56D */
#define NBA_INTRO_MAX_ZOOM_FACTOR       3.5f      /* Initial Mode 7 foreground zoom */

/* Top-level EA Intro Renderer */
void nba_ea_intro_render(const NbaAssetPack *assets, NbaRenderer *ren, float timer);

/* Modular Stage Renderers */
void nba_ea_intro_render_stage1(const NbaAssetPack *assets, NbaRenderer *ren, float local_t,
                               int start_x, int start_y, uint32_t width, uint32_t height);
void nba_ea_intro_render_stage2(const NbaAssetPack *assets, NbaRenderer *ren, float local_t,
                               int start_x, int start_y, uint32_t width, uint32_t height);
void nba_ea_intro_render_stage3(const NbaAssetPack *assets, NbaRenderer *ren, float local_t,
                               int start_x, int start_y, uint32_t width, uint32_t height);
void nba_ea_intro_render_stage4(const NbaAssetPack *assets, NbaRenderer *ren,
                               int start_x, int start_y, uint32_t width, uint32_t height);

#endif /* NBA_EA_INTRO_H */
