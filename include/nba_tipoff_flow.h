#ifndef NBA_TIPOFF_FLOW_H
#define NBA_TIPOFF_FLOW_H
#include "nba_gameplay_ball.h"
#include "nba_assets.h"

/* State written by the bounded $86:E056-$E0AB initializer. Native object-list
 * links are represented as data, not host pointers or a CPU emulation layer. */
typedef struct {
    uint16_t cursor, descriptor_record, descriptor_terminator, ball_descriptor;
    uint16_t object_list_raw_08fe, published_record;
    uint16_t x_fraction, x, y_fraction, y, z_fraction, z;
    uint16_t velocity_x, velocity_y, velocity_z, group, record_id;
    uint16_t context_4933, context_4935, event_08f0;
} NbaTipBallInitialization;
void nba_tip_ball_initialize(NbaTipBallInitialization *state);

/* EC32-EE75 decision boundary; child effects are a separate contract.
 * ED32/EDEF read literal WRAM0046 using Y, NOT team-context X. */
typedef struct {
    uint16_t actor_x,actor_y,actor_z,lower_state,distance,direction,movement;
    uint16_t subject_x,subject_y,subject_z,subject_vz,subject_direction;
    uint16_t paired_direction,ball_x,ball_z,ball_vz;
    uint16_t activity,owner,receiver,live_state,block_mode,raw_0046;
    uint16_t velocity_x,velocity_y,velocity_z,rng,rating_3c,rating_3d;
} NbaJumpReachInput;
typedef struct { uint32_t routine; uint16_t value; } NbaJumpReachRequest;
typedef struct {
    uint16_t velocity_x,velocity_y,velocity_z,rng;
    unsigned request_count;
    NbaJumpReachRequest requests[2];
} NbaJumpReachResult;
/* False rejects missing assets or an out-of-contract table index without
 * publishing partial output. Neither input nor asset data is changed. */
bool nba_jump_reach_decide(const NbaAssetPack *assets,
    const NbaJumpReachInput *input,NbaJumpReachResult *result);

/* EAA8-EC31: lead the focal actor, face the intercept, and launch. */
typedef struct {
    uint16_t actor_x,actor_y,subject_x,subject_y,subject_vx,subject_vy;
    uint16_t subject_distance,context_x,direction_4e,direction_50;
    uint16_t velocity_x,velocity_y,velocity_z,timer_091c;
} NbaReachLaunch;
void nba_reach_launch(NbaReachLaunch *state);

typedef struct { uint16_t record,current,timer; } NbaGraphicsScratchSlot;
typedef struct {
    uint16_t rng,scratch_0046;
    NbaGraphicsScratchSlot slots[3];
} NbaGraphicsScratchState;
/* 82:F02F-F15B: the non-rendering state/DP47 effect of the three-slot
 * presentation transfer scheduler. Payload DMA itself is intentionally absent. */
bool nba_graphics_scratch_step(const NbaAssetPack *assets,
    NbaGraphicsScratchState *state,uint16_t delta);

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

typedef struct {
    int16_t ball_x,ball_y,ball_z,ball_vx,ball_vy,ball_vz;
    int16_t receiver_x,receiver_y,receiver_vx,receiver_vy,passer_z,pass_family;
    uint16_t band,upper_state,passer_mode,receiver_mode,receiver_timer;
    uint16_t passer_group,active_group,passer_timer,behavior_timer,flags,status;
    uint16_t live_state,owner,latch,inhibit,ball_record,source_lo,source_hi;
    uint16_t launch_source_lo,launch_source_hi;
    uint16_t fraction_x,fraction_y,fraction_z;
} NbaTipLaunch;
/* $86:99C4-$9C6E. Tables come from the ROM asset pack, never captures. */
bool nba_tip_launch(const NbaAssetPack *assets,NbaTipLaunch *state);
typedef struct {
    uint16_t live_state,transfer,receiver,whistle,play,request,passer,aux,ball_vz;
} NbaTipCompletion;
/* $86:D365-$D3B0. True dispatches the initial tip wrapper; false attaches. */
bool nba_tip_complete_acquisition(NbaTipCompletion *state);
#endif
