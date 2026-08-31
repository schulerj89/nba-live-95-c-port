#ifndef NBA_HUMAN_PASS_RETURN_H
#define NBA_HUMAN_PASS_RETURN_H
#include "nba_types.h"

/* Source-owned saved words, captured at the caller's entry, not at return.
 * Actual CPU stack/SP/RTL mechanics remain the host C call structure. */
typedef struct {
    uint16_t b8, b6, bc, ba, c0, be, word_9c, word_9a;
} NbaHumanPassReturnWords;

bool nba_human_pass_return_save(const NbaHumanPassReturnWords *scratch,
    NbaHumanPassReturnWords *frame);
bool nba_human_pass_return_restore(NbaHumanPassReturnWords *scratch,
    const NbaHumanPassReturnWords *frame);
bool nba_human_pass_return_human_tail(NbaHumanPassReturnWords *scratch,
    uint16_t caller_b6);
bool nba_human_pass_return_finish(NbaHumanPassReturnWords *scratch,
    const NbaHumanPassReturnWords *initializer_frame,
    const NbaHumanPassReturnWords *selector_frame, uint16_t caller_b6);
#endif
