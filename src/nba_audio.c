#include "nba_audio.h"
#include "nba_spc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* `$80:A9B3-$AB05` owns the native $2140-$2143 wait/parameter/ack transport.
 * The host never races a separate SPC chip: frame/cycle-stamped port events
 * are applied to nba_spc.c in their recorded order before the corresponding
 * PCM slice is rendered. This preserves the complete hardware-visible
 * command stream while deliberately omitting busy-wait loops. Title, Setup,
 * Player Introduction and gameplay SFX tests all reject captured WAVs and
 * require ROM BRR/SPC/DSP assets from the asset pack. */

#define NBA_SETUP_LOOP_START_SAMPLE 2053956u
#define NBA_SETUP_LOOP_END_SAMPLE   4048365u
#define NBA_GAMEPLAY_MIX_FRAMES     1024u
#define NBA_GAMEPLAY_MIX_BUFFERS    4u
#define NBA_GAMEPLAY_VOICES         8u

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

typedef struct {
    HWAVEOUT device;
    WAVEHDR intro;
    WAVEHDR loop;
} NbaWaveLoop;

#endif

typedef struct {
    const int16_t *pcm;
    uint32_t count;
    uint32_t loop_start;
    uint32_t rate;
} NbaGameplaySample;

typedef struct {
    const NbaGameplaySample *sample;
    uint32_t cursor_16_16;
    uint32_t step_16_16;
    int16_t volume_l;
    int16_t volume_r;
    bool active;
    bool loop;
} NbaGameplayVoice;

typedef struct {
    NbaGameplaySample samples[30];
    NbaGameplayVoice voices[NBA_GAMEPLAY_VOICES];
    uint8_t next_effect_voice;
#if defined(_WIN32)
    HWAVEOUT device;
    HANDLE event;
    HANDLE thread;
    CRITICAL_SECTION lock;
    WAVEHDR headers[NBA_GAMEPLAY_MIX_BUFFERS];
    int16_t buffers[NBA_GAMEPLAY_MIX_BUFFERS][NBA_GAMEPLAY_MIX_FRAMES * 2u];
    volatile LONG running;
#endif
} NbaGameplayMixer;

typedef struct {
    uint8_t command;
    uint8_t srcn;
    uint16_t pitch;
    int16_t volume;
} NbaGameplayCommand;

/* `$82:F822-$F885`: low-byte values from the ROM's 16-bit command tables,
 * consumed by `gameplay_audio_event_dispatch` (`$82:FD65-$FF08`). */
static const uint8_t AUDIO_BOUNCE[4] = {0x23, 0x2B, 0x33, 0x23};
static const uint8_t AUDIO_INNER_RIM[16] = {
    0x09,0x0A,0x0B,0x11,0x12,0x13,0x19,0x1A,
    0x1B,0x0B,0x19,0x09,0x1A,0x13,0x0A,0x1B
};
static const uint8_t AUDIO_MADE[4] = {0x0C, 0x14, 0x1C, 0x0C};
static const uint8_t AUDIO_OUTER_RIM[4] = {0x08, 0x10, 0x18, 0x08};
static const uint8_t AUDIO_CATCH[4] = {0x24, 0x2C, 0x34, 0x24};
static const uint8_t AUDIO_CONTACT[4] = {0x0D, 0x15, 0x1D, 0x0D};
static const uint8_t AUDIO_SHOE[4] = {0x0E, 0x1E, 0x1E, 0x0E};
static const uint8_t AUDIO_COLLISION[8] = {
    0x21, 0x29, 0x31, 0x22, 0x2A, 0x32, 0x21, 0x29
};
static const uint8_t AUDIO_LANDING[4] = {0x20, 0x28, 0x30, 0x20};

static uint16_t audio_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t audio_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int16_t audio_clip_s16(int32_t value) {
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (int16_t)value;
}

static bool audio_load_gameplay_bank(NbaGameplayMixer *mixer,
                                     const NbaAssetPack *assets) {
    const NbaAssetItem *item = nba_assets_get(assets,
        NBA_ASSET_GAMEPLAY_AUDIO_BANK);
    if (!mixer || !item || item->size < 16u) return false;
    const uint8_t *data = (const uint8_t *)item->data;
    if (memcmp(data, "NBGAUD1\0", 8) != 0 || audio_u32(data + 8) != 1u)
        return false;
    uint32_t count = audio_u32(data + 12);
    if (count > 30u || 16u + count * 20u > item->size) return false;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t *entry = data + 16u + i * 20u;
        uint32_t srcn = audio_u32(entry);
        uint32_t rate = audio_u32(entry + 4);
        uint32_t samples = audio_u32(entry + 8);
        uint32_t loop_start = audio_u32(entry + 12);
        uint32_t offset = audio_u32(entry + 16);
        if (srcn >= 30u || rate == 0u || samples == 0u ||
            offset > item->size || samples > (item->size - offset) / 2u)
            return false;
        mixer->samples[srcn].pcm = (const int16_t *)(data + offset);
        mixer->samples[srcn].count = samples;
        mixer->samples[srcn].loop_start = loop_start;
        mixer->samples[srcn].rate = rate;
    }
    return mixer->samples[0x0Au].pcm && mixer->samples[0x0Bu].pcm &&
           mixer->samples[0x0Cu].pcm && mixer->samples[0x0Du].pcm &&
           mixer->samples[0x12u].pcm;
}

static void audio_gameplay_key_voice(NbaGameplayMixer *mixer, uint8_t voice,
                                     uint8_t srcn, uint16_t pitch,
                                     int16_t volume_l, int16_t volume_r,
                                     bool loop) {
    if (!mixer || voice >= NBA_GAMEPLAY_VOICES || srcn >= 30u ||
        !mixer->samples[srcn].pcm) return;
    NbaGameplayVoice *target = &mixer->voices[voice];
    target->sample = &mixer->samples[srcn];
    target->cursor_16_16 = 0u;
    /* DSP pitch $1000 advances one decoded BRR sample per 32-kHz output. */
    target->step_16_16 = (uint32_t)pitch << 4;
    target->volume_l = volume_l;
    target->volume_r = volume_r;
    target->loop = loop;
    target->active = true;
}

static void audio_gameplay_mix(NbaGameplayMixer *mixer, int16_t *output,
                               uint32_t frames) {
    memset(output, 0, (size_t)frames * 2u * sizeof(*output));
    for (uint32_t frame = 0; frame < frames; ++frame) {
        int32_t left = 0, right = 0;
        for (uint32_t index = 0; index < NBA_GAMEPLAY_VOICES; ++index) {
            NbaGameplayVoice *voice = &mixer->voices[index];
            if (!voice->active || !voice->sample) continue;
            uint32_t position = voice->cursor_16_16 >> 16;
            if (position >= voice->sample->count) {
                if (voice->loop && voice->sample->loop_start < voice->sample->count) {
                    position = voice->sample->loop_start;
                    voice->cursor_16_16 = position << 16;
                } else {
                    voice->active = false;
                    continue;
                }
            }
            int32_t sample = voice->sample->pcm[position];
            left += (sample * voice->volume_l) / 128;
            right += (sample * voice->volume_r) / 128;
            voice->cursor_16_16 += voice->step_16_16;
        }
        output[frame * 2u] = audio_clip_s16(left);
        output[frame * 2u + 1u] = audio_clip_s16(right);
    }
}

