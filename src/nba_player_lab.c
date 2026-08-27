#include "nba_player_lab.h"
#include "nba_font.h"
#include "nba_snes_ppu.h"
#include "nba_gameplay_ai.h"
#include <stdio.h>
#include <string.h>

#define PLAYER_HEADER_SIZE 24u
#define PLAYER_RECORD_SIZE 64u
#define PLAYER_ANIMATION_HEADER_SIZE 80u
#define PLAYER_ANIMATION_STATES NBA_PLAYER_ANIMATION_STATES
#define PLAYER_ANIMATION_BANK84_SIZE 0x8000u
#define PLAYER_ATTACHMENT_TABLE_SIZE 0x830u
#define PLAYER_LOWER_STATE_TABLE 0x4218u
#define PLAYER_LOWER_ALT_STATE_TABLE 0x428Au
#define PLAYER_UPPER_STATE_TABLE 0x42FCu

typedef struct {
    uint32_t rom_address;
    uint8_t jersey, position, height, weight;
    uint8_t appearance_a, appearance_b, appearance_key, slot;
    uint8_t palette_variant, head_raw, head_style, appearance_modifier;
    uint16_t head_resource_base, head_resource_front;
    uint8_t decision_profile_39, decision_profile_3e;
    uint8_t contact_rating_3a;
    uint8_t free_throw_rating_38;
    uint8_t shot_range_49;
    uint8_t stamina_rating_35;
    uint8_t decision_profile_3f, decision_profile_40;
    uint8_t movement_profile_42;
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
        read_u32((const uint8_t *)item->data + 8) != 6u ||
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

static bool ball_attachment_table_value(const NbaAssetPack *assets,
                                        unsigned header_offset,
                                        uint16_t resource, int8_t *value) {
    const NbaAssetItem *item;
    const uint8_t *data = animation_data(assets, &item);
    if (!data || !value || resource >= PLAYER_ATTACHMENT_TABLE_SIZE ||
        header_offset > PLAYER_ANIMATION_HEADER_SIZE - 4u) return false;
    uint32_t table = read_u32(data + header_offset);
    if (table > item->size || resource >= item->size - table) return false;
    *value = (int8_t)data[table + resource];
    return true;
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

/* `$87:AEC3-$AF74`: immediate resource refresh after an action install.
 * Unlike tick rendering this reads literal +$3A/+3C, never advances them,
 * and never wraps them by descriptor count. +$52 is preserved for facing 8.
 * ASL before ADC #8 supplies carry (zero for the valid 0..8 directions). */
bool nba_player_resolve_pose(const NbaAssetPack *assets,
    const NbaPlayerAnimationChannels *channels, uint16_t direction,
    bool alternate_lower, uint16_t variant, NbaPlayerResolvedPose *pose) {
    if (!channels || !pose || direction > 8u ||
        channels->upper_state >= PLAYER_ANIMATION_STATES ||
        channels->lower_state >= PLAYER_ANIMATION_STATES) return false;
    const uint8_t *bank = animation_bank84(assets, NULL);
    const uint8_t *descriptors[2] = {
        animation_descriptor(assets, PLAYER_UPPER_STATE_TABLE, (uint8_t)channels->upper_state),
        animation_descriptor(assets, alternate_lower ? PLAYER_LOWER_ALT_STATE_TABLE :
                            PLAYER_LOWER_STATE_TABLE, (uint8_t)channels->lower_state)
    };
    uint16_t phases[2] = {channels->upper_phase, channels->lower_phase};
    uint16_t resources[2];
    if (!bank || !descriptors[0] || !descriptors[1]) return false;
    for (unsigned i = 0; i < 2u; ++i) {
        size_t offset = (size_t)(descriptors[i] - bank) + 8u + direction * 2u;
        if (offset + 2u > PLAYER_ANIMATION_BANK84_SIZE) return false;
        uint16_t pointer = read_u16(bank + offset);
        /* ASL wraps the 16-bit index, but [dp],Y can carry into another
         * bank. Reject that out-of-pack address instead of wrapping it. */
        uint32_t address = pointer + (uint16_t)(phases[i] * 2u);
        if (address < 0x8000u || address >= 0xFFFFu) return false;
        resources[i] = read_u16(bank + address - 0x8000u);
    }
    if (resources[0] < 0xF0u &&
        (uint16_t)(variant ^ (direction < 3u ? 1u : 0u)) == 0u)
        resources[0] = (uint16_t)(resources[0] + 0x28u);
    NbaPlayerResolvedPose next = *pose;
    next.mirror_flags = (uint16_t)((next.mirror_flags & 0x7FFFu) |
                                  (direction < 3u ? 0x8000u : 0u));
    next.upper_resource = resources[0]; next.lower_resource = resources[1];
    next.upper_state = channels->upper_state; next.lower_state = channels->lower_state;
    next.upper_phase = channels->upper_phase; next.lower_phase = channels->lower_phase;
    if (direction != 8u) next.direction = direction;
    *pose = next;
    return true;
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

static bool draw_player_resources_at(NbaRenderer *ren,
                                     const NbaAssetPack *assets,
                                     const PlayerLabRecord *player,
                                     uint8_t team, uint8_t side,
                                     uint8_t direction,
                                     uint16_t upper_resource,
                                     uint16_t lower_resource,
                                     int lower_x, int lower_y, int scale) {
    const uint8_t *bank = animation_bank84(assets, NULL);
    if (!bank) return false;
    const uint8_t *palette = player_palette(assets, team, side,
                                            player->palette_variant);
    if (!palette) return false;
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
    uint8_t lower_frame = animation_frame_index(lower, bank, lower_tick);
    uint8_t upper_frame = (int16_t)read_u16(upper) < 0
        ? (uint8_t)(lower_frame % read_u16(upper + 6))
        : animation_frame_index(upper, bank, upper_tick);
    uint16_t lower_resource = animation_frame_resource(
        lower, bank, direction, lower_frame);
    uint16_t upper_resource = animation_frame_resource(
        upper, bank, direction, upper_frame);
    return draw_player_resources_at(ren, assets, player, team, side, direction,
                                    upper_resource, lower_resource, lower_x,
                                    lower_y, scale);
}

static bool player_record(const NbaAssetPack *assets, int team, int player,
                          PlayerLabRecord *out) {
    const NbaAssetItem *item = nba_assets_get(assets, NBA_ASSET_PLAYER_ROSTERS);
    if (!item || !item->data || item->size < PLAYER_HEADER_SIZE ||
        team < 0 || team >= NBA_TEAM_COUNT || player < 0 ||
        player >= NBA_PLAYER_ROSTER_SIZE || memcmp(item->data, "NBPROST2", 8) != 0)
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
    out->decision_profile_3f = p[20];
    out->decision_profile_40 = p[21];
    out->movement_profile_42 = p[22];
    out->decision_profile_39 = p[23];
    out->decision_profile_3e = p[24];
    out->contact_rating_3a = p[25];
    out->free_throw_rating_38 = p[26];
    out->shot_range_49 = p[27];
    out->stamina_rating_35 = p[28];
    memcpy(out->name, p + 32, 32); out->name[32] = '\0';
    return true;
}

/* `$87:AFA2-$B053`: ten active roster entries, not twelve roster reserves.
 * Y counts words (0,2,...18), so the second uniform begins at Y >= $0A.
 * This owns only the appearance/cache seed, not the preceding pointer setup
 * `$86:D7B8` or the following jersey-tile composition `$87:B059`. */
bool nba_player_appearance_setup(const NbaAssetPack *assets,
    const uint8_t teams[NBA_PLAYER_APPEARANCE_COUNT],
    const uint8_t roster[NBA_PLAYER_APPEARANCE_COUNT],
    NbaPlayerAppearanceSetup *setup) {
    if (!teams || !roster || !setup) return false;
    NbaPlayerAppearanceSetup next = {0};
    /* Overlapping STA $180C=$8080, STA $180B=$800C. */
    next.upload_address = 0x80800Cu;
    for (unsigned i = 0; i < NBA_PLAYER_APPEARANCE_COUNT; ++i) {
        PlayerLabRecord player;
        if (!player_record(assets, teams[i], roster[i], &player)) return false;
        NbaPlayerAppearance *out = &next.players[i];
        unsigned skin = player.palette_variant;
        if (skin >= 3u) skin = 2u;
        out->palette_offset = (uint16_t)((i >= 5u ? 0x600u : 0u) + skin * 0x200u);
        out->alternate_lower = player.height >= 0x51u;
        out->upper_variant = player.appearance_modifier;
        uint16_t head = player.head_raw;
        if (head >= 0x27u) head &= 0x1Fu;
        out->head_resource = (uint16_t)(0x049Cu + head * 5u);
        out->dirty = 0xFFFFu;
    }
    *setup = next;
    return true;
}

bool nba_player_gameplay_roster_address(const NbaAssetPack *assets,
    uint8_t team, uint8_t roster_slot, uint32_t *address) {
    PlayerLabRecord player;
    if (!address || !player_record(assets, team, roster_slot, &player)) return false;
    *address = player.rom_address;
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

bool nba_player_gameplay_stamina_rating(const NbaAssetPack *assets,
                                        uint8_t team,uint8_t slot,uint8_t *rating) {
    PlayerLabRecord player;
    if(!rating || !player_record(assets,team,slot,&player)) return false;
    *rating=player.stamina_rating_35;
    return true;
}

bool nba_player_gameplay_shot_range(const NbaAssetPack *assets,
                                    uint8_t team, uint8_t roster_slot,
                                    uint8_t *range_49) {
    PlayerLabRecord player;
    if (!range_49 || !player_record(assets, team, roster_slot, &player))
        return false;
    *range_49 = player.shot_range_49;
    return true;
}

bool nba_player_gameplay_free_throw_rating(const NbaAssetPack *assets,
                                            uint8_t team,
                                            uint8_t roster_slot,
                                            uint8_t *rating) {
    PlayerLabRecord player;
    if (!rating || !player_record(assets, team, roster_slot, &player))
        return false;
    *rating = player.free_throw_rating_38;
    return true;
}

bool nba_player_gameplay_free_throw_launch_half(const NbaAssetPack *assets,
                                                uint8_t team,
                                                uint8_t roster_slot,
                                                uint8_t *half) {
    PlayerLabRecord player;
    if (!half || !player_record(assets, team, roster_slot, &player))
        return false;
    /* `$87:AFC6-$B00D`: actor +$A8 selects the second miss table when
     * roster byte +$02 (the raw height field) is at least $51. */
    *half = player.height >= 0x51u ? 1u : 0u;
    return true;
}

bool nba_player_gameplay_decision_profiles(const NbaAssetPack *assets,
                                           uint8_t team, uint8_t roster_slot,
                                           uint8_t *profile_3f,
                                           uint8_t *profile_40) {
    PlayerLabRecord player;
    if (!profile_3f || !profile_40 ||
        !player_record(assets, team, roster_slot, &player)) return false;
    *profile_3f = player.decision_profile_3f;
    *profile_40 = player.decision_profile_40;
    return true;
}

bool nba_player_gameplay_movement_profile(const NbaAssetPack *assets,
                                          uint8_t team, uint8_t roster_slot,
                                          uint8_t *profile_42) {
    PlayerLabRecord player;
    if (!profile_42 || !player_record(assets, team, roster_slot, &player))
        return false;
    *profile_42 = player.movement_profile_42;
    return true;
}

bool nba_player_gameplay_pass_profiles(const NbaAssetPack *assets,
                                       uint8_t team, uint8_t roster_slot,
                                       uint8_t *profile_39,
                                       uint8_t *profile_3e) {
    PlayerLabRecord player;
    if (!profile_39 || !profile_3e ||
        !player_record(assets, team, roster_slot, &player)) return false;
    *profile_39 = player.decision_profile_39;
    *profile_3e = player.decision_profile_3e;
    return true;
}

bool nba_player_gameplay_contact_rating(const NbaAssetPack *assets,
                                        uint8_t team, uint8_t roster_slot,
                                        uint8_t *rating_3a) {
    PlayerLabRecord player;
    if (!rating_3a || !player_record(
            assets, team, roster_slot, &player)) return false;
    *rating_3a = player.contact_rating_3a;
    return true;
}

bool nba_player_gameplay_position(const NbaAssetPack *assets,
                                  uint8_t team, uint8_t roster_slot,
                                  uint8_t *position_raw_92) {
    PlayerLabRecord player;
    if (!position_raw_92 ||
        !player_record(assets, team, roster_slot, &player)) return false;
    *position_raw_92 = player.position;
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

bool nba_player_sprite_render_resources(NbaRenderer *renderer,
                                    const NbaAssetPack *assets, uint8_t team,
                                    uint8_t roster_slot, uint8_t side,
                                    uint8_t direction,
                                    uint16_t upper_resource,
                                    uint16_t lower_resource, int origin_x,
                                    int origin_y, int scale) {
    PlayerLabRecord player;
    if (!renderer || !assets || side > 1u || scale < 1 ||
        !player_record(assets, team, roster_slot, &player)) return false;
    return draw_player_resources_at(renderer, assets, &player, team, side,
                                    direction, upper_resource, lower_resource,
                                    origin_x, origin_y, scale);
}

/* `$87:B37C-$B571`: exact independent-channel action installation/cancel.
 * An unlocked replacement preserves a valid phase; nonzero descriptor +2
 * restarts the channel. Negative CURRENT locks reject replacement. */
bool nba_player_animation_command(const NbaAssetPack *assets,
    NbaPlayerAnimationChannels *c, NbaPlayerAnimationCommand command,
    uint16_t *request, bool boosted, bool alternate_lower) {
    if (!c || !request) return false;
    if (command == NBA_ANIMATION_CANCEL_UPPER ||
        command == NBA_ANIMATION_CANCEL_LOWER) {
        bool upper = command == NBA_ANIMATION_CANCEL_UPPER;
        uint16_t *lock = upper ? &c->upper_lock : &c->lower_lock;
        if (*lock) {
            *(upper ? &c->upper_state : &c->lower_state) = c->base_state;
            *(upper ? &c->upper_phase : &c->lower_phase) = 0;
            *(upper ? &c->upper_accumulator : &c->lower_accumulator) = 0;
            *(upper ? &c->upper_queue_cursor : &c->lower_queue_cursor) = 0xFFFF;
            *lock = 0;
        }
        return true;
    }
    if (command == NBA_ANIMATION_REVERSE_BOTH) {
        NbaPlayerAnimationChannels next = *c;
        uint16_t requested = *request;
        if (!nba_player_animation_command(assets, &next,
                NBA_ANIMATION_INSTALL_BOTH, &requested, boosted,
                alternate_lower)) return false;
        const uint8_t *lower = animation_descriptor(assets,
            alternate_lower ? PLAYER_LOWER_ALT_STATE_TABLE :
                              PLAYER_LOWER_STATE_TABLE,
            (uint8_t)next.lower_state);
        if (!lower) return false;
        uint16_t reversed = (uint16_t)(read_u16(lower + 6) - c->lower_phase - 1u);
        next.lower_phase = (int16_t)reversed < 0 ? 0 : reversed;
        next.upper_phase = c->upper_phase;
        *c = next;
        *request = requested;
        return true;
    }
    bool both = command == NBA_ANIMATION_INSTALL_BOTH;
    bool upper = command == NBA_ANIMATION_INSTALL_UPPER;
    if (!both && !upper && command != NBA_ANIMATION_INSTALL_LOWER) return false;
    uint16_t state = *request;
    if (both && boosted && (state == 3 || state == 5)) ++state;
    *request = state; /* DP $00 is rewritten even if the current lock rejects. */
    if ((both && c->upper_state == state && c->lower_state == state) ||
        (!both && (upper ? c->upper_state : c->lower_state) == state)) return true;
    if (((both || upper) && (int16_t)c->upper_lock < 0) ||
        ((both || !upper) && (int16_t)c->lower_lock < 0)) return true;
    if (state >= PLAYER_ANIMATION_STATES) return false;
    const uint8_t *ud = animation_descriptor(assets, PLAYER_UPPER_STATE_TABLE,
                                             (uint8_t)state);
    const uint8_t *ld = animation_descriptor(assets,
        alternate_lower ? PLAYER_LOWER_ALT_STATE_TABLE : PLAYER_LOWER_STATE_TABLE,
        (uint8_t)state);
    if ((both || upper) && !ud) return false;
    if ((both || !upper) && !ld) return false;
    if (both || upper) {
        c->upper_queue_cursor = 0xFFFF;
        c->upper_state = state;
        c->upper_lock = read_u16(ud + 2);
    }
    if (both || !upper) {
        c->lower_queue_cursor = 0xFFFF;
        c->lower_state = state;
        c->lower_lock = read_u16(ld + 2);
    }
    bool locked = both ? (c->upper_lock | c->lower_lock) != 0 :
                        (upper ? c->upper_lock : c->lower_lock) != 0;
    if (locked) {
        if (both || upper) c->upper_phase = c->upper_accumulator = 0;
        if (both || !upper) c->lower_phase = c->lower_accumulator = 0;
    } else {
        c->base_state = state;
        if ((both || !upper) && c->lower_phase >= read_u16(ld + 6)) c->lower_phase = 0;
        if (both || upper) {
            if ((int16_t)read_u16(ud) < 0) {
                c->upper_phase = c->lower_phase;
                c->upper_accumulator = c->lower_accumulator;
            } else if (c->upper_phase >= read_u16(ud + 6)) c->upper_phase = 0;
        }
    }
    return true;
}

static bool animation_channel_advance(const NbaAssetPack *assets,
    NbaPlayerAnimationChannels *c, bool upper, bool alternate, uint16_t speed,
    uint16_t delta, NbaGameplayRng *rng, const uint8_t **result) {
    uint16_t *state = upper ? &c->upper_state : &c->lower_state;
    uint16_t *phase = upper ? &c->upper_phase : &c->lower_phase;
    uint16_t *acc = upper ? &c->upper_accumulator : &c->lower_accumulator;
    uint16_t *lock = upper ? &c->upper_lock : &c->lower_lock;
    uint16_t *cursor = upper ? &c->upper_queue_cursor : &c->lower_queue_cursor;
    uint32_t table = upper ? PLAYER_UPPER_STATE_TABLE :
        alternate ? PLAYER_LOWER_ALT_STATE_TABLE : PLAYER_LOWER_STATE_TABLE;
    const uint8_t *bank = animation_bank84(assets, NULL);
    const uint8_t *d = animation_descriptor(assets, table, (uint8_t)*state);
    if (!bank || !d || *state >= PLAYER_ANIMATION_STATES) return false;
    *result = d;
    uint16_t mode = read_u16(d), count = read_u16(d + 6);
    if (!count) return false;
    if (upper && (int16_t)mode < 0) {
        /* `$87:AC55-$AC67`: phase sync requires matching state IDs. DP $AA
         * remains the lower accumulator even when the IDs differ. */
        *acc = c->lower_accumulator;
        if (c->upper_state == c->lower_state) *phase = c->lower_phase;
        return *phase < count;
    }
    if (mode == 2) {
        if (!upper) {
            if (*state == 7 || *state == 18) *phase = 0;
            return *phase < count;
        }
        /* `$87:ADBE-$AE88`: held-ball eight-phase traversal. +B0 is not
         * a frame counter: its high bit selects descending traversal and
         * the low bits hold a target. Advance at most once per call. */
        if (*state == 18) {
            if ((c->upper_phase_target & 0x7FFFu) >= 8u)
                c->upper_phase_target = 0;
            if (c->lower_accumulator < 0x400u) c->lower_accumulator = 0x600u;
            *acc = (uint16_t)(*acc + delta);
            if (*acc >= c->lower_accumulator) {
                *acc = (uint16_t)(*acc - c->lower_accumulator);
                if ((c->upper_phase_target & 0x7FFFu) != *phase) {
                    if (c->upper_phase_target & 0x8000u) {
                        *phase = (uint16_t)(*phase - 1u);
                        if ((int16_t)*phase < 0) *phase = 7;
                    } else {
                        *phase = (uint16_t)(*phase + 1u);
                        if (*phase >= 8u) *phase = 0;
                    }
                } else {
                    uint16_t opposite_half = (*phase & 4u) ^ 4u;
                    uint16_t choice = nba_gameplay_rng_next(rng);
                    if ((choice & 0x18u) == 0x18u) {
                        c->upper_phase_target = (choice & 0x8000u) | *phase;
                        /* AE5D's provisional duration is overwritten at AE84. */
                        c->lower_accumulator = (uint16_t)(((choice & 7u) << 8) + 0x600u);
                    }
                    c->upper_phase_target = (uint16_t)(
                        ((c->upper_phase_target & 0x8000u) ^ 0x8000u) |
                        ((choice & 3u) ^ opposite_half));
                    c->lower_accumulator = (uint16_t)(
                        ((nba_gameplay_rng_next(rng) & 3u) << 8) + 0x600u);
                }
            }
            return *phase < count;
        }
        /* `$87:AE89-$AEBC`: two-phase held-ball pose shares its phase with
         * lower +3C, but the lower resource was resolved earlier at AC38. */
        if (*state == 13) {
            *acc = (uint16_t)(*acc + delta);
            if (*acc >= c->lower_accumulator) {
                *phase = nba_gameplay_rng_next(rng) & 1u;
                c->lower_phase = *phase;
                *acc = 0;
                c->lower_accumulator = (uint16_t)(
                    (nba_gameplay_rng_next(rng) & 0x1FFFu) + 0x1000u);
            }
            return *phase < count;
        }
        /* `$87:AD86-$ADBB`: idle look-around is held by a randomized timer,
         * not a cyclic resource stream. Lower +44 holds its next duration. */
        if (*state != 7) return false;
        *acc = (uint16_t)(*acc + delta);
        if (*acc >= c->lower_accumulator) {
            uint16_t choice;
            do { choice = nba_gameplay_rng_next(rng) & 3; } while (!choice);
            *phase = (uint16_t)(choice - 1);
            *acc = 0;
            c->lower_accumulator = (uint16_t)((nba_gameplay_rng_next(rng) & 0xFFF) + 0x1000);
        }
        return *phase < count;
    }
    uint16_t duration = read_u16(d + 4);
    if (*phase >= count) return false;
    if (mode == 1) {
        size_t offset = duration >= 0x8000 ? duration - 0x8000u + *phase * 2u :
                                           PLAYER_ANIMATION_BANK84_SIZE;
        if (offset + 2 > PLAYER_ANIMATION_BANK84_SIZE) return false;
        duration = read_u16(bank + offset);
    }
    *acc = (uint16_t)(*acc + (mode >= 3 ? speed : delta));
    if (*acc < duration) return true;
    uint16_t next = (uint16_t)(*phase + 1);
    if (next < count || !*lock) {
        *phase = next < count ? next : 0;
        *acc = (uint16_t)(*acc - duration);
        return true;
    }
    /* `$87:ABC2-$ABFF/$ACE1-$AD16`: exhaust the channel queue, then unlock
     * into +38. Loading the next descriptor does NOT reload its lock. */
    *acc = 0;
    *phase = 0;
    if ((int16_t)*cursor >= 0) {
        if (*cursor >= 3) return false;
        *state = (upper ? c->upper_queue : c->lower_queue)[*cursor];
        --*cursor;
    } else {
        *lock = 0;
        *state = c->base_state;
    }
    if (*state >= PLAYER_ANIMATION_STATES) return false;
    *result = animation_descriptor(assets, table, (uint8_t)*state);
    if (!*result) return false;
    if (upper && (int16_t)read_u16(*result) < 0 && *state == c->lower_state) {
        *phase = c->lower_phase;
        /* DP $AA was cleared on the upper completion path, not reloaded. */
    }
    return *phase < read_u16(*result + 6);
}

bool nba_player_animation_step_channels(const NbaAssetPack *assets,
    NbaPlayerAnimationChannels *channels, uint16_t direction, uint16_t speed,
    uint16_t delta, bool alternate, uint16_t variant, uint16_t *rng_state,
    uint16_t *upper_resource, uint16_t *lower_resource) {
    if (!channels || !rng_state || !upper_resource || !lower_resource || direction >= 8) return false;
    NbaPlayerAnimationChannels next = *channels;
    NbaGameplayRng rng = {*rng_state};
    const uint8_t *upper, *lower;
    if (!animation_channel_advance(assets, &next, false, alternate, speed, delta, &rng, &lower)) return false;
    const uint8_t *bank = animation_bank84(assets, NULL);
    /* `$87:AC38` precedes upper cadence, which may change lower +3C. */
    uint16_t lr = animation_frame_resource(lower, bank, (uint8_t)direction, (uint8_t)next.lower_phase);
    if (!animation_channel_advance(assets, &next, true, alternate, speed, delta, &rng, &upper)) return false;
    uint16_t ur = animation_frame_resource(upper, bank, (uint8_t)direction, (uint8_t)next.upper_phase);
    if (ur < 0xF0 && (uint16_t)(variant ^ (direction < 3 ? 1 : 0)) == 0) ur += 0x28;
    *channels = next;
    *rng_state = rng.state;
    *upper_resource = ur;
    *lower_resource = lr;
    return true;
}

bool nba_player_animation_frame_count(const NbaAssetPack *assets,
                                      bool upper, uint8_t state,
                                      bool alternate_lower,
                                      uint16_t *count) {
    uint32_t table = upper ? PLAYER_UPPER_STATE_TABLE :
        alternate_lower ? PLAYER_LOWER_ALT_STATE_TABLE :
                          PLAYER_LOWER_STATE_TABLE;
    const uint8_t *descriptor = animation_descriptor(assets, table, state);
    if (!descriptor || !count) return false;
    *count = read_u16(descriptor + 6u);
    return *count != 0u;
}

uint8_t nba_player_locomotion_state(uint8_t state, bool stationary,
                                    bool boosted, bool owns_ball,
                                    bool airborne) {
    static const uint8_t owner[19] = {
        12,2,2,5,6,5,6,2,5,9,5,11,12,13,5,5,2,2,18
    };
    static const uint8_t nonowner[19] = {
        0,1,0,3,4,3,4,7,8,3,10,3,0,0,14,15,16,17,0
    };
    static const uint8_t stationary_map[19] = {
        0,1,2,0,0,2,2,7,8,9,10,11,12,13,16,17,16,17,18
    };
    static const uint8_t moving[19] = {
        3,3,5,3,4,5,6,3,8,9,10,11,5,5,14,15,14,15,5
    };
    static const uint8_t airborne_map[19] = {
        0,0,12,0,0,12,12,0,0,12,0,12,12,12,16,17,16,17,12
    };
    static const uint8_t boost[19] = {
        0,1,2,4,4,6,6,7,8,9,10,11,12,13,14,15,16,17,18
    };
    static const uint8_t normal[19] = {
        0,1,2,3,3,5,5,7,8,9,10,11,12,13,14,15,16,17,18
    };
    if (state >= 19u) return state;
    state = stationary ? stationary_map[state] : moving[state];
    state = boosted ? boost[state] : normal[state];
    state = owns_ball ? owner[state] : nonowner[state];
    return airborne ? airborne_map[state] : state;
}

static bool animation_rom_advance(const uint8_t *bank,
                                  const uint8_t *descriptor, uint16_t delta,
                                  uint16_t *accumulator, uint16_t *phase) {
    uint16_t mode = read_u16(descriptor);
    uint16_t count = read_u16(descriptor + 6u);
    if (!count || mode == 2u) return false; /* `$87:AD5B` is a separate path. */
    *phase %= count;
    *accumulator = (uint16_t)(*accumulator + delta);
    {
        uint16_t duration = read_u16(descriptor + 4u);
        if (mode == 1u) {
            uint16_t pointer = duration;
            size_t offset = pointer >= 0x8000u
                ? (size_t)(pointer - 0x8000u) + (size_t)*phase * 2u
                : PLAYER_ANIMATION_BANK84_SIZE;
            if (offset + 2u > PLAYER_ANIMATION_BANK84_SIZE) return false;
            duration = read_u16(bank + offset);
        }
        /* `$87:ABAD-$AC0C/$ACCD-$AD24` advances at most one frame per
         * logical pass, even when the accumulator exceeds two durations. */
        if (*accumulator >= duration) {
            *accumulator = (uint16_t)(*accumulator - duration);
            *phase = (uint16_t)((*phase + 1u) % count);
        }
    }
    return true;
}

bool nba_player_animation_rom_step(const NbaAssetPack *assets,
                                   uint8_t upper_state, uint8_t lower_state,
                                   uint8_t direction, uint16_t speed_raw_4a,
                                   bool alternate_lower, uint16_t variant_raw_6c,
                                   uint16_t *upper_accumulator,
                                   uint16_t *lower_accumulator,
                                   uint16_t *upper_phase,
                                   uint16_t *lower_phase,
                                   uint16_t *upper_resource,
                                   uint16_t *lower_resource) {
    const uint8_t *bank = animation_bank84(assets, NULL);
    const uint8_t *upper = animation_descriptor(
        assets, PLAYER_UPPER_STATE_TABLE, upper_state);
    const uint8_t *lower = animation_descriptor(
        assets, alternate_lower ? PLAYER_LOWER_ALT_STATE_TABLE :
                                  PLAYER_LOWER_STATE_TABLE, lower_state);
    if (!bank || !upper || !lower || !upper_accumulator ||
        !lower_accumulator || !upper_phase || !lower_phase ||
        !upper_resource || !lower_resource) return false;
    uint16_t lower_mode = read_u16(lower);
    uint16_t lower_delta = lower_mode == 3u ? speed_raw_4a : 0x0200u;
    if (!animation_rom_advance(bank, lower, lower_delta,
                               lower_accumulator, lower_phase)) return false;
    if ((int16_t)read_u16(upper) < 0) {
        uint16_t count = read_u16(upper + 6u);
        if (!count) return false;
        *upper_accumulator = *lower_accumulator;
        *upper_phase = (uint16_t)(*lower_phase % count);
    } else if (!animation_rom_advance(bank, upper, 0x0200u,
                                      upper_accumulator, upper_phase)) {
        return false;
    }
    *lower_resource = animation_frame_resource(
        lower, bank, direction, (uint8_t)*lower_phase);
    *upper_resource = animation_frame_resource(
        upper, bank, direction, (uint8_t)*upper_phase);
    /* `$87:AC76-$AC95/$AD38-$AD57`: low upper resources have a second
     * 40-resource variant. Roster +$08 is copied to actor +$6C at
     * `$87:B010-$B01A`; facing 0..2 toggles that word before selection. */
    if (*upper_resource < 0x00F0u &&
        (uint16_t)(variant_raw_6c ^ (direction < 3u ? 1u : 0u)) == 0u)
        *upper_resource = (uint16_t)(*upper_resource + 0x0028u);
    return true;
}

bool nba_player_gameplay_animation_variant(const NbaAssetPack *assets,
        uint8_t team, uint8_t roster_slot, uint16_t *variant_raw_6c) {
    PlayerLabRecord player;
    if (!variant_raw_6c || !player_record(assets, team, roster_slot, &player))
        return false;
    *variant_raw_6c = player.appearance_modifier;
    return true;
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
    /* Recomp `$87:B572-$B648` selects state descriptors at `$84:C2FC` and
     * `$84:C218` without treating resource word zero as failure. Resource
     * zero is a real upper-body frame (for example state 5, direction 3,
     * descriptor `$84:D546`, frame 6), not a null sentinel. The asset-pack
     * descriptor/pointer validation above is the validity boundary. */
    return true;
}

bool nba_player_animation_phases(const NbaAssetPack *assets,
                                 uint8_t upper_state, uint8_t lower_state,
                                 uint32_t upper_tick, uint32_t lower_tick,
                                 uint16_t *upper_phase,
                                 uint16_t *lower_phase) {
    const uint8_t *bank = animation_bank84(assets, NULL);
    const uint8_t *upper = animation_descriptor(
        assets, PLAYER_UPPER_STATE_TABLE, upper_state);
    const uint8_t *lower = animation_descriptor(
        assets, PLAYER_LOWER_STATE_TABLE, lower_state);
    if (!bank || !upper || !lower || !upper_phase || !lower_phase)
        return false;
    uint8_t lower_frame = animation_frame_index(lower, bank, lower_tick);
    uint16_t upper_count = read_u16(upper + 6);
    uint8_t upper_frame = (int16_t)read_u16(upper) < 0 && upper_count != 0u
        ? (uint8_t)(lower_frame % upper_count)
        : animation_frame_index(upper, bank, upper_tick);
    *upper_phase = upper_frame;
    *lower_phase = lower_frame;
    return true;
}

/* `$87:A6A9-$A6B2` stores `foot_y-head_anchor_y+11` in actor +$AA.
 * `$80:ADE8-$ADEB/$AE1B-$AE1E` builds that head anchor in DP $B8 by adding
 * lower and upper $A9:D03E offsets to the foot origin. It is NOT the top of
 * the head's OBJ parts; head style, jersey overlay and tile extents do not
 * participate. Preserve the ROM's 16-bit arithmetic. */
bool nba_player_animation_contact_height_from_resources(
        const NbaAssetPack *assets, uint16_t upper_resource,
        uint16_t lower_resource, uint16_t *height) {
    if (!height || !animation_resource(assets, upper_resource, NULL) ||
        !animation_resource(assets, lower_resource, NULL))
        return false;
    int head_origin = animation_attachment(assets, lower_resource, true) +
                      animation_attachment(assets, upper_resource, true);
    *height = (uint16_t)(11 - head_origin);
    return true;
}

bool nba_player_animation_contact_height(const NbaAssetPack *assets,
                                         uint8_t team, uint8_t roster_slot,
                                         uint16_t upper_resource,
                                         uint16_t lower_resource,
                                         uint8_t direction,
                                         uint16_t *height) {
    PlayerLabRecord player;
    if (!player_record(assets, team, roster_slot, &player)) return false;
    (void)direction;
    return nba_player_animation_contact_height_from_resources(
        assets, upper_resource, lower_resource, height);
}

/* Small durable witnesses from the live Mesen captures. These run in the
 * normal gameplay regression without requiring ignored local JSONL files. */
bool nba_player_animation_self_test(const NbaAssetPack *assets) {
    static const struct {
        uint16_t input[10];
        uint16_t output[6];
    } cadence[] = {
        {{0, 0, 2, 0, 1, 0, 0, 0, 0, 0},
         {0, 0, 0, 0, 0x00F1, 0x068A}},
        {{5, 5, 3, 0x0328, 0, 0, 0, 0x069E, 0, 7},
         {0x0200, 0x00C6, 0, 0, 0x002A, 0x0474}},
        {{5, 5, 3, 0x0370, 0, 0, 0x0200, 0x00C6, 0, 0},
         {0x0400, 0x0436, 0, 0, 0x002A, 0x0474}},
        {{5, 5, 3, 0x03CA, 0, 0, 0x0400, 0x0436, 0, 0},
         {0, 0x0800, 1, 0, 0x002B, 0x0474}},
        {{3, 3, 6, 0x0326, 0, 0, 0x0400, 0x0400, 7, 7},
         {0x0726, 0x0726, 7, 7, 0x0114, 0x0493}},
        {{3, 3, 6, 0x04DA, 0, 0, 0x04BA, 0x04BA, 0, 0},
         {0x0094, 0x0094, 1, 1, 0x010E, 0x048D}},
        {{3, 3, 0, 0x036E, 0, 0, 0x0864, 0x0864, 7, 7},
         {0x02D2, 0x02D2, 0, 0, 0x010D, 0x048C}}
    };
    for (size_t i = 0; i < sizeof(cadence) / sizeof(cadence[0]); ++i) {
        const uint16_t *in = cadence[i].input;
        uint16_t out[6] = {in[6], in[7], in[8], in[9], 0, 0};
        if (!nba_player_animation_rom_step(assets, (uint8_t)in[0],
                (uint8_t)in[1], (uint8_t)in[2], in[3], in[4] != 0u, in[5],
                &out[0], &out[1], &out[2], &out[3], &out[4], &out[5]) ||
            memcmp(out, cadence[i].output, sizeof(out)) != 0) return false;
    }
    static const uint16_t heights[][3] = {
        {0x00F0, 0x044C, 55}, {0x00F1, 0x044D, 55},
        {0x00F1, 0x068A, 61}, {0x00F3, 0x068C, 60},
        {0x00F4, 0x0450, 54}
    };
    for (size_t i = 0; i < sizeof(heights) / sizeof(heights[0]); ++i) {
        uint16_t height = 0u;
        if (!nba_player_animation_contact_height_from_resources(
                assets, heights[i][0], heights[i][1], &height) ||
            height != heights[i][2]) return false;
    }
    /* Invalid upper state must not partially commit the already-stepped
     * lower channel or the RNG. This is a safety test, not ROM coverage. */
    NbaPlayerAnimationChannels unsupported = {0};
    unsupported.upper_state = NBA_PLAYER_ANIMATION_STATES;
    unsupported.upper_queue_cursor = unsupported.lower_queue_cursor = 0xFFFF;
    NbaPlayerAnimationChannels before = unsupported;
    uint16_t rng = 0x9146, ur = 0x1234, lr = 0x5678;
    if (nba_player_animation_step_channels(assets, &unsupported, 0, 100,
            0x200, false, 0, &rng, &ur, &lr) ||
        memcmp(&unsupported, &before, sizeof(before)) ||
        rng != 0x9146 || ur != 0x1234 || lr != 0x5678) return false;
    /* Invalid public resolver inputs fail atomically; this is safety, not
     * additional live-ROM coverage. */
    NbaPlayerResolvedPose pose = {0x1234,0x5678,0x9ABC,1,2,3,4,5};
    NbaPlayerResolvedPose pose_before = pose;
    if (nba_player_resolve_pose(assets, &before, 9u, false, 0u, &pose) ||
        memcmp(&pose, &pose_before, sizeof(pose))) return false;
    uint8_t teams[10] = {0}, roster[10] = {0};
    NbaPlayerAppearanceSetup appearance, appearance_before;
    memset(&appearance, 0xA5, sizeof(appearance));
    appearance_before = appearance;
    roster[9] = NBA_PLAYER_ROSTER_SIZE;
    if (nba_player_appearance_setup(assets, teams, roster, &appearance) ||
        memcmp(&appearance, &appearance_before, sizeof(appearance))) return false;
    return true;
}

bool nba_player_ball_attachment_point_offsets(const NbaAssetPack *assets,
                                        uint16_t upper_resource,
                                        uint16_t lower_resource,
                                        uint16_t mirror_flags_raw,
                                        uint8_t point_selector,
                                        int16_t *x, int16_t *y, int16_t *z) {
    int8_t lower_y, lower_z, upper_x, upper_y, upper_z;
    unsigned upper_header = point_selector == 0u ? 56u :
                            point_selector == 1u ? 68u : 0u;
    if (!x || !y || !z ||
        upper_header == 0u ||
        !ball_attachment_table_value(assets, upper_header, upper_resource, &upper_x) ||
        !ball_attachment_table_value(assets, upper_header + 4u, upper_resource, &upper_y) ||
        !ball_attachment_table_value(assets, upper_header + 8u, upper_resource, &upper_z) ||
        !ball_attachment_table_value(assets, 24u, lower_resource, &lower_y))
        return false;
    const NbaAssetItem *item;
    const uint8_t *data = animation_data(assets, &item);
    uint32_t lower_z_table = read_u32(data + 24u) + PLAYER_ATTACHMENT_TABLE_SIZE;
    if (lower_resource >= PLAYER_ATTACHMENT_TABLE_SIZE ||
        lower_z_table > item->size || lower_resource >= item->size - lower_z_table)
        return false;
    lower_z = (int8_t)data[lower_z_table + lower_resource];

    /* `$87:B832`: negative actor `+$28` toggles masks 2/1 before mask 2
     * mirrors lower Y and mask 1 mirrors upper Y. Bit `$0004` is unrelated. */
    uint16_t flags = mirror_flags_raw;
    if ((int16_t)flags < 0) flags ^= 3u;
    if (flags & 2u) lower_y = (int8_t)-lower_y;
    if (flags & 1u) upper_y = (int8_t)-upper_y;
    int sum = (int)lower_y + (int)upper_y;
    int midpoint = sum >= 0 ? sum / 2 : -((-sum + 1) / 2);
    *x = (int16_t)(midpoint - 2 * (int)upper_x);
    *y = (int16_t)(midpoint + 2 * (int)upper_x);
    /* `$87:B953-$B995`: AC:A9CF - lower Z - AC:A583. The ROM and C
     * gameplay spaces both use this result as a positive-up actor offset. */
    *z = (int16_t)((int)upper_x - (int)lower_z - (int)upper_z);
    return true;
}

bool nba_player_ball_attachment_offsets(const NbaAssetPack *assets,
                                        uint16_t upper_resource,
                                        uint16_t lower_resource,
                                        uint16_t mirror_flags_raw,
                                        int16_t *x, int16_t *y, int16_t *z) {
    return nba_player_ball_attachment_point_offsets(
        assets, upper_resource, lower_resource, mirror_flags_raw, 0u,
        x, y, z);
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
