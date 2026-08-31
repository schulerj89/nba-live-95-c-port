#ifndef NBA_TYPES_H
#define NBA_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define NBA_SNES_WIDTH   256
#define NBA_SNES_HEIGHT  224
#define NBA_WINDOW_SCALE 3
#define NBA_WINDOW_WIDTH  (NBA_SNES_WIDTH * NBA_WINDOW_SCALE)
#define NBA_WINDOW_HEIGHT (NBA_SNES_HEIGHT * NBA_WINDOW_SCALE)

/* SNES Controller Button Masks (matches $4218 / $421A low/high bytes) */
#define NBA_BTN_B       (1 << 0)
#define NBA_BTN_Y       (1 << 1)
#define NBA_BTN_SELECT  (1 << 2)
#define NBA_BTN_START   (1 << 3)
#define NBA_BTN_UP      (1 << 4)
#define NBA_BTN_DOWN    (1 << 5)
#define NBA_BTN_LEFT    (1 << 6)
#define NBA_BTN_RIGHT   (1 << 7)
#define NBA_BTN_A       (1 << 8)
#define NBA_BTN_X       (1 << 9)
#define NBA_BTN_L          (1 << 10)
#define NBA_BTN_R          (1 << 11)
#define NBA_BTN_DEBUG_F10  (1 << 12)
#define NBA_BTN_DEBUG_F11  (1 << 13)
#define NBA_BTN_DEBUG_F12  (1 << 14)
#define NBA_BTN_DEBUG_F9   (1 << 15)
#define NBA_BTN_DEBUG_F8   (1u << 16)

typedef struct {
    uint32_t held;
    uint32_t pressed;
    uint32_t released;
} NbaInput;

static inline uint32_t nba_bgr555_to_argb8888(uint16_t bgr555) {
    uint8_t r = (uint8_t)((bgr555 & 0x001F) << 3);
    uint8_t g = (uint8_t)(((bgr555 >> 5) & 0x001F) << 3);
    uint8_t b = (uint8_t)(((bgr555 >> 10) & 0x001F) << 3);
    r |= (r >> 5);
    g |= (g >> 5);
    b |= (b >> 5);
    return (0xFF000000) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

#endif /* NBA_TYPES_H */
