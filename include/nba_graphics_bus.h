#ifndef NBA_GRAPHICS_BUS_H
#define NBA_GRAPHICS_BUS_H

#include "nba_setup_scheduler.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    NBA_GRAPHICS_WRAM_BYTES = 0x20000,
    NBA_GRAPHICS_QUEUE_RECORDS = 0x0100,
    NBA_GRAPHICS_QUEUE_BYTES = 0x0200,
    NBA_GRAPHICS_QUEUE_HEAD = 0x0035,
    NBA_GRAPHICS_QUEUE_TAIL = 0x0037,
    NBA_GRAPHICS_QUEUE_BUDGET = 0x0039,
    NBA_GRAPHICS_RECORD5_WORD = 0x012c
};

/* A view of the game-lifetime WRAM owner. It does not initialize, copy, or own
 * memory. Codecs, $2180 transfers, graphics producers, NMI and gameplay must
 * all receive views of the same bytes. */
typedef struct {
    uint8_t *wram;
    size_t size;
} NbaGraphicsBus;

/* Palette descriptor addresses are not yet integrated. These values are an
 * exclusive temporary projection from the future source owner, with explicit
 * size writeback. They are not a second persistent queue. */
typedef struct {
    uint16_t size;
    uint16_t destination;
    uint32_t source;
} NbaGraphicsPaletteBorrow;

bool nba_graphics_bus_view(NbaGraphicsBus *view, uint8_t *wram, size_t size);
bool nba_graphics_bus_read16(const NbaGraphicsBus *view, size_t address,
                             uint16_t *value);

/* Read the original B468 alternate-pose word at DBR:$012C. No fallback or
 * captured value is accepted. */
bool nba_graphics_bus_receiver_word(const NbaGraphicsBus *view, uint16_t *value);

/* Run the accepted bounded queue consumer as an endpoint-only adapter over
 * canonical WRAM. The sink must not inspect or mutate the bus or palette while
 * this call is active; observable source-order publication belongs to the
 * future NMI drain. Records are never copied back. On supported completion or
 * budget stop, final head/budget and palette size are committed together after
 * the sink returns. Tail remains producer-owned. Any unsupported record is
 * diagnosed before a sink call and leaves the bus, palette and sink untouched. */
NbaSetupQueueResult nba_graphics_bus_publish(
    NbaGraphicsBus *view, NbaGraphicsPaletteBorrow *palette,
    NbaSetupPublicationSink sink, void *context);

#endif
