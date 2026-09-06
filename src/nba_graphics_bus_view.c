#include "nba_graphics_bus.h"

/* Host-only lifetime support; no single native address. Construct a borrowed
 * view over the complete canonical 128 KiB SNES WRAM allocation. */
bool nba_graphics_bus_view(NbaGraphicsBus *view, uint8_t *wram, size_t size) {
    if (!view || !wram || size < NBA_GRAPHICS_WRAM_BYTES) return false;
    view->wram = wram;
    view->size = size;
    return true;
}

/* Host-only shared-memory reader; no single native address. Native graphics
 * routines address these bytes directly through DBR=$7E aliases. */
bool nba_graphics_bus_read16(const NbaGraphicsBus *view, size_t address,
                             uint16_t *value) {
    if (!view || !view->wram || view->size < NBA_GRAPHICS_WRAM_BYTES ||
        !value || address > view->size - 2u) return false;
    *value = (uint16_t)(view->wram[address] |
                       ((uint16_t)view->wram[address + 1u] << 8));
    return true;
}

/* `$87:B7D8-$B7E3`, graphics pose helper: Y=$0084 makes `$00A8,Y`
 * read canonical WRAM `$012C`, rather than a receiver-record field. */
bool nba_graphics_bus_receiver_word(const NbaGraphicsBus *view,
                                    uint16_t *value) {
    return nba_graphics_bus_read16(view, NBA_GRAPHICS_RECORD5_WORD, value);
}
