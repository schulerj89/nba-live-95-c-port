#include "nba_snes_ppu.h"
#include <stdlib.h>
#include <string.h>

#define MODE1_META_COLOR_MASK     0x000000ffu
#define MODE1_META_PALETTE_SHIFT  8u
#define MODE1_META_LAYER_SHIFT    16u
#define MODE1_META_PRIORITY_SHIFT 19u
#define MODE1_META_OAM_SHIFT      21u
#define MODE1_META_RANK_SHIFT     28u

typedef struct NbaSnesMode1State {
    uint32_t final_metadata[NBA_SNES_WIDTH * NBA_SNES_HEIGHT];
    uint32_t bg_metadata[NBA_SNES_WIDTH * NBA_SNES_HEIGHT];
    uint32_t bg_argb[NBA_SNES_WIDTH * NBA_SNES_HEIGHT];
    uint32_t obj_metadata[NBA_SNES_WIDTH * NBA_SNES_HEIGHT];
    uint32_t obj_argb[NBA_SNES_WIDTH * NBA_SNES_HEIGHT];
    bool bg3_priority_high;
} NbaSnesMode1State;

static uint8_t mode1_rank(NbaSnesLayer layer, uint8_t priority,
                          bool bg3_priority_high) {
    /* SNESdev BGMODE Mode-1 table, encoded back-to-front:
     * 3L,S0,3H,S1,2L,1L,S2,2H,1H,S3; high-BG3 moves 3H to the front. */
    switch (layer) {
        case NBA_SNES_LAYER_BACKDROP: return 0u;
        case NBA_SNES_LAYER_BG3:
            return priority ? (bg3_priority_high ? 11u : 3u) : 1u;
        case NBA_SNES_LAYER_BG2: return priority ? 8u : 5u;
        case NBA_SNES_LAYER_BG1: return priority ? 9u : 6u;
        case NBA_SNES_LAYER_OBJ: {
            static const uint8_t obj_rank[4] = { 2u, 4u, 7u, 10u };
            return obj_rank[priority & 3u];
        }
        default: return 0u;
    }
}

static uint32_t pack_metadata(NbaSnesLayer layer, uint8_t priority,
                              uint8_t palette_index, uint8_t color_index,
                              uint8_t oam_index, uint8_t rank) {
    return (uint32_t)color_index |
           ((uint32_t)palette_index << MODE1_META_PALETTE_SHIFT) |
           ((uint32_t)layer << MODE1_META_LAYER_SHIFT) |
           ((uint32_t)(priority & 3u) << MODE1_META_PRIORITY_SHIFT) |
           ((uint32_t)(oam_index & 0x7fu) << MODE1_META_OAM_SHIFT) |
           ((uint32_t)(rank & 0x0fu) << MODE1_META_RANK_SHIFT);
}

static int snes_tile_pixel(const uint8_t *vram, int chr_base, int tile,
                           int bits_per_pixel, int x, int y) {
    int offset = (chr_base + tile * 8 * bits_per_pixel) & 0xFFFF;
    int bit = 7 - x;
    int value = 0;
    for (int plane = 0; plane < bits_per_pixel; plane += 2) {
        int low = vram[(offset + y * 2 + plane * 8) & 0xFFFF];
        int high = vram[(offset + y * 2 + 1 + plane * 8) & 0xFFFF];
        value |= ((low >> bit) & 1) << plane;
        value |= ((high >> bit) & 1) << (plane + 1);
    }
    return value;
}

