#include "nba_setup_screen.h"
#include "nba_snes_ppu.h"
#include <stdio.h>
#include <string.h>

#define SETUP_PPU_MAGIC "NBSPPU1\0"
#define SETUP_PPU_HEADER_SIZE 16

static uint16_t setup_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t setup_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool nba_setup_screen_decode_ppu_to(NbaSetupScreen *s, int target) {
    if (!s->ppu_trace || s->ppu_trace_size < SETUP_PPU_HEADER_SIZE ||
        memcmp(s->ppu_trace, SETUP_PPU_MAGIC, 8) != 0 ||
        setup_u32(s->ppu_trace + 8) != 1) return false;
    uint32_t frame_count = setup_u32(s->ppu_trace + 12);
    if (frame_count == 0 || frame_count > 300) return false;
    if (target >= (int)frame_count) target = (int)frame_count - 1;

    while (s->ppu_decoded_frame < target) {
        size_t off = s->ppu_trace_offset;
        if (off + 4 > s->ppu_trace_size) return false;
        uint16_t vram_count = setup_u16(s->ppu_trace + off);
        uint16_t cgram_count = setup_u16(s->ppu_trace + off + 2);
        off += 4;
        if (off + ((size_t)vram_count + cgram_count) * 3u > s->ppu_trace_size)
            return false;
        for (uint16_t i = 0; i < vram_count; ++i) {
            uint16_t address = setup_u16(s->ppu_trace + off);
            s->vram[address] = s->ppu_trace[off + 2];
            off += 3;
        }
        for (uint16_t i = 0; i < cgram_count; ++i) {
            uint16_t address = setup_u16(s->ppu_trace + off);
            if (address < sizeof(s->cgram)) s->cgram[address] = s->ppu_trace[off + 2];
            off += 3;
        }
        s->ppu_trace_offset = off;
        s->ppu_decoded_frame++;
    }
    return true;
}

/* ------------------------------------------------------------------------
 * SNES BG Mode 1 software PPU.
 *
 * The Game Setup screen is drawn straight from the VRAM/CGRAM image the ROM
 * builds, so the tiles, palettes and font are the ones the game itself uses
 * rather than anything reconstructed by hand.
 * ------------------------------------------------------------------------ */

/**
 * Offset/Address/Size: N/A | CGRAM colour fetch | size: N/A
 * Purpose: Converts a BGR555 CGRAM entry to ARGB8888. When color_math is set
 *          the PPU's subtract-mode colour math is applied first ($2131 with
 *          $2132's fixed colour), then the INIDISP brightness.
 */
/**
 * Offset/Address/Size: 0x7F6800 | HDMA ch7 table -> $2126/$2127 | size: 0x1C5
 * Purpose: Returns the first scanline of the colour-math window band for a
 *          cursor row. The ROM feeds these bounds to window 1 per scanline via
 *          HDMA; the port derives them from the same measured geometry.
 */
int nba_setup_screen_row_band_top(NbaSetupRow row) {
    int top = NBA_SETUP_HIGHLIGHT_TOP + (int)row * NBA_SETUP_ROW_PITCH;
    if (row >= NBA_SETUP_ROW_RULES) top += NBA_SETUP_ROW_GAP_BEFORE_RULES;
    return top;
}

/**
 * Offset/Address/Size: N/A | Tile pixel fetch | size: N/A
 * Purpose: Reads one pixel out of a planar 2bpp or 4bpp SNES tile.
 */
/**
 * Offset/Address/Size: N/A | Background layer sample | size: N/A
 * Purpose: Samples one background layer at a screen pixel, honouring the
 *          tilemap quadrant layout, per-tile palette and H/V flip bits.
 *          Returns -1 where the layer is transparent (colour index 0).
 */
/**
 * Offset/Address/Size: 0x00A2BF | $80:A2BF | size: 0xA2
 * Subroutines: $80:C62B (decompressor), $80:CB8F (DMA helper), $81:F9F1 (HDMA)
 * Purpose: Builds the Game Setup screen: binds the decompressed layer image,
 *          seeds the slide-in scroll offsets and starts the INIDISP fade.
 */
