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

void nba_title_sequence_init(NbaTitleSequence *sequence) {
    if (!sequence) return;
    memset(sequence, 0, sizeof(*sequence));
    title_video_rewind(sequence);
}

void nba_title_sequence_render(NbaTitleSequence *sequence,
                               const NbaAssetPack *assets,
                               NbaRenderer *renderer,
                               float timer) {
    if (!sequence || !assets || !renderer) return;
    const NbaAssetItem *video = nba_assets_get(assets, NBA_ASSET_TITLE_SEQUENCE_VIDEO);
    uint32_t fps_num, fps_den, frame_count;
    if (title_video_header(video, &fps_num, &fps_den, &frame_count)) {
        int target = (int)(timer * (float)fps_num / (float)fps_den);
        if (title_video_decode_to(sequence, video, target)) {
            for (int i = 0; i < NBA_SNES_WIDTH * NBA_SNES_HEIGHT; ++i) {
                renderer->pixels[i] = title_rgb565_to_argb(sequence->framebuffer[i]);
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
