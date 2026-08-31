#include "nba_player_intro.h"
#include "nba_snes_ppu.h"
#include "nba_team_select.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROSTER_HEADER_SIZE 24u
#define ROSTER_RECORD_SIZE 64u
#define PORTRAIT_HEADER_SIZE 24u
#define PORTRAIT_RECORD_HEADER_SIZE 8u
#define SETUP_PANEL_SOURCE_X 168
#define SETUP_PANEL_SOURCE_Y 56

/* Host-visible implementation of `$83:F000-$FA90`: scene construction,
 * phase/cadence control, matchup panels, five rating rows and ten starting
 * lineup cards. Fresh Ghidra listings identify `$83:F790`, `$83:F891` and
 * `$83:F901`; the end-to-end gameplay65 probe protects their combined flow. */

typedef struct {
    uint8_t jersey;
    uint8_t position;
    char name[33];
} IntroPlayer;

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool intro_player(const NbaAssetPack *assets, uint8_t team, uint8_t slot,
                         IntroPlayer *out) {
    const NbaAssetItem *item = nba_assets_get(assets, NBA_ASSET_PLAYER_ROSTERS);
    if (!item || !item->data || item->size < ROSTER_HEADER_SIZE ||
        team >= NBA_TEAM_COUNT || slot >= 12 || memcmp(item->data, "NBPROST2", 8))
        return false;
    size_t offset = ROSTER_HEADER_SIZE +
        ((size_t)team * 12u + slot) * ROSTER_RECORD_SIZE;
    if (offset + ROSTER_RECORD_SIZE > item->size) return false;
    const uint8_t *record = (const uint8_t *)item->data + offset;
    out->jersey = record[4];
    out->position = record[5];
    memcpy(out->name, record + 32, 32);
    out->name[32] = '\0';
    for (size_t index = 0; out->name[index]; ++index)
        out->name[index] = (char)toupper((unsigned char)out->name[index]);
    return true;
}

static const uint32_t *intro_portrait(const NbaAssetPack *assets,
                                      uint8_t team, uint8_t slot, uint16_t side) {
    const NbaAssetItem *item = nba_assets_get(
        assets, NBA_ASSET_PLAYER_INTRO_PORTRAITS);
    if (!item || !item->data || item->size < PORTRAIT_HEADER_SIZE ||
        memcmp(item->data, "NBINTRO1", 8)) return NULL;
    const uint8_t *data = (const uint8_t *)item->data;
    uint32_t count = read_u32(data + 12);
    size_t offset = PORTRAIT_HEADER_SIZE;
    for (uint32_t index = 0; index < count; ++index) {
        if (offset + PORTRAIT_RECORD_HEADER_SIZE > item->size) return NULL;
        uint32_t size = read_u32(data + offset + 4);
        if (offset + PORTRAIT_RECORD_HEADER_SIZE + size > item->size) return NULL;
        uint16_t record_side = (uint16_t)data[offset + 2] |
                               ((uint16_t)data[offset + 3] << 8);
        if (data[offset] == team && data[offset + 1] == slot &&
            record_side == side &&
            size == 72u * 72u * sizeof(uint32_t))
            return (const uint32_t *)(data + offset + PORTRAIT_RECORD_HEADER_SIZE);
        offset += PORTRAIT_RECORD_HEADER_SIZE + size;
    }
    return NULL;
}

