#include "nba_player_lab.h"
#include "nba_font.h"
#include "nba_snes_ppu.h"
#include <stdio.h>
#include <string.h>

#define PLAYER_HEADER_SIZE 24u
#define PLAYER_RECORD_SIZE 64u
#define PLAYER_ANIMATION_HEADER_SIZE 56u
#define PLAYER_ANIMATION_STATES NBA_PLAYER_ANIMATION_STATES
#define PLAYER_ANIMATION_BANK84_SIZE 0x8000u
#define PLAYER_ATTACHMENT_TABLE_SIZE 0x830u
#define PLAYER_LOWER_STATE_TABLE 0x4218u
#define PLAYER_UPPER_STATE_TABLE 0x42FCu

typedef struct {
    uint32_t rom_address;
    uint8_t jersey, position, height, weight;
    uint8_t appearance_a, appearance_b, appearance_key, slot;
    uint8_t palette_variant, head_raw, head_style, appearance_modifier;
    uint16_t head_resource_base, head_resource_front;
    char name[33];
} PlayerLabRecord;

static void fill(NbaRenderer *ren, int x, int y, int w, int h, uint32_t color);

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t bgr555_to_argb(uint16_t word) {
    uint32_t r = word & 31u, g = (word >> 5) & 31u, b = (word >> 10) & 31u;
    return 0xFF000000u | ((r << 3 | r >> 2) << 16) |
           ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
}

static const uint8_t *player_tile(const NbaAssetItem *tiles, uint8_t tile_id) {
    if (!tiles || !tiles->data || tiles->size < 20 ||
        memcmp(tiles->data, "NBPTILE2", 8) != 0) return NULL;
    const uint8_t *data = (const uint8_t *)tiles->data;
    uint32_t count = read_u32(data + 12);
    if (count > (tiles->size - 20u) / 40u) return NULL;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t *record = data + 20u + i * 40u;
        if (record[0] == tile_id) return record + 8;
    }
    return NULL;
}

static const uint8_t *player_head_tile(const NbaAssetItem *tiles,
                                       uint16_t resource_id) {
    if (!tiles || !tiles->data || tiles->size < 20 ||
        memcmp(tiles->data, "NBPTILE2", 8) != 0) return NULL;
    const uint8_t *data = (const uint8_t *)tiles->data;
    uint32_t pose_count = read_u32(data + 12), head_count = read_u32(data + 16);
    size_t offset = 20u + (size_t)pose_count * 40u;
    if (pose_count > 64 || head_count > (tiles->size - offset) / 40u) return NULL;
    for (uint32_t i = 0; i < head_count; ++i) {
        const uint8_t *record = data + offset + i * 40u;
        if (read_u16(record) == resource_id) return record + 8;
    }
    return NULL;
}

static uint8_t tile_pixel(const uint8_t *tile, int x, int y) {
    int bit = 7 - x;
    return (uint8_t)(((tile[y * 2] >> bit) & 1) |
                     (((tile[y * 2 + 1] >> bit) & 1) << 1) |
                     (((tile[16 + y * 2] >> bit) & 1) << 2) |
                     (((tile[16 + y * 2 + 1] >> bit) & 1) << 3));
}

static const uint8_t *player_palette(const NbaAssetPack *assets, int team,
                                     int side, int variant) {
    const NbaAssetItem *item = nba_assets_get(assets, NBA_ASSET_PLAYER_PALETTE_TABLES);
    if (!item || !item->data || item->size < 36 ||
        memcmp(item->data, "NBPALET2", 8) != 0 || team < 0 || team >= 29 ||
        side < 0 || side >= 2 || variant < 0 || variant >= 3) return NULL;
    size_t offset = 36u + (((size_t)team * 2u + (size_t)side) * 3u +
                           (size_t)variant) * 32u;
    return offset + 32u <= item->size ? (const uint8_t *)item->data + offset : NULL;
}

static const uint8_t *animation_data(const NbaAssetPack *assets,
                                     const NbaAssetItem **item_out) {
    const NbaAssetItem *item = nba_assets_get(assets, NBA_ASSET_PLAYER_ANIMATIONS);
    if (!item || !item->data || item->size < PLAYER_ANIMATION_HEADER_SIZE ||
        memcmp(item->data, "NBPANIM1", 8) != 0 ||
        read_u32((const uint8_t *)item->data + 8) != 4u ||
        read_u32((const uint8_t *)item->data + 12) != PLAYER_ANIMATION_STATES)
        return NULL;
    if (item_out) *item_out = item;
    return (const uint8_t *)item->data;
}

static const uint8_t *animation_bank84(const NbaAssetPack *assets,
                                       const NbaAssetItem **item_out) {
    const NbaAssetItem *item;
    const uint8_t *data = animation_data(assets, &item);
    if (!data) return NULL;
    uint32_t offset = read_u32(data + 20);
    if (offset > item->size || PLAYER_ANIMATION_BANK84_SIZE > item->size - offset)
        return NULL;
    if (item_out) *item_out = item;
    return data + offset;
}

