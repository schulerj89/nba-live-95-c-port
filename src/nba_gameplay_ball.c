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

bool nba_gameplay_ball_self_test(void) {
    return nba_gameplay_ball_is_make(1, false, false, true, 0, 0, 74) &&
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
           nba_gameplay_shot_value(true, 300, 0, true) == 1u;
}
