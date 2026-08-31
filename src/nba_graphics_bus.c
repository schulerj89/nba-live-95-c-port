#include "nba_graphics_bus.h"
#include <string.h>

static uint16_t read16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
static void write16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}

bool nba_graphics_bus_view(NbaGraphicsBus *view, uint8_t *wram, size_t size) {
    if (!view || !wram || size < NBA_GRAPHICS_WRAM_BYTES) return false;
    view->wram = wram; view->size = size; return true;
}

bool nba_graphics_bus_read16(const NbaGraphicsBus *view, size_t address,
                             uint16_t *value) {
    if (!view || !view->wram || view->size < NBA_GRAPHICS_WRAM_BYTES ||
        !value || address > view->size - 2u) return false;
    *value = read16(view->wram + address); return true;
}

bool nba_graphics_bus_receiver_word(const NbaGraphicsBus *view, uint16_t *value) {
    /* Original quirk: 87:B7DA sets Y=$0084 and B7E1 reads $00A8,Y from
     * DBR=$7E, hence WRAM $012C rather than receiver+$A8. */
    return nba_graphics_bus_read16(view, NBA_GRAPHICS_RECORD5_WORD, value);
}

NbaSetupQueueResult nba_graphics_bus_publish(
    NbaGraphicsBus *view, NbaGraphicsPaletteBorrow *palette,
    NbaSetupPublicationSink sink, void *context) {
    NbaSetupPublicationQueue queue;
    NbaSetupQueueResult result;
    if (!view || !view->wram || view->size < NBA_GRAPHICS_WRAM_BYTES ||
        !palette || !sink) return NBA_SETUP_QUEUE_INVALID;
    memcpy(queue.records, view->wram + NBA_GRAPHICS_QUEUE_RECORDS,
           NBA_GRAPHICS_QUEUE_BYTES);
    queue.head = read16(view->wram + NBA_GRAPHICS_QUEUE_HEAD);
    queue.tail = read16(view->wram + NBA_GRAPHICS_QUEUE_TAIL);
    queue.budget = read16(view->wram + NBA_GRAPHICS_QUEUE_BUDGET);
    queue.palette_size = palette->size;
    queue.palette_destination = palette->destination;
    queue.palette_source = palette->source;

    /* Unsupported branches are not translated yet. Diagnose them before the
     * accepted value consumer can publish palette or earlier queue jobs; a
     * partial endpoint cannot be retried safely. */
    if (queue.head >= NBA_GRAPHICS_QUEUE_BYTES ||
        queue.tail >= NBA_GRAPHICS_QUEUE_BYTES || (queue.head & 7u) ||
        (queue.tail & 7u)) return NBA_SETUP_QUEUE_INVALID;
    for (uint16_t cursor = queue.head; cursor != queue.tail;
         cursor = (uint16_t)((cursor + 8u) & 0x1ffu)) {
        uint8_t mode = queue.records[cursor];
        if (mode != 1u && mode != 0xfcu && mode != 0xfdu && mode != 0xd6u)
            return NBA_SETUP_QUEUE_UNSUPPORTED;
    }
    result = nba_setup_queue_publish(&queue, sink, context);
    if (result != NBA_SETUP_QUEUE_COMPLETE &&
        result != NBA_SETUP_QUEUE_BUDGET_EXHAUSTED) return result;
    /* This adapter intentionally exposes only the final endpoint. The future
     * NMI owner must publish each source-visible write at its actual point. */
    write16(view->wram + NBA_GRAPHICS_QUEUE_BUDGET, queue.budget);
    write16(view->wram + NBA_GRAPHICS_QUEUE_HEAD, queue.head);
    palette->size = queue.palette_size;
    return result;
}
