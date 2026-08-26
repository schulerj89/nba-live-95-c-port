/* Replays `$85:9192-$93F4` through the actual C port. */
#include <stdio.h>
#include <string.h>
#include "nba_gameplay_camera.h"

int main(void) {
    unsigned x, y, previous_x, previous_y;
    unsigned subject_x_fraction, subject_x, subject_y_fraction, subject_y;
    unsigned subject_z;
    unsigned side_group, ball_height_path;
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x", &x, &y,
                 &previous_x, &previous_y, &subject_x_fraction, &subject_x,
                 &subject_y_fraction, &subject_y, &subject_z, &side_group,
                 &ball_height_path) == 11) {
        NbaGameplayCamera camera;
        memset(&camera, 0, sizeof(camera));
        camera.x = (int16_t)(uint16_t)x;
        camera.y = (int16_t)(uint16_t)y;
        camera.previous_x = (int16_t)(uint16_t)previous_x;
        camera.previous_y = (int16_t)(uint16_t)previous_y;
        int32_t x_fp = (int32_t)(int16_t)(uint16_t)subject_x * 256 +
                       ((subject_x_fraction >> 8) & 0xFFu);
        int32_t y_fp = (int32_t)(int16_t)(uint16_t)subject_y * 256 +
                       ((subject_y_fraction >> 8) & 0xFFu);
        int32_t z_fp = (int32_t)(int16_t)(uint16_t)subject_z * 256;
        nba_gameplay_camera_update(
            &camera, x_fp, y_fp, z_fp,
            (uint8_t)side_group, ball_height_path != 0u);
        printf("%04x %04x %04x %04x %04x %04x %04x %04x\n",
               (uint16_t)camera.x, (uint16_t)camera.y,
               (uint16_t)camera.previous_x, (uint16_t)camera.previous_y,
               camera.prior_displacement_x, camera.prior_displacement_y,
               camera.commanded_step_x, camera.commanded_step_y);
    }
    return 0;
}