bool nba_player_intro_init(NbaPlayerIntro *screen, const NbaAssetPack *assets,
                           NbaSession *session, const uint32_t *outgoing_pixels) {
    if (!screen || !assets || !session) return false;
    memset(screen, 0, sizeof(*screen));
    screen->assets = assets;
    screen->session = session;
    const uint32_t *court = nba_assets_home_court(assets, session->right_team);
    const NbaAssetItem *portraits = nba_assets_get(
        assets, NBA_ASSET_PLAYER_INTRO_PORTRAITS);
    const NbaAssetItem *font = nba_assets_get(
        assets, NBA_ASSET_PLAYER_INTRO_FONT);
    const NbaAssetItem *lineup_font = nba_assets_get(
        assets, NBA_ASSET_STARTING_LINEUP_FONT);
    if (!court ||
        !portraits || portraits->size < PORTRAIT_HEADER_SIZE ||
        !font || font->size < 0x1000u ||
        !lineup_font || lineup_font->size < 0x1000u) return false;
    screen->outgoing_pixels = (uint32_t *)malloc(
        256u * 224u * sizeof(uint32_t));
    if (!screen->outgoing_pixels) return false;
    if (outgoing_pixels)
        memcpy(screen->outgoing_pixels, outgoing_pixels,
               256u * 224u * sizeof(uint32_t));
    else
        memset(screen->outgoing_pixels, 0, 256u * 224u * sizeof(uint32_t));
    screen->phase = NBA_PLAYER_INTRO_TRANSITION;
    screen->is_initialized = true;
    printf("[PLAYER INTRO] Entered $87:BE92 flow: away=%u home=%u, "
           "five starters per team, %d-frame card cadence.\n",
           session->left_team, session->right_team, NBA_PLAYER_INTRO_CARD_FRAMES);
    return true;
}

void nba_player_intro_shutdown(NbaPlayerIntro *screen) {
    if (!screen) return;
    free(screen->outgoing_pixels);
    screen->outgoing_pixels = NULL;
    screen->is_initialized = false;
}

static void next_phase(NbaPlayerIntro *screen) {
    screen->phase = (NbaPlayerIntroPhase)(screen->phase + 1);
    screen->phase_frame = 0;
}

void nba_player_intro_update(NbaPlayerIntro *screen, const NbaInput *input) {
    if (!screen || !screen->is_initialized) return;
    screen->phase_frame++;

    /* The presentation screens consume pressed edges, never held state.
     * `$83:F277-$F2BA`, `$83:F5F0-$F63C`, and `$83:F7D0-$F830` each read
     * `$86:8042`, test Start bit $1000, and leave their team presentation.
     * In the lineup loop, `$87:BF5E-$BFA6` likewise exits the complete
     * presentation instead of advancing one card; `$0200/$0100` remain the
     * source's previous/next-card controls. */
    uint32_t pressed = input ? input->pressed : 0u;
    if ((pressed & NBA_BTN_START) &&
        screen->phase == NBA_PLAYER_INTRO_MATCHUP) {
        next_phase(screen);
        return;
    }
    if ((pressed & NBA_BTN_START) &&
        screen->phase == NBA_PLAYER_INTRO_RATINGS) {
        next_phase(screen);
        return;
    }
    if (screen->phase == NBA_PLAYER_INTRO_LINEUPS) {
        if (pressed & NBA_BTN_START) {
            screen->phase = NBA_PLAYER_INTRO_COMPLETE;
            screen->phase_frame = 0;
            return;
        }
        if ((pressed & NBA_BTN_LEFT) && screen->lineup_card > 0) {
            screen->lineup_card--;
            screen->phase_frame = 0;
            return;
        }
        if ((pressed & NBA_BTN_RIGHT) &&
            screen->lineup_card + 1 < NBA_PLAYER_INTRO_CARD_COUNT) {
            screen->lineup_card++;
            screen->phase_frame = 0;
            return;
        }
    }
    if (screen->phase == NBA_PLAYER_INTRO_TRANSITION &&
        screen->phase_frame >= NBA_PLAYER_INTRO_TRANSITION_FRAMES)
        next_phase(screen);
    else if (screen->phase == NBA_PLAYER_INTRO_MATCHUP &&
             screen->phase_frame >= NBA_PLAYER_INTRO_MATCHUP_FRAMES)
        next_phase(screen);
    else if (screen->phase == NBA_PLAYER_INTRO_RATINGS &&
             screen->phase_frame >= NBA_PLAYER_INTRO_RATINGS_FRAMES)
        next_phase(screen);
    else if (screen->phase == NBA_PLAYER_INTRO_LINEUPS) {
        bool advance = screen->phase_frame >= NBA_PLAYER_INTRO_CARD_FRAMES;
        if (advance && screen->lineup_card + 1 < NBA_PLAYER_INTRO_CARD_COUNT) {
            screen->lineup_card++;
            screen->phase_frame = 0;
            printf("[PLAYER INTRO] lineup card %d/%d\n",
                   screen->lineup_card + 1, NBA_PLAYER_INTRO_CARD_COUNT);
        }
    }
}

