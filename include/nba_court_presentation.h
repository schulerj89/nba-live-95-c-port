#ifndef NBA_COURT_PRESENTATION_H
#define NBA_COURT_PRESENTATION_H
#include "nba_assets.h"
#include <stdbool.h>
#include <stdint.h>

enum { NBA_COURT_WIDTH = 1184, NBA_COURT_HEIGHT = 416 };
typedef struct {
    uint16_t basket_x_3fef, window_x_087c, window_y_087e;
    uint16_t window_left_0880, window_right_0882;
} NbaCourtPresentation;
typedef struct {
    uint16_t coarse_x, coarse_y, row_bytes;
    uint16_t destination, source, next_scroll_x, next_scroll_y;
    uint16_t scroll_x, scroll_y, source_bank;
    uint16_t rows[99];
} NbaCourtStream;
typedef void (*NbaCourtTransfer)(void *context, uint16_t source,
    uint16_t bank, uint16_t bytes, uint16_t destination);

/* 85:8E28-8EDC, after subject resolver/audio/camera calls. */
void nba_court_presentation_update(NbaCourtPresentation *state,
    int16_t x, int16_t y, uint16_t period, int16_t left_basket, int16_t right_basket);
void nba_court_stream_init(NbaCourtStream *state, int16_t x, int16_t y);
/* 85:8EE6-90C3. Transfer callback exposes native DMA descriptors for proof;
 * runtime draws the same asset-pack map directly, without a SNES DMA engine. */
bool nba_court_stream_update(NbaCourtStream *state, const NbaAssetPack *assets,
    int16_t x, int16_t y, int16_t previous_x, int16_t previous_y,
    NbaCourtTransfer transfer, void *context);
void nba_court_viewport(int16_t camera_x, int16_t camera_y, int *x, int *y);
#endif
