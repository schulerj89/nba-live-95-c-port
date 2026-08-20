#ifndef NBA_SETUP_SCREEN_H
#define NBA_SETUP_SCREEN_H

#include "nba_types.h"
#include "nba_assets.h"
#include "nba_renderer.h"

/**
 * NBA Live '95 Game Setup screen.
 *
 * Every address and constant below was taken from the running ROM rather than
 * inferred: the routine set came from a live Mesen execution trace of the
 * screen (tools/mesen_decomp_trace.lua -> .analysis/setup_capture/
 * setup_exec_addrs.txt), and the PPU/scroll figures from a per-frame register
 * log (tools/mesen_scroll_log.lua).
 *
 * Routines observed executing while the screen is live (bank $80 unless noted):
 *   $80:A2BF  screen build / layer + scroll setup
 *   $80:A3B8  per-frame update driving the backdrop scroll
 *   $80:A62D  option row state
 *   $80:A77C  option value dispatch
 *   $80:A9E3  APU command write   ($2140-$2143)
 *   $80:AA7B  APU handshake       ($2140-$2143)
 *   $80:AACD  APU command queue   ($2140-$2143)
 *   $80:CB8F  DMA/transfer helper
 *   $81:F9F1  HDMA table setup    (writes $420C at $81:FA72/$81:FA7E)
 *   $80:C62B  ROM decompressor used to build the layer tile data
 */

/* Executed-routine addresses (live trace, not inferred) */
#define SNES_ADDR_SETUP_BUILD          0x80A2BF
#define SNES_ADDR_SETUP_FRAME_UPDATE   0x80A3B8
#define SNES_ADDR_SETUP_ROW_STATE      0x80A62D
#define SNES_ADDR_SETUP_OPTION_DISPATCH 0x80A77C
#define SNES_ADDR_SETUP_APU_COMMAND    0x80A9E3
#define SNES_ADDR_SETUP_APU_HANDSHAKE  0x80AA7B
#define SNES_ADDR_SETUP_APU_QUEUE      0x80AACD
#define SNES_ADDR_SETUP_HDMA_SETUP     0x81F9F1
#define SNES_ADDR_ROM_DECOMPRESSOR     0x80C62B

/* PPU layout captured on the live screen (BG Mode 1).
 * Addresses are VRAM byte offsets; Mesen reports them as word addresses. */
#define NBA_SETUP_BG1_TILEMAP   0x1800   /* 64x32, header banner            */
#define NBA_SETUP_BG1_CHR       0x6000   /* 4bpp                            */
#define NBA_SETUP_BG2_TILEMAP   0x1000   /* 64x32, scrolling backdrop       */
#define NBA_SETUP_BG2_CHR       0x2000   /* 4bpp                            */
#define NBA_SETUP_BG3_TILEMAP   0x0000   /* 32x64, menu text canvas         */
#define NBA_SETUP_BG3_CHR       0x8000   /* 2bpp                            */

/* Steady-state scroll values ($80:A3B8), verified per frame */
#define NBA_SETUP_BG1_HSCROLL   512
#define NBA_SETUP_BG1_VSCROLL   1023
#define NBA_SETUP_BG2_HSCROLL   0

/* Backdrop scrolls up one pixel every three frames (measured 0.3333 px/frame
 * over 210 frames with no drift). */
#define NBA_SETUP_SCROLL_PERIOD 3

/* Entrance animation: BG1 slides 768->512 and BG2 768->0 at 8 px/frame while
 * INIDISP brightness ramps 1->15 one step per frame. */
#define NBA_SETUP_ENTER_BG1_START   768
#define NBA_SETUP_ENTER_BG2_START   768
#define NBA_SETUP_ENTER_SCROLL_STEP 8
#define NBA_SETUP_ENTER_FRAMES      32
#define NBA_SETUP_FADE_FRAMES       15

/* Main/sub screen designation ($212C/$212D) before and after the slide-in */
#define NBA_SETUP_MAIN_ENTER    0x03   /* BG1 + BG2                          */
#define NBA_SETUP_MAIN_SETTLED  0x17   /* BG1 + BG2 + BG3 + OBJ              */
#define NBA_SETUP_SUB_SETTLED   0x04   /* BG3 on subscreen (colour math)     */

typedef enum {
    NBA_SETUP_ROW_MODE = 0,
    NBA_SETUP_ROW_STYLE,
    NBA_SETUP_ROW_LEVEL,
    NBA_SETUP_ROW_QUARTER,
    NBA_SETUP_ROW_RULES,
    NBA_SETUP_ROW_OPTIONS,
    NBA_SETUP_ROW_COUNT
} NbaSetupRow;

typedef struct {
    const uint8_t *vram;    /* 64 KiB VRAM image  (asset 16) */
    const uint8_t *cgram;   /* 512 B CGRAM image  (asset 17) */
    bool has_gfx;

    int frame;              /* frames since the screen was entered */
    int bg1_hscroll;
    int bg2_hscroll;
    int bg2_vscroll;
    int brightness;         /* INIDISP 0..15 */
    uint8_t main_screen;
    uint8_t sub_screen;

    NbaSetupRow row;
    bool bgm_started;
    bool is_initialized;
} NbaSetupScreen;

void nba_setup_screen_init(NbaSetupScreen *s, const NbaAssetPack *assets);
void nba_setup_screen_update(NbaSetupScreen *s, const NbaInput *input, float delta_time);
void nba_setup_screen_render(const NbaSetupScreen *s, NbaRenderer *ren);

#endif /* NBA_SETUP_SCREEN_H */