static void draw_asset_rgba(NbaRenderer *ren, const uint32_t *pixels,
                            int width, int height, int x, int y) {
    if (!pixels) return;
    for (int py = 0; py < height; ++py) for (int px = 0; px < width; ++px) {
        uint32_t color = pixels[py * width + px];
        int dx = x + px, dy = y + py;
        if ((color >> 24) && dx >= 0 && dx < 256 && dy >= 0 && dy < 224)
            ren->pixels[dy * 256 + dx] = color;
    }
}

static void draw_court(const NbaPlayerIntro *screen, NbaRenderer *ren) {
    const uint32_t *court = nba_assets_home_court(
        screen->assets, screen->session->right_team);
    memcpy(ren->pixels, court, 256u * 224u * sizeof(uint32_t));
}

static void draw_logo(const NbaPlayerIntro *screen, NbaRenderer *ren,
                      uint8_t team, int x, int y, int out_w, int out_h) {
    const NbaAssetItem *logo = nba_assets_get(screen->assets,
        (NbaAssetId)(NBA_ASSET_TEAM_LOGO_BASE + team));
    if (!logo || logo->size != 48u * 56u * sizeof(uint32_t)) return;
    const uint32_t *pixels = (const uint32_t *)logo->data;
    for (int py = 0; py < out_h; ++py) for (int px = 0; px < out_w; ++px) {
        uint32_t color = pixels[(py * 56 / out_h) * 48 + px * 48 / out_w];
        if (color >> 24) ren->pixels[(y + py) * 256 + x + px] = color;
    }
}

static int obj_pixel(const uint8_t *tile, int x, int y) {
    return ((tile[y * 2] >> (7 - x)) & 1) |
           (((tile[y * 2 + 1] >> (7 - x)) & 1) << 1) |
           (((tile[16 + y * 2] >> (7 - x)) & 1) << 2) |
           (((tile[17 + y * 2] >> (7 - x)) & 1) << 3);
}

/* $83:F891 reuses the same 48x56 gold plate object group as Player Setup.
 * Keeping the captured object geometry here preserves its irregular edge and
 * lets every team logo retain the ROM's common plate-relative origin. */
static void draw_plate_object(NbaRenderer *ren, const uint8_t *vram,
                              const uint8_t *cgram, const uint8_t *oam,
                              int index, int origin_x, int origin_y) {
    int high = (oam[512 + index / 4] >> ((index & 3) * 2)) & 3;
    int x = oam[index * 4] | ((high & 1) << 8);
    if (x >= 256) x -= 512;
    x += origin_x - SETUP_PANEL_SOURCE_X;
    int y = oam[index * 4 + 1] + origin_y - SETUP_PANEL_SOURCE_Y;
    int tile = oam[index * 4 + 2] +
               ((oam[index * 4 + 3] & 1) ? 256 : 0);
    int attr = oam[index * 4 + 3], size = (high & 2) ? 16 : 8;
    for (int py = 0; py < size; ++py) for (int px = 0; px < size; ++px) {
        int sx = (attr & 0x40) ? size - 1 - px : px;
        int sy = (attr & 0x80) ? size - 1 - py : py;
        int subtile = tile + (sx >> 3) + (sy >> 3) * 16;
        size_t offset = 0xc000u + (size_t)subtile * 32u;
        if (offset + 32u > 0x10000u) continue;
        int color = obj_pixel(vram + offset, sx & 7, sy & 7);
        int dx = x + px, dy = y + py;
        if (color && dx >= 0 && dx < 256 && dy >= 0 && dy < 224)
            ren->pixels[dy * 256 + dx] = nba_snes_cgram_color(
                cgram, 128 + 2 * 16 + color, 15, 0, 0, 0);
    }
}