uint32_t nba_snes_cgram_color(const uint8_t *cgram, int index, int brightness,
                              int subtract_r, int subtract_g, int subtract_b) {
    uint16_t word = (uint16_t)(cgram[(index * 2) & 0x1FF] |
                               ((uint16_t)cgram[(index * 2 + 1) & 0x1FF] << 8));
    int r = (word & 31) - subtract_r;
    int g = ((word >> 5) & 31) - subtract_g;
    int b = ((word >> 10) & 31) - subtract_b;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (brightness < 0) brightness = 0;
    if (brightness > 15) brightness = 15;

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

bool nba_snes_sample_bg(const uint8_t *vram, int map_base, int chr_base,
                        int bits_per_pixel, bool wide, bool tall,
                        int horizontal_scroll, int vertical_scroll,
                        int x, int y, NbaSnesBgPixel *pixel) {
    if (!vram || !pixel) return false;
    int map_width = wide ? 512 : 256;
    int map_height = tall ? 512 : 256;
    int px = ((x + horizontal_scroll) % map_width + map_width) % map_width;
    int py = ((y + vertical_scroll + 1) % map_height + map_height) % map_height;
    int tile_x = px >> 3;
    int tile_y = py >> 3;
    int quadrant = 0;
    if (wide && tile_x >= 32) quadrant++;
    if (tall && tile_y >= 32) quadrant += wide ? 2 : 1;

    int entry_offset = map_base + quadrant * 0x800 +
                       ((tile_y & 31) * 32 + (tile_x & 31)) * 2;
    uint16_t entry = (uint16_t)(vram[entry_offset & 0xFFFF] |
                                ((uint16_t)vram[(entry_offset + 1) & 0xFFFF] << 8));
    int sample_x = (entry & 0x4000) ? 7 - (px & 7) : (px & 7);
    int sample_y = (entry & 0x8000) ? 7 - (py & 7) : (py & 7);
    int color_index = snes_tile_pixel(vram, chr_base, entry & 0x3FF,
                                      bits_per_pixel, sample_x, sample_y);
    if (color_index == 0) return false;

    pixel->color_index = color_index;
    pixel->palette = (entry >> 10) & 7;
    pixel->priority = (entry >> 13) & 1;
    return true;
}

bool nba_snes_mode1_begin(NbaRenderer *renderer, uint32_t backdrop,
                          bool bg3_priority_high) {
    if (!renderer) return false;
    if (!renderer->ppu_state) {
        renderer->ppu_state = (struct NbaSnesMode1State *)calloc(
            1u, sizeof(NbaSnesMode1State));
        if (!renderer->ppu_state) return false;
    }
    NbaSnesMode1State *state = renderer->ppu_state;
    uint8_t rank = mode1_rank(NBA_SNES_LAYER_BACKDROP, 0u,
                              bg3_priority_high);
    uint32_t metadata = pack_metadata(NBA_SNES_LAYER_BACKDROP, 0u, 0u, 0u,
                                      127u, rank);
    for (size_t i = 0; i < NBA_SNES_WIDTH * NBA_SNES_HEIGHT; ++i) {
        renderer->pixels[i] = backdrop;
        state->final_metadata[i] = metadata;
        state->bg_metadata[i] = metadata;
        state->bg_argb[i] = backdrop;
        state->obj_metadata[i] = 0u;
        state->obj_argb[i] = 0u;
    }
    state->bg3_priority_high = bg3_priority_high;
    return true;
}

void nba_snes_mode1_release(NbaRenderer *renderer) {
    if (!renderer) return;
    free(renderer->ppu_state);
    renderer->ppu_state = NULL;
}

static void resolve_mode1_pixel(NbaRenderer *renderer, size_t offset) {
    NbaSnesMode1State *state = renderer->ppu_state;
    uint32_t bg_meta = state->bg_metadata[offset];
    uint8_t bg_rank = (uint8_t)((bg_meta >> MODE1_META_RANK_SHIFT) & 0x0fu);
    if (state->obj_argb[offset] != 0u) {
        uint32_t obj_meta = state->obj_metadata[offset];
        uint8_t obj_rank = (uint8_t)((obj_meta >> MODE1_META_RANK_SHIFT) & 0x0fu);
        if (obj_rank > bg_rank) {
            renderer->pixels[offset] = state->obj_argb[offset];
            state->final_metadata[offset] = obj_meta;
            return;
        }
    }
    renderer->pixels[offset] = state->bg_argb[offset];
    state->final_metadata[offset] = bg_meta;
}

bool nba_snes_mode1_submit_color(NbaRenderer *renderer, int x, int y,
                                 NbaSnesLayer layer, uint8_t priority,
                                 uint8_t oam_index, uint32_t argb) {
    if (!renderer || !renderer->ppu_state || (argb >> 24) == 0u ||
        x < 0 || y < 0 || x >= NBA_SNES_WIDTH ||
        y >= NBA_SNES_HEIGHT || layer <= NBA_SNES_LAYER_BACKDROP ||
        layer > NBA_SNES_LAYER_OBJ) return false;
    size_t offset = (size_t)y * NBA_SNES_WIDTH + (size_t)x;
    NbaSnesMode1State *state = renderer->ppu_state;
    bool bg3_high = state->bg3_priority_high;
    uint8_t rank = mode1_rank(layer, priority, bg3_high);
    uint32_t metadata = pack_metadata(
        layer, priority, 0xffu, 0xffu, oam_index, rank);
    if (layer == NBA_SNES_LAYER_OBJ) {
        if (state->obj_argb[offset] != 0u) {
            uint8_t old_oam = (uint8_t)((state->obj_metadata[offset] >>
                                         MODE1_META_OAM_SHIFT) & 0x7fu);
            /* Hardware selects the first opaque OAM pixel before applying
             * that object's BG priority (the SNES OBJ priority quirk). */
            if (oam_index >= old_oam) return false;
        }
        state->obj_argb[offset] = argb;
        state->obj_metadata[offset] = metadata;
    } else {
        uint8_t old_rank = (uint8_t)((state->bg_metadata[offset] >>
                                      MODE1_META_RANK_SHIFT) & 0x0fu);
        if (rank <= old_rank) return false;
        state->bg_argb[offset] = argb;
        state->bg_metadata[offset] = metadata;
    }
    resolve_mode1_pixel(renderer, offset);
    return true;
}

bool nba_snes_mode1_submit_indexed(NbaRenderer *renderer, const uint8_t *cgram,
                                   int brightness, int x, int y,
                                   NbaSnesLayer layer, uint8_t priority,
                                   uint8_t palette_index, uint8_t color_index,
                                   uint8_t oam_index) {
    if (!renderer || !renderer->ppu_state || !cgram || color_index == 0u ||
        x < 0 || y < 0 ||
        x >= NBA_SNES_WIDTH || y >= NBA_SNES_HEIGHT) return false;
    size_t offset = (size_t)y * NBA_SNES_WIDTH + (size_t)x;
    NbaSnesMode1State *state = renderer->ppu_state;
    bool bg3_high = state->bg3_priority_high;
    uint8_t rank = mode1_rank(layer, priority, bg3_high);
    uint32_t argb = nba_snes_cgram_color(
        cgram, palette_index, brightness, 0, 0, 0);
    uint32_t metadata = pack_metadata(
        layer, priority, palette_index, color_index, oam_index, rank);
    if (layer == NBA_SNES_LAYER_OBJ) {
        if (state->obj_argb[offset] != 0u) {
            uint8_t old_oam = (uint8_t)((state->obj_metadata[offset] >>
                                         MODE1_META_OAM_SHIFT) & 0x7fu);
            if (oam_index >= old_oam) return false;
        }
        state->obj_argb[offset] = argb;
        state->obj_metadata[offset] = metadata;
    } else {
        uint8_t old_rank = (uint8_t)((state->bg_metadata[offset] >>
                                      MODE1_META_RANK_SHIFT) & 0x0fu);
        if (rank <= old_rank) return false;
        state->bg_argb[offset] = argb;
        state->bg_metadata[offset] = metadata;
    }
    resolve_mode1_pixel(renderer, offset);
    return true;
}

typedef struct {
    bool opaque;
    int x;
    int y;
    uint8_t priority;
    uint8_t palette;
    uint8_t color;
    uint8_t oam_index;
} SnapshotObjPixel;

static bool snapshot_bg_visible(const NbaSnesMode1BgConfig *bg, int x) {
    if (!bg->window_mask_main) return true;
    return !nba_snes_window_masked(x, &bg->windows[0], &bg->windows[1],
                                   bg->window_logic);
}

bool nba_snes_mode1_render_snapshot(NbaRenderer *renderer,
                                    const uint8_t *vram,
                                    const uint8_t *cgram,
                                    const uint8_t *oam,
                                    const NbaSnesMode1Snapshot *snapshot) {
    if (!renderer || !vram || !cgram || !oam || !snapshot ||
        snapshot->oam_mode != 0u) return false;
    uint32_t backdrop = nba_snes_cgram_color(cgram, 0, snapshot->brightness,
                                             0, 0, 0);
    if (!nba_snes_mode1_begin(renderer, backdrop,
                              snapshot->bg3_priority_high)) return false;

    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        uint8_t main_layers = snapshot->raster_main_screen_layers ?
            snapshot->main_screen_layers_by_scanline[y] :
            snapshot->main_screen_layers;
        for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
            for (int layer = 0; layer < 3; ++layer) {
                const NbaSnesMode1BgConfig *bg = &snapshot->backgrounds[layer];
                if (!bg->enabled || !(main_layers & (1u << layer)) ||
                    !snapshot_bg_visible(bg, x)) continue;
                NbaSnesBgPixel pixel;
                if (!nba_snes_sample_bg(vram, bg->map_base, bg->chr_base,
                                        bg->bits_per_pixel, bg->wide, bg->tall,
                                        bg->horizontal_scroll, bg->vertical_scroll,
                                        x, y, &pixel)) continue;
                uint8_t palette = (uint8_t)(pixel.palette *
                    (bg->bits_per_pixel == 4u ? 16 : 4) + pixel.color_index);
                nba_snes_mode1_submit_indexed(
                    renderer, cgram, snapshot->brightness, x, y,
                    (NbaSnesLayer)(NBA_SNES_LAYER_BG1 + layer),
                    (uint8_t)pixel.priority, palette,
                    (uint8_t)pixel.color_index, 127u);
            }
        }
    }

    bool any_objects = false;
    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        uint8_t layers = snapshot->raster_main_screen_layers ?
            snapshot->main_screen_layers_by_scanline[y] :
            snapshot->main_screen_layers;
        if (layers & 0x10u) { any_objects = true; break; }
    }
    if (!any_objects) return true;
    SnapshotObjPixel *objects = (SnapshotObjPixel *)calloc(
        NBA_SNES_WIDTH * NBA_SNES_HEIGHT, sizeof(*objects));
    if (!objects) return false;
    int first = snapshot->enable_oam_priority ?
        (snapshot->oam_ram_address / 4u) % 128u : 0;
    for (int order = 0; order < 128; ++order) {
        int index = (first + order) & 127;
        int high = (oam[512 + index / 4] >> (2 * (index % 4))) & 3;
        int destination_x0 = oam[index * 4] | ((high & 1) << 8);
        if (destination_x0 >= 256) destination_x0 -= 512;
        int destination_y0 = oam[index * 4 + 1];
        int tile = oam[index * 4 + 2];
        int attributes = oam[index * 4 + 3];
        int size = (high & 2) ? 16 : 8;
        for (int py = 0; py < size; ++py) {
            int destination_y = (destination_y0 + py) & 255;
            if (destination_y >= NBA_SNES_HEIGHT) continue;
            int source_y = (attributes & 0x80) ? size - 1 - py : py;
            for (int px = 0; px < size; ++px) {
                int destination_x = destination_x0 + px;
                if (destination_x < 0 || destination_x >= NBA_SNES_WIDTH) continue;
                size_t position = (size_t)destination_y * NBA_SNES_WIDTH +
                                  (size_t)destination_x;
                if (objects[position].opaque) continue;
                int source_x = (attributes & 0x40) ? size - 1 - px : px;
                int tile_id = (tile + (source_x >> 3) +
                               (source_y >> 3) * 16) & 255;
                int chr_base = snapshot->oam_base + tile_id * 32 +
                    ((attributes & 1) ? snapshot->oam_name_offset : 0);
                int color = snes_tile_pixel(vram, chr_base, 0, 4,
                                            source_x & 7, source_y & 7);
                if (!color) continue;
                SnapshotObjPixel *pixel = &objects[position];
                pixel->opaque = true;
                pixel->x = destination_x;
                pixel->y = destination_y;
                pixel->priority = (uint8_t)((attributes >> 4) & 3);
                pixel->palette = (uint8_t)(128 + ((attributes >> 1) & 7) * 16 + color);
                pixel->color = (uint8_t)color;
                pixel->oam_index = (uint8_t)index;
            }
        }
    }
    for (size_t i = 0; i < NBA_SNES_WIDTH * NBA_SNES_HEIGHT; ++i) {
        const SnapshotObjPixel *pixel = &objects[i];
        uint8_t layers = snapshot->raster_main_screen_layers ?
            snapshot->main_screen_layers_by_scanline[pixel->y] :
            snapshot->main_screen_layers;
        if (!pixel->opaque || !(layers & 0x10u)) continue;
        nba_snes_mode1_submit_indexed(renderer, cgram, snapshot->brightness,
                                      pixel->x, pixel->y, NBA_SNES_LAYER_OBJ,
                                      pixel->priority, pixel->palette,
                                      pixel->color, pixel->oam_index);
    }
    free(objects);
    return true;
}

