#include "nba_setup_screen.h"
#include <stdio.h>
#include <string.h>

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
static uint32_t setup_color(const uint8_t *cgram, int index, int brightness,
                            bool color_math) {
    uint16_t w = (uint16_t)(cgram[(index * 2) & 0x1FF] |
                            ((uint16_t)cgram[(index * 2 + 1) & 0x1FF] << 8));
    int r = w & 0x1F;
    int g = (w >> 5) & 0x1F;
    int b = (w >> 10) & 0x1F;

    if (color_math) {
        /* $2131 subtract mode against the $2132 fixed colour, clamped at 0. */
        r -= NBA_SETUP_MATH_SUB_R; if (r < 0) r = 0;
        g -= NBA_SETUP_MATH_SUB_G; if (g < 0) g = 0;
        b -= NBA_SETUP_MATH_SUB_B; if (b < 0) b = 0;
    }

    /* Standard SNES 5->8 bit expansion, matching the PPU: (v << 3) | (v >> 2). */
    uint32_t r8 = (uint32_t)((r << 3) | (r >> 2));
    uint32_t g8 = (uint32_t)((g << 3) | (g >> 2));
    uint32_t b8 = (uint32_t)((b << 3) | (b >> 2));
    if (brightness < 15) {
        r8 = r8 * (uint32_t)brightness / 15u;
        g8 = g8 * (uint32_t)brightness / 15u;
        b8 = b8 * (uint32_t)brightness / 15u;
    }
    return 0xFF000000u | (r8 << 16) | (g8 << 8) | b8;
}

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
static int setup_tile_pixel(const uint8_t *vram, int chr_base, int tile,
                            int bpp, int x, int y) {
    int off = (chr_base + tile * 8 * bpp) & 0xFFFF;
    int bit = 7 - x;
    int value = 0;
    for (int plane = 0; plane < bpp; plane += 2) {
        int lo = vram[(off + y * 2 + plane * 8) & 0xFFFF];
        int hi = vram[(off + y * 2 + 1 + plane * 8) & 0xFFFF];
        value |= ((lo >> bit) & 1) << plane;
        value |= ((hi >> bit) & 1) << (plane + 1);
    }
    return value;
}

/**
 * Offset/Address/Size: N/A | Background layer sample | size: N/A
 * Purpose: Samples one background layer at a screen pixel, honouring the
 *          tilemap quadrant layout, per-tile palette and H/V flip bits.
 *          Returns -1 where the layer is transparent (colour index 0).
 */
static int setup_sample_bg(const uint8_t *vram, int map_base, int chr_base,
                           int bpp, bool wide, bool tall,
                           int hscroll, int vscroll, int x, int y,
                           int *out_palette) {
    int map_w = wide ? 512 : 256;
    int map_h = tall ? 512 : 256;
    int px = ((x + hscroll) % map_w + map_w) % map_w;
    /* The PPU's vertical scroll is offset by one: the first displayed scanline
     * shows tilemap line vscroll+1, not vscroll. */
    int py = ((y + vscroll + 1) % map_h + map_h) % map_h;
    int tx = px >> 3;
    int ty = py >> 3;

    int quadrant = 0;
    if (wide && tx >= 32) quadrant += 1;
    if (tall && ty >= 32) quadrant += wide ? 2 : 1;

    int entry_index = map_base + quadrant * 0x800 + ((ty & 31) * 32 + (tx & 31)) * 2;
    uint16_t entry = (uint16_t)(vram[entry_index & 0xFFFF] |
                                ((uint16_t)vram[(entry_index + 1) & 0xFFFF] << 8));
    int tile = entry & 0x3FF;
    int palette = (entry >> 10) & 7;
    int hflip = (entry >> 14) & 1;
    int vflip = (entry >> 15) & 1;

    int sx = hflip ? 7 - (px & 7) : (px & 7);
    int sy = vflip ? 7 - (py & 7) : (py & 7);
    int value = setup_tile_pixel(vram, chr_base, tile, bpp, sx, sy);
    if (value == 0) return -1;

    *out_palette = palette;
    return value;
}

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
    if (vram && vram->data && vram->size >= 0x10000 &&
        cgram && cgram->data && cgram->size >= 0x200) {
        s->vram = (const uint8_t *)vram->data;
        s->cgram = (const uint8_t *)cgram->data;
        s->has_gfx = true;
    } else {
        printf("[SETUP] Game Setup layer image missing from the asset pack.\n");
    }

    s->frame = 0;
    s->bg1_hscroll = NBA_SETUP_ENTER_BG1_START;
    s->bg2_hscroll = NBA_SETUP_ENTER_BG2_START;
    s->bg2_vscroll = 0;
    s->brightness = 1;
    s->main_screen = NBA_SETUP_MAIN_ENTER;
    s->sub_screen = 0;
    s->row = NBA_SETUP_ROW_MODE;
    s->is_initialized = true;

    printf("[SETUP] Game Setup screen built ($80:A2BF).\n");
}

