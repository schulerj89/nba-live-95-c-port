#ifndef NBA_AUDIO_H
#define NBA_AUDIO_H

#include "nba_types.h"
#include <stddef.h>

void nba_audio_init(void);
void nba_audio_shutdown(void);
void nba_audio_play_wav(const void *data, size_t size);
void nba_audio_stop(void);

#endif /* NBA_AUDIO_H */