bool nba_snes_mode1_pixel(const NbaRenderer *renderer, int x, int y,
                          NbaSnesMode1Pixel *pixel) {
    if (!renderer || !renderer->ppu_state || !pixel || x < 0 || y < 0 ||
        x >= NBA_SNES_WIDTH ||
        y >= NBA_SNES_HEIGHT) return false;
    size_t offset = (size_t)y * NBA_SNES_WIDTH + (size_t)x;
    uint32_t metadata = renderer->ppu_state->final_metadata[offset];
    pixel->color_index = (uint8_t)(metadata & MODE1_META_COLOR_MASK);
    pixel->palette_index = (uint8_t)(metadata >> MODE1_META_PALETTE_SHIFT);
    pixel->layer = (NbaSnesLayer)((metadata >> MODE1_META_LAYER_SHIFT) & 7u);
    pixel->priority = (uint8_t)((metadata >> MODE1_META_PRIORITY_SHIFT) & 3u);
    pixel->oam_index = (uint8_t)((metadata >> MODE1_META_OAM_SHIFT) & 0x7fu);
    pixel->rank = (uint8_t)((metadata >> MODE1_META_RANK_SHIFT) & 0x0fu);
    pixel->argb = renderer->pixels[offset];
    return pixel->layer <= NBA_SNES_LAYER_OBJ;
}