/**
 * Offset/Address/Size: 0x00A3B8 | $80:A3B8 | size: 0xAD
 * Subroutines: $80:A62D (row state), $80:A77C (option dispatch)
 * Purpose: Per-frame update. Runs the 32-frame slide-in (8 px/frame) with the
 *          15-step brightness ramp, then holds the settled scroll values and
 *          advances the backdrop one pixel every third frame.
 */
void nba_setup_screen_update(NbaSetupScreen *s, const NbaInput *input, float delta_time) {
    (void)delta_time;
    if (!s || !s->is_initialized) return;

    s->frame++;

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
        s->main_screen = NBA_SETUP_MAIN_SETTLED;
        s->sub_screen = NBA_SETUP_SUB_SETTLED;
    }

    /* Backdrop scroll: +1 px every third frame, running through the slide-in. */
    s->bg2_vscroll = (s->frame / NBA_SETUP_SCROLL_PERIOD) & 0x1FF;

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

    if (!s->has_gfx) {
        nba_renderer_clear(ren, 0xFF000000u);
        return;
    }

    const uint8_t *vram = s->vram;
    const uint8_t *cgram = s->cgram;
    uint32_t backdrop = setup_color(cgram, 0, s->brightness, false);

    /* HDMA ch7 opens window 1 over the active row only; colour math is enabled
     * for BG3 alone ($2131 = 4), so just this band of BG3 pixels is subtracted. */
    int band_top = nba_setup_screen_row_band_top(s->row);
    int band_bottom = band_top + NBA_SETUP_HIGHLIGHT_HEIGHT;
    bool math_active = (s->main_screen & NBA_SETUP_SUB_SETTLED) != 0;

    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        bool in_band = math_active && y >= band_top && y < band_bottom;

        for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
            uint32_t out = backdrop;
            int palette = 0;
            int value;

            if (s->main_screen & 0x02) {
                value = setup_sample_bg(vram, NBA_SETUP_BG2_TILEMAP, NBA_SETUP_BG2_CHR,
                                        4, true, false,
                                        s->bg2_hscroll, s->bg2_vscroll, x, y, &palette);
                if (value >= 0) {
                    out = setup_color(cgram, palette * 16 + value, s->brightness, false);
                }
            }

            if (s->main_screen & 0x01) {
                value = setup_sample_bg(vram, NBA_SETUP_BG1_TILEMAP, NBA_SETUP_BG1_CHR,
                                        4, true, false,
                                        s->bg1_hscroll, NBA_SETUP_BG1_VSCROLL, x, y, &palette);
                if (value >= 0) {
                    out = setup_color(cgram, palette * 16 + value, s->brightness, false);
                }
            }

            if (s->main_screen & 0x04) {
                value = setup_sample_bg(vram, NBA_SETUP_BG3_TILEMAP, NBA_SETUP_BG3_CHR,
                                        2, false, true, 0, 0, x, y, &palette);
                if (value >= 0) {
                    out = setup_color(cgram, palette * 4 + value, s->brightness, in_band);
                }
            }

            ren->pixels[y * NBA_SNES_WIDTH + x] = out;
        }
    }
}
