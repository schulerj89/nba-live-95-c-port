#ifndef NBA_AUDIO_EVENTS_H
#define NBA_AUDIO_EVENTS_H

#include <stdint.h>

/* $82:FD65-$FF84 input/output words. The caller owns their actual gameplay
 * storage. Unknown high bits remain pending, exactly as in the ROM. */
typedef struct {
    uint16_t event_bits_13e7;
    uint16_t crowd_bits_13e9;
    uint16_t bounce_strength_13e5;
    uint16_t crowd_enabled_17bb;
} NbaAudioEventState;

typedef enum {
    NBA_AUDIO_EVENT_COMMAND = 0,       /* $80:9DF3: A=command; returns A */
    NBA_AUDIO_EVENT_VOICE_VOLUME = 1,  /* $80:A82F: X=voice, Y=volume */
    NBA_AUDIO_EVENT_CROWD_QUEUE = 2    /* $80:9F0F: A=command, X=index, Y=delay */
} NbaAudioEventKind;

typedef struct {
    NbaAudioEventKind kind;
    uint32_t caller_pc;
    uint32_t target_pc;
    uint16_t command;
    uint16_t index;
    uint16_t value;
} NbaAudioEventOperation;

/* A command sink returns the native driver A result, whose low byte selects
 * the bounce voice for the following volume operation. Other operation return
 * values are ignored. Disabled host playback must not suppress this logical
 * dispatch or RNG consumption. A NULL sink discards playback operations only.
 * The sink must not mutate this routine's gameplay/event/RNG state. */
typedef uint16_t (*NbaAudioEventSink)(void *context,
                                    const NbaAudioEventOperation *operation);

/* Translate one actual native dispatch; this API does not invent its cadence.
 * shared_rng_07f6 MUST address the same word used by AI/physics/presentation,
 * not a separately seeded audio stream. NMI scheduling and the resident audio
 * driver's allocation/sequencing remain caller/downstream responsibilities. */
void nba_audio_events_dispatch(NbaAudioEventState *state,
                               uint16_t *shared_rng_07f6,
                               NbaAudioEventSink sink, void *context);

#endif