void nba_snes_mode1_stats(const NbaRenderer *renderer,
                          NbaSnesMode1Stats *stats) {
    if (!stats) return;
    memset(stats, 0, sizeof(*stats));
    if (!renderer) return;
    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
            NbaSnesMode1Pixel pixel;
            if (nba_snes_mode1_pixel(renderer, x, y, &pixel) &&
                pixel.layer <= NBA_SNES_LAYER_OBJ) stats->visible[pixel.layer]++;
        }
    }
}

const char *nba_snes_layer_name(NbaSnesLayer layer) {
    static const char *const names[] = { "BACKDROP", "BG1", "BG2", "BG3", "OBJ" };
    return (unsigned)layer < sizeof(names) / sizeof(names[0]) ? names[layer] : "INVALID";
}

bool nba_snes_mode1_write_jsonl(FILE *file, const NbaRenderer *renderer,
                                uint32_t game_frame, uint32_t state_frame) {
    if (!file || !renderer) return false;
    NbaSnesMode1Stats stats;
    nba_snes_mode1_stats(renderer, &stats);
    fprintf(file,
            "{\"type\":\"summary\",\"game_frame\":%u,\"state_frame\":%u,"
            "\"visible\":{\"backdrop\":%u,\"bg1\":%u,\"bg2\":%u,"
            "\"bg3\":%u,\"obj\":%u}}\n",
            game_frame, state_frame, stats.visible[0], stats.visible[1],
            stats.visible[2], stats.visible[3], stats.visible[4]);
    for (int y = 0; y < NBA_SNES_HEIGHT; ++y) {
        for (int x = 0; x < NBA_SNES_WIDTH; ++x) {
            NbaSnesMode1Pixel pixel;
            if (!nba_snes_mode1_pixel(renderer, x, y, &pixel)) return false;
            fprintf(file,
                    "{\"type\":\"pixel\",\"x\":%d,\"y\":%d,"
                    "\"layer\":\"%s\",\"priority\":%u,\"rank\":%u,"
                    "\"palette\":%u,\"color\":%u,\"oam\":%u,"
                    "\"argb\":\"%08X\"}\n",
                    x, y, nba_snes_layer_name(pixel.layer), pixel.priority,
                    pixel.rank, pixel.palette_index, pixel.color_index,
                    pixel.oam_index, pixel.argb);
        }
    }
    return !ferror(file);
}

