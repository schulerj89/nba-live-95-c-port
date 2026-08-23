#include "nba_session.h"
#include <string.h>

const uint16_t nba_default_main_values[NBA_SETUP_MAIN_VALUE_COUNT] = { 0, 1, 0, 0 };
const uint16_t nba_default_rules[NBA_SETUP_RULE_COUNT] = {
    45, 45, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
const uint16_t nba_default_options[NBA_SETUP_OPTION_COUNT] = {
    30, 30, 2, 1, 0, 0, 0
};

void nba_session_init(NbaSession *session) {
    if (!session) return;
    memset(session, 0, sizeof(*session));
    memcpy(session->config.main_values, nba_default_main_values,
           sizeof(nba_default_main_values));
    memcpy(session->config.rules, nba_default_rules, sizeof(nba_default_rules));
    memcpy(session->config.options, nba_default_options, sizeof(nba_default_options));
    session->left_team = 3;   /* Chicago */
    session->right_team = 18; /* Orlando */
    session->player_one_side = 1; /* Live Exhibition path defaults to home/right. */
}
