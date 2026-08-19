#include "nba_audio.h"
#include <stdio.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

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

#if defined(_WIN32)
    PlaySoundA((LPCSTR)data, NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#else
    (void)data;
    (void)size;
#endif
}

/**
 * Offset/Address/Size: N/A | Host Sound Stop | size: N/A
 * Purpose: Immediately terminates any running background audio playback.
 */
void nba_audio_stop(void) {
#if defined(_WIN32)
    PlaySoundA(NULL, NULL, 0);
#endif
}
