#ifndef NBA_PERIOD_RENDER_TAIL_H
#define NBA_PERIOD_RENDER_TAIL_H
#include "period_support.h"
typedef struct {
    NbaPeriodObjectSort collision;
    int16_t x[12],y[12];
    uint16_t depth[12],draw_order[12],camera_y,leading_sentinel;
    uint16_t frame_low,frame_high;
} NbaPeriodRenderTail;
/* E1F7-E207: collision X sort, FBFF render depth sort, 84A/84C counter.
 * CPU/DP execution and video timing are outside this typed data operation. */
bool nba_period_render_tail(NbaPeriodRenderTail *state);
#endif
