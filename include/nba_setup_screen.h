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

/* $80:E600 hands control to the next state after the title fade. The ROM then
 * holds forced blank for frames 1638..1742 while $80:A2BF builds the three
 * layers. Frame 1743 is the first visible Setup entrance frame. */
#define NBA_SETUP_FORCED_BLANK_FRAMES 105

/* After the BG1/BG2 slide, the ROM stages the BG3 text canvas through the
 * exact $212C/$212D and vertical-scroll sequence captured at frames 1782-1801. */
#define NBA_SETUP_BG3_PREP_FRAME      39
#define NBA_SETUP_BG3_FLASH_FRAME     40
#define NBA_SETUP_BG3_SETTLE_FRAME    57
#define NBA_SETUP_BG3_START_VSCROLL   252
#define NBA_SETUP_BG3_SCROLL_STEP     14

/* Main/sub screen designation ($212C/$212D) before and after the slide-in */
#define NBA_SETUP_MAIN_ENTER    0x03   /* BG1 + BG2                          */
#define NBA_SETUP_MAIN_SETTLED  0x17   /* BG1 + BG2 + BG3 + OBJ              */
#define NBA_SETUP_SUB_SETTLED   0x04   /* BG3 on subscreen (colour math)     */

/**
 * Selected-row highlight.
 *
 * The active row is drawn gold while every other row stays white, and it is
 * done with colour math rather than a second palette. Verified on hardware:
 *
 *   - $2131 CGADSUB  = colour math enabled for BG3 only, subtract mode
 *                      (ppu.colorMathEnabled=4, colorMathSubtractMode=true)
 *   - $2130 CGWSEL   = use the fixed colour, not the subscreen
 *                      (ppu.colorMathAddSubscreen=false)
 *   - $2132 COLDATA  = 25952 -> R 0, G 11, B 25 in 5-bit channels
 *   - window 1 gates the colour window (ppu.window[0].activeLayers[5]=true)
 *   - HDMA channel 7 rewrites $2126/$2127 (window 1 left/right) per scanline
 *     from a table at $7F:6800, opening the window only over the active row.
 *
 * Subtracting (0,11,25) from white (31,31,31) gives (31,20,6) = RGB
 * (255,165,49), and from the grey (22,22,22) gives (22,11,0) = (181,90,0).
 * Both match the ROM's pixels exactly.
 *
 * The HDMA table decodes to: window closed everywhere except a 16-scanline
 * band over the selected row. Measured band tops, cursor row 0 through 5:
 * 70, 88, 106, 124, 156, 174 - an 18px pitch with an extra 14px gap before
 * "Set Rules", matching the blank line on screen.
 */
#define SNES_ADDR_SETUP_HDMA_WINDOW    0x7F6800  /* HDMA ch7 table -> $2126/$2127 */
#define NBA_SETUP_FIXED_COLOR          25952     /* $2132 COLDATA: R0 G11 B25     */
#define NBA_SETUP_MATH_SUB_R           0
#define NBA_SETUP_MATH_SUB_G           11
#define NBA_SETUP_MATH_SUB_B           25
#define NBA_SETUP_HIGHLIGHT_TOP        70        /* first band, cursor row 0      */
#define NBA_SETUP_HIGHLIGHT_HEIGHT     16        /* scanlines the window is open  */
#define NBA_SETUP_ROW_PITCH            18
#define NBA_SETUP_ROW_GAP_BEFORE_RULES 14        /* blank line above "Set Rules"  */

typedef enum {
    NBA_SETUP_ROW_MODE = 0,
    NBA_SETUP_ROW_STYLE,
    NBA_SETUP_ROW_LEVEL,
    NBA_SETUP_ROW_QUARTER,
    NBA_SETUP_ROW_RULES,
    NBA_SETUP_ROW_OPTIONS,
    NBA_SETUP_ROW_COUNT
} NbaSetupRow;

typedef enum {
    NBA_SETUP_PAGE_MAIN = 0,
    NBA_SETUP_PAGE_RULES,
    NBA_SETUP_PAGE_OPTIONS
} NbaSetupPage;

typedef enum {
    NBA_SETUP_TRANSITION_NONE = 0,
    NBA_SETUP_TRANSITION_OPEN,
    NBA_SETUP_TRANSITION_RETURN
} NbaSetupTransition;

typedef enum {
    NBA_SETUP_SOUND_NONE = 0,
    NBA_SETUP_SOUND_ADJUST,   /* $80:9DF3 command $49, SRCN $1A */
    NBA_SETUP_SOUND_MOVE,     /* $80:9DF3 command $4A, SRCN $1B */
    NBA_SETUP_SOUND_CONFIRM   /* $80:9DF3 command $4B, SRCN $1C */
} NbaSetupSound;

#define NBA_SETUP_RULE_COUNT   13
#define NBA_SETUP_OPTION_COUNT 7

typedef struct {
    uint16_t rules[NBA_SETUP_RULE_COUNT];    /* ROM commit block $7E:17D1 */
    uint16_t options[NBA_SETUP_OPTION_COUNT];/* ROM commit block $7E:17B5 */
} NbaSetupConfig;

typedef struct {
    uint8_t vram[0x10000];  /* mutable $80:A2BF entrance VRAM + DMA trace */
    uint8_t cgram[0x200];   /* mutable entrance CGRAM + DMA trace          */
    const uint8_t *ppu_trace;
    size_t ppu_trace_size;
    size_t ppu_trace_offset;
    int ppu_decoded_frame;
    bool has_gfx;

    int frame;              /* -105 forced-blank load; 0 = first visible frame */
    int bg1_hscroll;
    int bg2_hscroll;
    int bg2_vscroll;
    int bg3_vscroll;
    int brightness;         /* INIDISP 0..15 */
    uint8_t main_screen;
    uint8_t sub_screen;

    NbaSetupRow row;
    NbaSetupPage page;
    int menu_row;
    int menu_scroll;
    NbaSetupTransition transition;
    NbaSetupPage transition_target;
    int transition_frame;
    bool transition_blank;
    const uint8_t *rules_vram;
    const uint8_t *rules_cgram;
    const uint8_t *options_vram;
    const uint8_t *options_cgram;
    const uint8_t *rules_oam;
    const uint8_t *options_oam;
    const uint8_t *options_off_vram;
    const uint8_t *options_mono_vram;
    const uint8_t *options_cpu_vram;
    NbaSetupConfig config;
    uint16_t working_rules[NBA_SETUP_RULE_COUNT];
    uint16_t working_options[NBA_SETUP_OPTION_COUNT];
    bool is_initialized;
} NbaSetupScreen;

void nba_setup_screen_init(NbaSetupScreen *s, const NbaAssetPack *assets);
NbaSetupSound nba_setup_screen_update(NbaSetupScreen *s, const NbaInput *input);
int  nba_setup_screen_row_band_top(NbaSetupRow row);
void nba_setup_screen_render(const NbaSetupScreen *s, NbaRenderer *ren);

#endif /* NBA_SETUP_SCREEN_H */
