#include "nba_gameplay_free_throw.h"
#include <stdio.h>

int main(void) {
    unsigned value[8];
    for (;;) {
        for (unsigned i = 0; i < 8u; ++i)
            if (scanf("%x", &value[i]) != 1) return i ? 2 : 0;
        NbaGameplayHumanFreeThrowAim state = {
            .state_raw_0978 = (uint16_t)value[0],
            .aim_x_raw_0980 = (uint16_t)value[1],
            .aim_y_raw_0982 = (uint16_t)value[2],
            .accumulator_raw_0984 = (uint16_t)value[3],
            .step_raw_0986 = (uint16_t)value[4],
            .controller_assignment_raw_16 = (int16_t)(uint16_t)value[5],
            .human_context_raw_3b = (uint16_t)value[6],
            .shoot_held = value[7] != 0u
        };
        NbaGameplayHumanFreeThrowResult result =
            nba_gameplay_free_throw_human_aim_step_frame(&state);
        printf("%x %x %x %x %x %x\n", (unsigned)result,
            state.state_raw_0978, state.aim_x_raw_0980,
            state.aim_y_raw_0982, state.accumulator_raw_0984,
            state.step_raw_0986);
    }
}
