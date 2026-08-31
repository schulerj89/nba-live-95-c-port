#ifndef NBA_GAMEPLAY_HUD_H
#define NBA_GAMEPLAY_HUD_H

#include "nba_assets.h"

/* `$83:D0AD-$D332` score-panel working canvas and native upload consumers.
 * These are indexed resources, never captured RGB or hand-drawn text. */
typedef struct {
    uint8_t working_map[0x600];       /* `$7E:4A70-$506F` */
    uint8_t working_characters[0x850];/* `$7E:5070-$58BF` */
    uint8_t visible_map[0x700];       /* VRAM words `$0400-$077F` */
    uint8_t published_characters[0x850]; /* VRAM bytes `$23F0-$2C3F` */
    uint16_t clock_mirror_raw_08f6;
    uint16_t clear_raw_08ee;
    uint16_t clock_frame_raw_08f4;
    uint8_t clock_text_raw_4a60[8];
    uint8_t published_mask; /* A1E7 uploads each grid only when dispatched */
    uint32_t publication_count;
    uint16_t phase_raw_08e4; /* reset by $87:8C86, incremented by CE36 */
    uint16_t advertisement_counter_raw_4941;
    uint16_t late_statistics_raw_4931;
    uint16_t assist_raw_493d;
    uint16_t shot_category_raw_4939;
    uint16_t statistics_kind_raw_08ea;
    uint16_t statistics_index_raw_08ec;
    uint16_t scratch_raw_00aa; /* CEFD's returned nonnegative random word */
    uint16_t canvas_state_raw_7a70;
    uint32_t pending_routine; /* explicit untranslated continuation, not ROM state */
    uint32_t reported_pending_routine; /* host diagnostic only */
    bool unsupported_child_pending;
    bool advertisement_upload_pending; /* CE B6 queue task is not emulated here */
    bool initialized;
} NbaGameplayHud;

typedef struct {
    uint16_t teams[2];  /* native context0 home, context1 visitor */
    uint16_t scores[2]; /* `$4711/$4791`, same native order */
    uint16_t period_raw_0926;
    uint16_t phase_raw_08e4;
    uint16_t clock_raw_0928;
    uint16_t clock_snapshot_raw_092a;
    uint16_t clock_gate_raw_492b;
    uint16_t presentation_timer_raw_08de;
    uint16_t presentation_kind_raw_08e8;
    uint16_t dead_ball_busy_raw_09b4;
    uint16_t event_bits_raw_13e7;
    uint16_t presentation_sequence_raw_08e6;
    uint16_t dispatch_mode_raw_0960;
    uint16_t requester_raw_095e;
    uint16_t shot_clock_raw_092c;
    uint16_t style_raw_17ab;
    uint16_t presentation_gate_raw_08e2;
    uint16_t rng_raw_07f6;
} NbaGameplayHudInput;

bool nba_gameplay_hud_init(NbaGameplayHud *hud, const NbaAssetPack *assets);
bool nba_gameplay_hud_lifecycle_assets_valid(const NbaAssetPack *assets);
/* Native score-panel child routines, callable only after init. Publication
 * results are queued-resource projections until a portable NMI queue owns
 * scanout. The parent owns when each child runs and shared08DE/08E6/08E8. */
bool nba_gameplay_hud_publish(NbaGameplayHud *hud, const NbaAssetPack *assets,
                              uint32_t native_routine,
                              NbaGameplayHudInput *input);
/* Source-data projections of $83:CC10 and $83:CE36. They publish only at
 * actual host caller boundaries; they do not synthesize native NMI latency.
 * A false request/dispatch result exposes pending_routine; unsupported
 * statistics/advertisements are not replaced with a score panel. */
bool nba_gameplay_hud_dispatch(NbaGameplayHud *hud, const NbaAssetPack *assets,
                               NbaGameplayHudInput *input);
bool nba_gameplay_hud_request_score(NbaGameplayHud *hud, const NbaAssetPack *assets,
                                    NbaGameplayHudInput *input);
void nba_gameplay_hud_timer_tick(int16_t *timer_raw_08de);
/* Apply only the native HUD-owned map/CHR/palette ranges to a full indexed
 * PPU input. Other court/goal resources remain untouched. */
bool nba_gameplay_hud_apply(const NbaGameplayHud *hud, const NbaAssetPack *assets,
                            uint8_t *vram, uint8_t *cgram);

#endif
