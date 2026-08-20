#ifndef NBA_AUDIO_H
#define NBA_AUDIO_H

#include "nba_types.h"
#include "nba_assets.h"
#include <stddef.h>

/* SNES APU Communication & Sound Driver Routine Addresses */
#define SNES_APU_PORT0              0x2140    /* APU I/O Port 0 (Command request) */
#define SNES_APU_PORT1              0x2141    /* APU I/O Port 1 (Command parameter/data) */
#define SNES_APU_PORT2              0x2142    /* APU I/O Port 2 */
#define SNES_APU_PORT3              0x2143    /* APU I/O Port 3 */

typedef struct {
    uint8_t *generated_wav;
    size_t generated_wav_size;
    void *loop_playback;
} NbaAudio;

void nba_audio_init(NbaAudio *audio);
void nba_audio_shutdown(NbaAudio *audio);
void nba_audio_play_wav(NbaAudio *audio, const void *data, size_t size);
bool nba_audio_play_title_spc(NbaAudio *audio, const NbaAssetPack *assets);
bool nba_audio_play_setup_dsp(NbaAudio *audio, const NbaAssetPack *assets);
bool nba_audio_save_generated_wav(const NbaAudio *audio, const char *path);
void nba_audio_stop(NbaAudio *audio);

#endif /* NBA_AUDIO_H */
