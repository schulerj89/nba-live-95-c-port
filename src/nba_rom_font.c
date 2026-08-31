#include "nba_rom_font.h"

/* ROM $81:9756-$9FFD; recomp IntroFont/FontWidth/CenteredFont, generated from
 * the original USA bank $81. Evidence: docs/intro-indexed-resources.md;
 * independent native HUD pre/post canvases and license/legal raster frames.
 * Only descriptor format $0010 (2bpp) is translated here. */
static uint16_t word(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static bool valid_font(const NbaRomFont *font) {
    return font && font->descriptor && font->descriptor_size >= 0x186u &&
           word(font->descriptor) == 16u &&
           word(font->descriptor + 2) > 0u &&
           word(font->descriptor + 2) <= 32u;
}

bool nba_rom_font_measure(const NbaRomFont *font, const uint8_t *text,
                          size_t text_size, uint16_t *width) {
    if (!valid_font(font) || !text || !width) return false;
    uint16_t total = 0;
    unsigned spacing = font->descriptor[4];
    for (size_t i = 0; i < text_size && text[i] && text[i] != 13u; ++i) {
        unsigned code = text[i];
        if (code >= 128u) return false;
        unsigned advance = font->descriptor[0x106u + code];
        if (code >= '0' && code <= '9' && font->fixed_digit_width)
            advance = font->fixed_digit_width;
        total = (uint16_t)(total + advance + spacing - 1u);
    }
    /* The 16-bit subtraction is intentional, including an empty line. */
    *width = (uint16_t)(total - spacing + 1u);
    return true;
}

bool nba_rom_font_draw(NbaRomFont *font, uint8_t *canvas, size_t stride,
                       int width, int height, int x, int y,
                       const uint8_t *text, size_t text_size) {
    if (!valid_font(font) || !canvas || !text || width <= 0 || height <= 0 ||
        stride < (size_t)width) return false;
    const uint8_t *data = font->descriptor;
    unsigned glyph_height = word(data + 2);
    unsigned source_height = (glyph_height + 7u) & ~7u;
    unsigned strip_size = source_height * 2u;
    for (size_t i = 0; i < text_size && text[i] && text[i] != 13u; ++i) {
        unsigned code = text[i];
        if (code >= 128u) return false;
        bool digit = code >= '0' && code <= '9';
        bool expand = font->case_mode &&
            ((code >= 'A' && code <= 'Z') || (digit && font->digit_caps));
        if (font->case_mode && code >= 'a' && code <= 'z') code -= 32u;
        unsigned glyph_width = data[0x106u + code];
        unsigned strips = (glyph_width + 7u) / 8u;
        unsigned offset = word(data + 6u + code * 2u);
        if (offset > font->descriptor_size ||
            strips * strip_size > font->descriptor_size - offset) return false;
        unsigned pad_after = 0;
        if (digit && font->fixed_digit_width) {
            if (glyph_width > font->fixed_digit_width)
                font->fixed_digit_width = (uint16_t)glyph_width;
            unsigned extra = font->fixed_digit_width - glyph_width;
            x += (int)((((glyph_width & 1u) * 2u + extra) & ~1u) / 2u);
            pad_after = extra / 2u;
        }
        unsigned repeat1 = 3u, repeat2 = glyph_height >= 12u ? 10u : glyph_height - 2u;
        if (expand && glyph_height == 16u) {
            unsigned row_offset = (code - '0') * 4u;
            if (!font->repeat_rows || row_offset + 4u > font->repeat_rows_size)
                return false;
            repeat1 = word(font->repeat_rows + row_offset);
            repeat2 = word(font->repeat_rows + row_offset + 2u);
        }
        unsigned output_height = source_height + (expand ? 2u : 0u);
        int origin_y = y + (font->case_mode && !expand ? 2 : 0);
        unsigned source_row = 0;
        for (unsigned row = 0; row < output_height; ++row) {
            /* $9B55-$9BA8 repeats after the numbered destination row; using
             * source-row numbers instead moves the second repeated stripe. */
            if (source_row >= source_height) return false;
            int py = origin_y + (int)row;
            for (unsigned strip = 0; strip < strips; ++strip) {
                const uint8_t *p = data + offset + strip * strip_size + source_row * 2u;
                for (unsigned bit = 0; bit < 8u; ++bit) {
                    int px = x + (int)(strip * 8u + bit);
                    uint8_t index = (uint8_t)(((p[0] >> (7u-bit)) & 1u) |
                                      (((p[1] >> (7u-bit)) & 1u) << 1));
                    if (px >= 0 && px < width && py >= 0 && py < height)
                        canvas[(size_t)py * stride + (size_t)px] |= index;
                }
            }
            if (!expand || (row + 1u != repeat1 && row + 1u != repeat2)) ++source_row;
        }
        x += (int)(glyph_width + data[4] - 1u + pad_after);
    }
    return true;
}
