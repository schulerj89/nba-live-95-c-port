#include "nba_title_sequence.h"
#include "nba_font.h"
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

static uint32_t title_color(const uint8_t *cgram, int index, int brightness) {
    uint16_t word = (uint16_t)(cgram[(index * 2) & 0x1FF] |
                               ((uint16_t)cgram[(index * 2 + 1) & 0x1FF] << 8));
    uint32_t r = word & 31u, g = (word >> 5) & 31u, b = (word >> 10) & 31u;
    r = (r << 3) | (r >> 2); g = (g << 3) | (g >> 2); b = (b << 3) | (b >> 2);
    if (brightness < 15) {
        if (brightness < 0) brightness = 0;
        r = r * (uint32_t)brightness / 15u;
        g = g * (uint32_t)brightness / 15u;
        b = b * (uint32_t)brightness / 15u;
    }
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static int title_tile_pixel(const uint8_t *vram, int chr_base, int tile,
                            int bpp, int x, int y) {
    int off = (chr_base + tile * 8 * bpp) & 0xFFFF;
    int bit = 7 - x, value = 0;
    for (int plane = 0; plane < bpp; plane += 2) {
        int lo = vram[(off + y * 2 + plane * 8) & 0xFFFF];
        int hi = vram[(off + y * 2 + 1 + plane * 8) & 0xFFFF];
        value |= ((lo >> bit) & 1) << plane;
        value |= ((hi >> bit) & 1) << (plane + 1);
    }
    return value;
}

static int title_sample_bg(const uint8_t *vram, int map_base, int chr_base,
                           int bpp, bool wide, bool tall, int hscroll, int vscroll,
                           int x, int y, int *palette, int *priority) {
    int map_w = wide ? 512 : 256, map_h = tall ? 512 : 256;
    int px = ((x + hscroll) % map_w + map_w) % map_w;
    int py = ((y + vscroll + 1) % map_h + map_h) % map_h;
    int tx = px >> 3, ty = py >> 3, quadrant = 0;
    if (wide && tx >= 32) quadrant++;
    if (tall && ty >= 32) quadrant += wide ? 2 : 1;
    int entry_off = map_base + quadrant * 0x800 +
                    ((ty & 31) * 32 + (tx & 31)) * 2;
    uint16_t entry = (uint16_t)(vram[entry_off & 0xFFFF] |
                                ((uint16_t)vram[(entry_off + 1) & 0xFFFF] << 8));
    int sx = (entry & 0x4000) ? 7 - (px & 7) : (px & 7);
    int sy = (entry & 0x8000) ? 7 - (py & 7) : (py & 7);
    int value = title_tile_pixel(vram, chr_base, entry & 0x3FF, bpp, sx, sy);
    if (!value) return -1;
    *palette = (entry >> 10) & 7;
    *priority = (entry >> 13) & 1;
    return value;
}

static void title_render_ppu(const NbaTitleSequence *s, NbaRenderer *renderer) {
    uint32_t backdrop = title_color(s->cgram, 0, s->brightness);
    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
            uint32_t color = backdrop;
            int palette = 0, priority = 0, value, best_z = 0;
            if (s->main_screen & 0x02) {
                value = title_sample_bg(s->vram, TITLE_BG2_MAP, TITLE_BG2_CHR, 4,
                                        false, true, s->bg_hscroll[1],
                                        s->bg_vscroll[1], x, y, &palette, &priority);
                if (value >= 0) {
                    int z = priority ? 8 : 4;
                    if (z > best_z) {
                        color = title_color(s->cgram, palette * 16 + value, s->brightness);
                        best_z = z;
                    }
                }
            }
            if (s->main_screen & 0x01) {
                value = title_sample_bg(s->vram, TITLE_BG1_MAP, TITLE_BG1_CHR, 4,
                                        false, true, s->bg_hscroll[0],
                                        s->bg_vscroll[0], x, y, &palette, &priority);
                if (value >= 0) {
                    int z = priority ? 9 : 5;
                    if (z > best_z) {
                        color = title_color(s->cgram, palette * 16 + value, s->brightness);
                        best_z = z;
                    }
                }
            }
            if (s->main_screen & 0x04) {
                bool attract = s->attract_index != 0xFFFF;
                int credit_top_x = s->credit_x -
                    ((s->attract_delay == 0 || s->attract_delay == 0xFFFF) ? 8 : 0);
                int bg3_x = attract ? (y < 186 ? credit_top_x : 0) : s->bg_hscroll[2];
                int bg3_y = attract ? (y < 186 ? -83 : s->credit_y) : s->bg_vscroll[2];
                value = (attract && y < 0x90) ? -1 :
                    title_sample_bg(s->vram, TITLE_BG3_MAP, TITLE_BG3_CHR, 2,
                                    true, false, bg3_x, bg3_y,
                                    x, y, &palette, &priority);
                if (value >= 0) {
                    int z = attract ? 10 : (priority ? 3 : 2);
                    if (z > best_z) {
                        color = title_color(s->cgram, palette * 4 + value, s->brightness);
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
