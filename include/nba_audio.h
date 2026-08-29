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
    void *gameplay_mixer;
    uint8_t *setup_sfx_wav;
    size_t setup_sfx_wav_size;
    size_t setup_sfx_wav_length;
    uint16_t setup_sfx_volume;
    uint16_t setup_music_volume;
    uint8_t last_setup_sfx_srcn;
    uint8_t last_gameplay_command;
    uint8_t last_gameplay_srcn;
    uint16_t last_gameplay_pitch;
    int16_t last_gameplay_volume;
    uint32_t gameplay_event_count;
    uint16_t gameplay_audio_rng_state;
    uint16_t gameplay_latched_13e7;
    uint16_t gameplay_latched_13e9;
    uint8_t active_track;
    uint8_t status;
    bool host_playback_enabled;
} NbaAudio;

typedef enum {
    NBA_AUDIO_TRACK_NONE = 0,
    NBA_AUDIO_TRACK_WAV,
    NBA_AUDIO_TRACK_TITLE_SPC,
    NBA_AUDIO_TRACK_SETUP_SPC,
    NBA_AUDIO_TRACK_PLAYER_INTRO_SPC
} NbaAudioTrack;

typedef enum {
    NBA_AUDIO_STATUS_IDLE = 0,
    NBA_AUDIO_STATUS_READY,
    NBA_AUDIO_STATUS_PLAYING,
    NBA_AUDIO_STATUS_HOST_FAILED,
    NBA_AUDIO_STATUS_SYNTH_FAILED
} NbaAudioStatus;

void nba_audio_init(NbaAudio *audio);
void nba_audio_set_host_playback_enabled(NbaAudio *audio, bool enabled);
void nba_audio_shutdown(NbaAudio *audio);
void nba_audio_play_wav(NbaAudio *audio, const void *data, size_t size);
void nba_audio_play_setup_sfx(NbaAudio *audio, const NbaAssetPack *assets, uint8_t srcn);
bool nba_audio_play_gameplay_whistle(NbaAudio *audio,
                                     const NbaAssetPack *assets);
bool nba_audio_start_gameplay(NbaAudio *audio, const NbaAssetPack *assets);
void nba_audio_dispatch_gameplay_events(NbaAudio *audio,
                                        uint16_t event_bits_raw_13e7,
                                        uint16_t crowd_bits_raw_13e9);
bool nba_audio_gameplay_self_test(const NbaAssetPack *assets);
void nba_audio_set_setup_music_volume(NbaAudio *audio, uint16_t value, uint16_t maximum);
void nba_audio_set_setup_sfx_volume(NbaAudio *audio, uint16_t value, uint16_t maximum);
bool nba_audio_play_title_spc(NbaAudio *audio, const NbaAssetPack *assets);
bool nba_audio_play_setup_dsp(NbaAudio *audio, const NbaAssetPack *assets);
bool nba_audio_play_player_intro_dsp(NbaAudio *audio, const NbaAssetPack *assets);
bool nba_audio_save_generated_wav(const NbaAudio *audio, const char *path);
bool nba_audio_save_setup_sfx_wav(const NbaAudio *audio, const char *path);
void nba_audio_stop(NbaAudio *audio);

#endif /* NBA_AUDIO_H */
