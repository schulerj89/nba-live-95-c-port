#include "nba_gameplay_camera.h"
#include <stdlib.h>
#include <string.h>

static int clamp_int(int value, int minimum, int maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

/* `$85:9352-$93F1`: one-pixel dead zone, 22-pixel maximum request,
 * acceleration limited to two pixels beyond the previous displacement, and
 * immediate deceleration. */
static int approach_axis(int previous, int target, unsigned prior,
                         uint16_t *commanded) {
    unsigned distance = (unsigned)abs(target - previous);
    unsigned requested = distance == 0u ? 0u :
                         distance >= 24u ? 22u : distance - 1u;
    unsigned step = requested <= prior ? requested :
                    requested < prior + 2u ? requested : prior + 2u;
    *commanded = (uint16_t)step;
    return previous < target ? previous + (int)step :
           previous > target ? previous - (int)step : previous;
}

/* `$85:9230-$92BE` shifts the subject ahead of play with asymmetric court
 * bands. This is deliberately integer-identical to the 65816 branches. */
static int horizontal_target(int value, uint8_t side_group) {
    if (side_group == 5u) {
        if (value - 256 < 0) value += (value - 256) >> 4;
        if (value < 230) {
            value -= 24;
            if (value >= 110) value = 110;
        } else if (value < 387) {
            value -= 120;
            if (value >= 195) value = 195;
        } else {
            value -= 192;
        }
    } else {
        if (value + 256 >= 0) value += (value + 256) >> 4;
        if (value >= -223) {
            value -= 232;
            if (value < -359) value = -359;
        } else if (value >= -388) {
            value -= 136;
            if (value < -452) value = -452;
        } else {
            value -= 64;
        }
    }
    return clamp_int(value, -582, 328);
}

void nba_gameplay_camera_init(NbaGameplayCamera *camera, int16_t x, int16_t y) {
    memset(camera, 0, sizeof(*camera));
    camera->x = camera->previous_x = camera->target_x = x;
    camera->y = camera->previous_y = camera->target_y = y;
    camera->coarse_x = (uint16_t)((x + 0x246) >> 3);
    camera->coarse_y = (uint16_t)((y + 0x0F2) >> 3);
    camera->stream_source = (uint16_t)(0x8006u +
        camera->coarse_x * 104u + camera->coarse_y * 2u);
    camera->subject_actor = 0xFFu;
}

void nba_gameplay_camera_update(NbaGameplayCamera *camera,
                                int32_t subject_x_fp, int32_t subject_y_fp,
                                int32_t subject_z_fp,
                                uint8_t side_group, bool ball_height_path) {
    int16_t subject_x = (int16_t)(subject_x_fp >= 0 ? subject_x_fp / 256 :
        -(((-subject_x_fp) + 255) / 256));
    int16_t subject_y = (int16_t)(subject_y_fp >= 0 ? subject_y_fp / 256 :
        -(((-subject_y_fp) + 255) / 256));
    int16_t subject_z = (int16_t)(subject_z_fp >= 0 ? subject_z_fp / 256 :
        -(((-subject_z_fp) + 255) / 256));
    int fractional_carry = ((subject_x_fp & 0xFF) +
                            (subject_y_fp & 0xFF)) >> 8;
    int old_x = camera->x, old_y = camera->y;
    camera->prior_displacement_x = (uint16_t)abs(camera->x - camera->previous_x);
    camera->prior_displacement_y = (uint16_t)abs(camera->y - camera->previous_y);
    camera->previous_x = camera->x;
    camera->previous_y = camera->y;
    /* Compatibility fallback for an absent team: hold X. This is NOT the
     * ROM's no-team centering branch (bank85:92C0). Its correction remains
     * outside the seven verified camera slices; see camera-presentation-plan.
     * Only raw groups 0 and 5 enter the asymmetric look-ahead tables here. */
    camera->target_x = side_group == 0u || side_group == 5u ?
        (int16_t)horizontal_target(
            (int16_t)(subject_x + subject_y + fractional_carry), side_group) :
        camera->x;
    int base_y = (subject_y - subject_x) >> 2;
    if (ball_height_path) {
        /* Tip/ball-height camera: the verified 9312-932E arithmetic treats
         * heights below 56 as zero. Upstream alternate-height flag selection
         * remains a compatibility boolean, not a verified ROM caller. */
        int height = subject_z >= 56 ? subject_z : 0;
        camera->target_y = (int16_t)clamp_int(
            base_y - height - 56, -242, -53);
    } else {
        /* Normal live-play vertical path at `$85:92F9-$930D`. */
        camera->target_y = (int16_t)clamp_int(
            base_y - ((subject_y + 272) >> 2) - 56, -242, -53);
    }
    camera->x = (int16_t)approach_axis(old_x, camera->target_x,
                                      camera->prior_displacement_x,
                                      &camera->commanded_step_x);
    camera->y = (int16_t)approach_axis(old_y, camera->target_y,
                                      camera->prior_displacement_y,
                                      &camera->commanded_step_y);
    camera->coarse_x = (uint16_t)((camera->x + 0x246) >> 3);
    camera->coarse_y = (uint16_t)((camera->y + 0x0F2) >> 3);
    camera->stream_source = (uint16_t)(0x8006u +
        camera->coarse_x * 104u + camera->coarse_y * 2u);
}
