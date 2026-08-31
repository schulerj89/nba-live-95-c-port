#include "nba_session.h"
#include <string.h>

const uint16_t nba_default_main_values[NBA_SETUP_MAIN_VALUE_COUNT] = { 0, 1, 0, 0 };
const uint16_t nba_default_rules[NBA_SETUP_RULE_COUNT] = {
    45, 45, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
const uint16_t nba_default_options[NBA_SETUP_OPTION_COUNT] = {
    30, 30, 2, 1, 0, 0, 0
};

/* `$86:DBDC-$DBE5` indexes `$86:E38A` with setup value `$17B1`. */
uint16_t nba_match_regulation_clock(uint16_t quarter_length_setting) {
    static const uint16_t ticks[4] = {10800u, 18000u, 28800u, 43200u};
    return quarter_length_setting < 4u ? ticks[quarter_length_setting] :
                                         ticks[0];
}

/* `$86:DD2D-$DD44` indexes `$86:E392` for period `$0926 >= 4`. */
uint16_t nba_match_overtime_clock(uint16_t quarter_length_setting) {
    static const uint16_t ticks[4] = {7200u, 10800u, 14400u, 18000u};
    return quarter_length_setting < 4u ? ticks[quarter_length_setting] :
                                         ticks[0];
}

uint16_t nba_match_period_clock(const NbaSession *session) {
    if (!session) return 0u;
    uint16_t setting = session->config.main_values[3];
    return session->match.period_raw_0926 >= 4u ?
        nba_match_overtime_clock(setting) :
        nba_match_regulation_clock(setting);
}

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
    session->match.period_raw_0926 = 0u;
    session->match.flow_state = NBA_MATCH_FLOW_LIVE;
    session->match.final_marker = NBA_MATCH_FINAL_ACTIVE;
    session->match.pause.state = NBA_MATCH_PAUSE_INACTIVE;
    session->match.pause.selection = NBA_MATCH_PAUSE_SELECT_TIMEOUT;
    /* UI left0/right1 -> native visitor1/home0 context. */
    session->match.pause.selected_side = session->player_one_side ? 0u : 1u;
    for (unsigned side = 0; side < NBA_MATCH_TEAM_COUNT; ++side) {
        session->match.timeouts_remaining[side] = NBA_MATCH_INITIAL_TIMEOUTS;
        static const uint8_t initial_lineup[NBA_MATCH_LINEUP_SIZE] = {
            2u, 0u, 1u, 3u, 4u
        };
        memcpy(session->match.active_lineup[side], initial_lineup,
               sizeof(initial_lineup));
        memcpy(session->match.roster_order[side], initial_lineup,
               sizeof(initial_lineup));
        for (uint8_t roster = NBA_MATCH_LINEUP_SIZE;
             roster < NBA_MATCH_ROSTER_SIZE; ++roster)
            session->match.roster_order[side][roster] = roster;
        for (uint8_t roster = 0; roster < NBA_MATCH_ROSTER_SIZE; ++roster)
            session->match.roster_available[side][roster] = true;
    }
}
