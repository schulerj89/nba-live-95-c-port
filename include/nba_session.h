#ifndef NBA_SESSION_H
#define NBA_SESSION_H

#include "nba_types.h"

#define NBA_SETUP_RULE_COUNT       13
#define NBA_SETUP_OPTION_COUNT      7
#define NBA_SETUP_MAIN_VALUE_COUNT  4
#define NBA_MATCH_TEAM_COUNT         2
#define NBA_MATCH_LINEUP_SIZE        5
#define NBA_MATCH_ROSTER_SIZE       12
#define NBA_MATCH_INITIAL_TIMEOUTS   7

typedef struct {
    uint16_t main_values[NBA_SETUP_MAIN_VALUE_COUNT]; /* $7E:16FB working block */
    uint16_t rules[NBA_SETUP_RULE_COUNT];             /* $7E:17D1 commit block  */
    uint16_t options[NBA_SETUP_OPTION_COUNT];         /* $7E:17B5 commit block  */
} NbaGameConfig;

typedef enum {
    NBA_MATCH_FLOW_LIVE = 0,
    NBA_MATCH_FLOW_HORN_BALL_LIVE,
    NBA_MATCH_FLOW_PERIOD_PRESENTATION_PENDING,
    NBA_MATCH_FLOW_PERIOD_RESTART_PENDING,
    NBA_MATCH_FLOW_POSTGAME_PRESENTATION_PENDING,
    NBA_MATCH_FLOW_FINAL
} NbaMatchFlowState;

typedef enum {
    NBA_MATCH_FINAL_ACTIVE = 0,
    NBA_MATCH_FINAL_CONFIRMED
} NbaMatchFinalMarker;

/* `$86:8300-$857B`: only TIMEOUT (index zero) and RESUME (index four) are
 * named by current native evidence. Indices one through three stay absent. */
typedef enum {
    NBA_MATCH_PAUSE_INACTIVE = 0,
    NBA_MATCH_PAUSE_MENU,
    NBA_MATCH_PAUSE_TIMEOUT_TRANSITION,
    NBA_MATCH_PAUSE_MENU_AFTER_TIMEOUT,
    NBA_MATCH_PAUSE_RESUME_TRANSITION
} NbaMatchPauseState;

typedef enum {
    NBA_MATCH_PAUSE_SELECT_TIMEOUT = 0,
    NBA_MATCH_PAUSE_SELECT_RESUME = 4
} NbaMatchPauseSelection;

typedef struct {
    NbaMatchPauseState state;
    NbaMatchPauseSelection selection;
    uint8_t selected_side; /* native `$08D2`: zero left, nonzero right */
    uint16_t saved_live_state_raw_4988;
    uint16_t transition_ticks_remaining;
} NbaMatchPauseFlow;

/* Persistent match-owned state. `$0926`, `$4715/$4795`, the selected five
 * roster slots, and lifecycle phase outlive an individual court presentation.
 * Timeout-menu orchestration is the bounded TIMEOUT/RESUME slice only. */
typedef struct {
    uint16_t period_raw_0926;
    uint16_t timeouts_remaining[NBA_MATCH_TEAM_COUNT];
    uint8_t active_lineup[NBA_MATCH_TEAM_COUNT][NBA_MATCH_LINEUP_SIZE];
    /* Native `$46F9/$4779` are twelve-word permutations: active five first,
     * then bench order. Keep the complete order so a proven bench scan does
     * not invent ordering from roster numbers. */
    uint8_t roster_order[NBA_MATCH_TEAM_COUNT][NBA_MATCH_ROSTER_SIZE];
    bool roster_available[NBA_MATCH_TEAM_COUNT][NBA_MATCH_ROSTER_SIZE];
    NbaMatchFlowState flow_state;
    NbaMatchFinalMarker final_marker;
    uint16_t presentation_ticks_remaining;
    NbaMatchPauseFlow pause;
} NbaMatchLifecycle;

typedef struct {
    NbaGameConfig config;
    uint8_t left_team;  /* Team Select $7E:16FB */
    uint8_t right_team; /* Team Select $7E:16FD */
    uint8_t player_one_side; /* Player Setup controller ownership, 0 left/1 right */
    /* Gameplay team records `$46EB/$476B`; +$26 are `$4711/$4791`. */
    uint16_t score[2];
    uint16_t game_clock_ticks;
    NbaMatchLifecycle match;
} NbaSession;

extern const uint16_t nba_default_main_values[NBA_SETUP_MAIN_VALUE_COUNT];
extern const uint16_t nba_default_rules[NBA_SETUP_RULE_COUNT];
extern const uint16_t nba_default_options[NBA_SETUP_OPTION_COUNT];

void nba_session_init(NbaSession *session);
/* Start a new Exhibition match while preserving session configuration,
 * selected teams and controller side. Never use for period/timeout resume. */
void nba_session_begin_match(NbaSession *session);
uint16_t nba_match_regulation_clock(uint16_t quarter_length_setting);
uint16_t nba_match_overtime_clock(uint16_t quarter_length_setting);
uint16_t nba_match_period_clock(const NbaSession *session);

#endif /* NBA_SESSION_H */
