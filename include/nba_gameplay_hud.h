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
} NbaGameplayHudInput;

bool nba_gameplay_hud_init(NbaGameplayHud *hud, const NbaAssetPack *assets);
/* Native score-panel child routines, callable only after init. Publication
 * results are queued-resource projections until a portable NMI queue owns
 * scanout. The parent owns when each child runs and shared08DE/08E6/08E8. */
bool nba_gameplay_hud_publish(NbaGameplayHud *hud, const NbaAssetPack *assets,
                              uint32_t native_routine,
                              NbaGameplayHudInput *input);
/* Apply only the native HUD-owned map/CHR/palette ranges to a full indexed
 * PPU input. Other court/goal resources remain untouched. */
bool nba_gameplay_hud_apply(const NbaGameplayHud *hud, const NbaAssetPack *assets,
                            uint8_t *vram, uint8_t *cgram);

#endif