static const uint8_t *animation_descriptor(const NbaAssetPack *assets,
                                           uint32_t table_offset,
                                           uint8_t state) {
    const uint8_t *bank = animation_bank84(assets, NULL);
    if (!bank || state >= PLAYER_ANIMATION_STATES ||
        table_offset + (size_t)state * 2u + 2u > PLAYER_ANIMATION_BANK84_SIZE)
        return NULL;
    uint16_t pointer = read_u16(bank + table_offset + (size_t)state * 2u);
    if (pointer < 0x8000u || (size_t)(pointer - 0x8000u) + 24u >
                              PLAYER_ANIMATION_BANK84_SIZE)
        return NULL;
    return bank + (pointer - 0x8000u);
}

static const uint8_t *animation_resource(const NbaAssetPack *assets,
                                         uint16_t resource_id,
                                         uint32_t *size_out) {
    const NbaAssetItem *item;
    const uint8_t *data = animation_data(assets, &item);
    if (!data) return NULL;
    uint32_t count = read_u32(data + 16);
    uint32_t directory = read_u32(data + 28);
    if (directory > item->size) return NULL;
    if (count > (item->size - directory) / 12u) return NULL;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t *entry = data + directory + (size_t)i * 12u;
        uint16_t id = read_u16(entry);
        if (id < resource_id) continue;
        if (id > resource_id) break;
        uint32_t offset = read_u32(entry + 4), size = read_u32(entry + 8);
        if (offset > item->size || size > item->size - offset) return NULL;
        if (size_out) *size_out = size;
        return data + offset;
    }
    return NULL;
}

static int8_t animation_attachment(const NbaAssetPack *assets,
                                   uint16_t resource_id, bool y_axis) {
    const NbaAssetItem *item;
    const uint8_t *data = animation_data(assets, &item);
    if (!data || resource_id >= PLAYER_ATTACHMENT_TABLE_SIZE) return 0;
    uint32_t offset = read_u32(data + 24) +
        (y_axis ? PLAYER_ATTACHMENT_TABLE_SIZE : 0u) + resource_id;
    if (offset >= item->size) return 0;
    return (int8_t)data[offset];
}

static int8_t number_attachment(const NbaAssetPack *assets,
                                uint16_t upper_resource, bool y_axis) {
    const NbaAssetItem *item;
    const uint8_t *data = animation_data(assets, &item);
    if (!data || upper_resource >= PLAYER_ATTACHMENT_TABLE_SIZE) return 0;
    uint32_t offset = read_u32(data + 44) +
        (y_axis ? PLAYER_ATTACHMENT_TABLE_SIZE : 0u) + upper_resource;
    if (offset >= item->size) return 0;
    return (int8_t)data[offset];
}

static int8_t number_visibility_for_upper(const NbaAssetPack *assets,
                                          uint16_t upper_resource) {
    const NbaAssetItem *item;
    const uint8_t *data = animation_data(assets, &item);
    if (!data || upper_resource >= PLAYER_ATTACHMENT_TABLE_SIZE) return -1;
    uint32_t table = read_u32(data + 52);
    if (table > item->size || upper_resource >= item->size - table) return -1;
    /* $87:A506-$A51E suppresses the overlay when this signed byte is negative. */
    return (int8_t)data[table + upper_resource];
}

static bool number_allowed_for_upper(const NbaAssetPack *assets,
                                     uint16_t upper_resource) {
    return number_visibility_for_upper(assets, upper_resource) >= 0;
}

static const uint8_t *jersey_asset_table(const NbaAssetPack *assets,
                                         bool bcd_table) {
    const NbaAssetItem *item;
    const uint8_t *data = animation_data(assets, &item);
    if (!data) return NULL;
    uint32_t offset = read_u32(data + (bcd_table ? 40u : 36u));
    size_t size = bcd_table ? 100u : 90u * 32u;
    return offset <= item->size && size <= item->size - offset
        ? data + offset : NULL;
}

static bool jersey_palette(const NbaAssetPack *assets, int team, int side,
                           uint8_t palette[32]) {
    const NbaAssetItem *item;
    const uint8_t *data = animation_data(assets, &item);
    if (!data || team < 0 || team >= NBA_TEAM_COUNT || side < 0 || side >= 2)
        return false;
    uint32_t offset = read_u32(data + 48);
    if (offset > item->size || 64u + NBA_TEAM_COUNT * 4u > item->size - offset)
        return false;
    /* Palette 7 is the second half of $AF:E99F copied by $85:8CAE-$8CB9. */
    memcpy(palette, data + offset + 32u, 32u);
    const uint8_t *team_colors = data + offset + 64u + (size_t)team * 4u;
    const uint8_t *uniform = player_palette(assets, team, side, 0);
    if (!uniform) return false;
    unsigned team_color = side == 0 ? 9u : 13u;
    unsigned uniform_color = side == 0 ? 10u : 14u;
    memcpy(palette + team_color * 2u, team_colors + side * 2u, 2u);
    /* $85:8CDB/$8CE1 patch the selected uniform's color 10 into palette 7. */
    memcpy(palette + uniform_color * 2u, uniform + 10u * 2u, 2u);
    return true;
}