static void draw_team_plate(const NbaPlayerIntro *screen, NbaRenderer *ren,
                            uint8_t team, int x, int y) {
    const NbaAssetItem *vram = nba_assets_get(
        screen->assets, NBA_ASSET_PLAYER_SETUP_VRAM);
    const NbaAssetItem *cgram = nba_assets_get(
        screen->assets, NBA_ASSET_PLAYER_SETUP_CGRAM);
    const NbaAssetItem *oam = nba_assets_get(
        screen->assets, NBA_ASSET_PLAYER_SETUP_OAM);
    const NbaAssetItem *cycle = nba_assets_get(
        screen->assets, NBA_ASSET_TEAM_SELECTED_PALETTE_CYCLE);
    if (!vram || vram->size != 0x10000u || !cgram || cgram->size != 0x200u ||
        !oam || oam->size != 0x220u || !cycle || cycle->size < 26u) return;
    uint8_t animated_cgram[0x200];
    memcpy(animated_cgram, cgram->data, sizeof(animated_cgram));
    int source_offset = ((screen->phase_frame + 1) & 0x38) >> 2;
    if (source_offset == 14) source_offset = 0;
    memcpy(animated_cgram + 0x142,
           (const uint8_t *)cycle->data + source_offset, 14);
    for (int index = 20; index >= 6; --index)
        draw_plate_object(ren, vram->data, animated_cgram, oam->data,
                          index, x, y);
    /* $83:F8A0/F8A3 and $83:F8D8/F8DB place each variable logo
     * two pixels right and four down from its fixed gold plate origin. */
    draw_logo(screen, ren, team, x + 2, y + 4, 48, 56);
}

static const uint8_t *rom_font(const NbaPlayerIntro *screen,
                               NbaAssetId asset_id) {
    const NbaAssetItem *font = nba_assets_get(
        screen->assets, asset_id);
    return font && font->size >= 0x1000u ? (const uint8_t *)font->data : NULL;
}

static int rom_font_width(const NbaPlayerIntro *screen, NbaAssetId asset_id,
                          const char *value) {
    const uint8_t *font = rom_font(screen, asset_id);
    if (!font) return 0;
    int width = 0;
    int spacing = font[4];
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p)
        width += font[0x106u + (*p & 0x7Fu)] + spacing - 1;
    return width ? width - spacing + 1 : 0;
}

/* $81:9756 consumes descriptor $A9:8000. Each glyph is an 8x16 SNES 2bpp
 * bitmap (two plane bytes per row); $81:9F54 gets its proportional advance
 * from descriptor+$106+character. Frame 2100 BG3 palette 0 establishes
 * colors 1/2/3 as black/$5294 gray/$7FFF white. */
static void rom_font_text(const NbaPlayerIntro *screen, NbaRenderer *ren,
                          NbaAssetId asset_id, int x, int y,
                          const char *value) {
    static const uint32_t colors[4] = {
        0, 0xFF000000u, 0xFFA5A5A5u, 0xFFFFFFFFu
    };
    const uint8_t *font = rom_font(screen, asset_id);
    if (!font) return;
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p) {
        unsigned character = *p & 0x7Fu;
        unsigned glyph_width = font[0x106u + character];
        uint16_t offset = (uint16_t)(font[6u + character * 2u] |
                                    ((uint16_t)font[7u + character * 2u] << 8));
        /* $81:9756 consumes as many 8-pixel strips as this character's
         * proportional descriptor width requires.  The two packed fonts do
         * not have fixed strip counts: for example, the small M/W glyphs use
         * two strips while the large I glyph uses one.  Treating the asset's
         * nominal width as a per-character width clipped the former and read
         * the following glyph into the latter. */
        unsigned tiles_x = (glyph_width + 7u) / 8u;
        size_t glyph_size = (size_t)tiles_x * 32u;
        if ((size_t)offset + glyph_size > 0x1000u) continue;
        for (unsigned tile = 0; tile < tiles_x; ++tile) {
            const uint8_t *glyph = font + offset + (size_t)tile * 32u;
            for (int dy = 0; dy < 16; ++dy) for (int tx = 0; tx < 8; ++tx) {
                int bit = 7 - tx;
                int color = ((glyph[dy * 2] >> bit) & 1) |
                            (((glyph[dy * 2 + 1] >> bit) & 1) << 1);
                int px = x + tile * 8 + tx, py = y + dy;
                if (color && px >= 0 && px < 256 && py >= 0 && py < 224)
                    ren->pixels[py * 256 + px] = colors[color];
            }
        }
        x += font[0x106u + character] + font[4] - 1;
    }
}

