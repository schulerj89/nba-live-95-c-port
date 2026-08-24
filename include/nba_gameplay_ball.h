#ifndef NBA_GAMEPLAY_BALL_H
#define NBA_GAMEPLAY_BALL_H

#include <stdbool.h>
#include <stdint.h>

uint16_t nba_gameplay_hoop_distance(int16_t dx, int16_t dy);
bool nba_gameplay_ball_is_make(uint16_t live_state, bool alternate_height,
                               bool inner_veto, bool correct_basket_side,
                               int16_t dx, int16_t dy, int16_t z);
uint16_t nba_gameplay_shot_value(bool one_point_attempt, int16_t shooter_x,
                                 int16_t shooter_y, bool right_basket);
uint8_t nba_gameplay_shot_chance(uint8_t rating, uint8_t raw_actor_8c,
                                 uint8_t difficulty,
                                 bool raw_actor_16_nonnegative);
void nba_gameplay_miss_offset(uint8_t index, bool left_basket,
                              int16_t *dx, int16_t *dy);
bool nba_gameplay_ball_self_test(void);

#endif