#if defined(_WIN32)
static DWORD WINAPI audio_gameplay_thread(LPVOID parameter) {
    NbaGameplayMixer *mixer = (NbaGameplayMixer *)parameter;
    while (InterlockedCompareExchange(&mixer->running, 1, 1) != 0) {
        WaitForSingleObject(mixer->event, 1000u);
        if (InterlockedCompareExchange(&mixer->running, 1, 1) == 0) break;
        for (uint32_t i = 0; i < NBA_GAMEPLAY_MIX_BUFFERS; ++i) {
            if ((mixer->headers[i].dwFlags & WHDR_DONE) == 0u) continue;
            EnterCriticalSection(&mixer->lock);
            audio_gameplay_mix(mixer, mixer->buffers[i],
                               NBA_GAMEPLAY_MIX_FRAMES);
            LeaveCriticalSection(&mixer->lock);
            waveOutWrite(mixer->device, &mixer->headers[i],
                         sizeof(mixer->headers[i]));
        }
    }
    return 0;
}

static bool audio_gameplay_start_host(NbaGameplayMixer *mixer) {
    WAVEFORMATEX format = { WAVE_FORMAT_PCM, 2, NBA_SPC_SAMPLE_RATE,
                            NBA_SPC_SAMPLE_RATE * 4u, 4, 16, 0 };
    InitializeCriticalSection(&mixer->lock);
    mixer->event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!mixer->event || waveOutOpen(&mixer->device, WAVE_MAPPER, &format,
            (DWORD_PTR)mixer->event, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
        if (mixer->event) CloseHandle(mixer->event);
        DeleteCriticalSection(&mixer->lock);
        return false;
    }
    for (uint32_t i = 0; i < NBA_GAMEPLAY_MIX_BUFFERS; ++i) {
        audio_gameplay_mix(mixer, mixer->buffers[i], NBA_GAMEPLAY_MIX_FRAMES);
        mixer->headers[i].lpData = (LPSTR)mixer->buffers[i];
        mixer->headers[i].dwBufferLength = sizeof(mixer->buffers[i]);
        if (waveOutPrepareHeader(mixer->device, &mixer->headers[i],
                sizeof(mixer->headers[i])) != MMSYSERR_NOERROR ||
            waveOutWrite(mixer->device, &mixer->headers[i],
                sizeof(mixer->headers[i])) != MMSYSERR_NOERROR) {
            waveOutReset(mixer->device);
            for (uint32_t j = 0; j <= i; ++j)
                waveOutUnprepareHeader(mixer->device, &mixer->headers[j],
                                       sizeof(mixer->headers[j]));
            waveOutClose(mixer->device);
            CloseHandle(mixer->event);
            DeleteCriticalSection(&mixer->lock);
            return false;
        }
    }
    InterlockedExchange(&mixer->running, 1);
    mixer->thread = CreateThread(NULL, 0, audio_gameplay_thread, mixer, 0, NULL);
    if (!mixer->thread) {
        InterlockedExchange(&mixer->running, 0);
        waveOutReset(mixer->device);
        for (uint32_t i = 0; i < NBA_GAMEPLAY_MIX_BUFFERS; ++i)
            waveOutUnprepareHeader(mixer->device, &mixer->headers[i],
                                   sizeof(mixer->headers[i]));
        waveOutClose(mixer->device);
        CloseHandle(mixer->event);
        DeleteCriticalSection(&mixer->lock);
        return false;
    }
    return true;
}

static void audio_gameplay_stop_host(NbaGameplayMixer *mixer) {
    if (!mixer || !mixer->device) return;
    InterlockedExchange(&mixer->running, 0);
    SetEvent(mixer->event);
    if (mixer->thread) {
        WaitForSingleObject(mixer->thread, 2000u);
        CloseHandle(mixer->thread);
    }
    waveOutReset(mixer->device);
    for (uint32_t i = 0; i < NBA_GAMEPLAY_MIX_BUFFERS; ++i)
        waveOutUnprepareHeader(mixer->device, &mixer->headers[i],
                               sizeof(mixer->headers[i]));
    waveOutClose(mixer->device);
    CloseHandle(mixer->event);
    DeleteCriticalSection(&mixer->lock);
}
#endif

static void audio_put_u16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
}

static void audio_put_u32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

typedef struct {
    NbaAssetId ram_id;
    NbaAssetId dsp_id;
    NbaAssetId state_id;
    NbaAssetId trace_id;
    const char *state_magic;
    const char *trace_magic;
    const char *name;
    uint32_t max_frames;
    uint32_t event_size;
} NbaSpcTrackSpec;

typedef struct {
    NbaSpc *spc;
    uint8_t *wav;
    int16_t *pcm;
    const uint8_t *trace;
    uint32_t frame_count;
    uint32_t event_count;
    uint32_t sample_count;
    size_t wav_size;
} NbaSpcTrackRender;

static bool audio_prepare_spc_track(const NbaAssetPack *assets,
                                    const NbaSpcTrackSpec *spec,
                                    NbaSpcTrackRender *render) {
    memset(render, 0, sizeof(*render));
    const NbaAssetItem *ram = nba_assets_get(assets, spec->ram_id);
    const NbaAssetItem *dsp = nba_assets_get(assets, spec->dsp_id);
    const NbaAssetItem *state_item = nba_assets_get(assets, spec->state_id);
    const NbaAssetItem *trace_item = nba_assets_get(assets, spec->trace_id);
    if (!ram || ram->size < NBA_SPC_RAM_SIZE ||
        !dsp || dsp->size < NBA_SPC_DSP_REGS ||
        !state_item || state_item->size < 20 ||
        !trace_item || trace_item->size < 20) {
        fprintf(stderr, "[AUDIO] %s SPC assets are missing or truncated.\n", spec->name);
        return false;
    }

    const uint8_t *state = (const uint8_t *)state_item->data;
    render->trace = (const uint8_t *)trace_item->data;
    if (memcmp(state, spec->state_magic, 8) != 0 || audio_u32(state + 8) != 1 ||
        memcmp(render->trace, spec->trace_magic, 8) != 0 ||
        audio_u32(render->trace + 8) != 1) {
        fprintf(stderr, "[AUDIO] Unsupported %s SPC asset format.\n", spec->name);
        return false;
    }

    render->frame_count = audio_u32(render->trace + 12);
    render->event_count = audio_u32(render->trace + 16);
    if (render->frame_count == 0 || render->frame_count > spec->max_frames ||
        render->event_count > (trace_item->size - 20u) / spec->event_size) {
        fprintf(stderr, "[AUDIO] Invalid %s APU trace dimensions.\n", spec->name);
        return false;
    }

    render->sample_count = (uint32_t)(((uint64_t)render->frame_count *
                                       NBA_SPC_SAMPLE_RATE) / 60u);
    size_t pcm_bytes = (size_t)render->sample_count * 2u * sizeof(int16_t);
    if (pcm_bytes > SIZE_MAX - 44u) return false;

    render->spc = (NbaSpc *)malloc(sizeof(*render->spc));
    render->wav = (uint8_t *)malloc(44u + pcm_bytes);
    if (!render->spc || !render->wav ||
        !nba_spc_load(render->spc, ram->data, ram->size, dsp->data, dsp->size,
                      audio_u16(state + 12), state[14], state[15], state[16],
                      state[17], state[18])) {
        free(render->spc);
        free(render->wav);
        memset(render, 0, sizeof(*render));
        return false;
    }

    render->wav_size = 44u + pcm_bytes;
    memcpy(render->wav, "RIFF", 4);
    audio_put_u32(render->wav + 4, 36u + (uint32_t)pcm_bytes);
    memcpy(render->wav + 8, "WAVEfmt ", 8);
    audio_put_u32(render->wav + 16, 16);
    audio_put_u16(render->wav + 20, 1);
    audio_put_u16(render->wav + 22, 2);
    audio_put_u32(render->wav + 24, NBA_SPC_SAMPLE_RATE);
    audio_put_u32(render->wav + 28, NBA_SPC_SAMPLE_RATE * 4u);
    audio_put_u16(render->wav + 32, 4);
    audio_put_u16(render->wav + 34, 16);
    memcpy(render->wav + 36, "data", 4);
    audio_put_u32(render->wav + 40, (uint32_t)pcm_bytes);
    render->pcm = (int16_t *)(render->wav + 44);
    return true;
}

