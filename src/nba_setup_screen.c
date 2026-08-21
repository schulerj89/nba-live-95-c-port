#include "nba_setup_screen.h"
#include "nba_snes_ppu.h"
#include <stdio.h>
#include <string.h>

#define SETUP_PPU_MAGIC "NBSPPU1\0"
#define SETUP_TRANSITION_PPU_MAGIC "NBSPPU2\0"
#define SETUP_PPU_HEADER_SIZE 16
#define SETUP_TRANSITION_STATE_SIZE 34

static const uint16_t setup_rule_max[NBA_SETUP_RULE_COUNT] = {
    45, 45, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static const uint16_t setup_option_max[NBA_SETUP_OPTION_COUNT] = {
    45, 45, 2, 1, 1, 1, 1
};
static const uint16_t setup_main_max[NBA_SETUP_MAIN_VALUE_COUNT] = { 3, 2, 2, 3 };

static NbaSetupUpdateResult setup_result(NbaSetupSound sound,
                                         NbaSetupAction action) {
    NbaSetupUpdateResult result = { sound, action };
    return result;
}

/* $81:9756/$81:9FD4 write proportional 2bpp glyphs into a mutable BG3
 * canvas, and $81:A1EE uploads the complete result.  The asset pack carries
 * independently captured ROM results for each non-default Options value.
 * Applying only bytes changed from the default canvas reproduces the same
 * row-local glyph+shadow writes while allowing multiple values to coexist. */
static void setup_apply_rom_text_delta(uint8_t *canvas, const uint8_t *base,
                                       const uint8_t *variant) {
    if (!canvas || !base || !variant) return;
    for (size_t i = 0; i < 0x10000u; ++i)
        if (variant[i] != base[i]) canvas[i] = variant[i];
}

static bool setup_rebuild_options_text_canvas(NbaSetupScreen *s) {
    if (!s || !s->options_vram || !s->options_off_vram ||
        !s->options_mono_vram || !s->options_crowd_off_vram ||
        !s->options_slow_on_vram || !s->options_cpu_vram ||
        !s->options_assistance_on_vram) return false;

    memcpy(s->options_text_vram, s->options_vram,
           sizeof(s->options_text_vram));
    if (s->working_options[2] == 0u)
        setup_apply_rom_text_delta(s->options_text_vram, s->options_vram,
                                   s->options_off_vram);
    else if (s->working_options[2] == 1u)
        setup_apply_rom_text_delta(s->options_text_vram, s->options_vram,
                                   s->options_mono_vram);
    if (s->working_options[3] == 0u)
        setup_apply_rom_text_delta(s->options_text_vram, s->options_vram,
                                   s->options_crowd_off_vram);
    if (s->working_options[4] != 0u)
        setup_apply_rom_text_delta(s->options_text_vram, s->options_vram,
                                   s->options_slow_on_vram);
    if (s->working_options[5] != 0u)
        setup_apply_rom_text_delta(s->options_text_vram, s->options_vram,
                                   s->options_cpu_vram);
    if (s->working_options[6] != 0u)
        setup_apply_rom_text_delta(s->options_text_vram, s->options_vram,
                                   s->options_assistance_on_vram);
    return true;
}

static bool setup_menu_assets_ready(const NbaSetupScreen *s) {
    return s && s->rules_vram && s->rules_cgram && s->rules_oam &&
           s->options_vram && s->options_cgram && s->options_oam &&
           s->options_off_vram && s->options_mono_vram &&
           s->options_cpu_vram && s->options_crowd_off_vram &&
           s->options_slow_on_vram && s->options_assistance_on_vram &&
           s->rules_open_vram &&
           s->rules_open_cgram && s->rules_open_trace &&
           s->options_open_vram && s->options_open_cgram &&
           s->options_open_trace && s->return_vram &&
           s->return_cgram && s->return_trace && s->rules_return_vram &&
           s->rules_return_cgram && s->rules_return_trace;
}

static uint16_t setup_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t setup_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool setup_decode_ppu_to(uint8_t *vram, uint8_t *cgram,
                                const uint8_t *trace, size_t trace_size,
                                size_t *trace_offset, int *decoded_frame,
                                int target);
static bool setup_decode_transition_to(NbaSetupScreen *s, int target);
static bool setup_transition_trace_complete(const NbaSetupScreen *s);

typedef enum {
    SETUP_REVEAL_RULES,
    SETUP_REVEAL_OPTIONS,
    SETUP_REVEAL_MAIN
} SetupTransitionReveal;

typedef struct {
    NbaSetupTransitionRoute route;
    NbaSetupPage source;
    NbaSetupPage target;
    NbaSetupTransition direction;
    int bg3_last_scroll;
    int slide_start;
    int target_switch;
    int entrance_start;
    int trace_frames;
    int forced_blank_start;
    int forced_blank_end;
    int construction_guard_start;
    int construction_guard_end;
    int bg3_scanout_delay_frames;
    SetupTransitionReveal reveal;
} SetupTransitionProfile;

/* Ghidra: $81:D318 (Rules) and $82:8CD1 (Options) enter the common
 * $80:A2BF/$80:A3B8 builder/sequencer.  These profiles retain the
 * screen-edge-specific cadence observed in the corresponding PPU traces. */
static const SetupTransitionProfile setup_transition_profiles[] = {
    { NBA_SETUP_TRANSITION_MAIN_TO_RULES, NBA_SETUP_PAGE_MAIN,
      NBA_SETUP_PAGE_RULES, NBA_SETUP_TRANSITION_OPEN,
      15, 21, 52, 76, 146, 51, 81, 0, 0, 1, SETUP_REVEAL_RULES },
    { NBA_SETUP_TRANSITION_MAIN_TO_OPTIONS, NBA_SETUP_PAGE_MAIN,
      NBA_SETUP_PAGE_OPTIONS, NBA_SETUP_TRANSITION_OPEN,
      15, 21, 52, 72, 132, 51, 77, 0, 0, 0, SETUP_REVEAL_OPTIONS },
    { NBA_SETUP_TRANSITION_RULES_TO_MAIN, NBA_SETUP_PAGE_RULES,
      NBA_SETUP_PAGE_MAIN, NBA_SETUP_TRANSITION_RETURN,
      16, 22, 53, 74, 132, 36, 63, 4, 6, 0, SETUP_REVEAL_MAIN },
    { NBA_SETUP_TRANSITION_OPTIONS_TO_MAIN, NBA_SETUP_PAGE_OPTIONS,
      NBA_SETUP_PAGE_MAIN, NBA_SETUP_TRANSITION_RETURN,
      16, 22, 53, 74, 132, 52, 79, 20, 23, 0, SETUP_REVEAL_MAIN }
};

static const SetupTransitionProfile *setup_transition_profile_for_route(
    NbaSetupTransitionRoute route) {
    size_t count = sizeof(setup_transition_profiles) /
                   sizeof(setup_transition_profiles[0]);
    for (size_t i = 0; i < count; ++i)
        if (setup_transition_profiles[i].route == route)
            return &setup_transition_profiles[i];
    return NULL;
}

static const SetupTransitionProfile *setup_transition_profile_for_edge(
    NbaSetupPage source, NbaSetupPage target) {
    size_t count = sizeof(setup_transition_profiles) /
                   sizeof(setup_transition_profiles[0]);
    for (size_t i = 0; i < count; ++i)
        if (setup_transition_profiles[i].source == source &&
            setup_transition_profiles[i].target == target)
            return &setup_transition_profiles[i];
    return NULL;
}

static void setup_finish_page_transition(NbaSetupScreen *s) {
    s->transition = NBA_SETUP_TRANSITION_NONE;
    s->transition_route = NBA_SETUP_TRANSITION_ROUTE_NONE;
    s->transition_release_pending = false;
}

/* $80:A3B8 owns one shared three-frame BG2 cadence across the settled page
 * and its transition.  Keep the phase produced by the transition instead of
 * deriving a new, unrelated position from the lifetime Setup frame counter. */
static void setup_advance_steady_bg2(NbaSetupScreen *s) {
    if (++s->bg2_scroll_hold_frames >= NBA_SETUP_SCROLL_PERIOD) {
        s->bg2_vscroll = (s->bg2_vscroll + 1) & 0x3FF;
        s->bg2_scroll_hold_frames = 0;
    }
}

static void setup_release_page_transition(NbaSetupScreen *s) {
    s->bg2_scroll_from_transition = true;
    setup_advance_steady_bg2(s);
    setup_finish_page_transition(s);
}

/* $80:A2BF/$80:A3B8 are the shared layer builder/frame sequencer reached by
 * both $81:D318 (Rules) and $82:8CD1 (Options).  The values below are the
 * complete $2100/$212C/$212D/$210D-$2112 trace around A/Start, captured by
 * tools/mesen_setup_menus_capture.lua rather than an invented host tween. */
static void setup_update_page_transition(NbaSetupScreen *s) {
    const SetupTransitionProfile *profile =
        setup_transition_profile_for_route(s->transition_route);
    if (!profile) {
        setup_finish_page_transition(s);
        return;
    }
    int t = ++s->transition_frame;
    int trace_start = 1;

    if (t >= trace_start && s->active_transition_trace) {
        if (t >= profile->target_switch && s->page != profile->target)
            s->page = profile->target;
        /* Mesen's screenBrightness property omits INIDISP bit 7.  The ROM
         * asserts forced blank after the 30-frame exit slide while $80:A2BF
         * rebuilds VRAM, then releases it at this edge's entrance frame. */
        s->transition_blank = t >= profile->forced_blank_start &&
                              t < profile->forced_blank_end;
        if (!setup_decode_transition_to(s, t - trace_start)) {
            s->has_gfx = false;
            return;
        }
        /* The packed trace contains the absolute BG2 position from the Mesen
         * recording.  On real hardware $80:A3B8 continues whatever position
         * the live page had during the visible exit, then resets BG2 only
         * inside $80:A2BF's forced-blank rebuild.  Rebase that visible prefix
         * to this run's current position; use the ROM values unchanged once
         * the rebuild is hidden. */
        int raw_bg2_vscroll = s->bg2_vscroll;
        if (!s->transition_bg2_origin_valid) {
            s->transition_bg2_trace_origin = raw_bg2_vscroll;
            s->transition_bg2_last_raw = raw_bg2_vscroll;
            s->bg2_scroll_hold_frames = 0;
            s->transition_bg2_origin_valid = true;
        } else if (raw_bg2_vscroll == s->transition_bg2_last_raw) {
            s->bg2_scroll_hold_frames++;
        } else {
            s->transition_bg2_last_raw = raw_bg2_vscroll;
            s->bg2_scroll_hold_frames = 0;
        }
        if (t < profile->forced_blank_start) {
            s->bg2_vscroll = (raw_bg2_vscroll +
                              s->transition_bg2_start_vscroll -
                              s->transition_bg2_trace_origin) & 0x3FF;
        }
        /* Keep the route alive through this render.  Clearing it here made
         * the renderer pair the final trace registers with the settled-page
         * VRAM path for one frame, dropping the header/text/OBJ layer. */
        if (setup_transition_trace_complete(s))
            s->transition_release_pending = true;
        return;
    }

    s->transition_blank = false;
    if (t < profile->slide_start) {
        s->brightness = 15;
        s->sub_screen = 4;
        if (t == 1) s->main_screen = 7;
        else if (t <= profile->bg3_last_scroll) {
            s->main_screen = 19;
            s->bg3_vscroll = t >= 3 ? (t - 2) * 14 : 0;
        } else {
            s->main_screen = 3;
            s->bg3_vscroll = profile->direction == NBA_SETUP_TRANSITION_OPEN ?
                                 182 : 196;
        }
        return;
    }

    int slide = t - profile->slide_start;
    if (slide >= 0 && slide <= 30) {
        s->main_screen = 3;
        s->sub_screen = slide == 30 ? 0 : 4;
        s->bg1_hscroll = slide * 8;
        s->bg2_hscroll = (-slide * 8) & 0x3FF;
        s->brightness = slide < 3 ? 15 :
                        (slide == 30 ? 15 : 15 - (slide - 1) / 2);
        return;
    }

    if (t < profile->entrance_start) {
        if (s->page != profile->target) s->page = profile->target;
        s->transition_blank = true;
        s->brightness = 0;
        s->main_screen = 3;
        s->sub_screen = 0;
        return;
    }

    if (s->page != profile->target) s->page = profile->target;
    int enter = t - profile->entrance_start;
    s->brightness = enter < 14 ? enter + 1 : 15;
    s->bg1_hscroll = enter <= 32 ? 768 - enter * 8 : 512;
    s->bg2_hscroll = enter <= 32 ? (768 + enter * 8) & 0x3FF : 0;
    s->bg2_vscroll = enter / 3 - 1;
    s->main_screen = 3;
    s->sub_screen = 0;
    s->bg3_vscroll = 280;

    if (profile->reveal != SETUP_REVEAL_MAIN) {
        if (profile->reveal == SETUP_REVEAL_RULES) {
            if (enter == 43) {
                s->main_screen = 23;
                s->bg3_vscroll = 252;
            } else if (enter >= 44 && enter <= 61) {
                s->main_screen = 19;
                s->bg3_vscroll = 252 - (enter - 44) * 14;
            } else if (enter >= 62) {
                s->bg3_vscroll = 0;
                s->sub_screen = enter >= 64 ? 4 : 0;
                if (enter >= 64 && setup_transition_trace_complete(s))
                    s->transition_release_pending = true;
            }
        } else {
            if (enter == 41) {
                s->bg3_vscroll = 252;
            } else if (enter == 42) {
                s->main_screen = 23;
                s->bg3_vscroll = 252;
            } else if (enter >= 43 && enter <= 58) {
                s->main_screen = 19;
                s->bg3_vscroll = 238 - (enter - 43) * 14;
            } else if (enter == 59) {
                s->main_screen = 23;
                s->sub_screen = 4;
                s->bg3_vscroll = 14;
            } else if (enter >= 60) {
                s->main_screen = 23;
                s->bg3_vscroll = 0;
                s->sub_screen = 4;
                if (setup_transition_trace_complete(s))
                    s->transition_release_pending = true;
            }
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
            if (setup_transition_trace_complete(s))
                s->transition_release_pending = true;
        }
    }
}

static bool setup_decode_ppu_to(uint8_t *vram, uint8_t *cgram,
                                const uint8_t *trace, size_t trace_size,
                                size_t *trace_offset, int *decoded_frame,
                                int target) {
    if (!vram || !cgram || !trace || !trace_offset || !decoded_frame ||
        trace_size < SETUP_PPU_HEADER_SIZE ||
        memcmp(trace, SETUP_PPU_MAGIC, 8) != 0 ||
        setup_u32(trace + 8) != 1) return false;
    uint32_t frame_count = setup_u32(trace + 12);
    if (frame_count == 0 || frame_count > 300) return false;
    if (target >= (int)frame_count) target = (int)frame_count - 1;

    while (*decoded_frame < target) {
        size_t off = *trace_offset;
        if (off + 4 > trace_size) return false;
        uint16_t vram_count = setup_u16(trace + off);
        uint16_t cgram_count = setup_u16(trace + off + 2);
        off += 4;
        if (off + ((size_t)vram_count + cgram_count) * 3u > trace_size)
            return false;
        for (uint16_t i = 0; i < vram_count; ++i) {
            uint16_t address = setup_u16(trace + off);
            vram[address] = trace[off + 2];
            off += 3;
        }
        for (uint16_t i = 0; i < cgram_count; ++i) {
            uint16_t address = setup_u16(trace + off);
            if (address < 0x200u) cgram[address] = trace[off + 2];
            off += 3;
        }
        *trace_offset = off;
        (*decoded_frame)++;
    }
    return true;
}

static bool nba_setup_screen_decode_ppu_to(NbaSetupScreen *s, int target) {
    return setup_decode_ppu_to(s->vram, s->cgram,
                               s->ppu_trace, s->ppu_trace_size,
                               &s->ppu_trace_offset, &s->ppu_decoded_frame,
                               target);
}

static bool setup_decode_transition_to(NbaSetupScreen *s, int target) {
    const uint8_t *trace = s->active_transition_trace;
    if (!trace || s->active_transition_trace_size < SETUP_PPU_HEADER_SIZE ||
        memcmp(trace, SETUP_TRANSITION_PPU_MAGIC, 8) != 0 ||
        setup_u32(trace + 8) != 2) return false;
    uint32_t frame_count = setup_u32(trace + 12);
    if (frame_count == 0 || frame_count > 300) return false;
    if (target >= (int)frame_count) target = (int)frame_count - 1;

    while (s->active_transition_decoded_frame < target) {
        size_t off = s->active_transition_trace_offset;
        if (off + SETUP_TRANSITION_STATE_SIZE + 4u >
            s->active_transition_trace_size) return false;
        s->brightness = trace[off];
        s->main_screen = trace[off + 1];
        s->sub_screen = trace[off + 2];
        off += 4;
        for (int layer = 0; layer < 3; ++layer) {
            int hscroll = setup_u16(trace + off);
            int vscroll = setup_u16(trace + off + 2);
            s->layer_tilemap[layer] = setup_u16(trace + off + 4);
            s->layer_chr[layer] = setup_u16(trace + off + 6);
            s->layer_double_width[layer] = trace[off + 8] != 0;
            s->layer_double_height[layer] = trace[off + 9] != 0;
            if (layer == 0) {
                s->bg1_hscroll = hscroll;
                s->bg1_vscroll = vscroll;
            } else if (layer == 1) {
                s->bg2_hscroll = hscroll;
                s->bg2_vscroll = vscroll;
            } else {
                s->bg3_hscroll = hscroll;
                s->bg3_vscroll = vscroll;
            }
            off += 10;
        }
        uint16_t vram_count = setup_u16(trace + off);
        uint16_t cgram_count = setup_u16(trace + off + 2);
        off += 4;
        if (off + ((size_t)vram_count + cgram_count) * 3u >
            s->active_transition_trace_size) return false;
        for (uint16_t i = 0; i < vram_count; ++i) {
            uint16_t address = setup_u16(trace + off);
            s->transition_vram[address] = trace[off + 2];
            off += 3;
        }
        for (uint16_t i = 0; i < cgram_count; ++i) {
            uint16_t address = setup_u16(trace + off);
            if (address < sizeof(s->transition_cgram))
                s->transition_cgram[address] = trace[off + 2];
            off += 3;
        }
        s->active_transition_trace_offset = off;
        s->active_transition_decoded_frame++;
    }
    return true;
}

static bool setup_transition_trace_complete(const NbaSetupScreen *s) {
    return !s->active_transition_trace ||
           s->active_transition_decoded_frame + 1 >=
               s->active_transition_frame_count;
}

static bool setup_begin_page_transition(NbaSetupScreen *s,
                                        NbaSetupPage target) {
    const SetupTransitionProfile *profile =
        setup_transition_profile_for_edge(s->page, target);
    const uint8_t *vram = NULL, *cgram = NULL, *trace = NULL;
    size_t trace_size = 0;
    if (!profile) return false;
    if (profile->route == NBA_SETUP_TRANSITION_RULES_TO_MAIN) {
        vram = s->rules_return_vram;
        cgram = s->rules_return_cgram;
        trace = s->rules_return_trace;
        trace_size = s->rules_return_trace_size;
    } else if (profile->route == NBA_SETUP_TRANSITION_OPTIONS_TO_MAIN) {
        vram = s->return_vram;
        cgram = s->return_cgram;
        trace = s->return_trace;
        trace_size = s->return_trace_size;
    } else if (profile->route == NBA_SETUP_TRANSITION_MAIN_TO_RULES) {
        vram = s->rules_open_vram;
        cgram = s->rules_open_cgram;
        trace = s->rules_open_trace;
        trace_size = s->rules_open_trace_size;
    } else if (profile->route == NBA_SETUP_TRANSITION_MAIN_TO_OPTIONS) {
        vram = s->options_open_vram;
        cgram = s->options_open_cgram;
        trace = s->options_open_trace;
        trace_size = s->options_open_trace_size;
    }
    if (!vram || !cgram || !trace || trace_size < SETUP_PPU_HEADER_SIZE ||
        memcmp(trace, SETUP_TRANSITION_PPU_MAGIC, 8) != 0 ||
        setup_u32(trace + 8) != 2)
        return false;
    uint32_t frames = setup_u32(trace + 12);
    if (frames != (uint32_t)profile->trace_frames) return false;
    memcpy(s->transition_vram, vram, sizeof(s->transition_vram));
    memcpy(s->transition_cgram, cgram, sizeof(s->transition_cgram));
    s->active_transition_base_vram = vram;
    s->active_transition_base_cgram = cgram;
    memcpy(s->transition_base_tilemap, s->layer_tilemap,
           sizeof(s->transition_base_tilemap));
    memcpy(s->transition_base_chr, s->layer_chr,
           sizeof(s->transition_base_chr));
    memcpy(s->transition_base_double_width, s->layer_double_width,
           sizeof(s->transition_base_double_width));
    memcpy(s->transition_base_double_height, s->layer_double_height,
           sizeof(s->transition_base_double_height));
    s->active_transition_trace = trace;
    s->active_transition_trace_size = trace_size;
    s->active_transition_trace_offset = SETUP_PPU_HEADER_SIZE;
    s->active_transition_decoded_frame = -1;
    s->active_transition_frame_count = (int)frames;
    s->transition = profile->direction;
    s->transition_route = profile->route;
    s->transition_target = profile->target;
    s->transition_frame = 0;
    s->transition_blank = false;
    s->transition_release_pending = false;
    s->transition_bg2_start_vscroll = s->bg2_vscroll;
    s->transition_bg2_trace_origin = 0;
    s->transition_bg2_last_raw = 0;
    s->transition_bg2_origin_valid = false;
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
void nba_setup_screen_init(NbaSetupScreen *s, const NbaAssetPack *assets,
                           NbaGameConfig *config) {
    if (!s || !config) return;
    memset(s, 0, sizeof(*s));
    s->config = config;

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
    s->bg1_vscroll = NBA_SETUP_BG1_VSCROLL;
    s->bg2_hscroll = NBA_SETUP_ENTER_BG2_START;
    s->bg2_vscroll = -1;
    s->bg3_vscroll = 280;
    s->layer_tilemap[0] = NBA_SETUP_BG1_TILEMAP;
    s->layer_chr[0] = NBA_SETUP_BG1_CHR;
    s->layer_double_width[0] = true;
    s->layer_tilemap[1] = NBA_SETUP_BG2_TILEMAP;
    s->layer_chr[1] = NBA_SETUP_BG2_CHR;
    s->layer_double_width[1] = true;
    s->layer_tilemap[2] = NBA_SETUP_BG3_TILEMAP;
    s->layer_chr[2] = NBA_SETUP_BG3_CHR;
    s->layer_double_height[2] = true;
    s->brightness = 0;
    s->main_screen = NBA_SETUP_MAIN_ENTER;
    s->sub_screen = 0;
    s->row = NBA_SETUP_ROW_MODE;
    s->page = NBA_SETUP_PAGE_MAIN;
    const NbaAssetItem *rules_vram = nba_assets_get(assets, NBA_ASSET_SET_RULES_VRAM);
    const NbaAssetItem *rules_cgram = nba_assets_get(assets, NBA_ASSET_SET_RULES_CGRAM);
    const NbaAssetItem *options_vram = nba_assets_get(assets, NBA_ASSET_SET_OPTIONS_VRAM);
    const NbaAssetItem *options_cgram = nba_assets_get(assets, NBA_ASSET_SET_OPTIONS_CGRAM);
    const NbaAssetItem *rules_oam = nba_assets_get(assets, NBA_ASSET_SET_RULES_OAM);
    const NbaAssetItem *options_oam = nba_assets_get(assets, NBA_ASSET_SET_OPTIONS_OAM);
    const NbaAssetItem *options_off = nba_assets_get(assets, NBA_ASSET_OPTIONS_OFF_VRAM);
    const NbaAssetItem *options_mono = nba_assets_get(assets, NBA_ASSET_OPTIONS_MONO_VRAM);
    const NbaAssetItem *options_cpu = nba_assets_get(assets, NBA_ASSET_OPTIONS_CPU_VRAM);
    const NbaAssetItem *options_crowd_off = nba_assets_get(
        assets, NBA_ASSET_OPTIONS_CROWD_OFF_VRAM);
    const NbaAssetItem *options_slow_on = nba_assets_get(
        assets, NBA_ASSET_OPTIONS_SLOW_ON_VRAM);
    const NbaAssetItem *options_assistance_on = nba_assets_get(
        assets, NBA_ASSET_OPTIONS_ASSISTANCE_ON_VRAM);
    const NbaAssetItem *rules_open_vram = nba_assets_get(assets, NBA_ASSET_RULES_OPEN_VRAM);
    const NbaAssetItem *rules_open_cgram = nba_assets_get(assets, NBA_ASSET_RULES_OPEN_CGRAM);
    const NbaAssetItem *rules_open_trace = nba_assets_get(assets, NBA_ASSET_RULES_OPEN_PPU_TRACE);
    const NbaAssetItem *options_open_vram = nba_assets_get(assets, NBA_ASSET_OPTIONS_OPEN_VRAM);
    const NbaAssetItem *options_open_cgram = nba_assets_get(assets, NBA_ASSET_OPTIONS_OPEN_CGRAM);
    const NbaAssetItem *options_open_trace = nba_assets_get(assets, NBA_ASSET_OPTIONS_OPEN_PPU_TRACE);
    const NbaAssetItem *return_vram = nba_assets_get(assets, NBA_ASSET_SETUP_RETURN_VRAM);
    const NbaAssetItem *return_cgram = nba_assets_get(assets, NBA_ASSET_SETUP_RETURN_CGRAM);
    const NbaAssetItem *return_trace = nba_assets_get(assets, NBA_ASSET_SETUP_RETURN_PPU_TRACE);
    const NbaAssetItem *rules_return_vram = nba_assets_get(assets, NBA_ASSET_RULES_RETURN_VRAM);
    const NbaAssetItem *rules_return_cgram = nba_assets_get(assets, NBA_ASSET_RULES_RETURN_CGRAM);
    const NbaAssetItem *rules_return_trace = nba_assets_get(assets, NBA_ASSET_RULES_RETURN_PPU_TRACE);
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
    if (options_crowd_off && options_crowd_off->size == 0x10000u)
        s->options_crowd_off_vram = options_crowd_off->data;
    if (options_slow_on && options_slow_on->size == 0x10000u)
        s->options_slow_on_vram = options_slow_on->data;
    if (options_assistance_on && options_assistance_on->size == 0x10000u)
        s->options_assistance_on_vram = options_assistance_on->data;
    if (rules_open_vram && rules_open_vram->size == 0x10000u)
        s->rules_open_vram = rules_open_vram->data;
    if (rules_open_cgram && rules_open_cgram->size == 0x200u)
        s->rules_open_cgram = rules_open_cgram->data;
    if (rules_open_trace && rules_open_trace->size >= SETUP_PPU_HEADER_SIZE) {
        s->rules_open_trace = rules_open_trace->data;
        s->rules_open_trace_size = rules_open_trace->size;
    }
    if (options_open_vram && options_open_vram->size == 0x10000u)
        s->options_open_vram = options_open_vram->data;
    if (options_open_cgram && options_open_cgram->size == 0x200u)
        s->options_open_cgram = options_open_cgram->data;
    if (options_open_trace && options_open_trace->size >= SETUP_PPU_HEADER_SIZE) {
        s->options_open_trace = options_open_trace->data;
        s->options_open_trace_size = options_open_trace->size;
    }
    if (return_vram && return_vram->size == 0x10000u)
        s->return_vram = return_vram->data;
    if (return_cgram && return_cgram->size == 0x200u)
        s->return_cgram = return_cgram->data;
    if (return_trace && return_trace->size >= SETUP_PPU_HEADER_SIZE) {
        s->return_trace = return_trace->data;
        s->return_trace_size = return_trace->size;
    }
    if (rules_return_vram && rules_return_vram->size == 0x10000u)
        s->rules_return_vram = rules_return_vram->data;
    if (rules_return_cgram && rules_return_cgram->size == 0x200u)
        s->rules_return_cgram = rules_return_cgram->data;
    if (rules_return_trace && rules_return_trace->size >= SETUP_PPU_HEADER_SIZE) {
        s->rules_return_trace = rules_return_trace->data;
        s->rules_return_trace_size = rules_return_trace->size;
    }
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
    memcpy(s->working_options, config->options, sizeof(s->working_options));
    (void)setup_rebuild_options_text_canvas(s);
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
NbaSetupUpdateResult nba_setup_screen_update(NbaSetupScreen *s,
                                             const NbaInput *input) {
    if (!s || !s->is_initialized)
        return setup_result(NBA_SETUP_SOUND_NONE, NBA_SETUP_ACTION_NONE);

    s->frame++;

    if (s->frame < 0)
        return setup_result(NBA_SETUP_SOUND_NONE, NBA_SETUP_ACTION_NONE);

    if (!nba_setup_screen_decode_ppu_to(s, s->frame)) {
        s->has_gfx = false;
        return setup_result(NBA_SETUP_SOUND_NONE, NBA_SETUP_ACTION_NONE);
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

    /* The initial live register trace starts BG2 at $3FF (-1) and advances to
     * zero on entrance frame 3.  After a submenu transition, $80:A3B8 keeps
     * the phase established by the rebuilt page instead of restarting it from
     * the lifetime Setup frame counter. */
    if (s->transition == NBA_SETUP_TRANSITION_NONE) {
        if (s->bg2_scroll_from_transition)
            setup_advance_steady_bg2(s);
        else
            s->bg2_vscroll = s->frame / NBA_SETUP_SCROLL_PERIOD - 1;
    }

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

    /* $80:A3B8's final trace state must reach scanout once before normal page
     * state resumes.  Release it on the following update, after the base
     * registers above have already been restored for the settled page. */
    if (s->transition_release_pending)
        setup_release_page_transition(s);

    if (!input) return setup_result(NBA_SETUP_SOUND_NONE, NBA_SETUP_ACTION_NONE);

    if (s->transition != NBA_SETUP_TRANSITION_NONE) {
        setup_update_page_transition(s);
        return setup_result(NBA_SETUP_SOUND_NONE, NBA_SETUP_ACTION_NONE);
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
            if (changed)
            {
                if (s->page == NBA_SETUP_PAGE_OPTIONS)
                    (void)setup_rebuild_options_text_canvas(s);
                return setup_result(NBA_SETUP_SOUND_ADJUST, NBA_SETUP_ACTION_NONE);
            }
        } else if (input->pressed & NBA_BTN_START) {
            /* $81:D516 copies 26 bytes to $17D1; $82:8CD9/$8D0A copies
             * 14 bytes to $17B5. B is deliberately ignored by the ROM. */
            if (s->page == NBA_SETUP_PAGE_RULES) {
                memcpy(s->config->rules, s->working_rules, sizeof(s->config->rules));
            } else {
                memcpy(s->config->options, s->working_options, sizeof(s->config->options));
            }
            s->menu_row = 0;
            s->menu_scroll = 0;
            if (!setup_begin_page_transition(s, NBA_SETUP_PAGE_MAIN))
                return setup_result(NBA_SETUP_SOUND_NONE, NBA_SETUP_ACTION_NONE);
            return setup_result(NBA_SETUP_SOUND_CONFIRM,
                                NBA_SETUP_ACTION_RETURN_MAIN);
        }

        if (s->menu_row < s->menu_scroll) s->menu_scroll = s->menu_row;
        if (s->menu_row >= s->menu_scroll + 7) s->menu_scroll = s->menu_row - 6;
        if (input->pressed & (NBA_BTN_UP | NBA_BTN_DOWN))
            return setup_result(NBA_SETUP_SOUND_MOVE, NBA_SETUP_ACTION_NONE);
        return setup_result(NBA_SETUP_SOUND_NONE, NBA_SETUP_ACTION_NONE);
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
        uint16_t old_value = s->config->main_values[row];
        uint16_t max = setup_main_max[row];
        uint16_t new_value = (input->pressed & NBA_BTN_LEFT) ?
            (old_value == 0u ? max : (uint16_t)(old_value - 1u)) :
            (old_value >= max ? 0u : (uint16_t)(old_value + 1u));
        /* The ROM redraws these values into BG3 at $7E:16FB + row*2.
         * Refuse a state whose captured game-authored glyph canvas is absent. */
        if (s->main_value_vram[row][new_value]) {
            s->config->main_values[row] = new_value;
            return setup_result(NBA_SETUP_SOUND_ADJUST, NBA_SETUP_ACTION_NONE);
        }
    }
    if (input->pressed & (NBA_BTN_A | NBA_BTN_START)) {
        if (s->row == NBA_SETUP_ROW_RULES && setup_menu_assets_ready(s)) {
            memcpy(s->working_rules, s->config->rules, sizeof(s->working_rules));
            s->menu_row = s->menu_scroll = 0;
            if (!setup_begin_page_transition(s, NBA_SETUP_PAGE_RULES))
                return setup_result(NBA_SETUP_SOUND_NONE, NBA_SETUP_ACTION_NONE);
            return setup_result(NBA_SETUP_SOUND_CONFIRM,
                                NBA_SETUP_ACTION_OPEN_RULES);
        }
        if (s->row == NBA_SETUP_ROW_OPTIONS && setup_menu_assets_ready(s)) {
            memcpy(s->working_options, s->config->options, sizeof(s->working_options));
            (void)setup_rebuild_options_text_canvas(s);
            s->menu_row = s->menu_scroll = 0;
            if (!setup_begin_page_transition(s, NBA_SETUP_PAGE_OPTIONS))
                return setup_result(NBA_SETUP_SOUND_NONE, NBA_SETUP_ACTION_NONE);
            return setup_result(NBA_SETUP_SOUND_CONFIRM,
                                NBA_SETUP_ACTION_OPEN_OPTIONS);
        }
        if (s->row < NBA_SETUP_ROW_RULES)
            return setup_result(NBA_SETUP_SOUND_CONFIRM,
                                NBA_SETUP_ACTION_CONFIRM_MODE);
    }
    if (input->pressed & (NBA_BTN_UP | NBA_BTN_DOWN))
        return setup_result(NBA_SETUP_SOUND_MOVE, NBA_SETUP_ACTION_NONE);
    return setup_result(NBA_SETUP_SOUND_NONE, NBA_SETUP_ACTION_NONE);
}

static void setup_restore_bg2_rect(const NbaSetupScreen *s, NbaRenderer *ren,
                                   const uint8_t *vram, const uint8_t *cgram,
                                   int x, int y, int w, int h) {
    int first_y = y < 0 ? 0 : y;
    int first_x = x < 0 ? 0 : x;
    for (int py = first_y; py < y + h && py < NBA_SNES_HEIGHT; ++py) {
        for (int px = first_x; px < x + w && px < NBA_SNES_WIDTH; ++px) {
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

/* Shared implementation of $81:9756/$81:9FD4's proportional 2bpp value
 * copy. Callers clear the complete destination cell first, then copy only the
 * measured span owned by the replacement word. Copying an entire captured
 * cell can reintroduce pixels belonging to the word that preceded it. */
static bool setup_copy_rom_text_span(const uint8_t *source_vram,
                                     const uint8_t *source_cgram,
                                     NbaRenderer *ren, int sx, int sy,
                                     int width, int dx, int dy,
                                     int brightness, bool highlighted) {
    if (!source_vram || !source_cgram || !ren || width <= 0) return false;
    bool copied = false;
    for (int py = 0; py < 16; ++py) {
        for (int px = 0; px < width; ++px) {
            NbaSnesBgPixel pixel;
            if (!nba_snes_sample_bg(source_vram, NBA_SETUP_BG3_TILEMAP,
                                    NBA_SETUP_BG3_CHR, 2, false, true, 0, 0,
                                    sx + px, sy + py, &pixel)) continue;
            int x = dx + px;
            int y = dy + py;
            if (x < 0 || x >= NBA_SNES_WIDTH || y < 0 || y >= NBA_SNES_HEIGHT)
                continue;
            ren->pixels[y * NBA_SNES_WIDTH + x] = nba_snes_cgram_color(
                source_cgram, pixel.palette * 4 + pixel.color_index,
                brightness,
                highlighted ? NBA_SETUP_MATH_SUB_R : 0,
                highlighted ? NBA_SETUP_MATH_SUB_G : 0,
                highlighted ? NBA_SETUP_MATH_SUB_B : 0);
            copied = true;
        }
    }
    return copied;
}

/* $80:A77C selects the active main-page value and the generic BG3 writer
 * stores it at $7E:16FB + row*2. These spans include each word's final shadow
 * column, measured from the independent Mesen VRAM states. */
static const uint8_t setup_main_value_span[NBA_SETUP_MAIN_VALUE_COUNT][4] = {
    { 0, 48, 60, 75 }, /* Exhibition, Season, Playoffs, Load Series */
    { 47, 0, 49, 0 },  /* Arcade, Simulation, Custom */
    { 0, 52, 59, 0 },  /* Rookie, Starter, All-Star */
    { 0, 65, 65, 70 }  /* 3, 5, 8, 12 Minutes */
};

static void setup_render_main_values(const NbaSetupScreen *s, NbaRenderer *ren,
                                     int bg3_scroll) {
    if (!s || s->page != NBA_SETUP_PAGE_MAIN) return;
    for (int row = 0; row < NBA_SETUP_MAIN_VALUE_COUNT; ++row) {
        uint16_t value = s->config->main_values[row];
        if (value == nba_default_main_values[row] || value > setup_main_max[row]) continue;
        const uint8_t *source_vram = s->main_value_vram[row][value];
        int copy_width = setup_main_value_span[row][value];
        if (!source_vram || copy_width == 0) continue;
        int source_top = nba_setup_screen_row_band_top((NbaSetupRow)row);
        int top = source_top - bg3_scroll;
        setup_restore_bg2_rect(s, ren, s->vram, s->cgram, 138, top, 110, 16);
        (void)setup_copy_rom_text_span(source_vram, s->cgram, ren,
                                       138, source_top, copy_width,
                                       138, top, s->brightness,
                                       row == (int)s->row);
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
    int copy_width;
    const uint8_t *source_vram = s->options_vram;
    if (strcmp(text, "STEREO") == 0) { sy = 104; copy_width = 56; }
    else if (strcmp(text, "ON") == 0) { sy = 122; copy_width = 24; }
    else if (strcmp(text, "OFF") == 0) {
        sy = 104;
        copy_width = 32;
        source_vram = s->options_off_vram;
    } else if (strcmp(text, "PLAYER") == 0) { sy = 158; copy_width = 56; }
    else if (strcmp(text, "MONO") == 0) {
        sy = 104;
        copy_width = 40;
        source_vram = s->options_mono_vram;
    } else if (strcmp(text, "CPU") == 0) {
        sy = 158;
        copy_width = 32;
        source_vram = s->options_cpu_vram;
    }
    else return false;
    if (!source_vram) return false;

    return setup_copy_rom_text_span(source_vram, s->options_cgram, ren,
                                    156, sy, copy_width, dx, dy,
                                    s->brightness, highlighted);
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
    int scroll_rows = s->page == NBA_SETUP_PAGE_RULES ? s->menu_scroll : 0;
    int dy = s->page == NBA_SETUP_PAGE_OPTIONS ? -8 :
             -scroll_rows * NBA_SETUP_ROW_PITCH;
    int bar_x = s->page == NBA_SETUP_PAGE_RULES ? 144 : 160;
    int bar_y = s->page == NBA_SETUP_PAGE_RULES ? 82 : 74;

    /* $81:D59B enables the slider objects while redrawing logical rows 0/1;
     * $81:D5AE-$D5C3 derives their Y position from the visible viewport slot.
     * Thus at scroll 1 only Offensive Fouls remains, shifted into slot 0. */
    for (int index = 23; index >= 0; --index) {
        int bar = index >= 12 ? 1 : 0;
        int visible_slot = bar - scroll_rows;
        if (visible_slot < 0 || visible_slot >= 7) continue;
        setup_draw_oam_sprite(ren, s->rules_vram, s->rules_cgram,
                              s->rules_oam, index, dx, dy, false,
                              bar_x, bar_y + visible_slot * NBA_SETUP_ROW_PITCH,
                              48, 8);
    }
    const uint16_t *values = s->page == NBA_SETUP_PAGE_RULES ?
                             s->working_rules : s->working_options;
    for (int bar = 0; bar < 2; ++bar) {
        int visible_slot = bar - scroll_rows;
        if (visible_slot < 0 || visible_slot >= 7) continue;
        int visible_y = bar_y + visible_slot * NBA_SETUP_ROW_PITCH;
        for (int py = 1; py < 7; ++py)
            for (int px = values[bar] + 2; px < 47; ++px)
                ren->pixels[(visible_y + py) * NBA_SNES_WIDTH + bar_x + px] =
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
                                     const uint8_t *vram, const uint8_t *cgram,
                                     int bg3_scroll, bool draw_objects) {
    if (s->page == NBA_SETUP_PAGE_MAIN) return;
    if (s->page == NBA_SETUP_PAGE_OPTIONS) {
        /* All discrete labels and their shadows are already in the shared
         * ROM-authored BG3 canvas, exactly like $81:9756->$81:A1EE. */
        if (draw_objects) setup_draw_rom_menu_objects(s, ren);
        return;
    }
    const uint16_t *values = s->page == NBA_SETUP_PAGE_RULES ?
                             s->working_rules : s->working_options;
    int count = s->page == NBA_SETUP_PAGE_RULES ?
                NBA_SETUP_RULE_COUNT : NBA_SETUP_OPTION_COUNT;
    const uint16_t *defaults = s->page == NBA_SETUP_PAGE_RULES ?
                               nba_default_rules : nba_default_options;

    if (s->menu_scroll > 0)
        setup_restore_bg2_rect(s, ren, vram, cgram, 16, 70, 212, 10);

    for (int visible = 0; visible < 7; ++visible) {
        int row = s->menu_scroll + visible;
        if (row >= count) break;
        int source_top = (s->page == NBA_SETUP_PAGE_RULES ? 76 : 68) + visible * 18;
        int top = source_top - bg3_scroll;
        /* Music Mode owns a complete ROM-captured BG3 canvas for each value.
         * $82:8F9C->$81:9FD4 redraws and uploads that canvas as a unit, so do
         * not composite a short word over the old STEREO cell. */
        if (s->page == NBA_SETUP_PAGE_OPTIONS && row == 2) continue;
        if (row < 2 || values[row] != defaults[row]) {
            /* $81:A1EE uploads the full BG3 canvas after every discrete
             * redraw. Clear the complete value cell before copying the
             * ROM-authored replacement glyphs; 82 pixels left tails from
             * PLAYER and other longer values visible. */
            setup_restore_bg2_rect(s, ren, vram, cgram, 140, top, 108, 16);
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
    if (draw_objects) setup_draw_rom_menu_objects(s, ren);
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
    const uint16_t *layer_tilemap = s->layer_tilemap;
    const uint16_t *layer_chr = s->layer_chr;
    const bool *layer_double_width = s->layer_double_width;
    const bool *layer_double_height = s->layer_double_height;
    bool transition_canvas = s->transition != NBA_SETUP_TRANSITION_NONE &&
                             s->active_transition_trace != NULL;
    const SetupTransitionProfile *transition_profile = transition_canvas ?
        setup_transition_profile_for_route(s->transition_route) : NULL;
    bool construction_guard = false;
    if (transition_canvas) {
        /* Around the return map switch, endFrame memory already contains the
         * common builder's writes although the outgoing scanout still uses
         * the pre-switch page.  Guard only those edge-specific DMA frames;
         * the rest of the scroll continues to use the cumulative trace. */
        construction_guard = transition_profile &&
            transition_profile->construction_guard_end >
                transition_profile->construction_guard_start &&
            s->transition_frame >=
                transition_profile->construction_guard_start &&
            s->transition_frame < transition_profile->construction_guard_end;
        vram = construction_guard && s->active_transition_base_vram ?
                   s->active_transition_base_vram : s->transition_vram;
        cgram = construction_guard && s->active_transition_base_cgram ?
                    s->active_transition_base_cgram : s->transition_cgram;
        if (construction_guard) {
            layer_tilemap = s->transition_base_tilemap;
            layer_chr = s->transition_base_chr;
            layer_double_width = s->transition_base_double_width;
            layer_double_height = s->transition_base_double_height;
        }
    } else if (s->page == NBA_SETUP_PAGE_RULES) {
        vram = s->rules_vram;
        cgram = s->rules_cgram;
    } else if (s->page == NBA_SETUP_PAGE_OPTIONS) {
        vram = s->options_text_vram;
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
                if (nba_snes_sample_bg(vram, layer_tilemap[1], layer_chr[1],
                                       4, layer_double_width[1],
                                       layer_double_height[1], s->bg2_hscroll,
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
                if (nba_snes_sample_bg(vram, layer_tilemap[0], layer_chr[0],
                                       4, layer_double_width[0],
                                       layer_double_height[0], s->bg1_hscroll,
                                       s->bg1_vscroll, x, y, &pixel)) {
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

            /* During $80:A3B8's submenu reveal, the captured layer state uses
             * the $10 designation while the freshly built BG3 canvas scrolls
             * on.  Treat it as part of the transition scanout; after the edge
             * settles, the normal $04 main/sub designation is authoritative. */
            bool bg3_designated = ((s->main_screen | s->sub_screen) & 0x04) != 0 ||
                                  (transition_canvas &&
                                   (s->main_screen & 0x10) != 0 &&
                                   s->bg3_vscroll <= 252) ||
                                  (transition_canvas && s->bg3_vscroll == 0 &&
                                   s->brightness == 15 &&
                                   s->bg1_hscroll == 512 &&
                                   s->bg2_hscroll == 0);
            if (construction_guard) bg3_designated = false;
            if (bg3_designated) {
                int bg3_scroll = s->bg3_vscroll;
                /* Rules' $81:A28E path queues the rebuilt BG3 DMA one frame
                 * before its vertically staged result reaches scanout.  The
                 * Options and return captures have no corresponding delay. */
                if (transition_canvas && (s->main_screen & 0x10) != 0 &&
                    ((s->main_screen | s->sub_screen) & 0x04) == 0 &&
                    bg3_scroll > 0 && transition_profile &&
                    transition_profile->bg3_scanout_delay_frames > 0) {
                    bg3_scroll += transition_profile->bg3_scanout_delay_frames *
                                  NBA_SETUP_BG3_SCROLL_STEP;
                    if (bg3_scroll > 252) bg3_scroll = 252;
                }
                if (s->page == NBA_SETUP_PAGE_RULES && s->menu_scroll > 0 && y >= 70)
                    bg3_scroll += s->menu_scroll * NBA_SETUP_ROW_PITCH;
                if (nba_snes_sample_bg(vram, layer_tilemap[2], layer_chr[2],
                                       2, layer_double_width[2],
                                       layer_double_height[2], s->bg3_hscroll, bg3_scroll,
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
    if (!transition_canvas && ((s->main_screen | s->sub_screen) & 0x04)) {
        int overlay_scroll = s->bg3_vscroll;
        if (s->page == NBA_SETUP_PAGE_RULES && s->menu_scroll > 0)
            overlay_scroll += s->menu_scroll * NBA_SETUP_ROW_PITCH;
        setup_render_main_values(s, ren, overlay_scroll);
        setup_render_menu_values(s, ren, vram, cgram, overlay_scroll,
                                 (s->main_screen & 0x10) != 0);
    }
}
