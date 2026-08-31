#ifndef NBA_ROM_FONT_H
#define NBA_ROM_FONT_H

#include "nba_types.h"
#include <stddef.h>

/* $81:9756-$9FFD proportional 2bpp font state. All graphics/table bytes are
 * caller-owned asset-pack resources; this module contains no substitute art.
 * descriptor: native $0C/$0E; repeat_rows: ROM $81:9CAA-$9D55;
 * case_mode/digit_caps/fixed_digit_width: $18CE/$18D0/$18D4. */
typedef struct {
    const uint8_t *descriptor;
    size_t descriptor_size;
    const uint8_t *repeat_rows;
    size_t repeat_rows_size;
    uint16_t case_mode;
    uint16_t digit_caps;
    uint16_t fixed_digit_width;
} NbaRomFont;

/* One original-case, bounded line, ending at NUL/CR or text_size. Width uses
 * the unmodified character code, as $81:9F54 does before $9756 remaps it. */
bool nba_rom_font_measure(const NbaRomFont *font, const uint8_t *text,
                          size_t text_size, uint16_t *width);

/* OR native glyph planes into an indexed 0..3 canvas. The caller owns its
 * clear, tile-grid packing, palette, clipping rectangle, and upload timing.
 * fixed_digit_width can grow just as native $81:98EF-$98FC does. */
bool nba_rom_font_draw(NbaRomFont *font, uint8_t *canvas, size_t stride,
                       int width, int height, int x, int y,
                       const uint8_t *text, size_t text_size);

#endif