bool nba_snes_window_visible(int x, uint8_t left, uint8_t right,
                             bool inverted) {
    NbaSnesWindow window = { true, inverted, left, right };
    /* TMW hides pixels for which the enabled window result is true. */
    return !nba_snes_window_masked(x, &window, NULL, NBA_SNES_WINDOW_OR);
}

bool nba_snes_window_masked(int x, const NbaSnesWindow *window1,
                            const NbaSnesWindow *window2,
                            NbaSnesWindowLogic logic) {
    bool active1 = window1 && window1->active;
    bool active2 = window2 && window2->active;
    bool first = false, second = false;
    if (active1) {
        first = x >= (int)window1->left && x <= (int)window1->right;
        if (window1->inverted) first = !first;
    }
    if (active2) {
        second = x >= (int)window2->left && x <= (int)window2->right;
        if (window2->inverted) second = !second;
    }
    if (!active1) return active2 ? second : false;
    if (!active2) return first;
    switch (logic) {
        case NBA_SNES_WINDOW_OR: return first || second;
        case NBA_SNES_WINDOW_AND: return first && second;
        case NBA_SNES_WINDOW_XOR: return first != second;
        case NBA_SNES_WINDOW_XNOR: return first == second;
        default: return false;
    }
}

bool nba_snes_mode1_self_test(void) {
    static NbaRenderer renderer;
    static bool renderer_initialized;
    static const struct {
        NbaSnesLayer layer;
        uint8_t priority;
        uint8_t expected_rank;
    } ladder[] = {
        { NBA_SNES_LAYER_BG3, 0u, 1u },
        { NBA_SNES_LAYER_OBJ, 0u, 2u },
        { NBA_SNES_LAYER_BG3, 1u, 3u },
        { NBA_SNES_LAYER_OBJ, 1u, 4u },
        { NBA_SNES_LAYER_BG2, 0u, 5u },
        { NBA_SNES_LAYER_BG1, 0u, 6u },
        { NBA_SNES_LAYER_OBJ, 2u, 7u },
        { NBA_SNES_LAYER_BG2, 1u, 8u },
        { NBA_SNES_LAYER_BG1, 1u, 9u },
        { NBA_SNES_LAYER_OBJ, 3u, 10u }
    };
    if (!renderer_initialized) {
        nba_renderer_init(&renderer);
        renderer_initialized = true;
    }
    NbaSnesMode1Pixel pixel;
    for (unsigned i = 0; i < sizeof(ladder) / sizeof(ladder[0]); ++i) {
        if (!nba_snes_mode1_begin(&renderer, 0xff000000u, false)) return false;
        if (!nba_snes_mode1_submit_color(
                &renderer, 8, 8, ladder[i].layer, ladder[i].priority,
                64u, 0xff000001u + i)) return false;
        if (!nba_snes_mode1_pixel(&renderer, 8, 8, &pixel) ||
            pixel.layer != ladder[i].layer ||
            pixel.priority != ladder[i].priority ||
            pixel.rank != ladder[i].expected_rank) return false;
    }

    /* A lower-OAM S0 pixel suppresses a later S3 pixel before BG comparison;
     * BG3H therefore hides the selected S0 pixel. */
    if (!nba_snes_mode1_begin(&renderer, 0xff000000u, false)) return false;
    if (!nba_snes_mode1_submit_color(&renderer, 7, 7, NBA_SNES_LAYER_OBJ,
                                     0u, 10u, 0xff001100u) ||
        nba_snes_mode1_submit_color(&renderer, 7, 7, NBA_SNES_LAYER_OBJ,
                                    3u, 20u, 0xff002200u) ||
        !nba_snes_mode1_submit_color(&renderer, 7, 7, NBA_SNES_LAYER_BG3,
                                     1u, 127u, 0xff003300u) ||
        !nba_snes_mode1_pixel(&renderer, 7, 7, &pixel) ||
        pixel.layer != NBA_SNES_LAYER_BG3 || pixel.argb != 0xff003300u)
        return false;

    if (!nba_snes_mode1_begin(&renderer, 0xff000000u, true)) return false;
    if (!nba_snes_mode1_submit_color(&renderer, 9, 9, NBA_SNES_LAYER_OBJ,
                                     3u, 1u, 0xff110000u) ||
        !nba_snes_mode1_submit_color(&renderer, 9, 9, NBA_SNES_LAYER_BG3,
                                     1u, 127u, 0xff220000u)) return false;
    if (!nba_snes_mode1_pixel(&renderer, 9, 9, &pixel) ||
        pixel.layer != NBA_SNES_LAYER_BG3 || pixel.rank != 11u) return false;

    if (!nba_snes_mode1_begin(&renderer, 0xff000000u, false)) return false;
    if (!nba_snes_mode1_submit_color(&renderer, 10, 10, NBA_SNES_LAYER_OBJ,
                                     2u, 20u, 0xff330000u) ||
        !nba_snes_mode1_submit_color(&renderer, 10, 10, NBA_SNES_LAYER_OBJ,
                                     2u, 10u, 0xff440000u) ||
        nba_snes_mode1_submit_color(&renderer, 10, 10, NBA_SNES_LAYER_OBJ,
                                    2u, 30u, 0xff550000u) ||
        !nba_snes_mode1_pixel(&renderer, 10, 10, &pixel) ||
        pixel.oam_index != 10u || pixel.argb != 0xff440000u) return false;

    uint8_t cgram[0x200] = {0};
    cgram[2] = 0x1fu;
    if (!nba_snes_mode1_submit_indexed(
            &renderer, cgram, 15, 11, 11, NBA_SNES_LAYER_BG1, 1u, 1u, 1u,
            127u) || !nba_snes_mode1_pixel(&renderer, 11, 11, &pixel) ||
        pixel.palette_index != 1u || pixel.color_index != 1u ||
        pixel.argb != 0xffff0000u) return false;

    NbaSnesWindow first = { true, false, 10u, 20u };
    NbaSnesWindow second = { true, false, 15u, 25u };
    return nba_snes_window_visible(166, 166u, 255u, true) &&
           nba_snes_window_visible(255, 166u, 255u, true) &&
           !nba_snes_window_visible(165, 166u, 255u, true) &&
           !nba_snes_window_visible(166, 166u, 255u, false) &&
           nba_snes_window_visible(165, 166u, 255u, false) &&
           nba_snes_window_masked(12, &first, &second, NBA_SNES_WINDOW_OR) &&
           !nba_snes_window_masked(12, &first, &second, NBA_SNES_WINDOW_AND) &&
           nba_snes_window_masked(12, &first, &second, NBA_SNES_WINDOW_XOR) &&
           !nba_snes_window_masked(12, &first, &second, NBA_SNES_WINDOW_XNOR) &&
           nba_snes_window_masked(17, &first, &second, NBA_SNES_WINDOW_AND) &&
           nba_snes_window_masked(17, &first, &second, NBA_SNES_WINDOW_XNOR);
}
