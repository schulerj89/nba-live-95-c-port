#ifndef NBA_PERIOD_ENTRY_PREFIX_H
#define NBA_PERIOD_ENTRY_PREFIX_H
#include "nba_types.h"

typedef struct {
    uint16_t upper_queue_18, lower_queue_1a, flags_28;
    uint16_t upper_30, lower_32, base_38, upper_phase_3a, lower_phase_3c;
    uint16_t upper_accumulator_42, lower_accumulator_44, upper_lock_46, lower_lock_48;
} NbaPeriodEntryPrefixActor;

typedef struct {
    NbaPeriodEntryPrefixActor actors[10];
    uint16_t bounce_091c, activity_0948, release_094a, field_0962, event_0964;
    uint16_t field_0966, field_096a, contact_09bc, field_097c, shot_value_094c;
    uint16_t field_09d0, field_0a02, field_0a04, object_4015, object_401b, field_1864;
    uint16_t busy_09b4, whistle_09b6, context_4713, context_4793, context_4741, context_47c1;
    uint16_t period_0926, quarter_option_17b1, selected_clock_0a0c, clock_0928;
    uint16_t shot_clock_092c, field_0994, field_0996, anchor_46f5, anchor_4775;
    uint16_t scratch_b6, owner_093e, assistance_09c0, cursor_9a, list_head_34d1;
    /* Source carry, not restart defaults. DCA6 bypasses DA18's bulk clear. */
    uint16_t ready_09ba, dead_x_09b0, dead_y_09b2;
} NbaPeriodEntryPrefixState;

typedef struct { uint16_t overtime_clock[4]; } NbaPeriodEntryPrefixTables;

/* Binary16 arithmetic, DP0; effective absolute-write bank7E. This module
 * owns data only, not CPU registers, interrupt scheduling or UI elapsed time.
 * Each named boundary is the next original instruction, still unexecuted. */
void nba_period_entry_prefix_reset(NbaPeriodEntryPrefixState *); /* DCA6 -> DD2D */
bool nba_period_entry_prefix_clock(const NbaPeriodEntryPrefixTables *,
    NbaPeriodEntryPrefixState *); /* DD2D -> DD47 */
void nba_period_entry_prefix_table(NbaPeriodEntryPrefixState *); /* DD47 -> DD97 */
/* No child calls or full-memory clear. Invalid OT table index leaves state
 * unchanged. Regulation does not read17B1 and preserves0A0C. */
bool nba_period_entry_prefix(const NbaPeriodEntryPrefixTables *, NbaPeriodEntryPrefixState *);
#endif