static bool audio_play_generated(NbaAudio *audio, NbaSpcTrackRender *render,
                                 bool loop, NbaAudioTrack track) {
    free(render->spc);
    render->spc = NULL;
    nba_audio_stop(audio);
    audio->generated_wav = render->wav;
    audio->generated_wav_size = render->wav_size;
    audio->active_track = (uint8_t)track;
    audio->status = NBA_AUDIO_STATUS_READY;
    render->wav = NULL;
    if (!audio->host_playback_enabled) return true;
#if defined(_WIN32)
    if (loop) {
        if (render->sample_count < NBA_SETUP_LOOP_END_SAMPLE) {
            nba_audio_stop(audio);
            return false;
        }
        NbaWaveLoop *stream = (NbaWaveLoop *)calloc(1, sizeof(*stream));
        WAVEFORMATEX format = { WAVE_FORMAT_PCM, 2, NBA_SPC_SAMPLE_RATE,
                                NBA_SPC_SAMPLE_RATE * 4u, 4, 16, 0 };
        if (!stream || waveOutOpen(&stream->device, WAVE_MAPPER, &format,
                                   0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
            free(stream);
            audio->status = NBA_AUDIO_STATUS_HOST_FAILED;
            fprintf(stderr, "[AUDIO] Host waveOut device unavailable; synthesized PCM retained.\n");
            return true;
        }
        char *pcm = (char *)audio->generated_wav + 44;
        stream->intro.lpData = pcm;
        stream->intro.dwBufferLength = NBA_SETUP_LOOP_START_SAMPLE * 4u;
        stream->loop.lpData = pcm + NBA_SETUP_LOOP_START_SAMPLE * 4u;
        stream->loop.dwBufferLength =
            (NBA_SETUP_LOOP_END_SAMPLE - NBA_SETUP_LOOP_START_SAMPLE) * 4u;
        stream->loop.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        stream->loop.dwLoops = 0xFFFFFFFFu;
        if (waveOutPrepareHeader(stream->device, &stream->intro,
                                 sizeof(stream->intro)) != MMSYSERR_NOERROR ||
            waveOutPrepareHeader(stream->device, &stream->loop,
                                 sizeof(stream->loop)) != MMSYSERR_NOERROR ||
            waveOutWrite(stream->device, &stream->intro,
                         sizeof(stream->intro)) != MMSYSERR_NOERROR ||
            waveOutWrite(stream->device, &stream->loop,
                         sizeof(stream->loop)) != MMSYSERR_NOERROR) {
            waveOutReset(stream->device);
            waveOutUnprepareHeader(stream->device, &stream->intro, sizeof(stream->intro));
            waveOutUnprepareHeader(stream->device, &stream->loop, sizeof(stream->loop));
            waveOutClose(stream->device);
            free(stream);
            audio->status = NBA_AUDIO_STATUS_HOST_FAILED;
            fprintf(stderr, "[AUDIO] Host waveOut playback failed; synthesized PCM retained.\n");
            return true;
        }
        audio->loop_playback = stream;
        audio->status = NBA_AUDIO_STATUS_PLAYING;
    } else if (!PlaySoundA((LPCSTR)audio->generated_wav, NULL,
                           SND_MEMORY | SND_ASYNC | SND_NODEFAULT)) {
        audio->status = NBA_AUDIO_STATUS_HOST_FAILED;
        fprintf(stderr, "[AUDIO] Host WAV playback failed; synthesized PCM retained.\n");
        return true;
    } else {
        audio->status = NBA_AUDIO_STATUS_PLAYING;
    }
#endif
    return true;
}

static void audio_discard_render(NbaSpcTrackRender *render) {
    free(render->spc);
    free(render->wav);
    memset(render, 0, sizeof(*render));
}

/**
 * Offset/Address/Size: 0x002C1B | $80:AC1B | size: 0x24
 * Purpose: Initializes the host audio playback subsystem (corresponds to SNES APU sound channel init).
 */
void nba_audio_init(NbaAudio *audio) {
    if (!audio) return;
    memset(audio, 0, sizeof(*audio));
    audio->setup_sfx_volume = 30u;
    audio->setup_music_volume = 30u;
    audio->last_setup_sfx_srcn = 0xFFu;
    audio->last_gameplay_command = 0xFFu;
    audio->last_gameplay_srcn = 0xFFu;
    audio->host_playback_enabled = true;
    printf("[AUDIO] Initializing Audio Subsystem...\n");
}

void nba_audio_set_host_playback_enabled(NbaAudio *audio, bool enabled) {
    if (!audio) return;
    audio->host_playback_enabled = enabled;
}

/**
 * Offset/Address/Size: N/A | Host Audio Shutdown | size: N/A
 * Purpose: Stops any active asynchronous audio stream and releases audio resources.
 */
void nba_audio_shutdown(NbaAudio *audio) {
    if (!audio) return;
    nba_audio_stop(audio);
    free(audio->setup_sfx_wav);
    audio->setup_sfx_wav = NULL;
    audio->setup_sfx_wav_size = 0;
    audio->setup_sfx_wav_length = 0;
    printf("[AUDIO] Audio Subsystem shutdown.\n");
}

/**
 * Offset/Address/Size: 0x002C89 | $80:AC89 | size: 0x30
 * Purpose: Plays a PCM/WAV sound buffer asynchronously (corresponds to SNES APU port $2140 dispatch).
 */
void nba_audio_play_wav(NbaAudio *audio, const void *data, size_t size) {
    if (!audio || !data || size == 0) return;

    nba_audio_stop(audio);
    audio->active_track = NBA_AUDIO_TRACK_WAV;
    audio->status = NBA_AUDIO_STATUS_READY;

#if defined(_WIN32)
    if (audio->host_playback_enabled) {
        audio->status = PlaySoundA((LPCSTR)data, NULL,
                                   SND_MEMORY | SND_ASYNC | SND_NODEFAULT) ?
                        NBA_AUDIO_STATUS_PLAYING : NBA_AUDIO_STATUS_HOST_FAILED;
    }
#else
    (void)data;
    (void)size;
#endif
}

static bool audio_play_spc_sfx(NbaAudio *audio, const NbaAssetPack *assets,
                               NbaAssetId ram_id, NbaAssetId dsp_id,
                               NbaAssetId state_id, const char state_magic[8],
                               uint8_t srcn, uint8_t voice, uint16_t pitch,
                               uint8_t volume_l, uint8_t volume_r,
                               uint8_t adsr1, uint8_t adsr2, uint8_t gain,
                               uint32_t sample_frames, int *peak_out) {
    if (!audio || !assets || voice >= 8u || sample_frames == 0u) return false;
    const NbaAssetItem *ram = nba_assets_get(assets, ram_id);
    const NbaAssetItem *dsp = nba_assets_get(assets, dsp_id);
    const NbaAssetItem *state_item = nba_assets_get(assets, state_id);
    if (!ram || ram->size < NBA_SPC_RAM_SIZE ||
        !dsp || dsp->size < NBA_SPC_DSP_REGS ||
        !state_item || state_item->size < 20u) return false;
    const uint8_t *state = (const uint8_t *)state_item->data;
    if (memcmp(state, state_magic, 8) != 0 ||
        audio_u32(state + 8) != 1u) return false;

    const size_t pcm_bytes = (size_t)sample_frames * 2u * sizeof(int16_t);
    const size_t wav_size = 44u + pcm_bytes;
#if defined(_WIN32)
    if (audio->host_playback_enabled) PlaySoundA(NULL, NULL, 0);
#endif
    if (audio->setup_sfx_wav_size < wav_size) {
        uint8_t *grown = (uint8_t *)realloc(audio->setup_sfx_wav, wav_size);
        if (!grown) return false;
        audio->setup_sfx_wav = grown;
        audio->setup_sfx_wav_size = wav_size;
    }
    NbaSpc *spc = (NbaSpc *)malloc(sizeof(*spc));
    if (!spc) return false;
    if (!nba_spc_load(spc, ram->data, ram->size, dsp->data, dsp->size,
                      audio_u16(state + 12), state[14], state[15], state[16],
                      state[17], state[18])) {
        free(spc);
        return false;
    }
    for (int index = 0; index < 8; ++index) {
        nba_spc_write_dsp(spc, (uint8_t)(index * 16 + NBA_DSP_VOL_L), 0);
        nba_spc_write_dsp(spc, (uint8_t)(index * 16 + NBA_DSP_VOL_R), 0);
    }
    nba_spc_write_dsp(spc, NBA_DSP_KOF, 0xFFu);
    nba_spc_write_dsp(spc, NBA_DSP_PMON, 0u);
    nba_spc_write_dsp(spc, NBA_DSP_NON, 0u);
    nba_spc_write_dsp(spc, NBA_DSP_EON, 0u);
    nba_spc_render_dsp(spc, (int16_t *)(audio->setup_sfx_wav + 44), 64);
    const uint8_t base = (uint8_t)(voice * 16u);
    nba_spc_write_dsp(spc, base + NBA_DSP_VOL_L, volume_l);
    nba_spc_write_dsp(spc, base + NBA_DSP_VOL_R, volume_r);
    nba_spc_write_dsp(spc, base + NBA_DSP_PITCH_L, (uint8_t)pitch);
    nba_spc_write_dsp(spc, base + NBA_DSP_PITCH_H, (uint8_t)(pitch >> 8));
    nba_spc_write_dsp(spc, base + NBA_DSP_SRCN, srcn);
    nba_spc_write_dsp(spc, base + NBA_DSP_ADSR1, adsr1);
    nba_spc_write_dsp(spc, base + NBA_DSP_ADSR2, adsr2);
    nba_spc_write_dsp(spc, base + NBA_DSP_GAIN, gain);
    nba_spc_write_dsp(spc, NBA_DSP_KOF, 0u);
    nba_spc_write_dsp(spc, NBA_DSP_KON, (uint8_t)(1u << voice));
    int16_t *pcm = (int16_t *)(audio->setup_sfx_wav + 44);
    nba_spc_render_dsp(spc, pcm, (int)sample_frames);
    free(spc);

    memcpy(audio->setup_sfx_wav, "RIFF", 4);
    audio_put_u32(audio->setup_sfx_wav + 4, 36u + (uint32_t)pcm_bytes);
    memcpy(audio->setup_sfx_wav + 8, "WAVEfmt ", 8);
    audio_put_u32(audio->setup_sfx_wav + 16, 16u);
    audio_put_u16(audio->setup_sfx_wav + 20, 1u);
    audio_put_u16(audio->setup_sfx_wav + 22, 2u);
    audio_put_u32(audio->setup_sfx_wav + 24, NBA_SPC_SAMPLE_RATE);
    audio_put_u32(audio->setup_sfx_wav + 28, NBA_SPC_SAMPLE_RATE * 4u);
    audio_put_u16(audio->setup_sfx_wav + 32, 4u);
    audio_put_u16(audio->setup_sfx_wav + 34, 16u);
    memcpy(audio->setup_sfx_wav + 36, "data", 4);
    audio_put_u32(audio->setup_sfx_wav + 40, (uint32_t)pcm_bytes);
    audio->setup_sfx_wav_length = wav_size;
    audio->last_setup_sfx_srcn = srcn;

    int peak = 0;
    for (uint32_t i = 0; i < sample_frames * 2u; ++i) {
        int magnitude = pcm[i] < 0 ? -(int)pcm[i] : (int)pcm[i];
        if (magnitude > peak) peak = magnitude;
    }
    if (peak_out) *peak_out = peak;
#if defined(_WIN32)
    if (audio->host_playback_enabled)
        PlaySoundA((LPCSTR)audio->setup_sfx_wav, NULL,
                   SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#endif
    return peak != 0;
}

/**
 * Offset/Address/Size: $80:9DF3 | menu SFX dispatch | commands $49-$4B
 * Subroutines: $80:A9E3/$80:AACD (APU queue), SPC SRCN $1A-$1C
 * Purpose: Keys the packed Setup ARAM/DIR source through the captured S-DSP
 *          voice registers without stopping Setup's independent music stream.
 */
void nba_audio_play_setup_sfx(NbaAudio *audio, const NbaAssetPack *assets,
                              uint8_t srcn) {
    if (!audio || !assets || srcn >= 30u) return;
    /* Hardware writes captured at the three $80:9DF3 commands.  The F11 WAVs
     * deliberately expose undecorated BRR sources; menu playback is different:
     * the resident driver keys voice 1 with these pitch/envelope registers. */
    uint16_t pitch = srcn == 0x1Au ? 0x05A8u :
                     srcn == 0x1Bu ? 0x050Au : 0x03C6u;
    const uint32_t sample_frames = 24000u; /* 0.75 s includes DSP release tail */
    /* The settled default capture writes $40 at the game's 30/45 setting.
     * Apply the option at the DSP voice, as $87:8C2D does, instead of
     * attenuating an already-rendered host PCM buffer. */
    uint32_t voice_volume = ((uint32_t)audio->setup_sfx_volume * 0x40u) / 30u;
    if (voice_volume > 0x7Fu) voice_volume = 0x7Fu;
    int peak = 0;
    if (!audio_play_spc_sfx(audio, assets, NBA_ASSET_SETUP_SPC_RAM,
            NBA_ASSET_SETUP_SPC_DSP, NBA_ASSET_SETUP_SPC_STATE,
            "NBTSSPC1", srcn, 1u, pitch, (uint8_t)voice_volume,
            (uint8_t)voice_volume, 0x8Eu, 0xE0u, 0u,
            sample_frames, &peak)) return;
    printf("[SETUP] DSP menu SFX SRCN $%02X pitch=$%04X ADSR1/2=$8E/$E0, "
           "volume=%u/45 DSPVOL=$%02X peak=%d.\n", srcn, pitch,
           audio->setup_sfx_volume, (unsigned)voice_volume, peak);
}

/* Exact `$80:8930` recurrence, kept on an audio-only state word. The ROM uses
 * the shared `$07F6` word; isolating the same recurrence preserves selection
 * without allowing host audio scheduling to perturb reconstructed gameplay. */
static uint16_t audio_gameplay_rng_next(uint16_t *state) {
    uint16_t old = *state;
    if (old == 0u) {
        *state = 0x9146u;
    } else {
        *state = (uint16_t)(old << 1u);
        if (old & 0x8000u) *state ^= 0x1D87u;
    }
    return *state;
}

/* Command-to-DSP vectors observed at `$80:9DF3` in the controlled Mesen
 * sweep. The three pitch deltas are native `$32` steps, not host tuning. */
static bool audio_gameplay_resolve(uint8_t command, NbaGameplayCommand *out) {
    NbaGameplayCommand value = {command, 0u, 0u, 0};
    switch (command) {
        case 0x08: value.srcn=0x00; value.pitch=0x0556; value.volume=0x56; break;
        case 0x10: value.srcn=0x00; value.pitch=0x0524; value.volume=0x56; break;
        case 0x18: value.srcn=0x00; value.pitch=0x0588; value.volume=0x56; break;
        case 0x09: value.srcn=0x01; value.pitch=0x0556; value.volume=0x28; break;
        case 0x11: value.srcn=0x01; value.pitch=0x0524; value.volume=0x28; break;
        case 0x19: value.srcn=0x01; value.pitch=0x0588; value.volume=0x28; break;
        case 0x0A: value.srcn=0x02; value.pitch=0x0556; value.volume=0x1D; break;
        case 0x12: value.srcn=0x02; value.pitch=0x0524; value.volume=0x1D; break;
        case 0x1A: value.srcn=0x02; value.pitch=0x0588; value.volume=0x1D; break;
        case 0x0B: value.srcn=0x03; value.pitch=0x0556; value.volume=0x2F; break;
        case 0x13: value.srcn=0x03; value.pitch=0x0524; value.volume=0x2F; break;
        case 0x1B: value.srcn=0x03; value.pitch=0x0588; value.volume=0x2F; break;
        case 0x0C: value.srcn=0x04; value.pitch=0x0800; value.volume=0x1A; break;
        case 0x14: value.srcn=0x04; value.pitch=0x07CE; value.volume=0x1A; break;
        case 0x1C: value.srcn=0x04; value.pitch=0x0832; value.volume=0x1A; break;
        case 0x0D: value.srcn=0x05; value.pitch=0x0556; value.volume=0x3D; break;
        case 0x15: value.srcn=0x05; value.pitch=0x0524; value.volume=0x3D; break;
        case 0x1D: value.srcn=0x05; value.pitch=0x0588; value.volume=0x3D; break;
        case 0x0E: value.srcn=0x06; value.pitch=0x0659; value.volume=0x24; break;
        case 0x1E: value.srcn=0x06; value.pitch=0x068B; value.volume=0x24; break;
        case 0x20: value.srcn=0x07; value.pitch=0x0400; value.volume=0x39; break;
        case 0x28: value.srcn=0x07; value.pitch=0x03CE; value.volume=0x39; break;
        case 0x30: value.srcn=0x07; value.pitch=0x0432; value.volume=0x39; break;
        case 0x21: value.srcn=0x08; value.pitch=0x0400; value.volume=0x21; break;
        case 0x29: value.srcn=0x08; value.pitch=0x03CE; value.volume=0x21; break;
        case 0x31: value.srcn=0x08; value.pitch=0x0432; value.volume=0x21; break;
        case 0x22: value.srcn=0x09; value.pitch=0x0400; value.volume=0x21; break;
        case 0x2A: value.srcn=0x09; value.pitch=0x03CE; value.volume=0x21; break;
        case 0x32: value.srcn=0x09; value.pitch=0x0432; value.volume=0x21; break;
        case 0x23: value.srcn=0x0A; value.pitch=0x0800; value.volume=0x0B; break;
        case 0x2B: value.srcn=0x0A; value.pitch=0x07CE; value.volume=0x0B; break;
        case 0x33: value.srcn=0x0A; value.pitch=0x0832; value.volume=0x0B; break;
        case 0x24: value.srcn=0x0B; value.pitch=0x0659; value.volume=0x24; break;
        case 0x2C: value.srcn=0x0B; value.pitch=0x0627; value.volume=0x24; break;
        case 0x34: value.srcn=0x0B; value.pitch=0x068B; value.volume=0x24; break;
        default: return false;
    }
    *out = value;
    return true;
}

static void audio_gameplay_command(NbaAudio *audio, uint8_t command,
                                   uint8_t srcn, uint16_t pitch,
                                   int16_t volume) {
    NbaGameplayMixer *mixer = audio ?
        (NbaGameplayMixer *)audio->gameplay_mixer : NULL;
    if (!mixer || srcn >= 30u || !mixer->samples[srcn].pcm) return;
#if defined(_WIN32)
    if (mixer->thread) EnterCriticalSection(&mixer->lock);
#endif
    uint8_t voice = (uint8_t)(2u + (mixer->next_effect_voice++ % 6u));
    audio_gameplay_key_voice(mixer, voice, srcn, pitch, volume, volume, false);
#if defined(_WIN32)
    if (mixer->thread) LeaveCriticalSection(&mixer->lock);
#endif
    audio->last_gameplay_command = command;
    audio->last_gameplay_srcn = srcn;
    audio->last_gameplay_pitch = pitch;
    audio->last_gameplay_volume = volume;
    ++audio->gameplay_event_count;
    printf("[GAMEPLAY AUDIO] $80:9DF3 command=$%02X SRCN=$%02X "
           "pitch=$%04X VOL=$%02X voice=%u event=%u.\n", command, srcn,
           pitch, (unsigned)(uint16_t)volume, voice,
           audio->gameplay_event_count);
}

static void audio_gameplay_random_family(NbaAudio *audio,
                                         const uint8_t *table,
                                         uint16_t mask) {
    NbaGameplayCommand resolved;
    uint16_t choice = audio_gameplay_rng_next(
        &audio->gameplay_audio_rng_state) & mask;
    if (audio_gameplay_resolve(table[choice], &resolved)) {
        audio_gameplay_command(audio, resolved.command, resolved.srcn,
                               resolved.pitch, resolved.volume);
    }
}

bool nba_audio_start_gameplay(NbaAudio *audio, const NbaAssetPack *assets) {
    if (!audio || !assets) return false;
    nba_audio_stop(audio);
    NbaGameplayMixer *mixer = (NbaGameplayMixer *)calloc(1, sizeof(*mixer));
    if (!mixer || !audio_load_gameplay_bank(mixer, assets)) {
        free(mixer);
        audio->status = NBA_AUDIO_STATUS_SYNTH_FAILED;
        return false;
    }
    /* Native steady-state voices 6/7: SRCN $0C/$0D at $0436/$04B6.
     * Host voices 0/1 preserve that independent continuous crowd bed. */
    audio_gameplay_key_voice(mixer, 0u, 0x0Cu, 0x0436u, 26, 2, true);
    audio_gameplay_key_voice(mixer, 1u, 0x0Du, 0x04B6u, 2, 24, true);
    audio->gameplay_mixer = mixer;
    audio->active_track = NBA_AUDIO_TRACK_WAV;
    audio->status = NBA_AUDIO_STATUS_READY;
    audio->last_gameplay_command = 0xFFu;
    audio->last_gameplay_srcn = 0xFFu;
    audio->last_gameplay_pitch = 0u;
    audio->last_gameplay_volume = 0;
    audio->gameplay_event_count = 0u;
    audio->gameplay_audio_rng_state = 0x9146u;
    audio->gameplay_latched_13e7 = 0u;
    audio->gameplay_latched_13e9 = 0u;
#if defined(_WIN32)
    if (audio->host_playback_enabled) {
        if (!audio_gameplay_start_host(mixer)) {
            audio->status = NBA_AUDIO_STATUS_HOST_FAILED;
            fprintf(stderr, "[GAMEPLAY AUDIO] waveOut mixer unavailable; "
                    "event synthesis remains testable.\n");
            return true;
        }
        audio->status = NBA_AUDIO_STATUS_PLAYING;
    }
#endif
    printf("[GAMEPLAY AUDIO] ROM BRR bank online; crowd SRCN $0C/$0D "
           "and six overlapping effect voices ready.\n");
    return true;
}

/* `$82:FD65-$FF84` consumes `$13E7/$13E9` after gameplay producers run.
 * It owns sound selection only: the audio mixer deliberately has an RNG
 * stream independent of CPU/ball state so host playback cannot alter play. */
void nba_audio_dispatch_gameplay_events(NbaAudio *audio,
                                        uint16_t event_bits_raw_13e7,
                                        uint16_t crowd_bits_raw_13e9) {
    if (!audio || !audio->gameplay_mixer) return;
    uint16_t fresh_13e7 = (uint16_t)(event_bits_raw_13e7 &
                                    ~audio->gameplay_latched_13e7);
    uint16_t fresh_13e9 = (uint16_t)(crowd_bits_raw_13e9 &
                                    ~audio->gameplay_latched_13e9);
    audio->gameplay_latched_13e7 = event_bits_raw_13e7;
    audio->gameplay_latched_13e9 = crowd_bits_raw_13e9;
    event_bits_raw_13e7 = fresh_13e7;
    crowd_bits_raw_13e9 = fresh_13e9;
    if (event_bits_raw_13e7 & 0x0001u)
        audio_gameplay_random_family(audio, AUDIO_BOUNCE, 0x0003u);
    if (event_bits_raw_13e7 & 0x0002u)
        audio_gameplay_random_family(audio, AUDIO_INNER_RIM, 0x000Fu);
    if (event_bits_raw_13e7 & 0x0004u)
        audio_gameplay_random_family(audio, AUDIO_MADE, 0x0003u);
    if (event_bits_raw_13e7 & 0x0008u)
        audio_gameplay_random_family(audio, AUDIO_OUTER_RIM, 0x0003u);
    if (event_bits_raw_13e7 & 0x0010u)
        audio_gameplay_random_family(audio, AUDIO_CATCH, 0x0003u);
    if (event_bits_raw_13e7 & 0x0020u)
        audio_gameplay_random_family(audio, AUDIO_CONTACT, 0x0003u);
    if (event_bits_raw_13e7 & 0x0040u)
        audio_gameplay_random_family(audio, AUDIO_SHOE, 0x0003u);
    if (event_bits_raw_13e7 & 0x0080u)
        audio_gameplay_random_family(audio, AUDIO_COLLISION, 0x0007u);
    if (event_bits_raw_13e7 & 0x0100u)
        audio_gameplay_random_family(audio, AUDIO_LANDING, 0x0003u);
    /* Both bits 7 and 9 intentionally use `$82:F872`; bit 9 was omitted by
     * the first port pass and is independently consumed by the ROM. */
    if (event_bits_raw_13e7 & 0x0200u)
        audio_gameplay_random_family(audio, AUDIO_COLLISION, 0x0007u);
    if (event_bits_raw_13e7 & 0x0400u)
        audio_gameplay_command(audio, 0x40u, 0x10u, 0x0556u, 40);
    if (event_bits_raw_13e7 & 0x0800u)
        audio_gameplay_command(audio, 0x41u, 0x11u, 0x0556u, 44);
    if (event_bits_raw_13e7 & 0x1000u)
        audio_gameplay_command(audio, 0x43u, 0x13u, 0x0556u, 44);
    if (event_bits_raw_13e7 & 0x2000u)
        audio_gameplay_command(audio, 0x44u, 0x12u, 0x0556u, 20);
    if (crowd_bits_raw_13e9) {
        uint8_t srcn = (crowd_bits_raw_13e9 & 0x0002u) ? 0x15u : 0x14u;
        uint8_t command = (crowd_bits_raw_13e9 & 0x0001u) ? 0x38u : 0x39u;
        audio_gameplay_command(audio, command, srcn, 0x0556u, 28);
    }
}

bool nba_audio_play_gameplay_whistle(NbaAudio *audio,
                                     const NbaAssetPack *assets) {
    if (!audio || !assets) return false;
    if (!audio->gameplay_mixer && !nba_audio_start_gameplay(audio, assets))
        return false;
    /* Keep the CLI/F11 proof artifact, now rendered from NBGAUD1 instead of
     * the incorrect Player Introduction SPC snapshot. */
    const uint32_t frames = 24000u;
    const size_t pcm_bytes = (size_t)frames * 4u;
    const size_t wav_size = 44u + pcm_bytes;
    if (audio->setup_sfx_wav_size < wav_size) {
        uint8_t *grown = (uint8_t *)realloc(audio->setup_sfx_wav, wav_size);
        if (!grown) return false;
        audio->setup_sfx_wav = grown;
        audio->setup_sfx_wav_size = wav_size;
    }
    NbaGameplayMixer preview;
    memset(&preview, 0, sizeof(preview));
    if (!audio_load_gameplay_bank(&preview, assets)) return false;
    audio_gameplay_key_voice(&preview, 4u, 0x12u, 0x0556u, 20, 20, false);
    audio_gameplay_mix(&preview, (int16_t *)(audio->setup_sfx_wav + 44), frames);
    memcpy(audio->setup_sfx_wav, "RIFF", 4);
    audio_put_u32(audio->setup_sfx_wav + 4, 36u + (uint32_t)pcm_bytes);
    memcpy(audio->setup_sfx_wav + 8, "WAVEfmt ", 8);
    audio_put_u32(audio->setup_sfx_wav + 16, 16u);
    audio_put_u16(audio->setup_sfx_wav + 20, 1u);
    audio_put_u16(audio->setup_sfx_wav + 22, 2u);
    audio_put_u32(audio->setup_sfx_wav + 24, NBA_SPC_SAMPLE_RATE);
    audio_put_u32(audio->setup_sfx_wav + 28, NBA_SPC_SAMPLE_RATE * 4u);
    audio_put_u16(audio->setup_sfx_wav + 32, 4u);
    audio_put_u16(audio->setup_sfx_wav + 34, 16u);
    memcpy(audio->setup_sfx_wav + 36, "data", 4);
    audio_put_u32(audio->setup_sfx_wav + 40, (uint32_t)pcm_bytes);
    audio->setup_sfx_wav_length = wav_size;
    audio->last_setup_sfx_srcn = 0x12u;
    audio_gameplay_command(audio, 0x44u, 0x12u, 0x0556u, 20);
    int peak = 0;
    const int16_t *pcm = (const int16_t *)(audio->setup_sfx_wav + 44);
    for (uint32_t i = 0; i < frames * 2u; ++i) {
        int value = pcm[i] < 0 ? -(int)pcm[i] : (int)pcm[i];
        if (value > peak) peak = value;
    }
    printf("[GAMEPLAY] command $44 SRCN $12 pitch=$0556 voice=4 "
           "ADSR1/2=$8E/$A0 VOL=$14/$14 peak=%d.\n", peak);
    return true;
}

bool nba_audio_gameplay_self_test(const NbaAssetPack *assets) {
    NbaGameplayMixer mixer;
    int16_t output[256 * 2];
    memset(&mixer, 0, sizeof(mixer));
    if (!assets || !audio_load_gameplay_bank(&mixer, assets)) return false;
    audio_gameplay_key_voice(&mixer, 0u, 0x0Cu, 0x0436u, 26, 2, true);
    audio_gameplay_key_voice(&mixer, 1u, 0x0Du, 0x04B6u, 2, 24, true);
    audio_gameplay_key_voice(&mixer, 2u, 0x0Au, 0x0800u, 42, 42, false);
    audio_gameplay_key_voice(&mixer, 3u, 0x12u, 0x0556u, 20, 20, false);
    audio_gameplay_mix(&mixer, output, 256u);
    int peak = 0;
    for (uint32_t i = 0; i < 512u; ++i) {
        int value = output[i] < 0 ? -(int)output[i] : (int)output[i];
        if (value > peak) peak = value;
    }
    return peak > 0 && mixer.voices[0].active && mixer.voices[1].active &&
           mixer.voices[2].active && mixer.voices[3].active;
}

bool nba_audio_save_setup_sfx_wav(const NbaAudio *audio, const char *path) {
    if (!audio || !path || !audio->setup_sfx_wav ||
        audio->setup_sfx_wav_length == 0u) return false;
    FILE *file = fopen(path, "wb");
    if (!file) return false;
    bool ok = fwrite(audio->setup_sfx_wav, 1, audio->setup_sfx_wav_length, file) ==
              audio->setup_sfx_wav_length;
    fclose(file);
    return ok;
}

void nba_audio_set_setup_sfx_volume(NbaAudio *audio, uint16_t value,
                                    uint16_t maximum) {
    if (!audio || maximum == 0u) return;
    audio->setup_sfx_volume = value > maximum ? maximum : value;
}

void nba_audio_set_setup_music_volume(NbaAudio *audio, uint16_t value,
                                      uint16_t maximum) {
    if (!audio || maximum == 0u) return;
    audio->setup_music_volume = value > maximum ? maximum : value;
    if (!audio->loop_playback) return;
#if defined(_WIN32)
    NbaWaveLoop *stream = (NbaWaveLoop *)audio->loop_playback;
    uint32_t channel = ((uint32_t)audio->setup_music_volume * 0xFFFFu) / maximum;
    waveOutSetVolume(stream->device, channel | (channel << 16));
#else
    (void)value;
#endif
}

/**
 * Offset/Address/Size: $82:AC0E -> $82:ABE0 -> $80:9829 | variable
 * The surrounding `$82:A900-$ADFF` post-EA resource family selects and
 * publishes the title sequence/instrument/BRR lists. The host consumes those
 * exact stamped asset-pack streams rather than a prerecorded song.
 * Subroutines: $80:987B (APU payload), $80:9CC8/$80:9D80 (sequence select)
 * Purpose: Runs NBA Live '95's resident SPC700 driver against its original
 *          SPC RAM/BRR bank and the frame-stamped $2140-$2143 command stream.
 *          The resulting PCM is synthesized by src/nba_spc.c at runtime; no
 *          mixed title WAV is stored in the asset pack.
 */
bool nba_audio_play_title_spc(NbaAudio *audio, const NbaAssetPack *assets) {
    static const NbaSpcTrackSpec spec = {
        NBA_ASSET_TITLE_SPC_RAM, NBA_ASSET_TITLE_SPC_DSP,
        NBA_ASSET_TITLE_SPC_STATE, NBA_ASSET_TITLE_APU_TRACE,
        "NBTSPC1\0", "NBTAPU1\0", "Title", 3600, 4
    };
    if (!audio || !assets) return false;
    NbaSpcTrackRender render;
    if (!audio_prepare_spc_track(assets, &spec, &render)) return false;

    uint32_t event_index = 0;
    uint32_t sample_offset = 0;
    for (uint32_t frame = 0; frame < render.frame_count; ++frame) {
        uint32_t frame_end = (uint32_t)(((uint64_t)(frame + 1u) * NBA_SPC_SAMPLE_RATE) / 60u);
        int frame_samples = (int)(frame_end - sample_offset);
        uint32_t first = event_index;
        while (event_index < render.event_count) {
            const uint8_t *event = render.trace + 20u + event_index * 4u;
            if (audio_u16(event) != frame) break;
            ++event_index;
        }
        uint32_t count = event_index - first;
        int rendered = 0;
        for (uint32_t i = 0; i < count; ++i) {
            const uint8_t *event = render.trace + 20u + (first + i) * 4u;
            nba_spc_write_port(render.spc, event[2], event[3]);
            int target = (int)(((uint64_t)(i + 1u) * (uint32_t)frame_samples) / count);
            int chunk = target - rendered;
            if (chunk > 0) {
                nba_spc_render(render.spc,
                    render.pcm + (sample_offset + (uint32_t)rendered) * 2u, chunk);
                rendered += chunk;
            }
        }
        if (rendered < frame_samples) {
            nba_spc_render(render.spc,
                           render.pcm + (sample_offset + (uint32_t)rendered) * 2u,
                           frame_samples - rendered);
        }
        sample_offset = frame_end;
    }
    uint32_t frame_count = render.frame_count;
    uint32_t event_count = render.event_count;
    if (!audio_play_generated(audio, &render, false,
                              NBA_AUDIO_TRACK_TITLE_SPC)) return false;
    printf("[AUDIO] Synthesized title through SPC700/S-DSP: %u frames, %u APU writes.\n",
           frame_count, event_count);
    return true;
}

/**
 * Offset/Address/Size: $80:E600 -> $80:A2BF/$80:A3B8 | 150-second trace
 * Subroutines: $80:A9E3 (command), $80:AA7B (handshake), $80:AACD (queue)
 * Purpose: $80:A9E3/$80:AA7B/$80:AACD produce the captured $2140-$2143
 *          command stream. The resident SPC700 driver first streams Setup's
 *          30-source BRR bank into ARAM, then emits the captured $F2/$F3
 *          program through more than two observed 62.34-second musical
 *          periods. Asset 88 is that bank at the first KON, so replay keeps
 *          the source/pitch/envelope choices while C decodes BRR live. The
 *          asset pack contains no mixed Setup PCM. The synthesized host
 *          buffer loops continuously while the Setup screen remains active.
 */
bool nba_audio_play_setup_dsp(NbaAudio *audio, const NbaAssetPack *assets) {
    static const NbaSpcTrackSpec spec = {
        NBA_ASSET_SETUP_SPC_RAM, NBA_ASSET_SETUP_SPC_DSP,
        NBA_ASSET_SETUP_SPC_STATE, NBA_ASSET_SETUP_DSP_TRACE,
        "NBTSSPC1", "NBTSDSP1", "Game Setup", 18000, 6
    };
    if (!audio || !assets) return false;
    NbaSpcTrackRender render;
    if (!audio_prepare_spc_track(assets, &spec, &render)) return false;

    uint32_t rendered = 0;
    uint32_t previous_cycle = 0;
    for (uint32_t i = 0; i < render.event_count; ++i) {
        const uint8_t *event = render.trace + 20u + i * 6u;
        uint32_t cycle = audio_u32(event);
        if (cycle < previous_cycle || event[4] >= NBA_SPC_DSP_REGS) {
            audio_discard_render(&render);
            fprintf(stderr, "[AUDIO] Invalid Game Setup DSP cycle event.\n");
            return false;
        }
        previous_cycle = cycle;
        uint32_t target = cycle / NBA_SPC_CYCLES_PER_SAMPLE;
        if (target > render.sample_count) target = render.sample_count;
        if (target > rendered) {
            nba_spc_render_dsp(render.spc, render.pcm + rendered * 2u,
                               (int)(target - rendered));
            rendered = target;
        }
        nba_spc_write_dsp(render.spc, event[4], event[5]);
    }
    if (rendered < render.sample_count) {
        nba_spc_render_dsp(render.spc, render.pcm + rendered * 2u,
                           (int)(render.sample_count - rendered));
    }

    int peak = 0;
    for (uint32_t i = 0; i < render.sample_count * 2u; ++i) {
        int magnitude = render.pcm[i] < 0 ? -(int)render.pcm[i] :
                                           (int)render.pcm[i];
        if (magnitude > peak) peak = magnitude;
    }
    if (peak == 0) {
        audio_discard_render(&render);
        fprintf(stderr, "[AUDIO] Game Setup SPC synthesis produced silence.\n");
        return false;
    }

    uint32_t frame_count = render.frame_count;
    uint32_t event_count = render.event_count;
    if (!audio_play_generated(audio, &render, true,
                              NBA_AUDIO_TRACK_SETUP_SPC)) return false;
    printf("[AUDIO] Synthesized Game Setup through ROM BRR/S-DSP: "
           "%u frames, %u cycle-timed DSP writes, peak=%d; "
           "seamless host loop %u..%u enabled.\n",
           frame_count, event_count, peak,
           NBA_SETUP_LOOP_START_SAMPLE, NBA_SETUP_LOOP_END_SAMPLE);
    return true;
}

/**
 * Offset/Address/Size: $80:9829 -> $80:A9E3/$80:AACD | Player Introduction
 * Subroutines: $80:9829 ($98CD transfer loop; ARAM/BRR upload),
 *              $80:A9E3 (command $0BFC),
 *              $80:AACD (APU queue/update)
 * Purpose: Replays the presentation's independent ROM sample bank and exact
 *          cycle-timed S-DSP program.  This is synthesized from BRR at runtime;
 *          asset 268 contains register control data, not mixed PCM.
 */
bool nba_audio_play_player_intro_dsp(NbaAudio *audio,
                                     const NbaAssetPack *assets) {
    static const NbaSpcTrackSpec spec = {
        NBA_ASSET_PLAYER_INTRO_SPC_RAM, NBA_ASSET_PLAYER_INTRO_SPC_DSP,
        NBA_ASSET_PLAYER_INTRO_SPC_STATE, NBA_ASSET_PLAYER_INTRO_DSP_TRACE,
        "NBPISPC1", "NBPIDSP1", "Player Introduction", 7200, 6
    };
    if (!audio || !assets) return false;
    NbaSpcTrackRender render;
    if (!audio_prepare_spc_track(assets, &spec, &render)) return false;

    uint32_t rendered = 0;
    uint32_t previous_cycle = 0;
    for (uint32_t i = 0; i < render.event_count; ++i) {
        const uint8_t *event = render.trace + 20u + i * 6u;
        uint32_t cycle = audio_u32(event);
        if (cycle < previous_cycle || event[4] >= NBA_SPC_DSP_REGS) {
            audio_discard_render(&render);
            fprintf(stderr, "[AUDIO] Invalid Player Introduction DSP cycle event.\n");
            return false;
        }
        previous_cycle = cycle;
        uint32_t target = cycle / NBA_SPC_CYCLES_PER_SAMPLE;
        if (target > render.sample_count) target = render.sample_count;
        if (target > rendered) {
            nba_spc_render_dsp(render.spc, render.pcm + rendered * 2u,
                               (int)(target - rendered));
            rendered = target;
        }
        nba_spc_write_dsp(render.spc, event[4], event[5]);
    }
    if (rendered < render.sample_count) {
        nba_spc_render_dsp(render.spc, render.pcm + rendered * 2u,
                           (int)(render.sample_count - rendered));
    }

    int peak = 0;
    for (uint32_t i = 0; i < render.sample_count * 2u; ++i) {
        int magnitude = render.pcm[i] < 0 ? -(int)render.pcm[i] :
                                           (int)render.pcm[i];
        if (magnitude > peak) peak = magnitude;
    }
    if (peak == 0) {
        audio_discard_render(&render);
        fprintf(stderr, "[AUDIO] Player Introduction SPC synthesis produced silence.\n");
        return false;
    }

    uint32_t frame_count = render.frame_count;
    uint32_t event_count = render.event_count;
    if (!audio_play_generated(audio, &render, false,
                              NBA_AUDIO_TRACK_PLAYER_INTRO_SPC)) return false;
    printf("[AUDIO] Synthesized Player Introduction through ROM BRR/S-DSP: "
           "%u frames, %u cycle-timed DSP writes, peak=%d.\n",
           frame_count, event_count, peak);
    return true;
}

bool nba_audio_save_generated_wav(const NbaAudio *audio, const char *path) {
    if (!audio || !path || !audio->generated_wav ||
        audio->generated_wav_size < 44) return false;
    FILE *file = fopen(path, "wb");
    if (!file) return false;
    bool ok = fwrite(audio->generated_wav, 1, audio->generated_wav_size, file) ==
              audio->generated_wav_size;
    fclose(file);
    return ok;
}

/**
 * Offset/Address/Size: N/A | Host Sound Stop | size: N/A
 * Purpose: Immediately terminates any running background audio playback.
 */
void nba_audio_stop(NbaAudio *audio) {
    if (!audio) return;
    NbaGameplayMixer *gameplay =
        (NbaGameplayMixer *)audio->gameplay_mixer;
#if defined(_WIN32)
    if (gameplay && gameplay->thread) audio_gameplay_stop_host(gameplay);
#endif
    free(gameplay);
    audio->gameplay_mixer = NULL;
#if defined(_WIN32)
    if (audio->host_playback_enabled) PlaySoundA(NULL, NULL, 0);
    NbaWaveLoop *stream = (NbaWaveLoop *)audio->loop_playback;
    if (stream) {
        waveOutReset(stream->device);
        waveOutUnprepareHeader(stream->device, &stream->intro, sizeof(stream->intro));
        waveOutUnprepareHeader(stream->device, &stream->loop, sizeof(stream->loop));
        waveOutClose(stream->device);
        free(stream);
        audio->loop_playback = NULL;
    }
#endif
    free(audio->generated_wav);
    audio->generated_wav = NULL;
    audio->generated_wav_size = 0;
    audio->active_track = NBA_AUDIO_TRACK_NONE;
    audio->status = NBA_AUDIO_STATUS_IDLE;
}
