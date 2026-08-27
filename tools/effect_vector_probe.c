/* Replays `$87:A9E3-$AA01` effect starts and `$87:AA02-$AAB1` steps. */
#include <stdio.h>
#include <string.h>
#include "nba_gameplay_effect.h"

static void print_state(const NbaGameplayEffectState *state) {
    printf("%04x %04x %04x %04x %04x\n", state->resource_raw_4015,
           state->effect_raw_401b, state->gate_raw_3f33,
           state->frame_raw_4025, state->timer_raw_402d);
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    if (strcmp(argv[1], "start") == 0) {
        unsigned id, resource, effect, gate, frame, timer;
        while (scanf_s("%x %x %x %x %x %x", &id, &resource, &effect,
                       &gate, &frame, &timer) == 6) {
            NbaGameplayEffectState state = {
                .gate_raw_3f33 = (uint16_t)gate,
                .resource_raw_4015 = (uint16_t)resource,
                .effect_raw_401b = (uint16_t)effect,
                .frame_raw_4025 = (uint16_t)frame,
                .timer_raw_402d = (uint16_t)timer
            };
            if (!nba_gameplay_effect_start(&state, (uint16_t)id)) return 3;
            print_state(&state);
        }
        return 0;
    }
    if (strcmp(argv[1], "step") == 0) {
        unsigned resource, effect, gate, frame, timer, reference_y;
        unsigned ball_y, ball_z, velocity_z, dt;
        while (scanf_s("%x %x %x %x %x %x %x %x %x %x", &resource,
                       &effect, &gate, &frame, &timer, &reference_y,
                       &ball_y, &ball_z, &velocity_z, &dt) == 10) {
            NbaGameplayEffectState state = {
                .gate_raw_3f33 = (uint16_t)gate,
                .resource_raw_4015 = (uint16_t)resource,
                .effect_raw_401b = (uint16_t)effect,
                .frame_raw_4025 = (uint16_t)frame,
                .timer_raw_402d = (uint16_t)timer,
                .reference_y_raw_3ff3 = (int16_t)reference_y
            };
            nba_gameplay_effect_step(&state, (int16_t)ball_y, (int16_t)ball_z,
                                     (int16_t)velocity_z, (uint16_t)dt);
            print_state(&state);
        }
        return 0;
    }
    return 2;
}
