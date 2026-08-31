#include "nba_audio_events.h"

#include <stddef.h>

/* Original ROM $82:F822-$F889, independently byte-checked against SHA256
 * 2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870.
 * These indexed commands are translated game logic, not captured PCM. */
static const uint8_t bounce[] = {0x23,0x2b,0x33,0x23};
static const uint8_t inner_rim[] = {0x09,0x0a,0x0b,0x11,0x12,0x13,0x19,0x1a,
                                  0x1b,0x0b,0x19,0x09,0x1a,0x13,0x0a,0x1b};
static const uint8_t made[] = {0x0c,0x14,0x1c,0x0c};
static const uint8_t outer_rim[] = {0x08,0x10,0x18,0x08};
static const uint8_t catch_ball[] = {0x24,0x2c,0x34,0x24};
static const uint8_t contact[] = {0x0d,0x15,0x1d,0x0d};
static const uint8_t shoe[] = {0x0e,0x1e,0x1e,0x0e};
static const uint8_t collision[] = {0x21,0x29,0x31,0x22,0x2a,0x32,0x21,0x29};
static const uint8_t landing[] = {0x20,0x28,0x30,0x20};

typedef struct {
    const uint8_t *commands;
    uint16_t mask;
    uint32_t call_pc;
} RandomFamily;

static const RandomFamily families[] = {
    {bounce,3,0x82fd8e}, {inner_rim,15,0x82fdc3}, {made,3,0x82fde1},
    {outer_rim,3,0x82fdff}, {catch_ball,3,0x82fe1d}, {contact,3,0x82fe3b},
    {shoe,3,0x82fe59}, {collision,7,0x82fe77}, {landing,3,0x82fe95},
    {collision,7,0x82feb3}
};

/* $80:8930-$8934 -> $80:CEE7-$CEFC. Advances caller-owned shared $07F6.
 * Recomp counterpart AudioSharedRng_M0X0; state is never cached by audio. */
static uint16_t next_shared_rng(uint16_t *state) {
    uint16_t old = *state;
    if (old == 0u) *state = 0x9146u;
    else {
        *state = (uint16_t)(old << 1);
        if ((old & 0x8000u) != 0u) *state ^= 0x1d87u;
    }
    return *state;
}

static uint16_t emit(NbaAudioEventSink sink, void *context,
                      NbaAudioEventKind kind, uint32_t caller,
                      uint16_t command, uint16_t index, uint16_t value) {
    NbaAudioEventOperation operation = {kind,caller,
        kind == NBA_AUDIO_EVENT_COMMAND ? 0x809df3u :
        kind == NBA_AUDIO_EVENT_VOICE_VOLUME ? 0x80a82fu : 0x809f0fu,
        command,index,value};
    return sink ? sink(context,&operation) : 0u;
}

/* ROM $82:FD65-$FF84, recomp GameplayAudioEvents_M0X0. Native witnesses:
 * .analysis/runtime-options-20260830/{natural-v3,controlled-v3}/events.jsonl.
 * $82:F89A-$F8B7 calls this during the presentation/NMI service, independently
 * of the main $87:8E15-$95E6 gameplay iteration. This function owns only the
 * event-consume/ordered-command contract, not that scheduler or SPC driver.
 * Compatibility: clear each bit before RNG/callee dispatch; do not edge-latch
 * events, skip RNG while muted, merge crowd bits, or clear unknown high bits.
 * The native accumulator keeps the remaining sound bits across each callee. */
void nba_audio_events_dispatch(NbaAudioEventState *state,
                               uint16_t *shared_rng_07f6,
                               NbaAudioEventSink sink, void *context) {
    if (state == NULL || shared_rng_07f6 == NULL) return;
    if (state->crowd_enabled_17bb == 0u) state->crowd_bits_13e9 = 0u;
    uint16_t remaining = state->event_bits_13e7;
    for (unsigned bit = 0; bit < 14u; ++bit) {
        uint16_t flag = (uint16_t)(1u << bit);
        if ((remaining & flag) == 0u) continue;
        remaining = (uint16_t)(remaining & (uint16_t)~flag);
        state->event_bits_13e7 = remaining;
        uint16_t command;
        uint32_t call_pc;
        if (bit < 10u) {
            const RandomFamily *family = &families[bit];
            command = family->commands[next_shared_rng(shared_rng_07f6) & family->mask];
            call_pc = family->call_pc;
        } else {
            static const uint8_t fixed[] = {0x40,0x41,0x43,0x44};
            static const uint32_t calls[] = {0x82fec7,0x82fedb,0x82feef,0x82ff03};
            command = fixed[bit - 10u]; call_pc = calls[bit - 10u];
        }
        uint16_t result_a = emit(sink,context,NBA_AUDIO_EVENT_COMMAND,
                                  call_pc,command,0u,0u);
        if (bit == 0u) {
            /* FD92-FDA5: A returned by9DF3 selects a voice. 13E5 affects
             * this bounce only; the0x20 bias and7-bit mask are original. */
            uint16_t volume = (uint16_t)(((state->bounce_strength_13e5 >> 4) & 0x7fu) + 0x20u);
            (void)emit(sink,context,NBA_AUDIO_EVENT_VOICE_VOLUME,
                       0x82fda5u,0u,(uint16_t)(result_a & 0xffu),volume);
        }
    }
    remaining = state->crowd_bits_13e9;
    for (unsigned bit = 0; bit < 4u; ++bit) {
        uint16_t flag = (uint16_t)(1u << bit);
        if ((remaining & flag) == 0u) continue;
        remaining = (uint16_t)(remaining & (uint16_t)~flag);
        state->crowd_bits_13e9 = remaining;
        uint16_t command,index;
        static const uint32_t calls[] = {0x82ff25,0x82ff3f,0x82ff67,0x82ff80};
        if (bit == 2u) {
            /* Separate parallel word tables $82:F88A/$F892. */
            static const uint8_t commands[] = {0x3a,0x3b,0x3c,0x3a};
            static const uint8_t indices[] = {0x1a,0x1b,0x1c,0x1a};
            uint16_t choice = next_shared_rng(shared_rng_07f6) & 3u;
            command = commands[choice]; index = indices[choice];
        } else {
            static const uint8_t commands[] = {0x38,0x39,0,0x2f};
            static const uint8_t indices[] = {0x18,0x19,0,0x1d};
            command = commands[bit]; index = indices[bit];
        }
        (void)emit(sink,context,NBA_AUDIO_EVENT_CROWD_QUEUE,
                   calls[bit],command,index,0x1eu);
    }
}
