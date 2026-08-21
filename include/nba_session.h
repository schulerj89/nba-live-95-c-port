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
} NbaSession;

extern const uint16_t nba_default_main_values[NBA_SETUP_MAIN_VALUE_COUNT];
extern const uint16_t nba_default_rules[NBA_SETUP_RULE_COUNT];
extern const uint16_t nba_default_options[NBA_SETUP_OPTION_COUNT];

void nba_session_init(NbaSession *session);

#endif /* NBA_SESSION_H */