static void rom_font_centered(const NbaPlayerIntro *screen, NbaRenderer *ren,
                              NbaAssetId asset_id, int y,
                              const char *value) {
    rom_font_text(screen, ren, asset_id,
                  (256 - rom_font_width(screen, asset_id, value)) / 2,
                  y, value);
}

static void presentation_text(const NbaPlayerIntro *screen, NbaRenderer *ren,
                              int x, int y, const char *value) {
    rom_font_text(screen, ren, NBA_ASSET_PLAYER_INTRO_FONT, x, y, value);
}

static void presentation_centered(const NbaPlayerIntro *screen,
                                  NbaRenderer *ren, int y, const char *value) {
    rom_font_centered(screen, ren, NBA_ASSET_PLAYER_INTRO_FONT, y, value);
}

static void lineup_text(const NbaPlayerIntro *screen, NbaRenderer *ren,
                        int x, int y, const char *value) {
    rom_font_text(screen, ren, NBA_ASSET_STARTING_LINEUP_FONT, x, y, value);
}

static void lineup_centered(const NbaPlayerIntro *screen, NbaRenderer *ren,
                            int y, const char *value) {
    rom_font_centered(screen, ren, NBA_ASSET_STARTING_LINEUP_FONT, y, value);
}

static void draw_matchup(const NbaPlayerIntro *screen, NbaRenderer *ren) {
    const NbaTeamRecord *away = nba_team_record(screen->session->left_team);
    const NbaTeamRecord *home = nba_team_record(screen->session->right_team);
    char arena[48];
    draw_team_plate(screen, ren, screen->session->left_team, 66, 30);
    draw_team_plate(screen, ren, screen->session->right_team, 66, 118);
    if (away) {
        presentation_text(screen, ren, 120, 40, away->name);
        presentation_text(screen, ren, 120, 54, away->nickname);
    }
    presentation_centered(screen, ren, 98, "VS");
    if (home) {
        presentation_text(screen, ren, 120, 128, home->name);
        presentation_text(screen, ren, 120, 142, home->nickname);
        snprintf(arena, sizeof(arena), "AT %s ARENA", home->name);
        presentation_centered(screen, ren, 184, arena);
    }
}

static int rating_ball_count(uint8_t rank) {
    if (rank <= 8) return 3;
    if (rank <= 18) return 2;
    return 1;
}

static void draw_rating_ball(const NbaPlayerIntro *screen, NbaRenderer *ren,
                             int pose, int x, int y) {
    const NbaAssetItem *asset = nba_assets_get(
        screen->assets, NBA_ASSET_PLAYER_INTRO_RATING_BALLS);
    if (!asset || asset->size != 6u * 16u * 16u * sizeof(uint32_t)) return;
    draw_asset_rgba(ren, (const uint32_t *)asset->data + pose * 16 * 16,
                    16, 16, x, y);
}

