#ifndef NBA_SESSION_H
#define NBA_SESSION_H

#include "nba_types.h"

#define NBA_SETUP_RULE_COUNT       13
#define NBA_SETUP_OPTION_COUNT      7
#define NBA_SETUP_MAIN_VALUE_COUNT  4

typedef struct {
    uint16_t main_values[NBA_SETUP_MAIN_VALUE_COUNT]; /* $7E:16FB working block */
    uint16_t rules[NBA_SETUP_RULE_COUNT];             /* $7E:17D1 commit block  */
    uint16_t options[NBA_SETUP_OPTION_COUNT];         /* $7E:17B5 commit block  */
} NbaGameConfig;

typedef struct {
    NbaGameConfig config;
    uint8_t left_team;  /* Team Select $7E:16FB */
    uint8_t right_team; /* Team Select $7E:16FD */
    uint8_t player_one_side; /* Player Setup controller ownership, 0 left/1 right */
    /* Gameplay team records `$46EB/$476B`; +$26 are `$4711/$4791`. */
    uint16_t score[2];
    uint16_t game_clock_ticks;
} NbaSession;

extern const uint16_t nba_default_main_values[NBA_SETUP_MAIN_VALUE_COUNT];
extern const uint16_t nba_default_rules[NBA_SETUP_RULE_COUNT];
extern const uint16_t nba_default_options[NBA_SETUP_OPTION_COUNT];

void nba_session_init(NbaSession *session);

#endif /* NBA_SESSION_H */
