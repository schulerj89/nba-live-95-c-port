#include "nba_gameplay_ball.h"
#include <stdlib.h>

/* Three-point arc table `$85:ABFB`, indexed by even Y offsets 0..356. */
static const int16_t three_point_arc[179] = {
    259,251,246,243,241,238,236,234,232,229,226,224,222,218,215,212,
    209,206,204,202,199,196,194,192,190,189,187,185,183,182,180,177,
    174,172,170,168,166,164,163,161,160,158,157,156,154,153,151,150,
    148,147,147,145,143,141,139,137,136,135,134,133,132,131,130,129,
    128,127,127,126,126,125,125,124,123,122,122,121,121,121,120,120,
    120,119,119,118,118,118,117,117,116,116,116,115,115,114,114,114,
    113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,113,
    113,113,113,113,113,113,113,113,113,113,113,114,
    115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,
    131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,
    147,148,149,151,152,154,157,159,162,164,167,170,173,176,178,181,
    184,189,194,200,208,216,224
};

/* `$85:F1C1`: high + low/4 for axis-dominant deltas, otherwise
 * high + 3*low/8. */
uint16_t nba_gameplay_hoop_distance(int16_t dx, int16_t dy) {
    unsigned ax = (unsigned)abs((int)dx);
    unsigned ay = (unsigned)abs((int)dy);
    unsigned high = ax > ay ? ax : ay;
    unsigned low = ax > ay ? ay : ax;
    return (uint16_t)(high + (high >= low * 2u ? low / 4u :
                             (low * 3u) / 8u));
}

/* Final `$85:9D40-$A079` inner-cylinder make classifier. The caller supplies
 * the already-proven collision-path/basket-side context. */
bool nba_gameplay_ball_is_make(uint16_t live_state, bool alternate_height,
                               bool inner_veto, bool correct_basket_side,
                               int16_t dx, int16_t dy, int16_t z) {
    int minimum_z = alternate_height ? 68 : 74;
    return live_state == 1u && correct_basket_side && !inner_veto &&
           z >= minimum_z && z < 83 &&
           nba_gameplay_hoop_distance(dx, dy) < 7u;
}

/* `$86:9DED-$9DFF` chooses 1/2, then `$86:A561-$A5AF` upgrades to three
 * using the ROM arc table. */
uint16_t nba_gameplay_shot_value(bool one_point_attempt, int16_t shooter_x,
                                 int16_t shooter_y, bool right_basket) {
    if (one_point_attempt) return 1u;
    if (shooter_y < -178 || shooter_y >= 179) return 3u;
    unsigned index = right_basket ? (unsigned)(shooter_y + 178) / 2u :
                                    (unsigned)(178 - shooter_y) / 2u;
    int16_t threshold = three_point_arc[index];
    if (right_basket ? shooter_x <= threshold : shooter_x > -threshold)
        return 3u;
    return 2u;
}

uint8_t nba_gameplay_shot_chance(uint8_t rating, uint8_t raw_actor_8c,
                                 uint8_t difficulty,
                                 bool raw_actor_16_nonnegative) {
    /* Base tier of `$86:9ED8-$A11D`; `$86:A110` compares its result to the
     * low byte from the exact gameplay LFSR. */
    static const int8_t high_adjust[3] = {50, 15, 0}; /* `$86:9F32` */
    static const int8_t low_adjust[3] = {40, 23, 0};  /* `$86:9F38` */
    bool high_branch = raw_actor_8c >= rating;
    int chance;
    if (high_branch)
        chance = rating >= 0xD9 ? 0xDC : rating >= 0xC0 ? 0xA0 :
                 rating >= 0xA8 ? 0x82 : 0x6E;
    else
        chance = rating >= 0xD9 ? 0xE6 : rating >= 0xC0 ? 0xC0 :
                 rating >= 0xA8 ? 0x99 : 0x73;
    if (raw_actor_16_nonnegative)
        chance += (high_branch ? high_adjust : low_adjust)[difficulty < 3u ?
                                                           difficulty : 2u];
    if (chance < 5) chance = 5;
    if (chance > 255) chance = 255;
    return (uint8_t)chance;
}

void nba_gameplay_miss_offset(uint8_t index, bool left_basket,
                              int16_t *dx, int16_t *dy) {
    /* `$86:A17D`, selected with the second `$80:CEE7` result & $0F. */
    static const int8_t offsets[16][2] = {
        {0,7},{0,-7},{0,-7},{-8,0},{6,0},{6,0},{0,7},{0,7},
        {4,-8},{4,-8},{3,7},{3,8},{5,7},{5,8},{2,-7},{2,-7}
    };
    int x = offsets[index & 15u][0], y = offsets[index & 15u][1];
    if (left_basket) { x = -x; y = -y; }
    if (dx) *dx = (int16_t)x;
    if (dy) *dy = (int16_t)y;
}

