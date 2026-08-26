/* Replays `$86:BAFD-$BB14` through the actual C catch-mode helper. */
#include <stdint.h>
#include <stdio.h>
#include "nba_gameplay_ball.h"

int main(void) {
    unsigned clock, context_clock, mode, timer, flags;
    while (scanf("%x %x %x %x %x", &clock, &context_clock, &mode,
                 &timer, &flags) == 5) {
        uint16_t out_context = (uint16_t)context_clock;
        uint16_t out_mode = (uint16_t)mode;
        uint16_t out_timer = (uint16_t)timer;
        uint16_t out_flags = (uint16_t)flags;
        nba_gameplay_apply_catch_mode(
            (uint16_t)clock, &out_context, &out_mode, &out_timer,
            &out_flags);
        printf("%04x %04x %04x %04x\n", out_context, out_mode,
               out_timer, out_flags);
    }
    return 0;
}
