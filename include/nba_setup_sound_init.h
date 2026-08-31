#ifndef NBA_SETUP_SOUND_INIT_H
#define NBA_SETUP_SOUND_INIT_H
#include "nba_setup_codec_work.h"

/* Experimental source initialization slice: 9B73 entry (native, DP=0, D=0,
 * M=0/X=0) through the first channel-off helper's unresolved AACD idle-port
 * read. Entry and memory are live caller state, not a production snapshot.
 * DB must be a WRAM/IO mirror or source-supported 7E/7F normalization case.
 */
typedef enum {
    NBA_SOUND_INIT_NONE,
    NBA_SOUND_INIT_SPC_RESPONSE,
    NBA_SOUND_INIT_UNIMPLEMENTED,
    NBA_SOUND_INIT_RETURN
} NbaSoundInitStop;
typedef struct {
    NbaSetupCodecWork bus;
    uint32_t boundary_pc;
    NbaSoundInitStop stop;
    uint8_t read_bank,rmw_width,change_kind,carry_in;
} NbaSetupSoundInit;
bool nba_setup_sound_init_begin(NbaSetupSoundInit *work,const NbaCodecWorkEntry *entry,uint64_t instruction_limit);
/* Pending SPC data read is observable but not accepted: the upload consumer
 * and remaining source helper must be implemented before it can complete. */
bool nba_setup_sound_init_peek(const NbaSetupSoundInit *work,NbaCodecBusCycle *cycle);
bool nba_setup_sound_init_accept(NbaSetupSoundInit *work,uint8_t read_value);
#endif
