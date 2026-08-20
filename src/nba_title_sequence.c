#include "nba_title_sequence.h"
#include "nba_font.h"
#include "nba_snes_ppu.h"
#include <string.h>

#define TITLE_PPU_MAGIC "NBTPPU1\0"
#define TITLE_PPU_RECORD_SIZE 26

/* PPU layout established by $80:E01E. Mesen reports VRAM byte addresses. */
#define TITLE_BG1_MAP 0x1800
#define TITLE_BG1_CHR 0x8000
#define TITLE_BG2_MAP 0x1000
#define TITLE_BG2_CHR 0x2000
#define TITLE_BG3_MAP 0x0000
#define TITLE_BG3_CHR 0xC000

/* $80:8640's H/V IRQ pair toggles TMW ($212E) after the title-logo band.
 * Window 1 is inverted at 0..178, clipping BG1 construction tiles which
 * extend into the following row/right margin. */
#define TITLE_BG1_WINDOW_RIGHT 178
#define TITLE_BG1_WINDOW_SCANLINE 92
#define TITLE_LIVE_CUE_FRAME 820
/* $87:800F installs $87:8211 at V=$64; it arms $87:8230 at V=$BE. */
#define TITLE_CREDIT_ENABLE_SCANLINE 144
/* IRQ V=$BE maps to visible line 186 after the SNES four-line raster offset. */
#define TITLE_CREDIT_SCROLL_SCANLINE 186
#define TITLE_BG_FETCH_X_OFFSET 8

static uint16_t title_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t title_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool title_trace_header(const NbaAssetItem *trace, uint32_t *frame_count) {
    if (!trace || !trace->data || trace->size < NBA_TITLE_PPU_HEADER_SIZE) return false;
    const uint8_t *data = (const uint8_t *)trace->data;
    if (memcmp(data, TITLE_PPU_MAGIC, 8) != 0 || title_u32(data + 8) != 1) return false;
    *frame_count = title_u32(data + 12);
    return *frame_count > 0 && *frame_count <= 3600;
}

static bool title_trace_rewind(NbaTitleSequence *s, const NbaAssetPack *assets) {
    const NbaAssetItem *vram = nba_assets_get(assets, NBA_ASSET_TITLE_VRAM);
    const NbaAssetItem *cgram = nba_assets_get(assets, NBA_ASSET_TITLE_CGRAM);
    if (!vram || vram->size < sizeof(s->vram) ||
        !cgram || cgram->size < sizeof(s->cgram)) return false;
    memcpy(s->vram, vram->data, sizeof(s->vram));
    memcpy(s->cgram, cgram->data, sizeof(s->cgram));
    s->decoded_frame = -1;
    s->trace_offset = NBA_TITLE_PPU_HEADER_SIZE;
    s->gfx_loaded = true;
    return true;
}

/**
 * Offset/Address/Size: $80:E1F6-$80:E5C4 | per-frame
 * Subroutines: $80:E381 (consume $064A), $80:8E14 (tilegroup draw),
 *              $80:E8D9 (light CGRAM cycle), $87:80CB (credits/attract)
 * Purpose: Replays the ROM's resulting VRAM/CGRAM writes and PPU registers.
 *          The trace contains hardware bytes, not screenshots or pixels.
 */
static bool title_trace_decode_to(NbaTitleSequence *s, const NbaAssetPack *assets,
                                  const NbaAssetItem *trace, int target) {
    uint32_t frame_count;
    if (!title_trace_header(trace, &frame_count)) return false;
    if (target < 0) target = 0;
    if ((uint32_t)target >= frame_count) target = (int)frame_count - 1;
    if (!s->gfx_loaded || target < s->decoded_frame) {
        if (!title_trace_rewind(s, assets)) return false;
    }

    const uint8_t *data = (const uint8_t *)trace->data;
    while (s->decoded_frame < target) {
        size_t off = s->trace_offset;
        if (off + TITLE_PPU_RECORD_SIZE > trace->size) return false;
        s->brightness = data[off + 0];
        s->main_screen = data[off + 1];
        for (int layer = 0; layer < 3; ++layer) {
            s->bg_hscroll[layer] = title_u16(data + off + 2 + layer * 4);
            s->bg_vscroll[layer] = title_u16(data + off + 4 + layer * 4);
        }
        s->credit_x = title_u16(data + off + 14);
        s->credit_y = title_u16(data + off + 16);
        s->attract_index = title_u16(data + off + 18);
        s->attract_delay = title_u16(data + off + 20);
        uint16_t vram_count = title_u16(data + off + 22);
        uint16_t cgram_count = title_u16(data + off + 24);
        off += TITLE_PPU_RECORD_SIZE;
        if (off + ((size_t)vram_count + cgram_count) * 3u > trace->size) return false;
        for (uint16_t i = 0; i < vram_count; ++i) {
            uint16_t address = title_u16(data + off);
            s->vram[address] = data[off + 2];
            off += 3;
        }
        for (uint16_t i = 0; i < cgram_count; ++i) {
            uint16_t address = title_u16(data + off);
            if (address < sizeof(s->cgram)) s->cgram[address] = data[off + 2];
            off += 3;
        }
        s->trace_offset = off;
        ++s->decoded_frame;
    }
    return true;
}