static uint8_t animation_frame_index(const uint8_t *descriptor,
                                     const uint8_t *bank, uint32_t tick) {
    uint16_t count = read_u16(descriptor + 6);
    int16_t mode = (int16_t)read_u16(descriptor);
    if (!count) return 0;
    if (mode == 1) {
        uint16_t timing_pointer = read_u16(descriptor + 4);
        if (timing_pointer >= 0x8000u) {
            const uint8_t *timings = bank + (timing_pointer - 0x8000u);
            uint32_t total = 0;
            for (uint16_t i = 0; i < count; ++i) {
                uint32_t duration = read_u16(timings + (size_t)i * 2u) >> 8;
                total += duration ? duration : 1u;
            }
            if (total) {
                uint32_t cursor = tick % total;
                for (uint16_t i = 0; i < count; ++i) {
                    uint32_t duration = read_u16(timings + (size_t)i * 2u) >> 8;
                    duration = duration ? duration : 1u;
                    if (cursor < duration) return (uint8_t)i;
                    cursor -= duration;
                }
            }
        }
    }
    uint32_t duration = read_u16(descriptor + 4) >> 8;
    if (!duration) duration = 6u;
    return (uint8_t)((tick / duration) % count);
}

static uint16_t animation_frame_resource(const uint8_t *descriptor,
                                         const uint8_t *bank, uint8_t direction,
                                         uint8_t frame) {
    uint16_t count = read_u16(descriptor + 6);
    if (!count) return 0;
    uint16_t pointer = read_u16(descriptor + 8u + (direction & 7u) * 2u);
    if (pointer < 0x8000u || (size_t)(pointer - 0x8000u) + count * 2u >
                              PLAYER_ANIMATION_BANK84_SIZE)
        return 0;
    return read_u16(bank + (pointer - 0x8000u) + (frame % count) * 2u);
}

static uint8_t animation_lower_state(uint8_t upper_state) {
    /* Controlled-player Mesen trace: these action overlays deliberately keep
     * a different locomotion state in player +$32 while +$30 drives arms. */
    switch (upper_state) {
        case 0x17: return 0x32;
        case 0x2b: return 0x0c;
        case 0x2f: return 0x05;
        case 0x30: return 0x0c;
        default: return upper_state;
    }
}

static bool draw_player_pose(NbaRenderer *ren, const NbaAssetPack *assets,
                             const PlayerLabRecord *player, int team) {
    const NbaAssetItem *tiles = nba_assets_get(assets, NBA_ASSET_PLAYER_TILE_SOURCES);
    const NbaAssetItem *layout = nba_assets_get(assets, NBA_ASSET_PLAYER_POSE_LAYOUT);
    if (!tiles || !layout || !layout->data || layout->size < 16 ||
        memcmp(layout->data, "NBPPOSE2", 8) != 0) return false;
    const uint8_t *data = (const uint8_t *)layout->data;
    uint32_t count = read_u32(data + 12);
    if (count > 32 || 16u + (size_t)count * 7u > layout->size) return false;
    const uint8_t *palette = player_palette(assets, team, 0,
                                            player->palette_variant);
    if (!palette) return false;
    const int screen_x = 188, screen_y = 52, origin_x = 16, origin_y = 47;
    fill(ren, 176, 40, 68, 156, 0xFF091522u);
    /* Low OAM indices win overlap, so draw the packed entries in reverse. */
    for (uint32_t n = count; n-- > 0;) {
        const uint8_t *entry = data + 16u + n * 7u;
        int x = (int16_t)read_u16(entry), y = (int16_t)read_u16(entry + 2);
        uint8_t base_tile = entry[4], attr = entry[5], size = entry[6];
        for (int py = 0; py < size; ++py) for (int px = 0; px < size; ++px) {
            int sx = (attr & 0x40) ? size - 1 - px : px;
            int sy = (attr & 0x80) ? size - 1 - py : py;
            uint8_t tile_id = (uint8_t)(base_tile + (sx >> 3) + (sy >> 3) * 16);
            const uint8_t *tile = base_tile == 0x4c
                ? player_head_tile(tiles, player->head_resource_front)
                : player_tile(tiles, tile_id);
            if (!tile) continue;
            uint8_t color_index = tile_pixel(tile, sx & 7, sy & 7);
            if (!color_index) continue;
            uint32_t color = bgr555_to_argb(read_u16(palette + color_index * 2u));
            int dx = screen_x + (x + px - origin_x) * 2;
            int dy = screen_y + (y + py - origin_y) * 2;
            if (dx < 0 || dx + 1 >= NBA_SNES_WIDTH || dy < 0 || dy + 1 >= NBA_SNES_HEIGHT)
                continue;
            ren->pixels[dy * NBA_SNES_WIDTH + dx] = color;
            ren->pixels[dy * NBA_SNES_WIDTH + dx + 1] = color;
            ren->pixels[(dy + 1) * NBA_SNES_WIDTH + dx] = color;
            ren->pixels[(dy + 1) * NBA_SNES_WIDTH + dx + 1] = color;
        }
    }
    return true;
}

