#include "nba_title_sequence.h"
#include "nba_font.h"
#include <string.h>

static uint32_t title_scale_color(uint32_t color, int brightness) {
    if (brightness >= 15) return color;
    if (brightness <= 0) return color & 0xFF000000u;
    uint32_t r = ((color >> 16) & 0xFFu) * (uint32_t)brightness / 15u;
    uint32_t g = ((color >> 8) & 0xFFu) * (uint32_t)brightness / 15u;
    uint32_t b = (color & 0xFFu) * (uint32_t)brightness / 15u;
    return (color & 0xFF000000u) | (r << 16) | (g << 8) | b;
}

static uint32_t title_blend(uint32_t a, uint32_t b, int amount) {
    if (amount <= 0) return a;
    if (amount >= 256) return b;
    int inverse = 256 - amount;
    uint32_t r = ((((a >> 16) & 0xFFu) * (uint32_t)inverse) +
                  (((b >> 16) & 0xFFu) * (uint32_t)amount)) >> 8;
    uint32_t g = ((((a >> 8) & 0xFFu) * (uint32_t)inverse) +
                  (((b >> 8) & 0xFFu) * (uint32_t)amount)) >> 8;
    uint32_t blue = (((a & 0xFFu) * (uint32_t)inverse) +
                     ((b & 0xFFu) * (uint32_t)amount)) >> 8;
    return 0xFF000000u | (r << 16) | (g << 8) | blue;
}

void nba_title_sequence_init(NbaTitleSequence *sequence) {
    if (!sequence) return;
    memset(sequence, 0, sizeof(*sequence));
}

void nba_title_sequence_render(const NbaTitleSequence *sequence,
                               const NbaAssetPack *assets,
                               NbaRenderer *renderer,
                               float timer) {
    (void)sequence;
    if (!assets || !renderer) return;

    int frame = (int)(timer * 60.0f + 0.5f);
    const NbaAssetItem *first = nba_assets_get(assets, NBA_ASSET_TITLE_SEQUENCE_FRAME1);
    const NbaAssetItem *second = nba_assets_get(assets, NBA_ASSET_TITLE_SEQUENCE_FRAME2);
    const NbaAssetItem *third = nba_assets_get(assets, NBA_ASSET_TITLE_SEQUENCE_FRAME3);
    const NbaAssetItem *base = first;
    const NbaAssetItem *next = first;
    int blend = 0;

    if (frame >= 90 && second && second->data) {
        base = first;
        next = second;
        blend = frame < 180 ? (frame - 90) * 256 / 90 : 256;
    }
    if (frame >= 420 && third && third->data) {
        base = second;
        next = third;
        blend = frame < 480 ? (frame - 420) * 256 / 60 : 256;
    }

    int brightness = 15;
    if (frame < NBA_TITLE_FADE_FRAMES) brightness = frame;
    if (frame > NBA_TITLE_SEQUENCE_FRAMES - NBA_TITLE_FADE_FRAMES) {
        brightness = NBA_TITLE_SEQUENCE_FRAMES - frame;
    }

    if (base && base->data && base->width == NBA_SNES_WIDTH &&
        base->height == NBA_SNES_HEIGHT) {
        const uint32_t *a = (const uint32_t *)base->data;
        const uint32_t *b = (next && next->data && next->width == NBA_SNES_WIDTH &&
                             next->height == NBA_SNES_HEIGHT)
                                ? (const uint32_t *)next->data : a;
        for (int i = 0; i < NBA_SNES_WIDTH * NBA_SNES_HEIGHT; ++i) {
            renderer->pixels[i] = title_scale_color(title_blend(a[i], b[i], blend), brightness);
        }
        return;
    }

    /* Asset-free fallback retains the ROM scene's dark perspective-court language. */
    nba_renderer_clear(renderer, 0xFF000000u);
    for (int y = 0; y < NBA_SNES_HEIGHT; y += 16) {
        nba_renderer_draw_rect(renderer, 0, y, NBA_SNES_WIDTH, 1,
                               title_scale_color(0xFF182028u, brightness));
    }
    nba_font_render_text_centered(renderer->pixels, NBA_SNES_WIDTH, 78,
                                  "NBA LIVE 95", title_scale_color(0xFF20A0C0u, brightness),
                                  0xFF000000u, 2);
    nba_font_render_text_centered(renderer->pixels, NBA_SNES_WIDTH, 190,
                                  "(C) 1994 ELECTRONIC ARTS",
                                  title_scale_color(0xFFFFFFFFu, brightness), 0xFF000000u, 1);
}
