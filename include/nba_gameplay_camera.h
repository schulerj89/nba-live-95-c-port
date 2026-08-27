#ifndef NBA_GAMEPLAY_CAMERA_H
#define NBA_GAMEPLAY_CAMERA_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t x_fraction, x_integer, y_fraction, y_integer;
} NbaCameraSubject;

typedef struct {
    NbaCameraSubject subject;
    int16_t ball_height, side_group, basket_left, basket_right;
    uint16_t alternate_08bc, alternate_mode_08cc, live_state;
} NbaCameraInput;

typedef struct {
    int16_t x, y;
    int16_t previous_x, previous_y;
    int16_t target_x, target_y;
    uint16_t prior_displacement_x, prior_displacement_y;
    uint16_t commanded_step_x, commanded_step_y;
    uint16_t coarse_x, coarse_y;
    uint16_t stream_source;
    uint8_t subject_actor;
    uint16_t initialized_4a54;
    NbaCameraSubject proxy;
    uint16_t subject_pointer_0940;
    uint16_t presentation_ticks_0564;
    bool caller_waiting;
} NbaGameplayCamera;

/* Exact raw-coordinate interface. Ball height is independent of the subject. */
void nba_gameplay_camera_step(NbaGameplayCamera *camera, const NbaCameraInput *input);
void nba_gameplay_camera_place(NbaGameplayCamera *camera, const NbaCameraInput *input);
uint16_t nba_gameplay_camera_resolve(int16_t selector);
void nba_gameplay_camera_copy(NbaGameplayCamera *camera, uint16_t pointer,
                              const NbaCameraSubject *subject);
bool nba_gameplay_camera_ready(uint16_t *presentation_ticks);
NbaCameraSubject nba_gameplay_camera_subject(int32_t x_fp, int32_t y_fp);
/* Legacy 8.8 probe adapter; live gameplay uses the raw interface above. */
void nba_gameplay_camera_init(NbaGameplayCamera *camera, int16_t x, int16_t y);
void nba_gameplay_camera_update(NbaGameplayCamera *camera,
                                int32_t subject_x_fp, int32_t subject_y_fp,
                                int32_t subject_z_fp,
                                uint8_t side_group, bool ball_height_path);

#endif
