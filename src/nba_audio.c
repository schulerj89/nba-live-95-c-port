#include "nba_audio.h"
#include <stdio.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

void nba_audio_init(void) {
    printf("[AUDIO] Initializing Audio Subsystem...\n");
}

void nba_audio_shutdown(void) {
    nba_audio_stop();
    printf("[AUDIO] Audio Subsystem shutdown.\n");
}

void nba_audio_play_wav(const void *data, size_t size) {
    if (!data || size == 0) return;

#if defined(_WIN32)
    PlaySoundA((LPCSTR)data, NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#else
    (void)data;
    (void)size;
#endif
}

void nba_audio_stop(void) {
#if defined(_WIN32)
    PlaySoundA(NULL, NULL, 0);
#endif
}