uint16_t nba_gameplay_shot_flight_duration(int16_t dx, int16_t dy) {
    /* First two words of each `$86:A4AB` record. The third word is not read by
     * the normal launch path and intentionally remains unlabeled. */
    static const uint16_t records[13][2] = {
        {0x0040,0x0028},{0x0060,0x002E},{0x0080,0x0032},
        {0x00A0,0x0038},{0x00B8,0x003C},{0x00E8,0x0042},
        {0x0118,0x0046},{0x0150,0x0049},{0x0180,0x004C},
        {0x01B0,0x004E},{0x01E0,0x0050},{0x0250,0x0056},
        {0x0640,0x005A}
    };
    uint16_t distance = nba_gameplay_hoop_distance(dx, dy);
    for (unsigned i = 0; i < 13u; ++i)
        if (distance < records[i][0]) return records[i][1];
    return records[12][1];
}

void nba_gameplay_shot_launch(int32_t ball_x_fp, int32_t ball_y_fp,
                              int32_t ball_z_fp, int16_t target_x,
                              int16_t target_y, int16_t *velocity_x,
                              int16_t *velocity_y, int16_t *velocity_z) {
    /* `$86:A1BD-$A292`. Host positions are 24.8, which exactly preserve the
     * ROM's signed 8.8 velocity increments used by this base launch branch. */
    int32_t dx_fp = (int32_t)target_x * 256 - ball_x_fp;
    int32_t dy_fp = (int32_t)target_y * 256 - ball_y_fp;
    uint16_t duration = nba_gameplay_shot_flight_duration(
        (int16_t)(dx_fp / 256), (int16_t)(dy_fp / 256));
    int32_t dz_fp = 80 * 256 - ball_z_fp;
    if (velocity_x) *velocity_x = (int16_t)(dx_fp / (int32_t)duration);
    if (velocity_y) *velocity_y = (int16_t)(dy_fp / (int32_t)duration);
    if (velocity_z) *velocity_z = (int16_t)(
        dz_fp / (int32_t)duration + 12 * (int32_t)duration + 0x18);
}

int16_t nba_gameplay_arithmetic_shift_right(int16_t value, unsigned amount) {
    /* Make the 65816 signed `ROR` result explicit instead of relying on the
     * implementation-defined result of shifting a negative C integer. */
    if (amount == 0u) return value;
    if (amount >= 15u) return value < 0 ? -1 : 0;
    uint16_t raw = (uint16_t)value;
    uint16_t shifted = (uint16_t)(raw >> amount);
    if (value < 0) shifted |= (uint16_t)(0xFFFFu << (16u - amount));
    return (int16_t)shifted;
}

bool nba_gameplay_ball_self_test(void) {
    int16_t vx = 0, vy = 0, vz = 0;
    nba_gameplay_shot_launch(0, 0, 20 * 256, 63, 0, &vx, &vy, &vz);
    bool launch_ok = vx == 403 && vy == 0 && vz == 888;
    return launch_ok &&
           nba_gameplay_ball_is_make(1, false, false, true, 0, 0, 74) &&
           nba_gameplay_ball_is_make(1, false, false, true, 6, 0, 82) &&
           !nba_gameplay_ball_is_make(1, false, false, true, 7, 0, 82) &&
           !nba_gameplay_ball_is_make(1, false, false, true, 0, 0, 73) &&
           !nba_gameplay_ball_is_make(1, false, true, true, 0, 0, 81) &&
           !nba_gameplay_ball_is_make(1, false, false, false, 0, 0, 81) &&
           nba_gameplay_ball_is_make(1, true, false, true, 0, 0, 68) &&
           !nba_gameplay_ball_is_make(1, true, false, true, 0, 0, 67) &&
           nba_gameplay_shot_value(false, 117, 0, true) == 2u &&
           nba_gameplay_shot_value(false, 116, 0, true) == 3u &&
           nba_gameplay_shot_value(false, -116, 0, false) == 2u &&
           nba_gameplay_shot_value(false, -115, 0, false) == 3u &&
           nba_gameplay_shot_value(false, 225, 178, true) == 2u &&
           nba_gameplay_shot_value(false, 224, 178, true) == 3u &&
           nba_gameplay_shot_value(true, 300, 0, true) == 1u &&
           nba_gameplay_shot_chance(0xC0, 0xC0, 0, true) == 210u &&
           nba_gameplay_shot_chance(0xA0, 0xA0, 2, true) == 110u &&
           nba_gameplay_shot_flight_duration(63, 0) == 40u &&
           nba_gameplay_shot_flight_duration(64, 0) == 46u &&
           nba_gameplay_shot_flight_duration(1599, 0) == 90u &&
           nba_gameplay_arithmetic_shift_right(-17, 4) == -2 &&
           nba_gameplay_arithmetic_shift_right(17, 4) == 1;
}