static void draw_ratings(const NbaPlayerIntro *screen, NbaRenderer *ren) {
    static const char *const labels[5] = {
        "SCORING", "REBOUNDS", "BALL CONTROL", "DEFENSE", "OVERALL"
    };
    const NbaTeamRecord *away = nba_team_record(screen->session->left_team);
    const NbaTeamRecord *home = nba_team_record(screen->session->right_team);
    draw_team_plate(screen, ren, screen->session->left_team, 38, 20);
    draw_team_plate(screen, ren, screen->session->right_team, 174, 20);
    /* The original ratings scene leaves the home court fully visible. On
     * Orlando, the dark center oval therefore crosses behind BALL CONTROL;
     * normal native frame 2300 shows the same overlap. Preserve that artwork
     * quirk instead of moving the source-owned row or erasing the court. */
    for (int row = 0; row < 5; ++row) {
        int y = 80 + row * 20;
        int pose = (screen->phase_frame / 12 + row) % 6;
        int away_count = rating_ball_count(away ? away->rank[row] : 27);
        int home_count = rating_ball_count(home ? home->rank[row] : 27);
        presentation_centered(screen, ren, y + 1, labels[row]);
        for (int ball = 0; ball < away_count; ++ball)
            draw_rating_ball(screen, ren, pose,
                             68 - (away_count - 1 - ball) * 20, y);
        for (int ball = 0; ball < home_count; ++ball)
            draw_rating_ball(screen, ren, pose, 172 + ball * 20, y);
    }
}

static void draw_lineup(const NbaPlayerIntro *screen, NbaRenderer *ren) {
    int card = screen->lineup_card;
    uint8_t team = card < 5 ? screen->session->left_team : screen->session->right_team;
    uint8_t slot = (uint8_t)(card % 5);
    IntroPlayer player;
    static const char *const positions[] = {
        "CENTER", "POWER FORWARD", "SMALL FORWARD", "SHOOTING GUARD", "POINT GUARD"
    };
    lineup_centered(screen, ren, 48, "STARTING");
    lineup_centered(screen, ren, 68, "LINEUP");
    const NbaTeamRecord *record = nba_team_record(team);
    presentation_text(screen, ren, 80, 136, record ? record->name : "TEAM");
    if (!intro_player(screen->assets, team, slot, &player)) return;
    const uint32_t *portrait = intro_portrait(
        screen->assets, team, slot, (uint16_t)(card >= 5));
    draw_asset_rgba(ren, portrait, 72, 72, 8, 144);
    char jersey[8];
    snprintf(jersey, sizeof(jersey), "%u", player.jersey);
    lineup_text(screen, ren, 80, 165, jersey);
    lineup_text(screen, ren, 112, 165, player.name);
    nba_renderer_draw_rect(ren, 80, 186, 158, 3, 0xFFFFFF00u);
    presentation_text(screen, ren, 80, 192,
                      player.position < 5 ? positions[player.position] : "PLAYER");
}

void nba_player_intro_render(const NbaPlayerIntro *screen, NbaRenderer *ren) {
    if (!screen || !screen->is_initialized || !ren) return;
    if (screen->phase == NBA_PLAYER_INTRO_TRANSITION) {
        int frame = screen->phase_frame;
        if (frame < 32) {
            int brightness = 15 - frame * 15 / 31;
            for (int i = 0; i < 256 * 224; ++i) {
                uint32_t color = screen->outgoing_pixels[i];
                uint32_t r = ((color >> 16) & 255u) * (uint32_t)brightness / 15u;
                uint32_t g = ((color >> 8) & 255u) * (uint32_t)brightness / 15u;
                uint32_t b = (color & 255u) * (uint32_t)brightness / 15u;
                ren->pixels[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
            }
        } else if (frame < 116) {
            nba_renderer_clear(ren, 0xFF000000u);
        } else {
            draw_court(screen, ren);
            int brightness = 1 + (frame - 116) * 14 / 63;
            if (brightness > 15) brightness = 15;
            for (int i = 0; i < 256 * 224; ++i) {
                uint32_t color = ren->pixels[i];
                uint32_t r = ((color >> 16) & 255u) * (uint32_t)brightness / 15u;
                uint32_t g = ((color >> 8) & 255u) * (uint32_t)brightness / 15u;
                uint32_t b = (color & 255u) * (uint32_t)brightness / 15u;
                ren->pixels[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
            }
        }
        return;
    }
    draw_court(screen, ren);
    if (screen->phase == NBA_PLAYER_INTRO_MATCHUP) draw_matchup(screen, ren);
    else if (screen->phase == NBA_PLAYER_INTRO_RATINGS) draw_ratings(screen, ren);
    else draw_lineup(screen, ren);
}
