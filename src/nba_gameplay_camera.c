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
    camera->initialized_4a54 = 0xFFFFu;
}

NbaCameraSubject nba_gameplay_camera_subject(int32_t x_fp, int32_t y_fp) {
    /* Convert host 8.8 into the ROM's two-word 16.16 representation. */
    NbaCameraSubject subject = {(uint16_t)((uint32_t)x_fp << 8),
        (uint16_t)((uint32_t)x_fp >> 8), (uint16_t)((uint32_t)y_fp << 8),
        (uint16_t)((uint32_t)y_fp >> 8)};
    return subject;
}

/* `$87:A9D0-$A9E2`: literal actor-pointer table at 879C7B. Negative
 * selectors clear 0940; a zero POINTER means ball, not actor zero. */
uint16_t nba_gameplay_camera_resolve(int16_t selector) {
    return selector >= 0 && selector < 10 ? (uint16_t)(0x34EB + selector * 0x100) : 0;
}

/* `$87:95AC-$95BA`: caller resolves the subject before waiting. The host
 * yields instead of busy-waiting for VBlank. Surplus ticks are discarded. */
bool nba_gameplay_camera_ready(uint16_t *ticks) {
    if (*ticks < 2u) return false;
    *ticks = 0;
    return true;
}

/* `$87:95BB-$95DE`: snapshot XY before the presentation wrapper dispatch. */
void nba_gameplay_camera_copy(NbaGameplayCamera *camera, uint16_t pointer,
                              const NbaCameraSubject *subject) {
    camera->subject_pointer_0940 = pointer;
    camera->subject_actor = pointer ? (uint8_t)((pointer - 0x34EB) / 0x100) : 0xFF;
    camera->proxy = *subject;
}

/* `$85:8B98-$8BBE`: first placement uses the selected subject, not fixed
 * screen coordinates. Child9192 has its own full entry/exit replay. */
void nba_gameplay_camera_place(NbaGameplayCamera *camera, const NbaCameraInput *input) {
    camera->initialized_4a54 = 0;
    nba_gameplay_camera_step(camera, input);
}

/* `$85:9192-$93F4`: complete camera arithmetic and initialization state.
 * Raw 16-bit fractions are kept until the projection carry is consumed. */
void nba_gameplay_camera_step(NbaGameplayCamera *camera, const NbaCameraInput *in) {
    camera->proxy = in->subject;
    int16_t subject_x = (int16_t)in->subject.x_integer;
    int16_t subject_y = (int16_t)in->subject.y_integer;
    uint16_t fraction_x = in->subject.x_fraction, fraction_y = in->subject.y_fraction;
    bool initialized = camera->initialized_4a54 != 0;
    if (initialized) {
        camera->prior_displacement_x = (uint16_t)abs((int16_t)(camera->x - camera->previous_x));
        camera->prior_displacement_y = (uint16_t)abs((int16_t)(camera->y - camera->previous_y));
        camera->previous_x = camera->x;
        camera->previous_y = camera->y;
    } else {
        camera->prior_displacement_x = camera->prior_displacement_y = 0;
    }
    /* 91DF-91FA changes scratch fractions, not the persistent proxy. */
    if (subject_x == 394 || subject_x == -394) fraction_x = 0;
    if (subject_y == 224 || subject_y == -224) fraction_y = 0;
    unsigned carry = ((unsigned)fraction_x + fraction_y) >> 16;
    int projected_x = (int16_t)(subject_x + subject_y + carry);
    int16_t basket = in->side_group == 0 ? in->basket_left : in->basket_right;
    camera->target_x = (int16_t)(in->side_group < 0 ?
        clamp_int((int16_t)(projected_x - 128), -582, 328) :
        horizontal_target(projected_x, basket < 0 ? 0u : 5u));
    int base_y = (int16_t)(subject_y - subject_x) >> 2;
    if (in->live_state == 1u || (in->alternate_08bc != 0u && in->alternate_mode_08cc == 1u)) {
        /* 9312 explicitly reads the BALL height, even with an actor proxy. */
        int height = in->ball_height >= 56 ? in->ball_height : 0;
        camera->target_y = (int16_t)clamp_int(
            (int16_t)(base_y - height - 56), -242, -53);
    } else {
        camera->target_y = (int16_t)clamp_int(
            (int16_t)(base_y - ((uint16_t)(subject_y + 272) >> 2) - 56), -242, -53);
    }
    if (!initialized) {
        camera->x = camera->target_x;
        camera->y = camera->target_y;
        camera->initialized_4a54 = 0xFFFFu;
        /* 934E/9351 preserves old-history and commanded-step words. */
    } else {
        camera->x = (int16_t)approach_axis(camera->previous_x, camera->target_x,
                                          camera->prior_displacement_x, &camera->commanded_step_x);
        camera->y = (int16_t)approach_axis(camera->previous_y, camera->target_y,
                                          camera->prior_displacement_y, &camera->commanded_step_y);
    }
    camera->coarse_x = (uint16_t)((camera->x + 0x246) >> 3);
    camera->coarse_y = (uint16_t)((camera->y + 0x0F2) >> 3);
    camera->stream_source = (uint16_t)(0x8006u +
        camera->coarse_x * 104u + camera->coarse_y * 2u);
}

void nba_gameplay_camera_update(NbaGameplayCamera *camera,
    int32_t x, int32_t y, int32_t z, uint8_t side, bool ball_height_path) {
    NbaCameraInput in = {0};
    in.subject = nba_gameplay_camera_subject(x, y);
    in.ball_height = (int16_t)((uint32_t)z >> 8);
    in.side_group = side == 0xFFu ? -1 : side;
    in.basket_left = -336; in.basket_right = 336;
    in.live_state = ball_height_path ? 1u : 2u;
    camera->initialized_4a54 = 0xFFFFu;
    nba_gameplay_camera_step(camera, &in);
}
