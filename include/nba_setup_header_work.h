#ifndef NBA_SETUP_HEADER_WORK_H
#define NBA_SETUP_HEADER_WORK_H

#include "nba_setup_codec_work.h"

/* $80:EEC6 through entry to $80:EF1A, before the wait's JSL. Native mode,
 * direct page zero, M=X=decimal=0, FastROM and immediate publication are
 * caller contracts. DMA requests/effects and elapsed servicing are external.
 * This source work neither samples nor waits for a publication epoch. */
typedef struct {
    NbaSetupCodecWork bus;
    uint8_t read_bank;
} NbaSetupHeaderWork;

bool nba_setup_header_work_begin(NbaSetupHeaderWork *work,
                                 const NbaCodecWorkEntry *entry,
                                 uint64_t instruction_limit);
bool nba_setup_header_work_peek(const NbaSetupHeaderWork *work, NbaCodecBusCycle *cycle);
bool nba_setup_header_work_accept(NbaSetupHeaderWork *work, uint8_t read_value);

#endif
