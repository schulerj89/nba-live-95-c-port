/* Replays the `$85:A692-$A755` clamp core through the actual C port. */
#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned x_fraction, x, y_fraction, y, velocity_x, velocity_y;
    while (scanf("%x %x %x %x %x %x", &x_fraction, &x, &y_fraction,
                 &y, &velocity_x, &velocity_y) == 6) {
        int32_t x_fp = (int32_t)(int16_t)(uint16_t)x * 256 +
                       ((x_fraction >> 8) & 0xFFu);
        int32_t y_fp = (int32_t)(int16_t)(uint16_t)y * 256 +
                       ((y_fraction >> 8) & 0xFFu);
        int16_t vx = (int16_t)(uint16_t)velocity_x;
        int16_t vy = (int16_t)(uint16_t)velocity_y;
        (void)nba_gameplay_court_finish_y_step(&x_fp, &y_fp, &vx, &vy);
        int16_t out_x = (int16_t)(x_fp >= 0 ? x_fp / 256 :
            -(((-x_fp) + 255) / 256));
        int16_t out_y = (int16_t)(y_fp >= 0 ? y_fp / 256 :
            -(((-y_fp) + 255) / 256));
        printf("%04x %04x %04x %04x %04x %04x\n",
               (uint16_t)((x_fp & 0xFF) << 8), (uint16_t)out_x,
               (uint16_t)((y_fp & 0xFF) << 8), (uint16_t)out_y,
               (uint16_t)vx, (uint16_t)vy);
    }
    return 0;
}
