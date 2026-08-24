#ifndef NBA_GAMEPLAY_FOUL_H
#define NBA_GAMEPLAY_FOUL_H

#include <stdbool.h>
#include <stdint.h>

#define NBA_GAMEPLAY_FOUL_DEFENSIVE 1u
#define NBA_GAMEPLAY_FOUL_CHARGING  2u
#define NBA_GAMEPLAY_FOUL_OFFENSIVE 13u

typedef struct {
    uint16_t foul_event_raw_0964;
    uint16_t shooting_foul_raw_09bc;
    int8_t offender_actor_raw;
    int8_t victim_actor_raw;
    uint16_t team_fouls[2];
    uint8_t personal_fouls[10];
    uint16_t free_throw_state_raw_0978;
    uint16_t free_throw_sequence_raw_097a;
} NbaGameplayFoulState;

void nba_gameplay_foul_init(NbaGameplayFoulState *state);
bool nba_gameplay_foul_record_contact(NbaGameplayFoulState *state,
                                      uint8_t event_code,
                                      uint8_t offender_actor,
                                      uint8_t victim_actor,
                                      uint8_t offender_team,
                                      bool shot_detached,
                                      uint16_t period_raw_0926);
bool nba_gameplay_foul_record_made_basket(NbaGameplayFoulState *state);
bool nba_gameplay_foul_self_test(void);

#endif
