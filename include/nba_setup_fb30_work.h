#ifndef NBA_SETUP_FB30_WORK_H
#define NBA_SETUP_FB30_WORK_H

#include "nba_setup_codec_work.h"

/* Independent FB30 continuation. The frozen FB46 API/source is unchanged.
 * bus contains the shared event/register representation, not a CPU interpreter.
 * Source control flow is statically compiled; no opcode or trace is dispatched.
 * Entry is $80:C62B; completion is before $80:C682 RTL. Caller supplies native
 * mode, direct page zero, M=X=decimal=0, FastROM and noncrossing ROM streams,
 * ordinary live WRAM/IO mirrors, and an empty immediate-publication queue.
 * Instruction limit is a safety bound, never a timing prediction. No NMI,
 * refresh, DMA or audio/SPC time is included in these intrinsic bus events.
 */
typedef struct {
    NbaSetupCodecWork bus;
    uint16_t pointer_read;
    uint8_t read_bank;
    uint8_t indirect_cycle[10];
    uint8_t rmw_width;
    uint8_t change_kind;
    uint8_t carry_in;
} NbaSetupFb30Work;

bool nba_setup_fb30_work_begin(NbaSetupFb30Work *work,
                               const NbaCodecWorkEntry *entry,
                               uint64_t instruction_limit);
bool nba_setup_fb30_work_peek(const NbaSetupFb30Work *work, NbaCodecBusCycle *cycle);
bool nba_setup_fb30_work_accept(NbaSetupFb30Work *work, uint8_t read_value);

#endif