static void title_render_ppu(const NbaTitleSequence *s, NbaRenderer *renderer) {
    uint32_t backdrop = nba_snes_cgram_color(s->cgram, 0, s->brightness, 0, 0, 0);
    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
            uint32_t color = backdrop;
            NbaSnesBgPixel pixel = {0};
            int best_z = 0;
            if (s->main_screen & 0x02) {
                if (nba_snes_sample_bg(s->vram, TITLE_BG2_MAP, TITLE_BG2_CHR, 4,
                                       false, true, s->bg_hscroll[1],
                                       s->bg_vscroll[1], x, y, &pixel)) {
                    int z = pixel.priority ? 8 : 4;
                    if (z > best_z) {
                        color = nba_snes_cgram_color(s->cgram,
                            pixel.palette * 16 + pixel.color_index,
                            s->brightness, 0, 0, 0);
                        best_z = z;
                    }
                }
            }
            bool logo_window_clips = s->decoded_frame < TITLE_LIVE_CUE_FRAME &&
                y >= TITLE_BG1_WINDOW_SCANLINE && x > TITLE_BG1_WINDOW_RIGHT;
            if ((s->main_screen & 0x01) && !logo_window_clips) {
                if (nba_snes_sample_bg(s->vram, TITLE_BG1_MAP, TITLE_BG1_CHR, 4,
                                       false, true, s->bg_hscroll[0],
                                       s->bg_vscroll[0], x, y, &pixel)) {
                    int z = pixel.priority ? 9 : 5;
                    if (z > best_z) {
                        color = nba_snes_cgram_color(s->cgram,
                            pixel.palette * 16 + pixel.color_index,
                            s->brightness, 0, 0, 0);
                        best_z = z;
                    }
                }
            }
            if (s->main_screen & 0x04) {
                bool attract = s->attract_index != 0xFFFF;
                /* $87:8211/$87:8230 split BG3 by scanline. $186E gates the
                 * animation in $87:80CB but never changes scroll position. */
                int credit_top_x = s->credit_x -
                    (s->credit_x != 0 ? TITLE_BG_FETCH_X_OFFSET : 0);
                int bg3_x = attract ?
                    (y < TITLE_CREDIT_SCROLL_SCANLINE ? credit_top_x : 0) :
                    s->bg_hscroll[2];
                int bg3_y = attract ?
                    (y < TITLE_CREDIT_SCROLL_SCANLINE ? -83 : s->credit_y) :
                    s->bg_vscroll[2];
                bool visible = !(attract && y < TITLE_CREDIT_ENABLE_SCANLINE) &&
                    nba_snes_sample_bg(s->vram, TITLE_BG3_MAP, TITLE_BG3_CHR, 2,
                                       true, false, bg3_x, bg3_y, x, y, &pixel);
                if (visible) {
                    int z = attract ? 10 : (pixel.priority ? 3 : 2);
                    if (z > best_z) {
                        color = nba_snes_cgram_color(s->cgram,
                            pixel.palette * 4 + pixel.color_index,
                            s->brightness, 0, 0, 0);
                        best_z = z;
                    }
                }
            }
            renderer->pixels[y * NBA_SNES_WIDTH + x] = color;
        }
    }
}

void nba_title_sequence_init(NbaTitleSequence *sequence) {
    if (!sequence) return;
    memset(sequence, 0, sizeof(*sequence));
    sequence->decoded_frame = -1;
    sequence->trace_offset = NBA_TITLE_PPU_HEADER_SIZE;
    sequence->phase = NBA_TITLE_PHASE_BUILD;
    sequence->fade_level = 15;
    sequence->snap_frame = -1;
}

/**
 * Offset/Address/Size: 0x00707E | $80:F07E | size: 0x1F
 * Subroutines: $80:8627, $80:868C, $80:8BA1 (0x680-byte finished tilemap DMA)
 * Purpose: Advances the hardware trace to the ROM's completed-title state.
 */
void nba_title_sequence_snap_complete(NbaTitleSequence *sequence) {
    if (!sequence) return;
    sequence->snap_frame = NBA_TITLE_BUILD_COMPLETE_FRAMES;
}

bool nba_title_sequence_advance(NbaTitleSequence *sequence) {
    if (!sequence) return false;
    if (sequence->phase == NBA_TITLE_PHASE_HOLD) {
        if (sequence->hold_frames_left > 0) sequence->hold_frames_left--;
        else { sequence->phase = NBA_TITLE_PHASE_FADE_OUT; sequence->fade_level = 15; }
    } else if (sequence->phase == NBA_TITLE_PHASE_FADE_OUT) {
        if (sequence->fade_level > 0) sequence->fade_level--;
        if (sequence->fade_level == 0) return true;
    }
    return false;
}

void nba_title_sequence_render(NbaTitleSequence *sequence,
                               const NbaAssetPack *assets,
                               NbaRenderer *renderer,
                               int frame) {
    if (!sequence || !assets || !renderer) return;
    if (sequence->snap_frame >= 0) frame = sequence->snap_frame;
    const NbaAssetItem *trace = nba_assets_get(assets, NBA_ASSET_TITLE_PPU_TRACE);
    if (title_trace_decode_to(sequence, assets, trace, frame)) {
        int captured_brightness = sequence->brightness;
        if (sequence->fade_level < 15) {
            sequence->brightness = captured_brightness * sequence->fade_level / 15;
        }
        title_render_ppu(sequence, renderer);
        sequence->brightness = captured_brightness;
        return;
    }
    nba_renderer_clear(renderer, 0xFF000000u);
    nba_font_render_text_centered(renderer->pixels, NBA_SNES_WIDTH, 100,
                                  "TITLE HARDWARE ASSETS MISSING",
                                  0xFFFFFFFFu, 0xFF000000u, 1);
}
