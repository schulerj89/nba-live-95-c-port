#include "nba_graphics_bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { NbaSetupPublication jobs[65]; unsigned count; } Output;
static uint32_t rng = 0x95c012cU;
static uint32_t next(void) { rng = rng * 1664525U + 1013904223U; return rng; }
static uint16_t rd(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static void wr(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void sink(void *p, const NbaSetupPublication *job) {
    Output *o = p;
    if (o->count < 65) o->jobs[o->count++] = *job;
}
static int equal_jobs(const Output *a, const Output *b) {
    if (a->count != b->count) return 0;
    for (unsigned i = 0; i < a->count; i++) {
        const NbaSetupPublication *x = &a->jobs[i], *y = &b->jobs[i];
        if (x->mode != y->mode || x->bbus != y->bbus || x->vmain != y->vmain ||
            x->source != y->source || x->size != y->size ||
            x->destination != y->destination) return 0;
    }
    return 1;
}
static int unsupported_is_atomic(uint8_t *wram, unsigned prior) {
    uint8_t *before = malloc(NBA_GRAPHICS_WRAM_BYTES);
    NbaGraphicsBus bus = {0}; NbaGraphicsPaletteBorrow palette = {1, 7, 0x123456};
    Output out = {0}; int ok;
    if (!before) return 0;
    memset(wram, 0xa5, NBA_GRAPHICS_WRAM_BYTES);
    wr(wram + NBA_GRAPHICS_QUEUE_HEAD, 0);
    wr(wram + NBA_GRAPHICS_QUEUE_TAIL, (uint16_t)((prior + 1u) * 8u));
    wr(wram + NBA_GRAPHICS_QUEUE_BUDGET, 0xffff);
    if (prior) { wram[NBA_GRAPHICS_QUEUE_RECORDS] = 1; wr(wram + 0x104, 1); }
    wram[NBA_GRAPHICS_QUEUE_RECORDS + prior * 8u] = 0x44;
    memcpy(before, wram, NBA_GRAPHICS_WRAM_BYTES);
    if (!nba_graphics_bus_view(&bus, wram, NBA_GRAPHICS_WRAM_BYTES)) return 0;
    ok = nba_graphics_bus_publish(&bus, &palette, sink, &out) == NBA_SETUP_QUEUE_UNSUPPORTED &&
         out.count == 0 && memcmp(before, wram, NBA_GRAPHICS_WRAM_BYTES) == 0 &&
         palette.size == 1 && palette.destination == 7 && palette.source == 0x123456;
    free(before); return ok;
}

int main(void) {
    static const uint8_t modes[] = {1, 0xfc, 0xfd, 0xd6};
    uint8_t *wram = malloc(NBA_GRAPHICS_WRAM_BYTES);
    uint8_t *before = malloc(NBA_GRAPHICS_WRAM_BYTES);
    unsigned cases = 0, wraps = 0, budget_stops = 0, palette_jobs = 0;
    unsigned mode_counts[4] = {0};
    if (!wram || !before) return 2;

    for (unsigned trial = 0; trial < 20000; trial++) {
        for (unsigned i = 0; i < NBA_GRAPHICS_WRAM_BYTES; i++) wram[i] = (uint8_t)next();
        uint16_t head = (uint16_t)(((next() >> 16) & 63u) * 8u);
        uint16_t count = (uint16_t)((next() >> 16) % 64u);
        uint16_t tail = (uint16_t)((head + count * 8u) & 511u);
        wr(wram + NBA_GRAPHICS_QUEUE_HEAD, head); wr(wram + NBA_GRAPHICS_QUEUE_TAIL, tail);
        wr(wram + NBA_GRAPHICS_QUEUE_BUDGET, (uint16_t)next());
        if (count && tail < head) wraps++;
        for (unsigned n = 0; n < count; n++) {
            uint16_t at = (uint16_t)((head + n * 8u) & 511u);
            uint8_t *record = wram + NBA_GRAPHICS_QUEUE_RECORDS + at;
            unsigned index = (next() >> 16) & 3u;
            record[0] = modes[index]; mode_counts[index]++;
            wr(record + 4, (uint16_t)next());
        }
        memcpy(before, wram, NBA_GRAPHICS_WRAM_BYTES);
        NbaSetupPublicationQueue queue;
        memcpy(queue.records, wram + NBA_GRAPHICS_QUEUE_RECORDS, NBA_GRAPHICS_QUEUE_BYTES);
        queue.head = head; queue.tail = tail; queue.budget = rd(wram + NBA_GRAPHICS_QUEUE_BUDGET);
        queue.palette_size = (uint16_t)next(); queue.palette_destination = (uint16_t)next();
        queue.palette_source = next() & 0xffffffu;
        NbaGraphicsPaletteBorrow palette = {queue.palette_size, queue.palette_destination,
                                            queue.palette_source};
        uint16_t original_destination = palette.destination; uint32_t original_source = palette.source;
        NbaGraphicsBus bus = {0}; Output expected = {0}, actual = {0};
        if (!nba_graphics_bus_view(&bus, wram, NBA_GRAPHICS_WRAM_BYTES)) return 3;
        NbaSetupQueueResult wanted = nba_setup_queue_publish(&queue, sink, &expected);
        NbaSetupQueueResult got = nba_graphics_bus_publish(&bus, &palette, sink, &actual);
        if (wanted != got || !equal_jobs(&expected, &actual)) return 4;
        if (got == NBA_SETUP_QUEUE_BUDGET_EXHAUSTED) budget_stops++;
        if (expected.count && expected.jobs[0].bbus == 0x22) palette_jobs++;
        if (rd(wram + NBA_GRAPHICS_QUEUE_HEAD) != queue.head ||
            rd(wram + NBA_GRAPHICS_QUEUE_TAIL) != tail ||
            rd(wram + NBA_GRAPHICS_QUEUE_BUDGET) != queue.budget ||
            palette.size != queue.palette_size || palette.destination != original_destination ||
            palette.source != original_source) return 5;
        for (unsigned i = 0; i < NBA_GRAPHICS_WRAM_BYTES; i++)
            if (wram[i] != before[i] && i != 0x35 && i != 0x36 && i != 0x39 && i != 0x3a)
                return 6;
        uint16_t value;
        wram[0x12c] = (uint8_t)trial; wram[0x12d] = (uint8_t)(trial >> 8);
        if (!nba_graphics_bus_receiver_word(&bus, &value) || value != (uint16_t)trial) return 7;
        cases++;
    }

    if (!unsupported_is_atomic(wram, 0) || !unsupported_is_atomic(wram, 1)) return 8;
    memset(wram, 0, NBA_GRAPHICS_WRAM_BYTES);
    NbaGraphicsBus bus = {0}; Output out = {0}; NbaGraphicsPaletteBorrow palette = {0x100, 9, 0x654321};
    if (!nba_graphics_bus_view(&bus, wram, NBA_GRAPHICS_WRAM_BYTES) ||
        nba_graphics_bus_publish(&bus, &palette, sink, &out) != NBA_SETUP_QUEUE_COMPLETE ||
        out.count != 0 || palette.size != 0x100 || palette.destination != 9 ||
        palette.source != 0x654321) return 9; /* low-byte zero and head==tail/64 ambiguity */
    wr(wram + 0x1fffe, 0x95c0); uint16_t upper = 0;
    if (!nba_graphics_bus_read16(&bus, 0x1fffe, &upper) || upper != 0x95c0 ||
        nba_graphics_bus_read16(&bus, 0x1ffff, &upper)) return 10;
    NbaGraphicsBus bad = {0};
    if (nba_graphics_bus_view(&bad, wram, NBA_GRAPHICS_WRAM_BYTES - 1u) ||
        nba_graphics_bus_read16(&bad, 0, &upper)) return 11;
    if (!wraps || !budget_stops || !palette_jobs || !mode_counts[0] || !mode_counts[1] ||
        !mode_counts[2] || !mode_counts[3]) {
        fprintf(stderr, "missing category wraps=%u stops=%u palette=%u modes=%u/%u/%u/%u\n",
                wraps, budget_stops, palette_jobs, mode_counts[0], mode_counts[1],
                mode_counts[2], mode_counts[3]);
        return 12;
    }
    free(before); free(wram);
    printf("PASS endpoint cases=%u wraps=%u budget_stops=%u palette_jobs=%u "
           "modes=%u/%u/%u/%u unsupported_atomic=2 upper_read=1\n",
           cases, wraps, budget_stops, palette_jobs, mode_counts[0], mode_counts[1],
           mode_counts[2], mode_counts[3]);
    return 0;
}
