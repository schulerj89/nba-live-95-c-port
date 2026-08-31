#include "nba_human_pass_return.h"

bool nba_human_pass_return_save(const NbaHumanPassReturnWords *scratch,
    NbaHumanPassReturnWords *frame) {
    if (!scratch || !frame) return false;
    /* 86:AB2D-AB3B and 84:DF7A-DF88 save these same eight caller words. */
    *frame=*scratch;
    return true;
}
bool nba_human_pass_return_restore(NbaHumanPassReturnWords *scratch,
    const NbaHumanPassReturnWords *frame) {
    if (!scratch || !frame) return false;
    /* 86:AF4D-AF63 and 84:E09C-E0B2 restore in reverse push order.
     * These are two distinct nested frames; neither uses a later snapshot. */
    scratch->word_9a=frame->word_9a;
    scratch->word_9c=frame->word_9c;
    scratch->be=frame->be;
    scratch->c0=frame->c0;
    scratch->ba=frame->ba;
    scratch->bc=frame->bc;
    scratch->b6=frame->b6;
    scratch->b8=frame->b8;
    return true;
}
bool nba_human_pass_return_human_tail(NbaHumanPassReturnWords *scratch,
    uint16_t caller_b6) {
    if (!scratch) return false;
    /* E2E8 jumps directly to E3E6. E3E6/E3E7 restores only the word saved
     * at E2AC. Do not retain DF7A's B-button mask in the outer caller. */
    scratch->b6=caller_b6;
    return true;
}
bool nba_human_pass_return_finish(NbaHumanPassReturnWords *scratch,
    const NbaHumanPassReturnWords *initializer_frame,
    const NbaHumanPassReturnWords *selector_frame, uint16_t caller_b6) {
    if (!scratch || !initializer_frame || !selector_frame) return false;
    return nba_human_pass_return_restore(scratch,initializer_frame)
        && nba_human_pass_return_restore(scratch,selector_frame)
        && nba_human_pass_return_human_tail(scratch,caller_b6);
    /* Stops before 84:E3E9 RTL; no launch/animation/control continuation. */
}
