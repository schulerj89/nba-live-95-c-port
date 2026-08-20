#include "nba_title_sequence.h"
#include "nba_font.h"
#include <string.h>

#define TITLE_VIDEO_MAGIC "NBTITLE1"

static uint16_t title_read_u16(const uint8_t *data) {
    return (uint16_t)(data[0] | ((uint16_t)data[1] << 8));
}

static uint32_t title_read_u32(const uint8_t *data) {
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static bool title_video_header(const NbaAssetItem *video, uint32_t *fps_num,
                               uint32_t *fps_den, uint32_t *frame_count) {
    if (!video || !video->data || video->size < NBA_TITLE_VIDEO_HEADER_SIZE) return false;
    const uint8_t *data = (const uint8_t *)video->data;
    if (memcmp(data, TITLE_VIDEO_MAGIC, 8) != 0 || title_read_u32(data + 8) != 1 ||
        title_read_u32(data + 12) != NBA_SNES_WIDTH ||
        title_read_u32(data + 16) != NBA_SNES_HEIGHT) return false;
    *fps_num = title_read_u32(data + 20);
    *fps_den = title_read_u32(data + 24);
    *frame_count = title_read_u32(data + 28);
    return *fps_num != 0 && *fps_den != 0 && *frame_count != 0;
}

static void title_video_rewind(NbaTitleSequence *sequence) {
    memset(sequence->framebuffer, 0, sizeof(sequence->framebuffer));
    sequence->decoded_frame = -1;
    sequence->video_offset = NBA_TITLE_VIDEO_HEADER_SIZE;
}

/* Decode the ROM-reference delta stream. Each record contains absolute pixel
 * runs, preserving the gym fade, logo/letter hits, lights, and credit motion. */
static bool title_video_decode_to(NbaTitleSequence *sequence,
                                  const NbaAssetItem *video, int target_frame) {
    uint32_t fps_num, fps_den, frame_count;
    if (!title_video_header(video, &fps_num, &fps_den, &frame_count)) return false;
    if (target_frame < 0) target_frame = 0;
    if ((uint32_t)target_frame >= frame_count) target_frame = (int)frame_count - 1;
    if (target_frame < sequence->decoded_frame) title_video_rewind(sequence);

    const uint8_t *data = (const uint8_t *)video->data;
    while (sequence->decoded_frame < target_frame) {
        size_t offset = sequence->video_offset;
        if (offset + 8 > video->size) return false;
        uint32_t record_size = title_read_u32(data + offset);
        size_t record_end = offset + 4u + record_size;
        if (record_size < 4 || record_end > video->size) return false;
        uint32_t run_count = title_read_u32(data + offset + 4);
        offset += 8;
        for (uint32_t run = 0; run < run_count; ++run) {
            if (offset + 4 > record_end) return false;
            uint32_t start = title_read_u16(data + offset);
            uint32_t length = title_read_u16(data + offset + 2);
            offset += 4;
            if (start + length > NBA_SNES_WIDTH * NBA_SNES_HEIGHT ||
                offset + length * 2u > record_end) return false;
            for (uint32_t i = 0; i < length; ++i) {
                sequence->framebuffer[start + i] = title_read_u16(data + offset + i * 2u);
            }
            offset += length * 2u;
        }
        sequence->video_offset = record_end;
        ++sequence->decoded_frame;
    }
    return true;
}

static uint32_t title_rgb565_to_argb(uint16_t color) {
    uint32_t r = (color >> 11) & 0x1Fu;
    uint32_t g = (color >> 5) & 0x3Fu;
    uint32_t b = color & 0x1Fu;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

/**
 * Offset/Address/Size: 0x004F1B | $80:CF1B | size: 0x20
 * Purpose: Applies the INIDISP master-brightness level ($0562, 15..0) that the
 *          ROM ramps down during the title fade-out.
 */
static uint32_t title_apply_brightness(uint32_t argb, int level) {
    if (level >= 15) return argb;
    if (level <= 0) return argb & 0xFF000000u;
    uint32_t r = ((argb >> 16) & 0xFFu) * (uint32_t)level / 15u;
    uint32_t g = ((argb >> 8) & 0xFFu) * (uint32_t)level / 15u;
    uint32_t b = (argb & 0xFFu) * (uint32_t)level / 15u;
    return (argb & 0xFF000000u) | (r << 16) | (g << 8) | b;
}

/**
 * Offset/Address/Size: 0x00601E | $80:E01E | size: 0x1E0
 * Purpose: Enters the title scene and rewinds the reference stream.
 */
void nba_title_sequence_init(NbaTitleSequence *sequence) {
    if (!sequence) return;
    memset(sequence, 0, sizeof(*sequence));
    title_video_rewind(sequence);
    sequence->phase = NBA_TITLE_PHASE_BUILD;
    sequence->hold_frames_left = 0;
    sequence->fade_level = 15;
    sequence->snap_timer = -1.0f;
}

/**
 * Offset/Address/Size: 0x00707E | $80:F07E | size: 0x1F
 * Subroutines: $80:8627, $80:868C, $80:8BA1 (0x680-byte transfer from $7F:4006)
 * Purpose: Snaps the title to its finished state. The ROM does this by DMAing
 *          the completed title tilemap into VRAM in one transfer, so every
 *          remaining piece appears at once instead of animating in. Here the
 *          equivalent is jumping the reference stream to the frame where the
 *          build has finished.
 */
void nba_title_sequence_snap_complete(NbaTitleSequence *sequence) {
    if (!sequence) return;
    sequence->snap_timer = (float)NBA_TITLE_BUILD_COMPLETE_FRAMES / 60.0f;
}

/**
 * Offset/Address/Size: 0x0065C7 | $80:E5C7 | size: 0x4A
 * Subroutines: $80:F07E (snap), $80:86B0 (frame wait), $80:CF1B (fade out)
 * Purpose: Runs the title exit. Holds for the ROM's fixed frame count - 120
 *          frames when the title had to be snapped, 40 when it had already
 *          finished building - then ramps INIDISP brightness down one step per
 *          frame. Returns true once the fade has completed and the next scene
 *          may be entered.
 */
bool nba_title_sequence_advance(NbaTitleSequence *sequence, float timer) {
    (void)timer;
    if (!sequence) return false;

    switch (sequence->phase) {
        case NBA_TITLE_PHASE_BUILD:
            break;

        case NBA_TITLE_PHASE_HOLD:
            /* $80:E5F9 JSL $80:86B0 / DEC A / BPL $E5F9 */
            if (sequence->hold_frames_left > 0) {
                sequence->hold_frames_left--;
            } else {
                sequence->phase = NBA_TITLE_PHASE_FADE_OUT;
                sequence->fade_level = 15;
            }
            break;

        case NBA_TITLE_PHASE_FADE_OUT:
            /* $80:CF1B - DEC $0562 once per frame until it reaches zero */
            if (sequence->fade_level > 0) {
                sequence->fade_level--;
            }
            if (sequence->fade_level == 0) return true;
            break;
    }
    return false;
}

/**
 * Offset/Address/Size: 0x00601E | $80:E01E | size: 0x1E0
 * Purpose: Draws the title scene, applying the $80:CF1B INIDISP brightness
 *          level so the fade-out is visible.
 */
void nba_title_sequence_render(NbaTitleSequence *sequence,
                               const NbaAssetPack *assets,
                               NbaRenderer *renderer,
                               float timer) {
    if (!sequence || !assets || !renderer) return;

    /* $80:F07E pinned the scene at the finished build; hold there. */
    if (sequence->snap_timer >= 0.0f) {
        timer = sequence->snap_timer;
    }

    const NbaAssetItem *video = nba_assets_get(assets, NBA_ASSET_TITLE_SEQUENCE_VIDEO);
    uint32_t fps_num, fps_den, frame_count;
    if (title_video_header(video, &fps_num, &fps_den, &frame_count)) {
        int target = (int)(timer * (float)fps_num / (float)fps_den);
        if (title_video_decode_to(sequence, video, target)) {
            int level = sequence->fade_level;
            for (int i = 0; i < NBA_SNES_WIDTH * NBA_SNES_HEIGHT; ++i) {
                uint32_t argb = title_rgb565_to_argb(sequence->framebuffer[i]);
                renderer->pixels[i] = title_apply_brightness(argb, level);
            }
            return;
        }
    }

    nba_renderer_clear(renderer, 0xFF000000u);
    for (int y = 0; y < NBA_SNES_HEIGHT; y += 16) {
        nba_renderer_draw_rect(renderer, 0, y, NBA_SNES_WIDTH, 1, 0xFF182028u);
    }
    nba_font_render_text_centered(renderer->pixels, NBA_SNES_WIDTH, 78,
                                  "NBA LIVE 95", 0xFF20A0C0u, 0xFF000000u, 2);
    nba_font_render_text_centered(renderer->pixels, NBA_SNES_WIDTH, 190,
                                  "(C) 1994 ELECTRONIC ARTS",
                                  0xFFFFFFFFu, 0xFF000000u, 1);
}
