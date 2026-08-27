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
#endif