static const uint8_t *resource_tile(const uint8_t *resource, uint32_t size,
                                    uint8_t target) {
    uint16_t part_count = read_u16(resource) & 0x7fffu;
    size_t graphics_offset = 10u + (size_t)part_count * 7u;
    if (graphics_offset > size) return NULL;
    bool present[256] = { false };
    for (uint16_t i = 0; i < part_count; ++i) {
        const uint8_t *part = resource + 10u + (size_t)i * 7u;
        uint8_t tile = (uint8_t)read_u16(part + 4);
        present[tile] = true;
        if (part[6] == 0xffu) {
            present[(uint8_t)(tile + 1u)] = true;
            present[(uint8_t)(tile + 16u)] = true;
            present[(uint8_t)(tile + 17u)] = true;
        }
    }
    if (!present[target]) return NULL;
    size_t rank = 0;
    for (unsigned i = 0; i < target; ++i) if (present[i]) ++rank;
    return graphics_offset + rank * 32u + 32u <= size
        ? resource + graphics_offset + rank * 32u : NULL;
}

static void draw_animation_resource(NbaRenderer *ren, const NbaAssetPack *assets,
                                    uint16_t resource_id, const uint8_t *palette,
                                    int origin_x, int origin_y, bool flip,
                                    const uint8_t *tile_override, int scale) {
    uint32_t resource_size;
    const uint8_t *resource = animation_resource(assets, resource_id, &resource_size);
    if (!resource || resource_size < 10u) return;
    uint16_t part_count = read_u16(resource) & 0x7fffu;
    if (part_count > 32u || 10u + (size_t)part_count * 7u > resource_size) return;
    /* Lower OAM indexes win, matching $80:B391's queued part order. */
    for (uint16_t n = part_count; n-- > 0;) {
        const uint8_t *part = resource + 10u + (size_t)n * 7u;
        int x = (int16_t)read_u16(part), y = (int16_t)read_u16(part + 2);
        uint16_t attributes = read_u16(part + 4);
        uint8_t base_tile = (uint8_t)attributes;
        int extent = part[6] == 0xffu ? 16 : 8;
        /* $80:B452-$B498 mirrors an OBJ part as origin - descriptor_x - 7
         * for an 8x8 part, or -15 for a 16x16 part.  Those are the final
         * pixel indices (extent - 1), not the pixel counts. */
        int part_x = flip ? -x - (extent - 1) : x;
        for (int py = 0; py < extent; ++py) for (int px = 0; px < extent; ++px) {
            bool x_flip = (attributes & 0x4000u) != 0;
            int sx = x_flip != flip ? extent - 1 - px : px;
            int sy = attributes & 0x8000u ? extent - 1 - py : py;
            uint8_t tile_id = (uint8_t)(base_tile + (sx >> 3) +
                                        (sy >> 3) * 16);
            const uint8_t *tile = tile_override && tile_id == base_tile
                ? tile_override : resource_tile(resource, resource_size, tile_id);
            if (!tile) continue;
            uint8_t color_index = tile_pixel(tile, sx & 7, sy & 7);
            if (!color_index) continue;
            uint32_t color = bgr555_to_argb(read_u16(palette + color_index * 2u));
            int dx = origin_x + (part_x + px) * scale;
            int dy = origin_y + (y + py) * scale;
            for (int oy = 0; oy < scale; ++oy) for (int ox = 0; ox < scale; ++ox) {
                int tx = dx + ox, ty = dy + oy;
                if (tx >= 0 && tx < NBA_SNES_WIDTH && ty >= 0 && ty < NBA_SNES_HEIGHT)
                    ren->pixels[ty * NBA_SNES_WIDTH + tx] = color;
            }
        }
    }
}

static uint16_t jersey_mask_word(const uint8_t *source, unsigned offset,
                                 bool first_side) {
    uint16_t word = read_u16(source + offset);
    if (offset >= 16u && first_side) word &= 0xff00u;
    uint16_t low = offset >= 16u
        ? read_u16(source + offset - 16u) & 0xffu : word & 0xffu;
    uint16_t mask = (uint16_t)(word & (low | (low << 8)));
    return offset >= 16u && first_side ? mask & 0xff00u : mask;
}

