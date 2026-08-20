#ifndef NBA_TITLE_SEQUENCE_H
#define NBA_TITLE_SEQUENCE_H

#include "nba_types.h"
#include "nba_assets.h"
#include "nba_renderer.h"

#define SNES_ADDR_POST_EA_ASSET_SETUP  0x82AC0E
#define SNES_ADDR_TITLE_SCENE          0x80E01E
#define SNES_ADDR_ATTRACT_FRAME_UPDATE 0x8780CB

/**
 * Title-screen exit path, taken straight from the ROM.
 *
 * $80:E5C7 is the routine that runs when the title is dismissed. It branches on
 * bit 7 of $0A4C - the "title build already finished" flag:
 *
 *   $80:E5C7  LDA !$0A4C
 *   $80:E5CA  BIT #$0080
 *   $80:E5CD  BNE $E5D9        ; build already complete -> skip the snap
 *   $80:E5CF  JSL $80:F07E     ; snap the title to its finished state
 *   $80:E5D3  LDA #$0078       ; ...and hold for 120 frames
 *   $80:E5D6  PHA
 *   $80:E5D7  BRA $E5DD
 *   $80:E5D9  LDA #$0028       ; already complete -> hold for 40 frames
 *   $80:E5DC  PHA
 *   $80:E5DD  ...              ; load palette $80:E7D1 via $80:8A02
 *   $80:E5F9  JSL $80:86B0     ; wait one frame
 *   $80:E5FD  DEC A
 *   $80:E5FE  BPL $E5F9        ; loop the held frame count
 *   $80:E600  JSL $80:CF1B     ; fade out, then hand off to the next scene
 *
 * $80:F07E performs the snap by DMAing the finished title tilemap - 0x680
 * bytes from $7F:4006 - into VRAM in one transfer, so the remaining pieces
 * appear instantly rather than continuing to animate in.
 *
 * $80:CF1B is the INIDISP fade: it decrements the brightness level at $0562
 * once per frame until it reaches zero.
 *
 * Verified against the running ROM: pressing Start mid-build snaps the title
 * complete within ~4 frames, and the fade begins exactly 124 frames after the
 * press every time (121 frame waits plus detection latency). Pressing Start a
 * second time during the hold has no effect - the hold is a fixed count.
 */
#define SNES_ADDR_TITLE_EXIT           0x80E5C7  /* $80:E5C7 - title dismiss/transition */
#define SNES_ADDR_TITLE_SNAP_COMPLETE  0x80F07E  /* $80:F07E - DMA finished title tilemap */
#define SNES_ADDR_TITLE_FADE_OUT       0x80CF1B  /* $80:CF1B - INIDISP fade-out loop */
#define SNES_ADDR_TITLE_BUILD_FLAG     0x000A4C  /* $0A4C bit 7 - build-complete flag */

/* Title-relative frame at which the build finishes on its own (the "NBA LIVE
 * 95" wall plus the (C)1994 ELECTRONIC ARTS line). Measured against the port's
 * own reference stream: the title scene starts at frame 649 and the build
 * completes at 1614. This is the point $80:F07E snaps to. */
#define NBA_TITLE_BUILD_COMPLETE_FRAMES 965

#define NBA_TITLE_SNAP_HOLD_FRAMES     120  /* $80:E5D3 - #$0078, after a snap  */
#define NBA_TITLE_COMPLETE_HOLD_FRAMES 40   /* $80:E5D9 - #$0028, already built */
#define NBA_TITLE_FADE_FRAMES          15   /* $80:CF1B - INIDISP 15..0         */

#define NBA_TITLE_SEQUENCE_FRAMES      2160 /* 36 seconds: build plus cue-timed credits */
#define NBA_TITLE_VIDEO_HEADER_SIZE    32

typedef enum {
    NBA_TITLE_PHASE_BUILD = 0,  /* pieces still assembling                  */
    NBA_TITLE_PHASE_HOLD,       /* $80:E5F9 - fixed frame-wait loop         */
    NBA_TITLE_PHASE_FADE_OUT    /* $80:CF1B - brightness ramp to zero       */
} NbaTitlePhase;

typedef struct {
    bool audio_started;
    int decoded_frame;
    size_t video_offset;

    NbaTitlePhase phase;
    int hold_frames_left;
    int fade_level;             /* $0562 - INIDISP master brightness 15..0 */
    float snap_timer;           /* title time forced by $80:F07E, <0 if none */

    uint16_t framebuffer[NBA_SNES_WIDTH * NBA_SNES_HEIGHT];
} NbaTitleSequence;

void nba_title_sequence_init(NbaTitleSequence *sequence);
void nba_title_sequence_snap_complete(NbaTitleSequence *sequence);
bool nba_title_sequence_advance(NbaTitleSequence *sequence, float timer);
void nba_title_sequence_render(NbaTitleSequence *sequence,
                               const NbaAssetPack *assets,
                               NbaRenderer *renderer,
                               float timer);

#endif /* NBA_TITLE_SEQUENCE_H */
