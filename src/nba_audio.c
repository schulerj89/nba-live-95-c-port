#include "nba_audio.h"
#include "nba_spc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

static uint16_t audio_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t audio_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

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

static bool audio_play_generated(NbaAudio *audio, NbaSpcTrackRender *render) {
    free(render->spc);
    render->spc = NULL;
    nba_audio_stop(audio);
    audio->generated_wav = render->wav;
    audio->generated_wav_size = render->wav_size;
    render->wav = NULL;
#if defined(_WIN32)
    if (!PlaySoundA((LPCSTR)audio->generated_wav, NULL,
                    SND_MEMORY | SND_ASYNC | SND_NODEFAULT)) {
        nba_audio_stop(audio);
        return false;
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
    printf("[AUDIO] Initializing Audio Subsystem...\n");
}

/**
 * Offset/Address/Size: N/A | Host Audio Shutdown | size: N/A
 * Purpose: Stops any active asynchronous audio stream and releases audio resources.
 */
void nba_audio_shutdown(NbaAudio *audio) {
    nba_audio_stop(audio);
    printf("[AUDIO] Audio Subsystem shutdown.\n");
}

/**
 * Offset/Address/Size: 0x002C89 | $80:AC89 | size: 0x30
 * Purpose: Plays a PCM/WAV sound buffer asynchronously (corresponds to SNES APU port $2140 dispatch).
 */
void nba_audio_play_wav(NbaAudio *audio, const void *data, size_t size) {
    if (!audio || !data || size == 0) return;

    nba_audio_stop(audio);

#if defined(_WIN32)
    PlaySoundA((LPCSTR)data, NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#else
    (void)data;
    (void)size;
#endif
}

/**
 * Offset/Address/Size: $82:AC0E -> $82:ABE0 -> $80:9829 | variable
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
    if (!audio_play_generated(audio, &render)) return false;
    printf("[AUDIO] Synthesized title through SPC700/S-DSP: %u frames, %u APU writes.\n",
           frame_count, event_count);
    return true;
}

/**
 * Offset/Address/Size: $80:E600 -> $80:A2BF/$80:A3B8 | 30-second trace
 * Subroutines: $80:A9E3 (command), $80:AA7B (handshake), $80:AACD (queue)
 * Purpose: Resumes the ROM's SPC700 driver on the final title-fade frame and
 *          delivers the Game Setup CPU command stream at its captured SPC
 *          cycle offsets. The asset pack contains SPC/BRR hardware state and
 *          control writes, never a mixed Setup WAV.
 */
bool nba_audio_play_setup_spc(NbaAudio *audio, const NbaAssetPack *assets) {
    static const NbaSpcTrackSpec spec = {
        NBA_ASSET_SETUP_SPC_RAM, NBA_ASSET_SETUP_SPC_DSP,
        NBA_ASSET_SETUP_SPC_STATE, NBA_ASSET_SETUP_APU_TRACE,
        "NBTSSPC1", "NBTSAPU1", "Game Setup", 7200, 6
    };
    if (!audio || !assets) return false;
    NbaSpcTrackRender render;
    if (!audio_prepare_spc_track(assets, &spec, &render)) return false;

    uint32_t rendered = 0;
    uint32_t previous_cycle = 0;
    for (uint32_t i = 0; i < render.event_count; ++i) {
        const uint8_t *event = render.trace + 20u + i * 6u;
        uint32_t cycle = audio_u32(event);
        if (cycle < previous_cycle || event[4] > 3) {
            audio_discard_render(&render);
            fprintf(stderr, "[AUDIO] Invalid Game Setup cycle event.\n");
            return false;
        }
        previous_cycle = cycle;
        uint32_t target = cycle / NBA_SPC_CYCLES_PER_SAMPLE;
        if (target > render.sample_count) target = render.sample_count;
        if (target > rendered) {
            nba_spc_render(render.spc, render.pcm + rendered * 2u,
                           (int)(target - rendered));
            rendered = target;
        }
        nba_spc_write_port(render.spc, event[4], event[5]);
    }
    if (rendered < render.sample_count) {
        nba_spc_render(render.spc, render.pcm + rendered * 2u,
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
    if (!audio_play_generated(audio, &render)) return false;
    printf("[AUDIO] Synthesized Game Setup through SPC700/S-DSP: "
           "%u frames, %u cycle-timed APU writes, peak=%d.\n",
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
#if defined(_WIN32)
    PlaySoundA(NULL, NULL, 0);
#endif
    free(audio->generated_wav);
    audio->generated_wav = NULL;
    audio->generated_wav_size = 0;
}
