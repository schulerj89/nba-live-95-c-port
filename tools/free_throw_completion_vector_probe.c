#include "nba_gameplay_free_throw.h"
#include <stdio.h>

int main(void) {
    unsigned v[22];
    for (;;) {
        for (unsigned i = 0; i < 22u; ++i)
            if (scanf("%x", &v[i]) != 1) return i ? 2 : 0;
        NbaGameplayFreeThrowCompletion s = {
            (uint16_t)v[1], (uint16_t)v[2], (int16_t)(uint16_t)v[3],
            (uint16_t)v[4], (uint16_t)v[5], (uint16_t)v[6],
            (uint16_t)v[7], (uint16_t)v[8], (int16_t)(uint16_t)v[9],
            (int16_t)(uint16_t)v[10], (int16_t)(uint16_t)v[11],
            (int16_t)(uint16_t)v[12], (int16_t)(uint16_t)v[13]
        };
        NbaGameplayRng rng = {(uint16_t)v[14]};
        uint16_t aim_x = (uint16_t)v[17], aim_y = (uint16_t)v[18];
        switch (v[0]) {
        case 0:
            (void)nba_gameplay_free_throw_presentation_gate(
                &s, v[15] != 0u, (uint16_t)v[16]);
            break;
        case 1:
            (void)nba_gameplay_free_throw_cpu_aim_step(
                &s, &rng, (uint16_t)v[15], (uint8_t)v[16], &aim_x, &aim_y);
            break;
        case 2:
            (void)nba_gameplay_free_throw_release_complete(&s);
            break;
        case 3:
            (void)nba_gameplay_free_throw_resolution_step(
                &s, v[15] != 0u, v[16] != 0u, (uint16_t)v[17],
                (uint16_t)v[18], (uint16_t)v[19],
                (int16_t)(uint16_t)v[20], (int16_t)(uint16_t)v[21]);
            aim_x = aim_y = 0u;
            break;
        default: return 3;
        }
        printf("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
            s.state_raw_0978, s.attempts_raw_097a,
            (uint16_t)s.whistle_timer_raw_08de, s.audio_raw_08e6,
            s.audio_mirror_raw_08e8, s.upload_raw_180b, s.upload_raw_180c,
            s.ball_z_raw_3ef7, (uint16_t)s.ball_x_raw_3eef,
            (uint16_t)s.ball_y_raw_3ef3, (uint16_t)s.ball_vx_raw_3ef9,
            (uint16_t)s.ball_vy_raw_3efb, (uint16_t)s.ball_vz_raw_3efd,
            rng.state, aim_x, aim_y);
    }
}