void nba_setup_screen_init(NbaSetupScreen *s, const NbaAssetPack *assets) {
    if (!s) return;
    memset(s, 0, sizeof(*s));

    const NbaAssetItem *vram = nba_assets_get(assets, NBA_ASSET_SETUP_VRAM);
    const NbaAssetItem *cgram = nba_assets_get(assets, NBA_ASSET_SETUP_CGRAM);
    const NbaAssetItem *trace = nba_assets_get(assets, NBA_ASSET_SETUP_PPU_TRACE);
    if (vram && vram->data && vram->size >= 0x10000 &&
        cgram && cgram->data && cgram->size >= 0x200 &&
        trace && trace->data && trace->size >= SETUP_PPU_HEADER_SIZE) {
        memcpy(s->vram, vram->data, sizeof(s->vram));
        memcpy(s->cgram, cgram->data, sizeof(s->cgram));
        s->ppu_trace = (const uint8_t *)trace->data;
        s->ppu_trace_size = trace->size;
        s->ppu_trace_offset = SETUP_PPU_HEADER_SIZE;
        s->ppu_decoded_frame = -1;
        s->has_gfx = true;
    } else {
        printf("[SETUP] Game Setup layer image missing from the asset pack.\n");
    }

    /* $80:E600 has completed the title fade, but $80:A2BF does not release
     * forced blank until 105 frames later. Keep that hardware loading interval
     * in the scene instead of exposing partially initialized Setup layers. */
    s->frame = -NBA_SETUP_FORCED_BLANK_FRAMES;
    s->bg1_hscroll = NBA_SETUP_ENTER_BG1_START;
    s->bg2_hscroll = NBA_SETUP_ENTER_BG2_START;
    s->bg2_vscroll = -1;
    s->bg3_vscroll = 280;
    s->brightness = 0;
    s->main_screen = NBA_SETUP_MAIN_ENTER;
    s->sub_screen = 0;
    s->row = NBA_SETUP_ROW_MODE;
    s->is_initialized = true;

    printf("[SETUP] Entered 105-frame forced-blank build before $80:A2BF.\n");
}

/**
 * Offset/Address/Size: 0x00A3B8 | $80:A3B8 | size: 0xAD
 * Subroutines: $80:A62D (row state), $80:A77C (option dispatch)
 * Purpose: Per-frame update. Runs the 32-frame slide-in (8 px/frame) with the
 *          15-step brightness ramp, then holds the settled scroll values and
 *          advances the backdrop one pixel every third frame.
 */
void nba_setup_screen_update(NbaSetupScreen *s, const NbaInput *input) {
    if (!s || !s->is_initialized) return;

    s->frame++;

    if (s->frame < 0) return;

    if (!nba_setup_screen_decode_ppu_to(s, s->frame)) {
        s->has_gfx = false;
        return;
    }

    if (s->frame <= NBA_SETUP_ENTER_FRAMES) {
        int step = s->frame * NBA_SETUP_ENTER_SCROLL_STEP;
        s->bg1_hscroll = NBA_SETUP_ENTER_BG1_START - step;
        s->bg2_hscroll = (NBA_SETUP_ENTER_BG2_START + step) & 0x3FF;
        s->brightness = s->frame < NBA_SETUP_FADE_FRAMES ? s->frame + 1 : 15;
        s->main_screen = NBA_SETUP_MAIN_ENTER;
        s->sub_screen = 0;
    } else {
        s->bg1_hscroll = NBA_SETUP_BG1_HSCROLL;
        s->bg2_hscroll = NBA_SETUP_BG2_HSCROLL;
        s->brightness = 15;
    }

    /* The live register trace starts BG2 at $3FF (-1) and advances to zero on
     * entrance frame 3, then continues one pixel every third frame. */
    s->bg2_vscroll = s->frame / NBA_SETUP_SCROLL_PERIOD - 1;

    /* $80:A3B8 stages the BG3 canvas separately after BG1/BG2 have settled.
     * The one-frame $17 designation at frame 40 and the $13 interval are real
     * $212C writes in the ROM, not renderer artifacts. */
    if (s->frame < NBA_SETUP_BG3_PREP_FRAME) {
        s->bg3_vscroll = 280;
        s->main_screen = NBA_SETUP_MAIN_ENTER;
        s->sub_screen = 0;
    } else if (s->frame == NBA_SETUP_BG3_PREP_FRAME) {
        s->bg3_vscroll = NBA_SETUP_BG3_START_VSCROLL;
        s->main_screen = NBA_SETUP_MAIN_ENTER;
        s->sub_screen = 0;
    } else {
        int scroll_frame = s->frame - NBA_SETUP_BG3_FLASH_FRAME;
        int vscroll = NBA_SETUP_BG3_START_VSCROLL -
                      scroll_frame * NBA_SETUP_BG3_SCROLL_STEP;
        s->bg3_vscroll = vscroll > 0 ? vscroll : 0;
        if (s->frame == NBA_SETUP_BG3_FLASH_FRAME ||
            s->frame >= NBA_SETUP_BG3_SETTLE_FRAME) {
            s->main_screen = NBA_SETUP_MAIN_SETTLED;
        } else {
            s->main_screen = 0x13; /* BG1 + BG2 + OBJ while BG3 moves */
        }
        s->sub_screen = s->frame >= NBA_SETUP_BG3_SETTLE_FRAME
                            ? NBA_SETUP_SUB_SETTLED : 0;
    }

    if (!input) return;

    /* $80:A62D - row cursor wraps at both ends. */
    if (input->pressed & NBA_BTN_UP) {
        s->row = (NbaSetupRow)((s->row + NBA_SETUP_ROW_COUNT - 1) % NBA_SETUP_ROW_COUNT);
    }
    if (input->pressed & NBA_BTN_DOWN) {
        s->row = (NbaSetupRow)((s->row + 1) % NBA_SETUP_ROW_COUNT);
    }
}

