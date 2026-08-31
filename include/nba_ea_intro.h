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
#define SNES_ADDR_OAM_BUILD_BEGIN       0x80AC1B  /* $80:AC1B - Prepare fixed OBJ/OAM group buffer */
#define SNES_ADDR_OAM_BUILD_FINISH      0x80AC89  /* $80:AC89 - Finish/swap fixed OBJ/OAM buffer */
#define SNES_ADDR_OAM_GROUP_DRAW        0x80B344  /* $80:B344 - Draw fixed sprite group at X/Y */
#define SNES_ADDR_FLASH_SUBROUTINE      0x82F4C4  /* $82:F4C4 - 8-frame specular highlight flash loop */
#define SNES_ADDR_COLOR_STEP            0x82F5E7  /* $82:F5E7 - Palette RGB channel stepping algorithm */
#define SNES_ADDR_STAGE_STEP_LOOP       0x82F56D  /* $82:F56D - 22-frame entrance animation step loop */
#define SNES_ADDR_SCROLL_MATRIX_UPDATE  0x82962D  /* $82:962D - Mode 7 matrix & scroll register dispatcher */

/* SNES Frame Timing Counts (at 60 FPS) */
#define NBA_INTRO_STAGE1_FRAMES         33        /* motion 0..32; A resets at 33 */
#define NBA_INTRO_STAGE2_FRAMES         34        /* motion 33..66; SPORTS resets at 67 */
#define NBA_INTRO_STAGE3_FRAMES         33        /* motion 67..99; Stage 4 at 100 */
#define NBA_INTRO_STAGE4_FRAMES         203       /* Legacy dispatcher duration; not native hold proof. */
#define NBA_INTRO_TOTAL_FRAMES          303       /* Whole intro hold/audio/handoff still pending. */

#define NBA_INTRO_ZOOM_FRAMES           22        /* $000C increments after duplicated $0001 */
#define NBA_INTRO_FLASH_FRAMES          8         /* Eight waits in $82:F4C4 */
#define NBA_INTRO_MODE7_UNIT            0x0100    /* 8.8 fixed-point identity matrix */
#define NBA_INTRO_MODE7_START           0x0001    /* Written by $82:94DF */
#define NBA_INTRO_MODE7_STEP            0x000C    /* Added once per frame by $82:F584-$F594 */

/* Top-level EA Intro Renderer */
void nba_ea_intro_render(const NbaAssetPack *assets, NbaRenderer *ren, uint32_t motion_frame);

bool nba_ea_intro_payload_valid(const uint8_t *data, size_t size);

#endif /* NBA_EA_INTRO_H */
