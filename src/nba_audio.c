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

static uint8_t *g_generated_wav;

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

/**
 * Offset/Address/Size: 0x002C1B | $80:AC1B | size: 0x24
 * Purpose: Initializes the host audio playback subsystem (corresponds to SNES APU sound channel init).
 */
void nba_audio_init(void) {
    printf("[AUDIO] Initializing Audio Subsystem...\n");
}

/**
 * Offset/Address/Size: N/A | Host Audio Shutdown | size: N/A
 * Purpose: Stops any active asynchronous audio stream and releases audio resources.
 */
void nba_audio_shutdown(void) {
    nba_audio_stop();
    printf("[AUDIO] Audio Subsystem shutdown.\n");
}

/**
 * Offset/Address/Size: 0x002C89 | $80:AC89 | size: 0x30
 * Purpose: Plays a PCM/WAV sound buffer asynchronously (corresponds to SNES APU port $2140 dispatch).
 */
void nba_audio_play_wav(const void *data, size_t size) {
    if (!data || size == 0) return;

    nba_audio_stop();

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
bool nba_audio_play_title_spc(const NbaAssetPack *assets) {
    const NbaAssetItem *ram_item = nba_assets_get(assets, NBA_ASSET_TITLE_SPC_RAM);
    const NbaAssetItem *dsp_item = nba_assets_get(assets, NBA_ASSET_TITLE_SPC_DSP);
    const NbaAssetItem *state_item = nba_assets_get(assets, NBA_ASSET_TITLE_SPC_STATE);
    const NbaAssetItem *trace_item = nba_assets_get(assets, NBA_ASSET_TITLE_APU_TRACE);
    if (!ram_item || ram_item->size < NBA_SPC_RAM_SIZE ||
        !dsp_item || dsp_item->size < NBA_SPC_DSP_REGS ||
        !state_item || state_item->size < 20 ||
        !trace_item || trace_item->size < 20) {
        fprintf(stderr, "[AUDIO] Title SPC assets are missing or truncated.\n");
        return false;
    }

    const uint8_t *state = (const uint8_t *)state_item->data;
    const uint8_t *trace = (const uint8_t *)trace_item->data;
    if (memcmp(state, "NBTSPC1\0", 8) != 0 || audio_u32(state + 8) != 1 ||
        memcmp(trace, "NBTAPU1\0", 8) != 0 || audio_u32(trace + 8) != 1) {
        fprintf(stderr, "[AUDIO] Unsupported title SPC asset format.\n");
        return false;
    }

    uint32_t frame_count = audio_u32(trace + 12);
    uint32_t event_count = audio_u32(trace + 16);
    if (frame_count == 0 || frame_count > 3600 ||
        event_count > (trace_item->size - 20u) / 4u) {
        fprintf(stderr, "[AUDIO] Invalid title APU trace dimensions.\n");
        return false;
    }

    NbaSpc *spc = (NbaSpc *)malloc(sizeof(*spc));
    if (!spc) return false;
    bool loaded = nba_spc_load(spc, ram_item->data, ram_item->size,
                               dsp_item->data, dsp_item->size,
                               audio_u16(state + 12), state[14], state[15],
                               state[16], state[17], state[18]);
    if (!loaded) {
        free(spc);
        return false;
    }

    uint32_t sample_count = (uint32_t)(((uint64_t)frame_count * NBA_SPC_SAMPLE_RATE) / 60u);
    size_t pcm_bytes = (size_t)sample_count * 2u * sizeof(int16_t);
    if (pcm_bytes > SIZE_MAX - 44u) {
        free(spc);
        return false;
    }
    uint8_t *wav = (uint8_t *)malloc(44u + pcm_bytes);
    if (!wav) {
        free(spc);
        return false;
    }
    memcpy(wav, "RIFF", 4); audio_put_u32(wav + 4, 36u + (uint32_t)pcm_bytes);
    memcpy(wav + 8, "WAVEfmt ", 8); audio_put_u32(wav + 16, 16);
    audio_put_u16(wav + 20, 1); audio_put_u16(wav + 22, 2);
    audio_put_u32(wav + 24, NBA_SPC_SAMPLE_RATE);
    audio_put_u32(wav + 28, NBA_SPC_SAMPLE_RATE * 4u);
    audio_put_u16(wav + 32, 4); audio_put_u16(wav + 34, 16);
    memcpy(wav + 36, "data", 4); audio_put_u32(wav + 40, (uint32_t)pcm_bytes);

    int16_t *pcm = (int16_t *)(wav + 44);
    uint32_t event_index = 0;
    uint32_t sample_offset = 0;
    for (uint32_t frame = 0; frame < frame_count; ++frame) {
        uint32_t frame_end = (uint32_t)(((uint64_t)(frame + 1u) * NBA_SPC_SAMPLE_RATE) / 60u);
        int frame_samples = (int)(frame_end - sample_offset);
        uint32_t first = event_index;
        while (event_index < event_count) {
            const uint8_t *event = trace + 20u + event_index * 4u;
            if (audio_u16(event) != frame) break;
            ++event_index;
        }
        uint32_t count = event_index - first;
        int rendered = 0;
        for (uint32_t i = 0; i < count; ++i) {
            const uint8_t *event = trace + 20u + (first + i) * 4u;
            nba_spc_write_port(spc, event[2], event[3]);
            int target = (int)(((uint64_t)(i + 1u) * (uint32_t)frame_samples) / count);
            int chunk = target - rendered;
            if (chunk > 0) {
                nba_spc_render(spc, pcm + (sample_offset + (uint32_t)rendered) * 2u, chunk);
                rendered += chunk;
            }
        }
        if (rendered < frame_samples) {
            nba_spc_render(spc, pcm + (sample_offset + (uint32_t)rendered) * 2u,
                           frame_samples - rendered);
        }
        sample_offset = frame_end;
    }
    free(spc);

    nba_audio_stop();
    g_generated_wav = wav;
#if defined(_WIN32)
    if (!PlaySoundA((LPCSTR)g_generated_wav, NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT)) {
        free(g_generated_wav); g_generated_wav = NULL;
        return false;
    }
#endif
    printf("[AUDIO] Synthesized title through SPC700/S-DSP: %u frames, %u APU writes.\n",
           frame_count, event_count);
    return true;
}

/**
 * Offset/Address/Size: N/A | Host Sound Stop | size: N/A
 * Purpose: Immediately terminates any running background audio playback.
 */
void nba_audio_stop(void) {
#if defined(_WIN32)
    PlaySoundA(NULL, NULL, 0);
#endif
    free(g_generated_wav);
    g_generated_wav = NULL;
}
