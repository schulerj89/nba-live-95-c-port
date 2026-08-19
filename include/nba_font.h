#ifndef NBA_FONT_H
#define NBA_FONT_H

#include "nba_types.h"

#define NBA_FONT_CHAR_WIDTH  8
#define NBA_FONT_CHAR_HEIGHT 8

void nba_font_init(void);
const uint8_t *nba_font_get_glyph_8x8(char c);

int nba_font_get_text_width(const char *text, int scale);

void nba_font_render_text(uint32_t *pixels, int stride, int x, int y,
                          const char *text, uint32_t fg_color, uint32_t shadow_color,
                          int scale);

void nba_font_render_text_centered(uint32_t *pixels, int stride, int y,
                                   const char *text, uint32_t fg_color, uint32_t shadow_color,
                                   int scale);

void nba_font_render_licensed_by_nintendo(uint32_t *pixels, int stride, int x, int y,
                                         uint32_t fg_color, int scale);

#endif /* NBA_FONT_H */
