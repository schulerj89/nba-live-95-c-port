#include "nba_gameplay_effect.h"

#include <stddef.h>

/* ROM `$85:8AB4-$8B0F`: descriptors selected by `$87:A9E3-$AA01` and
 * advanced by `$87:AA02-$AAB1`. These are gameplay resources, not host
 * animation approximations. */
typedef struct {
    uint16_t count;
    uint16_t gate;
    const uint16_t *resources;
    const uint16_t *durations;
} NbaGameplayEffectDescriptor;

static const uint16_t resources_0[] = {0x0823u};
static const uint16_t resources_1[] = {0x0824u, 0x0825u, 0x0826u, 0x0825u, 0x0824u};
static const uint16_t resources_2[] = {0x0827u, 0x0828u, 0x0829u, 0x0828u, 0x0827u};
static const uint16_t resources_3[] = {0x082Au};
static const uint16_t resources_4[] = {0x082Fu};
static const uint16_t resources_5[] = {0x082Bu};
static const uint16_t durations_0[] = {0x0014u};
static const uint16_t durations_1[] = {6u, 6u, 6u, 6u, 6u};
static const uint16_t durations_2[] = {4u, 4u, 6u, 4u, 4u};
static const uint16_t durations_3[] = {0x000Au};
static const uint16_t durations_4[] = {0x001Cu};
static const uint16_t durations_5[] = {0x0014u};

static const NbaGameplayEffectDescriptor descriptors[] = {
    {1u, 0u, resources_0, durations_0},
    {5u, 0u, resources_1, durations_1},
    {5u, 0u, resources_2, durations_2},
    {1u, 1u, resources_3, durations_3},
    {1u, 1u, resources_4, durations_4},
    {1u, 0u, resources_5, durations_5}
};

void nba_gameplay_effect_init(NbaGameplayEffectState *state) {
    if (!state) return;
    state->gate_raw_3f33 = 1u;
    state->resource_raw_4015 = 0x0822u;
    state->effect_raw_401b = 0xFFFFu;
    state->frame_raw_4025 = 0u;
    state->timer_raw_402d = 0u;
    state->reference_y_raw_3ff3 = 0;
}

bool nba_gameplay_effect_start(NbaGameplayEffectState *state,
                               uint16_t effect_id) {
    if (!state || effect_id >= (uint16_t)(sizeof(descriptors) / sizeof(descriptors[0])))
        return false;
    /* `$87:A9E3-$AA01`: starting an effect deliberately leaves `$4015`
     * untouched until the next `$87:AA02` step. */
    state->effect_raw_401b = effect_id;
    state->gate_raw_3f33 = descriptors[effect_id].gate;
    state->frame_raw_4025 = 0u;
    state->timer_raw_402d = 0u;
    return true;
}

static uint16_t absolute_signed_difference(int16_t a, int16_t b) {
    uint16_t difference = (uint16_t)((uint16_t)a - (uint16_t)b);
    if ((int16_t)difference < 0)
        difference = (uint16_t)(0u - difference);
    return difference;
}

