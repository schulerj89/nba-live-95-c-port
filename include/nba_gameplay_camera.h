#ifndef NBA_GAMEPLAY_CAMERA_H
#define NBA_GAMEPLAY_CAMERA_H

#include <stdint.h>

typedef struct {
    int16_t x, y;
    int16_t previous_x, previous_y;
    int16_t target_x, target_y;
    uint16_t prior_displacement_x, prior_displacement_y;
    uint16_t commanded_step_x, commanded_step_y;
    uint16_t coarse_x, coarse_y;
    uint16_t stream_source;
    uint8_t subject_actor;
} NbaGameplayCamera;

void nba_gameplay_camera_init(NbaGameplayCamera *camera, int16_t x, int16_t y);
void nba_gameplay_camera_update(NbaGameplayCamera *camera, int16_t subject_x,
                                int16_t subject_y, uint8_t side_group);

#endif
