#ifndef NBA_SETUP_PRODUCER_WORK_H
#define NBA_SETUP_PRODUCER_WORK_H

#include "nba_setup_fb30_work.h"

/* $80:EC68 through the final $EDF8 RTL. The caller's following JSL to the
 * header is separate source work. Native M=X=decimal=D=0, FastROM, live
 * WRAM/IO mirrors and immediate empty publication queue are caller contracts.
 * DMA requests are emitted at their actual $420B write; the external hardware
 * adapter owns pending-DMA service, alignment, transfers and elapsed time.
 * The five codecs use the unchanged, separately audited continuations. */
typedef struct {
    NbaSetupFb30Work local;
    struct { NbaSetupCodecWork fb46; NbaSetupFb30Work fb30; } child;
    uint32_t return_pc;
    uint8_t pointer_bank;
    uint8_t codec_kind;
    bool child_active;
} NbaSetupProducerWork;

bool nba_setup_producer_work_begin(NbaSetupProducerWork *work,
                                   const NbaCodecWorkEntry *entry,
                                   uint64_t instruction_limit);
bool nba_setup_producer_work_peek(const NbaSetupProducerWork *work, NbaCodecBusCycle *cycle);
bool nba_setup_producer_work_accept(NbaSetupProducerWork *work, uint8_t read_value);

#endif