void nba_gameplay_effect_step(NbaGameplayEffectState *state,
                              int16_t ball_y, int16_t ball_z,
                              int16_t velocity_z, uint16_t dt) {
    if (!state) return;

    /* `$87:AA02-$AA3E`: descending through the net may replace the current
     * resource and returns before the normal frame accumulator advances. */
    if ((uint16_t)ball_z >= 0x004Au &&
        (int16_t)((uint16_t)velocity_z - 1u) < 0 &&
        ((int16_t)state->effect_raw_401b < 0 || state->gate_raw_3f33 == 0u) &&
        absolute_signed_difference(ball_y, state->reference_y_raw_3ff3) < 8u) {
        if (state->resource_raw_4015 < 0x082Cu)
            state->resource_raw_4015 = 0x082Du;
        state->gate_raw_3f33 = 0u;
        return;
    }

    if ((int16_t)state->effect_raw_401b < 0) {
        state->resource_raw_4015 = 0x0822u;
        state->gate_raw_3f33 = 1u;
        return;
    }
    if (state->effect_raw_401b >=
        (uint16_t)(sizeof(descriptors) / sizeof(descriptors[0]))) {
        state->effect_raw_401b = 0xFFFFu;
        state->resource_raw_4015 = 0x0822u;
        state->frame_raw_4025 = state->timer_raw_402d = 0u;
        return;
    }

    const NbaGameplayEffectDescriptor *descriptor =
        &descriptors[state->effect_raw_401b];
    uint16_t frame = state->frame_raw_4025;
    if (frame >= descriptor->count) frame = 0u;
    state->timer_raw_402d = (uint16_t)(state->timer_raw_402d + dt);
    if (state->timer_raw_402d >= descriptor->durations[frame]) {
        /* Native code advances at most one frame per invocation. */
        state->timer_raw_402d =
            (uint16_t)(state->timer_raw_402d - descriptor->durations[frame]);
        frame = (uint16_t)(frame + 1u);
        state->frame_raw_4025 = frame;
    }
    if (frame >= descriptor->count) {
        state->timer_raw_402d = 0u;
        state->frame_raw_4025 = 0u;
        state->effect_raw_401b = 0xFFFFu;
        state->resource_raw_4015 = 0x0822u;
        return;
    }
    state->resource_raw_4015 = descriptor->resources[frame];
}

bool nba_gameplay_effect_self_test(void) {
    NbaGameplayEffectState state;
    nba_gameplay_effect_init(&state);
    state.resource_raw_4015 = 0x1234u;
    bool start = nba_gameplay_effect_start(&state, 3u) &&
        state.effect_raw_401b == 3u && state.gate_raw_3f33 == 1u &&
        state.frame_raw_4025 == 0u && state.timer_raw_402d == 0u &&
        state.resource_raw_4015 == 0x1234u;
    for (unsigned i = 0; i < 4u; ++i)
        nba_gameplay_effect_step(&state, 20, 0, 0, 2u);
    bool active = state.effect_raw_401b == 3u &&
        state.timer_raw_402d == 8u && state.resource_raw_4015 == 0x082Au;
    nba_gameplay_effect_step(&state, 20, 0, 0, 2u);
    bool terminal = state.effect_raw_401b == 0xFFFFu &&
        state.frame_raw_4025 == 0u && state.timer_raw_402d == 0u &&
        state.resource_raw_4015 == 0x0822u;

    nba_gameplay_effect_start(&state, 1u);
    state.resource_raw_4015 = 0x0800u;
    nba_gameplay_effect_step(&state, 7, 0x004A, 0, 2u);
    bool landing = state.resource_raw_4015 == 0x082Du &&
        state.gate_raw_3f33 == 0u && state.frame_raw_4025 == 0u &&
        state.timer_raw_402d == 0u;
    nba_gameplay_effect_start(&state, 1u);
    nba_gameplay_effect_step(&state, 8, 0x004A, 0, 2u);
    bool landing_boundary = state.timer_raw_402d == 2u &&
        state.resource_raw_4015 == 0x0824u;
    state.frame_raw_4025 = 0u;
    state.timer_raw_402d = 5u;
    nba_gameplay_effect_step(&state, 20, 0, 0, 2u);
    bool carry = state.frame_raw_4025 == 1u &&
        state.timer_raw_402d == 1u && state.resource_raw_4015 == 0x0825u;
    state.frame_raw_4025 = state.timer_raw_402d = 0u;
    nba_gameplay_effect_step(&state, 20, 0, 0, 20u);
    bool single_advance = state.frame_raw_4025 == 1u &&
        state.timer_raw_402d == 14u && state.resource_raw_4015 == 0x0825u;
    return start && active && terminal && landing && landing_boundary &&
           carry && single_advance &&
           !nba_gameplay_effect_start(&state, 6u);
}
