#include "nba_setup_screen.h"
#include "nba_snes_ppu.h"
#include <stdio.h>
#include <string.h>

#define SETUP_PPU_MAGIC "NBSPPU1\0"
#define SETUP_PPU_HEADER_SIZE 16

static const uint16_t setup_rule_max[NBA_SETUP_RULE_COUNT] = {
    45, 45, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static const uint16_t setup_option_max[NBA_SETUP_OPTION_COUNT] = {
    45, 45, 2, 1, 1, 1, 1
};
static const uint16_t setup_main_max[NBA_SETUP_MAIN_VALUE_COUNT] = { 3, 2, 2, 3 };
static const uint16_t setup_main_defaults[NBA_SETUP_MAIN_VALUE_COUNT] = { 0, 1, 0, 0 };

static bool setup_menu_assets_ready(const NbaSetupScreen *s) {
    return s && s->rules_vram && s->rules_cgram && s->rules_oam &&
           s->options_vram && s->options_cgram && s->options_oam &&
           s->options_off_vram && s->options_mono_vram &&
           s->options_cpu_vram;
}

static uint16_t setup_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t setup_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* $80:A2BF/$80:A3B8 are the shared layer builder/frame sequencer reached by
 * both $81:D318 (Rules) and $82:8CD1 (Options).  The values below are the
 * complete $2100/$212C/$212D/$210D-$2112 trace around A/Start, captured by
 * tools/mesen_setup_menus_capture.lua rather than an invented host tween. */
static void setup_update_page_transition(NbaSetupScreen *s) {
    int t = ++s->transition_frame;
    bool opening = s->transition == NBA_SETUP_TRANSITION_OPEN;
    int slide_start = opening ? 21 : 6;
    int entrance_start = opening ? 76 : 58;

    s->transition_blank = false;
    if (opening && t <= 20) {
        s->brightness = 15;
        s->sub_screen = 4;
        if (t == 1) s->main_screen = 7;
        else if (t <= 15) {
            s->main_screen = 19;
            s->bg3_vscroll = t >= 3 ? (t - 2) * 14 : 0;
        } else {
            s->main_screen = 3;
            s->bg3_vscroll = 182;
        }
        return;
    }

    int slide = t - slide_start;
    if (slide >= 0 && slide <= 30) {
        s->main_screen = 3;
        s->sub_screen = slide == 30 ? 0 : 4;
        s->bg1_hscroll = slide * 8;
        s->bg2_hscroll = (-slide * 8) & 0x3FF;
        s->brightness = slide < 3 ? 15 :
                        (slide == 30 ? 15 : 15 - (slide - 1) / 2);
        return;
    }

    if (t < entrance_start) {
        if (s->page != s->transition_target) s->page = s->transition_target;
        s->transition_blank = true;
        s->brightness = 0;
        s->main_screen = 3;
        s->sub_screen = 0;
        return;
    }

    if (s->page != s->transition_target) s->page = s->transition_target;
    int enter = t - entrance_start;
    s->brightness = enter < 14 ? enter + 1 : 15;
    s->bg1_hscroll = enter <= 32 ? 768 - enter * 8 : 512;
    s->bg2_hscroll = enter <= 32 ? (768 + enter * 8) & 0x3FF : 0;
    s->bg2_vscroll = enter / 3 - 1;
    s->main_screen = 3;
    s->sub_screen = 0;
    s->bg3_vscroll = 280;

    if (opening) {
        if (enter == 43) {
            s->main_screen = 23;
            s->bg3_vscroll = 252;
        } else if (enter >= 44 && enter <= 61) {
            s->main_screen = 19;
            s->bg3_vscroll = 252 - (enter - 44) * 14;
        } else if (enter >= 62) {
            s->bg3_vscroll = 0;
            s->sub_screen = enter >= 64 ? 4 : 0;
            if (enter >= 64) s->transition = NBA_SETUP_TRANSITION_NONE;
        }
    } else {
        if (enter == 39) {
            s->bg3_vscroll = 252;
        } else if (enter == 40) {
            s->main_screen = 23;
            s->bg3_vscroll = 252;
        } else if (enter >= 41 && enter <= 56) {
            s->main_screen = 19;
            s->bg3_vscroll = 238 - (enter - 41) * 14;
        } else if (enter == 57) {
            s->main_screen = 23;
            s->sub_screen = 4;
            s->bg3_vscroll = 14;
        } else if (enter >= 58) {
            s->main_screen = 23;
            s->sub_screen = 4;
            s->bg3_vscroll = 0;
            s->transition = NBA_SETUP_TRANSITION_NONE;
        }
    }
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
    s->page = NBA_SETUP_PAGE_MAIN;
    {
        memcpy(s->config.main_values, setup_main_defaults,
               sizeof(s->config.main_values));
        static const uint16_t default_rules[NBA_SETUP_RULE_COUNT] = {
            45, 45, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
        };
        static const uint16_t default_options[NBA_SETUP_OPTION_COUNT] = {
            30, 30, 2, 1, 0, 0, 0
        };
        memcpy(s->config.rules, default_rules, sizeof(default_rules));
        memcpy(s->config.options, default_options, sizeof(default_options));
    }
    const NbaAssetItem *rules_vram = nba_assets_get(assets, NBA_ASSET_SET_RULES_VRAM);
    const NbaAssetItem *rules_cgram = nba_assets_get(assets, NBA_ASSET_SET_RULES_CGRAM);
    const NbaAssetItem *options_vram = nba_assets_get(assets, NBA_ASSET_SET_OPTIONS_VRAM);
    const NbaAssetItem *options_cgram = nba_assets_get(assets, NBA_ASSET_SET_OPTIONS_CGRAM);
    const NbaAssetItem *rules_oam = nba_assets_get(assets, NBA_ASSET_SET_RULES_OAM);
    const NbaAssetItem *options_oam = nba_assets_get(assets, NBA_ASSET_SET_OPTIONS_OAM);
    const NbaAssetItem *options_off = nba_assets_get(assets, NBA_ASSET_OPTIONS_OFF_VRAM);
    const NbaAssetItem *options_mono = nba_assets_get(assets, NBA_ASSET_OPTIONS_MONO_VRAM);
    const NbaAssetItem *options_cpu = nba_assets_get(assets, NBA_ASSET_OPTIONS_CPU_VRAM);
    static const NbaAssetId main_variant_ids[10] = {
        NBA_ASSET_SETUP_MODE_SEASON_VRAM,
        NBA_ASSET_SETUP_MODE_PLAYOFFS_VRAM,
        NBA_ASSET_SETUP_MODE_LOAD_SERIES_VRAM,
        NBA_ASSET_SETUP_STYLE_CUSTOM_VRAM,
        NBA_ASSET_SETUP_STYLE_ARCADE_VRAM,
        NBA_ASSET_SETUP_LEVEL_STARTER_VRAM,
        NBA_ASSET_SETUP_LEVEL_ALL_STAR_VRAM,
        NBA_ASSET_SETUP_QUARTER_5_VRAM,
        NBA_ASSET_SETUP_QUARTER_8_VRAM,
        NBA_ASSET_SETUP_QUARTER_12_VRAM
    };
    if (rules_vram && rules_vram->size == 0x10000u) s->rules_vram = rules_vram->data;
    if (rules_cgram && rules_cgram->size == 0x200u) s->rules_cgram = rules_cgram->data;
    if (options_vram && options_vram->size == 0x10000u) s->options_vram = options_vram->data;
    if (options_cgram && options_cgram->size == 0x200u) s->options_cgram = options_cgram->data;
    if (rules_oam && rules_oam->size == 0x220u) s->rules_oam = rules_oam->data;
    if (options_oam && options_oam->size == 0x220u) s->options_oam = options_oam->data;
    if (options_off && options_off->size == 0x10000u) s->options_off_vram = options_off->data;
    if (options_mono && options_mono->size == 0x10000u) s->options_mono_vram = options_mono->data;
    if (options_cpu && options_cpu->size == 0x10000u) s->options_cpu_vram = options_cpu->data;
    if (s->has_gfx) {
        s->main_value_vram[0][0] = s->vram; /* Exhibition */
        s->main_value_vram[1][1] = s->vram; /* Simulation */
        s->main_value_vram[2][0] = s->vram; /* Rookie */
        s->main_value_vram[3][0] = s->vram; /* 3 Minutes */
    }
    for (int index = 0; index < 10; ++index) {
        const NbaAssetItem *item = nba_assets_get(assets, main_variant_ids[index]);
        const uint8_t *data = item && item->size == 0x10000u ? item->data : NULL;
        if (index < 3) s->main_value_vram[0][index + 1] = data;
        else if (index < 5) s->main_value_vram[1][index == 3 ? 2 : 0] = data;
        else if (index < 7) s->main_value_vram[2][index - 4] = data;
        else s->main_value_vram[3][index - 6] = data;
    }
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
NbaSetupSound nba_setup_screen_update(NbaSetupScreen *s, const NbaInput *input) {
    if (!s || !s->is_initialized) return NBA_SETUP_SOUND_NONE;

    s->frame++;

    if (s->frame < 0) return NBA_SETUP_SOUND_NONE;

    if (!nba_setup_screen_decode_ppu_to(s, s->frame)) {
        s->has_gfx = false;
        return NBA_SETUP_SOUND_NONE;
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

    if (!input) return NBA_SETUP_SOUND_NONE;

    if (s->transition != NBA_SETUP_TRANSITION_NONE) {
        setup_update_page_transition(s);
        return NBA_SETUP_SOUND_NONE;
    }

    if (s->page != NBA_SETUP_PAGE_MAIN) {
        int count = s->page == NBA_SETUP_PAGE_RULES ?
                    NBA_SETUP_RULE_COUNT : NBA_SETUP_OPTION_COUNT;
        if (input->pressed & NBA_BTN_UP) {
            s->menu_row = (s->menu_row + count - 1) % count;
        } else if (input->pressed & NBA_BTN_DOWN) {
            s->menu_row = (s->menu_row + 1) % count;
        } else if (input->pressed & (NBA_BTN_LEFT | NBA_BTN_RIGHT)) {
            uint16_t *values = s->page == NBA_SETUP_PAGE_RULES ?
                               s->working_rules : s->working_options;
            const uint16_t *maximums = s->page == NBA_SETUP_PAGE_RULES ?
                                       setup_rule_max : setup_option_max;
            uint16_t max = maximums[s->menu_row];
            bool changed = false;
            if (s->menu_row < 2) {
                /* Rules $81:D4A9-$D4B9/$81:D4FA-$D508 and Options
                 * $82:8E43-$8E54/$82:8E97-$8EA5 clamp bar values. */
                if ((input->pressed & NBA_BTN_LEFT) && values[s->menu_row] > 0u) {
                    values[s->menu_row]--;
                    changed = true;
                } else if ((input->pressed & NBA_BTN_RIGHT) &&
                           values[s->menu_row] < max) {
                    values[s->menu_row]++;
                    changed = true;
                }
            } else if (input->pressed & NBA_BTN_LEFT) {
                values[s->menu_row] = values[s->menu_row] == 0u ? max :
                                      (uint16_t)(values[s->menu_row] - 1u);
                changed = true;
            } else {
                values[s->menu_row] = values[s->menu_row] >= max ? 0u :
                                      (uint16_t)(values[s->menu_row] + 1u);
                changed = true;
            }
            if (changed) return NBA_SETUP_SOUND_ADJUST;
        } else if (input->pressed & NBA_BTN_START) {
            /* $81:D516 copies 26 bytes to $17D1; $82:8CD9/$8D0A copies
             * 14 bytes to $17B5. B is deliberately ignored by the ROM. */
            if (s->page == NBA_SETUP_PAGE_RULES) {
                memcpy(s->config.rules, s->working_rules, sizeof(s->config.rules));
            } else {
                memcpy(s->config.options, s->working_options, sizeof(s->config.options));
            }
            s->menu_row = 0;
            s->menu_scroll = 0;
            s->transition = NBA_SETUP_TRANSITION_RETURN;
            s->transition_target = NBA_SETUP_PAGE_MAIN;
            s->transition_frame = 0;
            return NBA_SETUP_SOUND_CONFIRM;
        }

        if (s->menu_row < s->menu_scroll) s->menu_scroll = s->menu_row;
        if (s->menu_row >= s->menu_scroll + 7) s->menu_scroll = s->menu_row - 6;
        if (input->pressed & (NBA_BTN_UP | NBA_BTN_DOWN))
            return NBA_SETUP_SOUND_MOVE;
        return NBA_SETUP_SOUND_NONE;
    }

    /* $80:A62D - row cursor wraps at both ends. */
    if (input->pressed & NBA_BTN_UP) {
        s->row = (NbaSetupRow)((s->row + NBA_SETUP_ROW_COUNT - 1) % NBA_SETUP_ROW_COUNT);
    }
    if (input->pressed & NBA_BTN_DOWN) {
        s->row = (NbaSetupRow)((s->row + 1) % NBA_SETUP_ROW_COUNT);
    }
    if ((input->pressed & (NBA_BTN_LEFT | NBA_BTN_RIGHT)) &&
        s->row < NBA_SETUP_ROW_RULES) {
        int row = (int)s->row;
        uint16_t old_value = s->config.main_values[row];
        uint16_t max = setup_main_max[row];
        uint16_t new_value = (input->pressed & NBA_BTN_LEFT) ?
            (old_value == 0u ? max : (uint16_t)(old_value - 1u)) :
            (old_value >= max ? 0u : (uint16_t)(old_value + 1u));
        /* The ROM redraws these values into BG3 at $7E:16FB + row*2.
         * Refuse a state whose captured game-authored glyph canvas is absent. */
        if (s->main_value_vram[row][new_value]) {
            s->config.main_values[row] = new_value;
            return NBA_SETUP_SOUND_ADJUST;
        }
    }
    if (input->pressed & NBA_BTN_A) {
        if (s->row == NBA_SETUP_ROW_RULES && setup_menu_assets_ready(s)) {
            memcpy(s->working_rules, s->config.rules, sizeof(s->working_rules));
            s->menu_row = s->menu_scroll = 0;
            s->transition = NBA_SETUP_TRANSITION_OPEN;
            s->transition_target = NBA_SETUP_PAGE_RULES;
            s->transition_frame = 0;
            return NBA_SETUP_SOUND_CONFIRM;
        }
        if (s->row == NBA_SETUP_ROW_OPTIONS && setup_menu_assets_ready(s)) {
            memcpy(s->working_options, s->config.options, sizeof(s->working_options));
            s->menu_row = s->menu_scroll = 0;
            s->transition = NBA_SETUP_TRANSITION_OPEN;
            s->transition_target = NBA_SETUP_PAGE_OPTIONS;
            s->transition_frame = 0;
            return NBA_SETUP_SOUND_CONFIRM;
        }
    }
    if (input->pressed & (NBA_BTN_UP | NBA_BTN_DOWN)) return NBA_SETUP_SOUND_MOVE;
    return NBA_SETUP_SOUND_NONE;
}

static void setup_restore_bg2_rect(const NbaSetupScreen *s, NbaRenderer *ren,
                                   const uint8_t *vram, const uint8_t *cgram,
                                   int x, int y, int w, int h) {
    for (int py = y; py < y + h && py < NBA_SNES_HEIGHT; ++py) {
        for (int px = x; px < x + w && px < NBA_SNES_WIDTH; ++px) {
            uint32_t out = nba_snes_cgram_color(cgram, 0, 15, 0, 0, 0);
            NbaSnesBgPixel pixel;
            if (nba_snes_sample_bg(vram, NBA_SETUP_BG2_TILEMAP, NBA_SETUP_BG2_CHR,
                                   4, true, false, 0, s->bg2_vscroll,
                                   px, py, &pixel)) {
                out = nba_snes_cgram_color(cgram,
                    pixel.palette * 16 + pixel.color_index, 15, 0, 0, 0);
            }
            ren->pixels[py * NBA_SNES_WIDTH + px] = out;
        }
    }
}

/* $80:A77C selects the active main-page value and the generic BG3 writer
 * stores it at $7E:16FB + row*2. Copy the resulting game-authored glyph
 * pixels from the corresponding captured VRAM state. */
static void setup_render_main_values(const NbaSetupScreen *s, NbaRenderer *ren) {
    if (!s || s->page != NBA_SETUP_PAGE_MAIN) return;
    for (int row = 0; row < NBA_SETUP_MAIN_VALUE_COUNT; ++row) {
        uint16_t value = s->config.main_values[row];
        if (value == setup_main_defaults[row] || value > setup_main_max[row]) continue;
        const uint8_t *source_vram = s->main_value_vram[row][value];
        if (!source_vram) continue;
        int top = nba_setup_screen_row_band_top((NbaSetupRow)row);
        setup_restore_bg2_rect(s, ren, s->vram, s->cgram, 138, top, 110, 16);
        for (int py = 0; py < 16; ++py) {
            for (int px = 0; px < 110; ++px) {
                NbaSnesBgPixel pixel;
                int x = 138 + px, y = top + py;
                if (!nba_snes_sample_bg(source_vram, NBA_SETUP_BG3_TILEMAP,
                                        NBA_SETUP_BG3_CHR, 2, false, true,
                                        0, 0, x, y, &pixel)) continue;
                bool highlighted = row == (int)s->row;
                ren->pixels[y * NBA_SNES_WIDTH + x] = nba_snes_cgram_color(
                    s->cgram, pixel.palette * 4 + pixel.color_index,
                    s->brightness,
                    highlighted ? NBA_SETUP_MATH_SUB_R : 0,
                    highlighted ? NBA_SETUP_MATH_SUB_G : 0,
                    highlighted ? NBA_SETUP_MATH_SUB_B : 0);
            }
        }
    }
}

static const char *setup_menu_value_text(NbaSetupPage page, int row,
                                         uint16_t value) {
    if (page == NBA_SETUP_PAGE_RULES) return value ? "ON" : "OFF";
    if (row == 2) {
        static const char *modes[] = { "OFF", "MONO", "STEREO" };
        return modes[value <= 2u ? value : 2u];
    }
    if (row == 5) return value ? "CPU" : "PLAYER";
    return value ? "ON" : "OFF";
}

/* $81:D675/$82:9028 draw the menu's proportional 2bpp glyphs into BG3.
 * Reuse those exact glyph pixels from the captured Options canvas for values
 * that already occur there, rather than substituting the port's debug font. */
static bool setup_copy_rom_value(const NbaSetupScreen *s, NbaRenderer *ren,
                                 const char *text, int dx, int dy,
                                 bool highlighted) {
    if (!s->options_vram || !s->options_cgram) return false;
    int sy;
    const uint8_t *source_vram = s->options_vram;
    if (strcmp(text, "STEREO") == 0) sy = 104;
    else if (strcmp(text, "ON") == 0) sy = 122;
    else if (strcmp(text, "OFF") == 0) {
        sy = 104;
        source_vram = s->options_off_vram;
    } else if (strcmp(text, "PLAYER") == 0) sy = 158;
    else if (strcmp(text, "MONO") == 0) {
        sy = 104;
        source_vram = s->options_mono_vram;
    } else if (strcmp(text, "CPU") == 0) {
        sy = 158;
        source_vram = s->options_cpu_vram;
    }
    else return false;
    if (!source_vram) return false;

    bool copied = false;
    for (int py = 0; py < 16; ++py) {
        for (int px = 0; px < 68; ++px) {
            NbaSnesBgPixel pixel;
            if (!nba_snes_sample_bg(source_vram, NBA_SETUP_BG3_TILEMAP,
                                    NBA_SETUP_BG3_CHR, 2, false, true, 0, 0,
                                    156 + px, sy + py, &pixel)) continue;
            int x = dx + px;
            int y = dy + py;
            if (x < 0 || x >= NBA_SNES_WIDTH || y < 0 || y >= NBA_SNES_HEIGHT)
                continue;
            ren->pixels[y * NBA_SNES_WIDTH + x] = nba_snes_cgram_color(
                s->options_cgram, pixel.palette * 4 + pixel.color_index,
                s->brightness,
                highlighted ? NBA_SETUP_MATH_SUB_R : 0,
                highlighted ? NBA_SETUP_MATH_SUB_G : 0,
                highlighted ? NBA_SETUP_MATH_SUB_B : 0);
            copied = true;
        }
    }
    return copied;
}

static int setup_obj_tile_pixel(const uint8_t *tile, int x, int y) {
    return ((tile[y * 2] >> (7 - x)) & 1) |
           (((tile[y * 2 + 1] >> (7 - x)) & 1) << 1) |
           (((tile[16 + y * 2] >> (7 - x)) & 1) << 2) |
           (((tile[17 + y * 2] >> (7 - x)) & 1) << 3);
}

static void setup_draw_oam_sprite(NbaRenderer *ren, const uint8_t *vram,
                                  const uint8_t *cgram, const uint8_t *oam,
                                  int index, int dx, int dy, bool flip_y,
                                  int clip_x, int clip_y, int clip_w, int clip_h) {
    int high = (oam[512 + index / 4] >> ((index & 3) * 2)) & 3;
    int x = oam[index * 4] | ((high & 1) << 8);
    if (x >= 256) x -= 512;
    int y = oam[index * 4 + 1];
    int tile = oam[index * 4 + 2];
    int attr = oam[index * 4 + 3];
    int size = (high & 2) ? 16 : 8;
    bool hflip = (attr & 0x40) != 0;
    bool vflip = ((attr & 0x80) != 0) ^ flip_y;
    int palette = (attr >> 1) & 7;
    tile += (attr & 1) ? 256 : 0;
    x += dx; y += dy;
    for (int py = 0; py < size; ++py) {
        int sy = vflip ? size - 1 - py : py;
        for (int px = 0; px < size; ++px) {
            int sx = hflip ? size - 1 - px : px;
            int subtile = tile + (sx >> 3) + (sy >> 3) * 16;
            /* Mesen reports OBSEL base $6000 in VRAM words; the byte-addressed
             * capture therefore stores OBJ CHR at $C000. */
            size_t offset = 0xC000u + (size_t)subtile * 32u;
            if (offset + 32u > 0x10000u) continue;
            int color = setup_obj_tile_pixel(vram + offset, sx & 7, sy & 7);
            int ox = x + px, oy = y + py;
            bool in_clip = clip_w <= 0 ||
                           (ox >= clip_x && ox < clip_x + clip_w &&
                            oy >= clip_y && oy < clip_y + clip_h);
            if (color && in_clip && ox >= 0 && ox < NBA_SNES_WIDTH &&
                oy >= 0 && oy < NBA_SNES_HEIGHT)
                ren->pixels[oy * NBA_SNES_WIDTH + ox] =
                    nba_snes_cgram_color(cgram, 128 + palette * 16 + color,
                                         15, 0, 0, 0);
        }
    }
}

static void setup_draw_rom_menu_objects(const NbaSetupScreen *s,
                                        NbaRenderer *ren) {
    if (!s->rules_oam || !s->rules_vram || !s->rules_cgram) return;
    int dx = s->page == NBA_SETUP_PAGE_OPTIONS ? 16 : 0;
    int dy = s->page == NBA_SETUP_PAGE_OPTIONS ? -8 : 0;
    for (int index = 23; index >= 0; --index) {
        int bar = index >= 12 ? 1 : 0;
        setup_draw_oam_sprite(ren, s->rules_vram, s->rules_cgram,
                              s->rules_oam, index, dx, dy, false,
                              (s->page == NBA_SETUP_PAGE_RULES ? 144 : 160),
                              (s->page == NBA_SETUP_PAGE_RULES ? 82 : 74) + bar * 18,
                              48, 8);
    }
    const uint16_t *values = s->page == NBA_SETUP_PAGE_RULES ?
                             s->working_rules : s->working_options;
    int bar_x = s->page == NBA_SETUP_PAGE_RULES ? 144 : 160;
    int bar_y = s->page == NBA_SETUP_PAGE_RULES ? 82 : 74;
    for (int bar = 0; bar < 2; ++bar) {
        for (int py = 1; py < 7; ++py)
            for (int px = values[bar] + 2; px < 47; ++px)
                ren->pixels[(bar_y + bar * 18 + py) * NBA_SNES_WIDTH + bar_x + px] =
                    0xFF000000u;
    }
    if (s->page == NBA_SETUP_PAGE_RULES) {
        if (s->menu_scroll > 0)
            setup_draw_oam_sprite(ren, s->rules_vram, s->rules_cgram,
                                  s->rules_oam, 24, 0, -103, true, 0, 0, 0, 0);
        if (s->menu_scroll + 7 < NBA_SETUP_RULE_COUNT)
            setup_draw_oam_sprite(ren, s->rules_vram, s->rules_cgram,
                                  s->rules_oam, 24, 0, 0, false, 0, 0, 0, 0);
    }
}

static void setup_render_menu_values(const NbaSetupScreen *s, NbaRenderer *ren,
                                     const uint8_t *vram, const uint8_t *cgram) {
    if (s->page == NBA_SETUP_PAGE_MAIN) return;
    const uint16_t *values = s->page == NBA_SETUP_PAGE_RULES ?
                             s->working_rules : s->working_options;
    int count = s->page == NBA_SETUP_PAGE_RULES ?
                NBA_SETUP_RULE_COUNT : NBA_SETUP_OPTION_COUNT;
    static const uint16_t default_rules[NBA_SETUP_RULE_COUNT] = {
        45, 45, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };
    static const uint16_t default_options[NBA_SETUP_OPTION_COUNT] = {
        30, 30, 2, 1, 0, 0, 0
    };
    const uint16_t *defaults = s->page == NBA_SETUP_PAGE_RULES ?
                               default_rules : default_options;

    if (s->menu_scroll > 0)
        setup_restore_bg2_rect(s, ren, vram, cgram, 16, 70, 212, 10);

    for (int visible = 0; visible < 7; ++visible) {
        int row = s->menu_scroll + visible;
        if (row >= count) break;
        int top = (s->page == NBA_SETUP_PAGE_RULES ? 76 : 68) + visible * 18;
        if (row < 2 || values[row] != defaults[row]) {
            setup_restore_bg2_rect(s, ren, vram, cgram, 140, top, 82, 16);
            if (row < 2) {
                /* $81:D675/$82:9028 populate OAM; rendered after BG values. */
            } else {
                const char *text = setup_menu_value_text(s->page, row, values[row]);
                int value_x = s->page == NBA_SETUP_PAGE_RULES ? 140 : 156;
                (void)setup_copy_rom_value(s, ren, text, value_x, top,
                                           row == s->menu_row);
            }
        }
    }
    setup_draw_rom_menu_objects(s, ren);
}

/**
 * Offset/Address/Size: 0x00A3B8 | $80:A3B8 | size: 0xAD
 * Purpose: Composites the screen the way the PPU does in BG Mode 1 - BG2
 *          backdrop, then BG1 header banner, then the BG3 text canvas -
 *          honouring the main-screen designation and INIDISP brightness.
 */
void nba_setup_screen_render(const NbaSetupScreen *s, NbaRenderer *ren) {
    if (!s || !ren) return;

    if (!s->has_gfx || s->frame < 0 || s->transition_blank) {
        nba_renderer_clear(ren, 0xFF000000u);
        return;
    }

    const uint8_t *vram = s->vram;
    const uint8_t *cgram = s->cgram;
    if (s->page == NBA_SETUP_PAGE_RULES) {
        vram = s->rules_vram;
        cgram = s->rules_cgram;
    } else if (s->page == NBA_SETUP_PAGE_OPTIONS) {
        vram = s->options_vram;
        cgram = s->options_cgram;
    }
    uint32_t backdrop = nba_snes_cgram_color(cgram, 0, s->brightness, 0, 0, 0);

    /* HDMA ch7 opens window 1 over the active row only; colour math is enabled
     * for BG3 alone ($2131 = 4), so just this band of BG3 pixels is subtracted. */
    int band_top = s->page == NBA_SETUP_PAGE_MAIN ?
                   nba_setup_screen_row_band_top(s->row) :
                   (s->page == NBA_SETUP_PAGE_RULES ? 76 : 68) +
                   (s->menu_row - s->menu_scroll) * 18;
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

            if ((s->main_screen | s->sub_screen) & 0x04) {
                int bg3_scroll = s->bg3_vscroll;
                if (s->page == NBA_SETUP_PAGE_RULES && s->menu_scroll > 0 && y >= 70)
                    bg3_scroll += s->menu_scroll * NBA_SETUP_ROW_PITCH;
                if (nba_snes_sample_bg(vram, NBA_SETUP_BG3_TILEMAP, NBA_SETUP_BG3_CHR,
                                       2, false, true, 0, bg3_scroll,
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
    if (s->transition == NBA_SETUP_TRANSITION_NONE) {
        setup_render_main_values(s, ren);
        setup_render_menu_values(s, ren, vram, cgram);
    }
}
