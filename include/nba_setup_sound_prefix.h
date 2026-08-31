#ifndef NBA_SETUP_SOUND_PREFIX_H
#define NBA_SETUP_SOUND_PREFIX_H
#include "nba_setup_codec_work.h"

/* Source-specific experimental prefix, not production audio state. Native
 * entry: A137, E=0, D=0, DP=0, M=0, X=1, DB=80. Live memory belongs to the
 * caller. Do not initialize it from snapshots in a normal game journey.
 * Scope includes inactive stream/fade/queued-command gates and active-channel
 * pitch publication up to AAE6's first unresolved SPC port read. Other source
 * branches stop explicitly, notably A2CE's sequencer and pending sound events.
 */
typedef enum {
    NBA_SOUND_PREFIX_NONE,
    NBA_SOUND_PREFIX_SPC_RESPONSE,
    NBA_SOUND_PREFIX_UNIMPLEMENTED,
    NBA_SOUND_PREFIX_RETURN
} NbaSoundPrefixStop;

typedef struct {
    NbaSetupCodecWork bus;
    uint32_t boundary_pc;
    NbaSoundPrefixStop stop;
    uint8_t read_bank, rmw_width, change_kind, carry_in;
} NbaSetupSoundPrefix;

bool nba_setup_sound_prefix_begin(NbaSetupSoundPrefix *work,
                                 const NbaCodecWorkEntry *entry,
                                 uint64_t instruction_limit);
/* At SPC_RESPONSE, peek exposes the pending READ without consuming it;
 * accept rejects every supplied value without changing state. A future live
 * SPC adapter and subsequent source continuation must own its completion.
 * Already completed fetch cycles retain their intrinsic cost. */
bool nba_setup_sound_prefix_peek(const NbaSetupSoundPrefix *work, NbaCodecBusCycle *cycle);
bool nba_setup_sound_prefix_accept(NbaSetupSoundPrefix *work, uint8_t read_value);
#endif
