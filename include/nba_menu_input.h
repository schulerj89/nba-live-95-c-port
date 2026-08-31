#ifndef NBA_MENU_INPUT_H
#define NBA_MENU_INPUT_H

#include "nba_types.h"

#define NBA_MENU_CONTROLLER_COUNT 5

typedef struct {
    uint16_t previous;   /* $15CF + controller*2 */
    uint16_t pending;    /* $15D9 + controller*2 */
    uint16_t fast;       /* $15E3 + controller*2 */
    uint16_t delay;      /* $15ED + controller*2 */
    uint16_t speed;      /* $1601 + controller*2 */
    uint16_t auxiliary;  /* $1846 + controller*2; released by this producer */
} NbaMenuControllerInput;

typedef struct {
    NbaMenuControllerInput controller[NBA_MENU_CONTROLLER_COUNT];
    uint16_t selected_offset; /* $1615: 0,2,4,6,8, including idle fallback8 */
    uint16_t accelerate;      /* $1639: set by bar adjustment, cleared by text rows */
} NbaMenuInput;

/* Host button masks are not SNES register bit positions. Preserve the full
 * combination when converting to the native $0576 held word. */
uint16_t nba_menu_input_native_buttons(uint32_t host_buttons);
void nba_menu_input_produce(NbaMenuInput *state, const uint16_t held[5],
                            const uint8_t controller_type[5]);
uint16_t nba_menu_input_consume(NbaMenuInput *state);

#endif