static void write_u16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static bool compose_jersey_tile(const NbaAssetPack *assets, uint8_t jersey,
                                uint8_t direction, uint8_t tile[32]) {
    /* $87:A99E maps directions 0..7 to the raw/derived WRAM buffers generated
       by $87:B05B-$B354. Keep the perspective glyph and handedness exact. */
    static const uint8_t orientation[8] = { 1, 0xff, 2, 0, 1, 0xff, 2, 0 };
    static const bool derived[8] = { false, false, true, true,
                                    true, false, false, false };
    direction &= 7u;
    if (jersey == 0xffu || jersey >= 100u || orientation[direction] == 0xffu)
        return false;
    const uint8_t *digits = jersey_asset_table(assets, false);
    const uint8_t *bcd = jersey_asset_table(assets, true);
    if (!digits || !bcd) return false;
    uint8_t packed = bcd[jersey];
    unsigned base = orientation[direction] * 30u;
    unsigned first = packed < 0x10u
        ? base + 20u + packed : base + (packed & 0x0fu);
    unsigned second = packed < 0x10u
        ? 0xffu : base + 10u + (packed >> 4);
    const uint8_t *source_a = digits + first * 32u;
    const uint8_t *source_b = second == 0xffu ? NULL : digits + second * 32u;
    /* The lab shows the home/first-side uniform, matching $87:B085-$B08E. */
    for (unsigned offset = 0; offset < 32u; offset += 2u) {
        uint16_t word;
        if (derived[direction]) {
            word = jersey_mask_word(source_a, offset, true);
            if (source_b) word |= jersey_mask_word(source_b, offset, true);
        } else {
            word = read_u16(source_a + offset);
            if (source_b) word |= read_u16(source_b + offset);
            if (offset >= 16u) word &= 0xff00u;
        }
        write_u16(tile + offset, word);
    }
    return true;
}

static bool draw_player_animation_at(NbaRenderer *ren, const NbaAssetPack *assets,
                                     const PlayerLabRecord *player, uint8_t team,
                                     uint8_t side, uint8_t upper_state,
                                     uint8_t lower_state, uint8_t direction,
                                     uint32_t upper_tick, uint32_t lower_tick,
                                     int lower_x, int lower_y, int scale) {
    const uint8_t *bank = animation_bank84(assets, NULL);
    const uint8_t *upper = animation_descriptor(
        assets, PLAYER_UPPER_STATE_TABLE, upper_state);
    const uint8_t *lower = animation_descriptor(
        assets, PLAYER_LOWER_STATE_TABLE, lower_state);
    if (!bank || !upper) return false;
    if (!lower) lower = animation_descriptor(assets, PLAYER_LOWER_STATE_TABLE, 0);
    if (!lower) return false;
    const uint8_t *palette = player_palette(assets, team, side,
                                            player->palette_variant);
    if (!palette) return false;

    uint8_t lower_frame = animation_frame_index(lower, bank, lower_tick);
    uint8_t upper_frame = (int16_t)read_u16(upper) < 0
        ? (uint8_t)(lower_frame % read_u16(upper + 6))
        : animation_frame_index(upper, bank, upper_tick);
    uint16_t lower_resource = animation_frame_resource(
        lower, bank, direction, lower_frame);
    uint16_t upper_resource = animation_frame_resource(
        upper, bank, direction, upper_frame);
    uint16_t head_resource = (uint16_t)(player->head_resource_base +
        read_u16(bank + 0x436eu + (direction & 7u) * 2u));
    bool flip = (direction & 7u) < 3u;

    int lower_attach_x = animation_attachment(assets, lower_resource, false);
    int upper_x = lower_x + (flip ? -lower_attach_x : lower_attach_x) * scale;
    int upper_y = lower_y + animation_attachment(assets, lower_resource, true) * scale;
    int upper_attach_x = animation_attachment(assets, upper_resource, false);
    int head_x = upper_x + (flip ? -upper_attach_x : upper_attach_x) * scale;
    int head_y = upper_y + animation_attachment(assets, upper_resource, true) * scale;
    draw_animation_resource(ren, assets, lower_resource, palette, lower_x, lower_y,
                            flip, NULL, scale);
    draw_animation_resource(ren, assets, upper_resource, palette, upper_x, upper_y,
                            flip, NULL, scale);
    uint8_t number_tile[32];
    if (number_allowed_for_upper(assets, upper_resource) &&
        compose_jersey_tile(assets, player->jersey, direction, number_tile)) {
        static const uint16_t number_resources[8] = {
            0x0593, 0xffff, 0x0591, 0x0592, 0x0593, 0xffff, 0x0591, 0x0592
        };
        int number_attach_x = number_attachment(assets, upper_resource, false);
        int number_x = upper_x + (flip ? -number_attach_x : number_attach_x) * scale;
        int number_y = upper_y + number_attachment(assets, upper_resource, true) * scale;
        uint8_t number_palette[32];
        if (!jersey_palette(assets, team, side, number_palette)) return false;
        uint16_t number_resource = number_resources[direction & 7u];
        /* $80:AE78-$80:AE86 applies X flip only to overlay $0591. The
           direction-specific number resources must not inherit the body flip. */
        bool number_flip = number_resource == 0x0591u;
        draw_animation_resource(ren, assets, number_resource, number_palette,
                                number_x, number_y, number_flip, number_tile, scale);
    }
    draw_animation_resource(ren, assets, head_resource, palette, head_x, head_y,
                            flip, NULL, scale);
    return true;
}

