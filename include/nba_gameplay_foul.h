#ifndef NBA_GAMEPLAY_FOUL_H
#define NBA_GAMEPLAY_FOUL_H

#include <stdbool.h>
#include <stdint.h>
#include "nba_gameplay_ai.h"

#define NBA_GAMEPLAY_FOUL_DEFENSIVE 1u
#define NBA_GAMEPLAY_FOUL_CHARGING  2u
#define NBA_GAMEPLAY_FOUL_OFFENSIVE 13u
#define NBA_GAMEPLAY_VIOLATION_INTERFERENCE 6u

typedef struct {
    uint16_t foul_event_raw_0964;
    uint16_t shooting_foul_raw_09bc;
    int8_t offender_actor_raw;
    int8_t victim_actor_raw;
    uint16_t team_fouls[2];
    uint8_t personal_fouls[10];
    uint16_t team_active_roster_count[2]; /* team context `+$54` */
    uint16_t game_foul_stats[5];          /* `$879C71` record `+$26` */
    uint16_t foul_out_state_raw_09ca;
    uint16_t substitution_request_raw_0a08;
    int8_t substitution_actor_raw_492d;
    uint16_t free_throw_state_raw_0978;
    uint16_t free_throw_sequence_raw_097a;
    uint16_t latched_event_raw_08f0;
    uint16_t whistle_active_raw_09b6;
    uint16_t presentation_pending_raw_4937;
    uint16_t contact_context_raw_497f;
    int16_t whistle_timer_raw_08de;
    uint16_t whistle_state_raw_08e6;
    uint16_t whistle_state_mirror_raw_08e8;
    uint16_t presentation_gate_raw_08e2;
    uint16_t whistle_presentation_queued_raw;
    uint16_t side_event_bits_raw_13e9;
} NbaGameplayFoulState;

/* Proven incremental repair criteria from `$83:939D-$9468` and
 * `$83:947D-$9548`. The caller owns selector semantics and supplies the
 * complete native-order permutation explicitly. */
typedef struct {
    uint8_t outgoing_lineup_index; /* active index 0..4 */
    uint8_t roster_order[12];      /* active five, then ordered bench */
    bool eligible[12];             /* indexed by roster slot */
    uint8_t position[12];          /* roster position byte */
} NbaGameplaySubstitutionInput;

typedef struct {
    uint8_t roster_order[12];
    uint8_t outgoing_roster;
    uint8_t replacement_roster;
    uint8_t replacement_order_index;
} NbaGameplaySubstitutionResult;

/* Portable inputs consumed by the primary player/player contact classifier
 * at `$86:C4FE-$C6AC`. Register Y is the offender/hitter and register X is
 * the victim. Team group is the actor-record +$6E value (0 or 5); team index
 * is the +$70 bookkeeping context represented by the port as 0 or 1. */
typedef struct {
    uint8_t offender_actor;
    uint8_t victim_actor;
    uint8_t offender_team;
    uint8_t offender_group;
    uint8_t offender_mode;
    uint16_t offender_movement_magnitude;
    uint16_t offender_boost_raw_72;
    int8_t ball_owner_actor;
    int8_t last_shooter_actor;
    uint8_t offense_group_raw_093a;
    uint16_t live_state_raw_0936;
    uint16_t ball_activity_raw_0948;
    uint16_t period_raw_0926;
    uint16_t context_tag; /* 0 or $87 */
    uint16_t defensive_rule_raw_17d1;
    uint16_t offensive_rule_raw_17d3;
    int8_t game_stat_slot_raw_16;
    bool foul_out_rule_raw_17df;
} NbaGameplayContactFoulInput;

void nba_gameplay_foul_init(NbaGameplayFoulState *state);
bool nba_gameplay_foul_record_bookkeeping(
    NbaGameplayFoulState *state, uint8_t offender_actor,
    uint8_t offender_team, int8_t game_stat_slot,
    bool foul_out_rule_enabled);
bool nba_gameplay_select_foul_out_replacement(
    const NbaGameplaySubstitutionInput *input,
    NbaGameplaySubstitutionResult *result);
bool nba_gameplay_foul_record_contact(NbaGameplayFoulState *state,
                                      uint8_t event_code,
                                      uint8_t offender_actor,
                                      uint8_t victim_actor,
                                      uint8_t offender_team,
                                      bool shot_detached,
                                      uint16_t period_raw_0926);
bool nba_gameplay_foul_record_contact_full(
    NbaGameplayFoulState *state, uint8_t event_code,
    uint8_t offender_actor, uint8_t victim_actor, uint8_t offender_team,
    bool shot_detached, uint16_t period_raw_0926,
    int8_t game_stat_slot, bool foul_out_rule_enabled);
bool nba_gameplay_foul_classify_contact(
    NbaGameplayFoulState *state, NbaGameplayRng *rng,
    const NbaGameplayContactFoulInput *input,
    uint16_t *event_bits_raw_13e7);
bool nba_gameplay_foul_record_made_basket(NbaGameplayFoulState *state);
bool nba_gameplay_foul_record_violation(NbaGameplayFoulState *state,
                                        uint8_t event_code,
                                        uint8_t actor,
                                        uint8_t shooter);
bool nba_gameplay_foul_consume_pending(NbaGameplayFoulState *state,
                                       uint8_t camera_side_group,
                                       uint16_t *event_bits_raw_13e7,
                                       uint16_t *inbound_ready_raw_09ba,
                                       bool short_timer);
bool nba_gameplay_foul_self_test(void);

#endif