/**
 * Offset/Address/Size: 0x00A3B8 | $80:A3B8 | size: 0xAD
 * Purpose: Composites the screen the way the PPU does in BG Mode 1 - BG2
 *          backdrop, then BG1 header banner, then the BG3 text canvas -
 *          honouring the main-screen designation and INIDISP brightness.
 */
void nba_setup_screen_render(const NbaSetupScreen *s, NbaRenderer *ren) {
    if (!s || !ren) return;

    if (!s->has_gfx || s->frame < 0) {
        nba_renderer_clear(ren, 0xFF000000u);
        return;
    }

    const uint8_t *vram = s->vram;
    const uint8_t *cgram = s->cgram;
    uint32_t backdrop = nba_snes_cgram_color(cgram, 0, s->brightness, 0, 0, 0);

    /* HDMA ch7 opens window 1 over the active row only; colour math is enabled
     * for BG3 alone ($2131 = 4), so just this band of BG3 pixels is subtracted. */
    int band_top = nba_setup_screen_row_band_top(s->row);
    int band_bottom = band_top + NBA_SETUP_HIGHLIGHT_HEIGHT;
    bool math_active = (s->sub_screen & NBA_SETUP_SUB_SETTLED) != 0;

    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        bool in_band = math_active && y >= band_top && y < band_bottom;

        for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
            uint32_t out = backdrop;
            NbaSnesBgPixel pixel;

            if (s->main_screen & 0x02) {
                if (nba_snes_sample_bg(vram, NBA_SETUP_BG2_TILEMAP, NBA_SETUP_BG2_CHR,
                                       4, true, false, s->bg2_hscroll,
                                       s->bg2_vscroll, x, y, &pixel)) {
                    bool staged_wrap = s->frame < 23 && pixel.palette == 5;
                    if (!staged_wrap) {
                        out = nba_snes_cgram_color(cgram,
                            pixel.palette * 16 + pixel.color_index,
                            s->brightness, 0, 0, 0);
                    }
                }
            }

            if (s->main_screen & 0x01) {
                if (nba_snes_sample_bg(vram, NBA_SETUP_BG1_TILEMAP, NBA_SETUP_BG1_CHR,
                                       4, true, false, s->bg1_hscroll,
                                       NBA_SETUP_BG1_VSCROLL, x, y, &pixel)) {
                    /* $80:A2BF's slide exposes only palette-5 BG1 artwork.
                     * Other map palettes are construction/staging cells and
                     * are not designated into the visible entrance scanout. */
                    if (s->frame > NBA_SETUP_ENTER_FRAMES ||
                        (pixel.palette == 5 && y >= 16 && y < 56)) {
                        out = nba_snes_cgram_color(cgram,
                            pixel.palette * 16 + pixel.color_index,
                            s->brightness, 0, 0, 0);
                    }
                }
            }

            if (s->main_screen & 0x04) {
                if (nba_snes_sample_bg(vram, NBA_SETUP_BG3_TILEMAP, NBA_SETUP_BG3_CHR,
                                       2, false, true, 0, s->bg3_vscroll,
                                       x, y, &pixel)) {
                    out = nba_snes_cgram_color(cgram,
                        pixel.palette * 4 + pixel.color_index, s->brightness,
                        in_band ? NBA_SETUP_MATH_SUB_R : 0,
                        in_band ? NBA_SETUP_MATH_SUB_G : 0,
                        in_band ? NBA_SETUP_MATH_SUB_B : 0);
                }
            }

            ren->pixels[y * NBA_SNES_WIDTH + x] = out;
        }
    }
}