static bool player_record(const NbaAssetPack *assets, int team, int player,
                          PlayerLabRecord *out) {
    const NbaAssetItem *item = nba_assets_get(assets, NBA_ASSET_PLAYER_ROSTERS);
    if (!item || !item->data || item->size < PLAYER_HEADER_SIZE ||
        team < 0 || team >= NBA_TEAM_COUNT || player < 0 ||
        player >= NBA_PLAYER_ROSTER_SIZE || memcmp(item->data, "NBPROST1", 8) != 0)
        return false;
    const uint8_t *base = (const uint8_t *)item->data;
    size_t offset = PLAYER_HEADER_SIZE +
        ((size_t)team * NBA_PLAYER_ROSTER_SIZE + (size_t)player) * PLAYER_RECORD_SIZE;
    if (offset + PLAYER_RECORD_SIZE > item->size) return false;
    const uint8_t *p = base + offset;
    memset(out, 0, sizeof(*out));
    out->rom_address = read_u32(p);
    out->jersey = p[4]; out->position = p[5]; out->height = p[6]; out->weight = p[7];
    out->appearance_a = p[8]; out->appearance_b = p[9];
    out->appearance_key = p[10]; out->slot = p[11];
    out->palette_variant = p[12]; out->head_raw = p[13];
    out->head_style = p[14]; out->appearance_modifier = p[15];
    out->head_resource_base = read_u16(p + 16);
    out->head_resource_front = read_u16(p + 18);
    memcpy(out->name, p + 32, 32); out->name[32] = '\0';
    return true;
}

bool nba_player_gameplay_shot_ratings(const NbaAssetPack *assets,
                                      uint8_t team, uint8_t roster_slot,
                                      uint8_t *two_point, uint8_t *three_point) {
    PlayerLabRecord player;
    if (!two_point || !three_point ||
        !player_record(assets, team, roster_slot, &player)) return false;
    /* Roster profile `+$36/+$37`, selected by `$86:A4A5` for 2/3 points. */
    *two_point = player.appearance_a;
    *three_point = player.appearance_b;
    return true;
}

bool nba_player_sprite_render(NbaRenderer *renderer, const NbaAssetPack *assets,
                              uint8_t team, uint8_t roster_slot, uint8_t side,
                              uint8_t upper_state, uint8_t direction,
                              uint32_t animation_tick, int origin_x,
                              int origin_y, int scale) {
    return nba_player_sprite_render_split(renderer, assets, team, roster_slot,
        side, upper_state, animation_lower_state(upper_state), direction,
        animation_tick, animation_tick, origin_x, origin_y, scale);
}

bool nba_player_sprite_render_split(NbaRenderer *renderer,
                                    const NbaAssetPack *assets, uint8_t team,
                                    uint8_t roster_slot, uint8_t side,
                                    uint8_t upper_state, uint8_t lower_state,
                                    uint8_t direction, uint32_t upper_tick,
                                    uint32_t lower_tick, int origin_x,
                                    int origin_y, int scale) {
    PlayerLabRecord player;
    if (!renderer || !assets || side > 1u || scale < 1 ||
        !player_record(assets, team, roster_slot, &player)) return false;
    return draw_player_animation_at(renderer, assets, &player, team, side,
                                    upper_state, lower_state, direction,
                                    upper_tick, lower_tick, origin_x, origin_y,
                                    scale);
}

bool nba_player_animation_resources(const NbaAssetPack *assets,
                                    uint8_t upper_state, uint8_t lower_state,
                                    uint8_t direction, uint32_t upper_tick,
                                    uint32_t lower_tick,
                                    uint16_t *upper_resource,
                                    uint16_t *lower_resource) {
    const uint8_t *bank = animation_bank84(assets, NULL);
    const uint8_t *upper = animation_descriptor(
        assets, PLAYER_UPPER_STATE_TABLE, upper_state);
    const uint8_t *lower = animation_descriptor(
        assets, PLAYER_LOWER_STATE_TABLE, lower_state);
    if (!bank || !upper || !lower || !upper_resource || !lower_resource)
        return false;
    uint8_t lower_frame = animation_frame_index(lower, bank, lower_tick);
    uint8_t upper_frame = (int16_t)read_u16(upper) < 0
        ? (uint8_t)(lower_frame % read_u16(upper + 6))
        : animation_frame_index(upper, bank, upper_tick);
    *lower_resource = animation_frame_resource(lower, bank, direction,
                                                lower_frame);
    *upper_resource = animation_frame_resource(upper, bank, direction,
                                                upper_frame);
    return *lower_resource != 0u && *upper_resource != 0u;
}

static void fill(NbaRenderer *ren, int x, int y, int w, int h, uint32_t color) {
    for (int py = y; py < y + h && py < NBA_SNES_HEIGHT; ++py)
        for (int px = x; px < x + w && px < NBA_SNES_WIDTH; ++px)
            if (px >= 0 && py >= 0) ren->pixels[py * NBA_SNES_WIDTH + px] = color;
}

