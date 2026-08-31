#include "nba_session.h"
#include <string.h>

/* `$81:C19A-$C231`, InitializeOrLoadConfiguration: a genuinely fresh native
 * save defaults to Exhibition/Arcade/Rookie/12 minutes. Earlier C defaults
 * came from a configured Simulation/3-minute capture, not factory state.
 * Native witnesses: docs/setup-config-native-contract.md. */
const uint16_t nba_default_main_values[NBA_SETUP_MAIN_VALUE_COUNT] = { 0, 0, 0, 3 };
const uint16_t nba_default_rules[NBA_SETUP_RULE_COUNT] = {
    0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0
};
const uint16_t nba_default_options[NBA_SETUP_OPTION_COUNT] = {
    30, 30, 2, 1, 0, 0, 0
};
const uint16_t nba_default_custom_rules[NBA_SETUP_RULE_COUNT] = {
    45, 45, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0
};

/* `$81:BFAA-$C00A`, ApplySelectedStyle: change active Rules immediately,
 * while Main's edited Style is still working state at $16FD. Committing
 * Main is a separate caller boundary. Any nonzero/non-Custom native value
 * takes the Simulation branch; UI cycling itself restricts values to0..2. */
void nba_config_apply_style(NbaGameConfig *config, uint16_t working_style) {
    if (!config) return;
    if (working_style == 2u) {
        /* `$81:C398-$C3D3`, LoadCustomRules. Selecting another preset does
         * not overwrite this separately stored Custom profile. */
        memcpy(config->rules, config->custom_rules, sizeof(config->rules));
    } else if (working_style == 0u) {
        memcpy(config->rules, nba_default_rules, sizeof(config->rules));
    } else {
        config->rules[0] = config->rules[1] = 45u;
        for (unsigned i = 2u; i < NBA_SETUP_RULE_COUNT; ++i)
            config->rules[i] = 1u;
    }
}

/* `$81:D516-$D537` commits all13 words and calls SaveCustomRules
 * (`$81:C3D5-$C41D`) when the adjustment dispatcher marked Style Custom.
 * This models the separately retained profile; global save validity and
 * disk serialization remain a distinct match-confirm transaction. */
void nba_config_commit_rules(NbaGameConfig *config, const uint16_t *working_rules) {
    if (!config || !working_rules) return;
    memcpy(config->rules, working_rules, sizeof(config->rules));
    if (config->main_values[1] == 2u)
        memcpy(config->custom_rules, working_rules, sizeof(config->custom_rules));
}

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

void nba_session_begin_match(NbaSession *session) {
    if (!session) return;
    /* New Exhibition initialization, not the later-period path:
     * `$86:DBE8` clears `$0926`; `$86:DD2D-$DD44` instead preserves the
     * current period when selecting the next period's clock. Native first-
     * court state confirms zero scores, seven timeouts and the twelve-entry
     * lineup below. Clear host-only final/pause phases at the same logical
     * new-match boundary. See docs/new-match-reset.md and the native-start
     * fixture. Session setup/menu values deliberately survive this call. */
    memset(&session->match, 0, sizeof(session->match));
    session->score[0] = session->score[1] = 0u;
    session->game_clock_ticks = 0u;
    session->match.period_raw_0926 = 0u;
    session->match.flow_state = NBA_MATCH_FLOW_LIVE;
    session->match.final_marker = NBA_MATCH_FINAL_ACTIVE;
    session->match.pause.state = NBA_MATCH_PAUSE_INACTIVE;
    session->match.pause.selection = NBA_MATCH_PAUSE_SELECT_TIMEOUT;
    session->match.pause.selected_side = session->player_one_side;
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

void nba_session_init(NbaSession *session) {
    if (!session) return;
    memset(session, 0, sizeof(*session));
    memcpy(session->config.main_values, nba_default_main_values,
           sizeof(nba_default_main_values));
    memcpy(session->config.rules, nba_default_rules, sizeof(nba_default_rules));
    memcpy(session->config.options, nba_default_options, sizeof(nba_default_options));
    memcpy(session->config.custom_rules, nba_default_custom_rules,
           sizeof(nba_default_custom_rules));
    session->left_team = 3;   /* Chicago */
    session->right_team = 18; /* Orlando */
    session->player_one_side = 1; /* Live Exhibition path defaults to home/right. */
    nba_session_begin_match(session);
}
