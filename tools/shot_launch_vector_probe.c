/* Replay `$86:A1BD-$A292` base shot velocity construction. */
#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <stdio.h>
#include "nba_gameplay_ball.h"

static int32_t split_fixed(unsigned lo, unsigned hi) {
    int32_t raw = (int32_t)(((uint32_t)(uint16_t)hi << 16) |
                           (uint16_t)lo);
    return raw / 256;
}

int main(void) {
    unsigned dx_lo, dx_hi, dy_lo, dy_hi, z_frac, z_int;
    while (scanf("%x %x %x %x %x %x", &dx_lo, &dx_hi, &dy_lo, &dy_hi,
                 &z_frac, &z_int) == 6) {
        int32_t z_fp = (int32_t)(int16_t)z_int * 256 +
                       (uint8_t)((uint16_t)z_frac >> 8);
        int16_t vx = 0, vy = 0, vz = 0;
        nba_gameplay_shot_launch_delta(
            split_fixed(dx_lo, dx_hi), split_fixed(dy_lo, dy_hi), z_fp,
            &vx, &vy, &vz);
        printf("%04x %04x %04x\n", (uint16_t)vx, (uint16_t)vy,
               (uint16_t)vz);
    }
    return 0;
}