static void text(NbaRenderer *ren, int x, int y, const char *value, uint32_t color) {
    nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, x, y, value,
                         color, 0xFF081018u, 1);
}

void nba_player_lab_init(NbaPlayerLab *lab, const NbaAssetPack *assets) {
    if (!lab) return;
    memset(lab, 0, sizeof(*lab));
    lab->team = 3; /* Chicago, matching the captured live-game evidence. */
    lab->animation_state = 3; /* Eight-frame movement state proven at $84:D23E. */
    lab->direction = 6;       /* Front/three-quarter gameplay presentation. */
    lab->has_assets = nba_assets_get(assets, NBA_ASSET_PLAYER_ROSTERS) &&
                      nba_assets_get(assets, NBA_ASSET_PLAYER_TILE_SOURCES) &&
                      nba_assets_get(assets, NBA_ASSET_PLAYER_PALETTE_TABLES) &&
                      nba_assets_get(assets, NBA_ASSET_PLAYER_POSE_LAYOUT) &&
                      animation_data(assets, NULL);
}

void nba_player_lab_toggle(NbaPlayerLab *lab, const NbaAssetPack *assets) {
    if (!lab) return;
    lab->has_assets = nba_assets_get(assets, NBA_ASSET_PLAYER_ROSTERS) &&
                      nba_assets_get(assets, NBA_ASSET_PLAYER_TILE_SOURCES) &&
                      nba_assets_get(assets, NBA_ASSET_PLAYER_PALETTE_TABLES) &&
                      nba_assets_get(assets, NBA_ASSET_PLAYER_POSE_LAYOUT) &&
                      animation_data(assets, NULL);
    lab->is_active = !lab->is_active;
    printf("[PLAYER LAB] %s (F9), asset-pack=%s team=%u player=%u\n",
           lab->is_active ? "opened" : "closed",
           lab->has_assets ? "verified" : "missing",
           (unsigned)lab->team, (unsigned)lab->player);
}

void nba_player_lab_update(NbaPlayerLab *lab, const NbaAssetPack *assets,
                           const NbaInput *input) {
    if (!lab || !lab->is_active || !input) return;
    if (input->pressed & NBA_BTN_LEFT)
        lab->team = (uint8_t)((lab->team + NBA_TEAM_COUNT - 1) % NBA_TEAM_COUNT);
    if (input->pressed & NBA_BTN_RIGHT)
        lab->team = (uint8_t)((lab->team + 1) % NBA_TEAM_COUNT);
    if (input->pressed & NBA_BTN_UP)
        lab->player = (uint8_t)((lab->player + NBA_PLAYER_ROSTER_SIZE - 1) %
                                NBA_PLAYER_ROSTER_SIZE);
    if (input->pressed & NBA_BTN_DOWN)
        lab->player = (uint8_t)((lab->player + 1) % NBA_PLAYER_ROSTER_SIZE);
    int animation_step = (input->pressed & NBA_BTN_L) ? -1 :
                         (input->pressed & NBA_BTN_R) ? 1 : 0;
    if (animation_step) {
        uint8_t state = lab->animation_state;
        do {
            state = (uint8_t)((state + PLAYER_ANIMATION_STATES + animation_step) %
                              PLAYER_ANIMATION_STATES);
        } while (!animation_descriptor(assets, PLAYER_UPPER_STATE_TABLE, state));
        lab->animation_state = state;
        lab->animation_tick = 0;
    }
    if (input->pressed & NBA_BTN_Y) {
        lab->direction = (uint8_t)((lab->direction + 7u) & 7u);
        lab->animation_tick = 0;
    }
    if (input->pressed & NBA_BTN_X) {
        lab->direction = (uint8_t)((lab->direction + 1u) & 7u);
        lab->animation_tick = 0;
    }
    if (input->pressed & NBA_BTN_A) lab->animation_paused = !lab->animation_paused;
    if (!lab->animation_paused) ++lab->animation_tick;
}

