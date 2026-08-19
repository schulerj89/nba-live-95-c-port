#ifndef NBA_AUDIO_H
#define NBA_AUDIO_H

#include "nba_types.h"
#include <stddef.h>

/* SNES APU Communication & Sound Driver Routine Addresses */
#define SNES_APU_PORT0              0x2140    /* APU I/O Port 0 (Command request) */
#define SNES_APU_PORT1              0x2141    /* APU I/O Port 1 (Command parameter/data) */
#define SNES_APU_PORT2              0x2142    /* APU I/O Port 2 */
#define SNES_APU_PORT3              0x2143    /* APU I/O Port 3 */

#define SNES_ADDR_SOUND_QUEUE_INIT  0x80AC1B  /* $80:AC1B - Init sound queue / reset channels */
#define SNES_ADDR_SOUND_DISPATCH    0x80AC89  /* $80:AC89 - Dispatch sound trigger to APU ($2140) */
#define SNES_ADDR_SOUND_PARAM_LOAD  0x80B344  /* $80:B344 - Load sound sample parameters */

void nba_audio_init(void);
void nba_audio_shutdown(void);
void nba_audio_play_wav(const void *data, size_t size);
void nba_audio_stop(void);

#endif /* NBA_AUDIO_H */
