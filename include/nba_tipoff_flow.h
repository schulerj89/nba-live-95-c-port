#ifndef NBA_TIPOFF_FLOW_H
#define NBA_TIPOFF_FLOW_H
#include "nba_gameplay_ball.h"

typedef struct {
    uint16_t actor_id, actor_inhibit, actor_group, upper_state, upper_lock;
    uint16_t head_height, free_throw, free_throw_actor, live_state, inbound_group;
    int16_t owner, receiver, side_group, hoop_x;
    uint16_t shot_latch;
    int16_t actor_x, actor_y, actor_z, ball_x, ball_y, ball_z, ball_vz;
    NbaGameplayPosePoint points[2];
    unsigned point_count;
} NbaTipContactInput;

typedef enum { NBA_TIP_CONTACT_REJECT, NBA_TIP_CONTACT_ACCEPT,
               NBA_TIP_CONTACT_DEFLECT } NbaTipContactRoute;
typedef struct {
    NbaTipContactRoute route;
    bool request_reach;
    bool reset_inbound_timer;
} NbaTipContactResult;

/* Geometric caller, not the later rating/steal classifier or acquisition.
 * The descending-shot CD97-CE87 child retains its separate implementation. */
NbaTipContactResult nba_tip_contact_geometry(const NbaTipContactInput *input);

typedef struct {
    uint16_t timer_140f, active_148f, enabled_14a7, duration_1477, address_14bf;
    uint8_t kind_1430, bank_1448;
} NbaTipEvent;
typedef struct {
    uint16_t rng, actor_id, team_group, event_bits;
    uint16_t passer, receiver, pass_family, pass_band, receiver_mode;
    NbaTipEvent event;
} NbaTipReceiver;
/* $86:B04C-$B0E1, excluding the $99C4 child. Call suffix after launch. */
void nba_tip_receiver_select(NbaTipReceiver *state);
void nba_tip_receiver_finish(NbaTipReceiver *state);
#endif