void nba_player_lab_print(const NbaPlayerLab *lab, const NbaAssetPack *assets) {
    PlayerLabRecord p;
    if (!lab || !player_record(assets, lab->team, lab->player, &p)) return;
    const uint8_t *bank = animation_bank84(assets, NULL);
    const uint8_t *upper = animation_descriptor(
        assets, PLAYER_UPPER_STATE_TABLE, lab->animation_state);
    uint16_t upper_resource = 0xffffu;
    int8_t number_gate = -1;
    if (bank && upper) {
        uint8_t frame = animation_frame_index(upper, bank, lab->animation_tick);
        upper_resource = animation_frame_resource(
            upper, bank, lab->direction, frame);
        number_gate = number_visibility_for_upper(assets, upper_resource);
    }
    bool number_visible = p.jersey != 0xffu && lab->direction != 1u &&
                          lab->direction != 5u && number_gate >= 0;
    printf("[PLAYER LAB] team=%02u %s roster=%02u name=%s jersey=%u pos=%u "
           "height=%u appearance=%02X+%02X=%02X head=%02X/$%04X palette=%u "
           "animation=$%02X lower=$%02X direction=%u angle=%u flip=%s tick=%u "
           "upper=$%04X number=%s gate=$%02X rom=$%06X source=asset-pack\n",
           (unsigned)lab->team, nba_team_records[lab->team].name,
           (unsigned)lab->player, p.name, (unsigned)p.jersey,
           (unsigned)p.position, (unsigned)p.height, p.appearance_a,
           p.appearance_b, p.appearance_key, p.head_style,
           p.head_resource_front, p.palette_variant, lab->animation_state,
           animation_lower_state(lab->animation_state), lab->direction,
           (unsigned)lab->direction * 45u, lab->direction < 3u ? "on" : "off",
           (unsigned)lab->animation_tick, upper_resource,
           number_visible ? "visible" : "hidden", (uint8_t)number_gate,
           p.rom_address);
}

void nba_player_lab_render(const NbaPlayerLab *lab, const NbaAssetPack *assets,
                           NbaRenderer *ren) {
    if (!lab || !lab->is_active || !ren) return;
    fill(ren, 0, 0, 256, 224, 0xFF07111Du);
    fill(ren, 4, 4, 248, 216, 0xFF102238u);
    fill(ren, 6, 6, 244, 18, 0xFF173B59u);
    text(ren, 12, 11, "PLAYER LAB [F9]  ROM ASSET PACK", 0xFF79D7FFu);
    text(ren, 12, 28, "ARROWS TEAM/ROSTER  Q/E ANIM", 0xFF9EF7A9u);
    if (!lab->has_assets) {
        text(ren, 12, 52, "PLAYER ASSETS MISSING - REBUILD PACK", 0xFFFF806Eu);
        return;
    }

    PlayerLabRecord p;
    if (!player_record(assets, lab->team, lab->player, &p)) {
        text(ren, 12, 52, "INVALID ROSTER ASSET", 0xFFFF806Eu); return;
    }
    char line[80];
    snprintf(line, sizeof(line), "TEAM %02u  %s", (unsigned)lab->team,
             nba_team_records[lab->team].name);
    text(ren, 12, 45, line, 0xFFFFD66Du);
    snprintf(line, sizeof(line), "PLAYER %02u/12 %.13s",
             (unsigned)lab->player + 1, p.name);
    text(ren, 12, 57, line, 0xFFFFFFFFu);
    static const char *const positions[] = { "C", "PF", "SF", "SG", "PG" };
    char jersey[8];
    if (p.jersey == 0xff) snprintf(jersey, sizeof(jersey), "-");
    else snprintf(jersey, sizeof(jersey), "%u", (unsigned)p.jersey);
    snprintf(line, sizeof(line), "NO:%s  POS:%s  HT:%u'%u\"", jersey,
             p.position < 5 ? positions[p.position] : "?",
             (unsigned)(p.height / 12), (unsigned)(p.height % 12));
    text(ren, 12, 69, line, 0xFFBED0E2u);
    snprintf(line, sizeof(line), "APPEAR +36:%02X +37:%02X KEY:%02X", p.appearance_a,
             p.appearance_b, p.appearance_key);
    text(ren, 12, 81, line, 0xFFFFB06Eu);
    snprintf(line, sizeof(line), "ROM $%06X / $84:E640", p.rom_address);
    text(ren, 12, 93, line, 0xFF9FB2C8u);

    fill(ren, 176, 40, 68, 156, 0xFF091522u);
    if (!draw_player_animation_at(ren, assets, &p, lab->team, 0,
                                  lab->animation_state,
                                  animation_lower_state(lab->animation_state),
                                  lab->direction, lab->animation_tick,
                                  lab->animation_tick, 210, 174, 2))
        draw_player_pose(ren, assets, &p, lab->team);
    snprintf(line, sizeof(line), "ANIM U$%02X L$%02X D%u %s", lab->animation_state,
             animation_lower_state(lab->animation_state), lab->direction,
             lab->animation_paused ? "PAUSE" : "PLAY");
    text(ren, 12, 116, line, 0xFF79D7FFu);
    snprintf(line, sizeof(line), "HEAD %02X RES $%04X (+2)",
             p.head_style, p.head_resource_front);
    text(ren, 12, 132, line, 0xFFBED0E2u);
    snprintf(line, sizeof(line), "DIR %u %3uDEG FLIP:%s", lab->direction,
             (unsigned)lab->direction * 45u, lab->direction < 3u ? "ON" : "OFF");
    text(ren, 12, 144, line, 0xFFBED0E2u);
    text(ren, 12, 164, "U/I DIR  K PAUSE", 0xFF9FB2C8u);
    text(ren, 12, 176, "GHD $87:AB38 FRAME", 0xFF9FB2C8u);
    text(ren, 12, 202, "RAW ROM / NO CAPTURE ART", 0xFF9EF7A9u);
}
