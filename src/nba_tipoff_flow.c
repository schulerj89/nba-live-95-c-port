#include "nba_tipoff_flow.h"
#include "nba_gameplay_ai.h"

static int16_t difference(int16_t a,int16_t b) {
    return (int16_t)(uint16_t)((uint16_t)a-(uint16_t)b);
}
static uint16_t magnitude(int16_t value) {
    return value<0?(uint16_t)(0u-(uint16_t)value):(uint16_t)value;
}

/* `$86:CCFC-$CD96` / `$86:CE88-$CF9F`: exact integer-word gates.
 * Native D549 pose-point outputs are a separate asset-backed child boundary.
 * First tip has owner/receiver negative and shot_latch=0; no frame constants. */
NbaTipContactResult nba_tip_contact_geometry(const NbaTipContactInput *in) {
    NbaTipContactResult out={NBA_TIP_CONTACT_REJECT,false,false};
    if(in->actor_id==10u || in->actor_inhibit)return out;
    if(in->free_throw && in->actor_id!=in->free_throw_actor)return out;
    if(in->owner>=0) {
        if((in->owner>=5)==(in->actor_id>=5u))return out;
    } else {
        int16_t hoop_delta=difference(in->ball_x,in->hoop_x);
        if((int16_t)magnitude(in->ball_y)<8 && hoop_delta<8 && hoop_delta>=-8 &&
           in->ball_z<86 && in->ball_z>=56)return out;
        int16_t dy=difference(in->actor_y,in->ball_y);
        if(dy>=16 || dy< -16)return out;
        int16_t dz=difference(in->ball_z,in->actor_z);
        int limit=in->receiver==(int16_t)in->actor_id?96:72;
        if(dz<0 || dz>=limit)return out;
    }
    if(in->owner<0 && in->actor_group==(uint16_t)in->side_group &&
       in->receiver>=0 && in->receiver!=(int16_t)in->actor_id)return out;
    if(in->live_state==0x82 &&
       !(in->owner<0 && in->actor_group==(uint16_t)in->side_group && in->receiver==(int16_t)in->actor_id)) {
        out.reset_inbound_timer=true;
        if(in->actor_group!=in->inbound_group)return out;
    }
    unsigned radius=8, body_radius=8;
    if(in->owner>=0 || (in->receiver!=(int16_t)in->actor_id && in->shot_latch && in->ball_vz<0)) {
        radius=in->upper_state==0x13?12:4;
        if(radius==12)body_radius=12;
    } else if(in->receiver==(int16_t)in->actor_id || in->shot_latch || in->receiver<0) {
        radius=body_radius=16;
    }
    for(unsigned p=0;p<in->point_count && p<2;++p) {
        const NbaGameplayPosePoint *point=&in->points[p];
        if((int16_t)magnitude(difference(point->x,in->ball_x))<(int)radius &&
           (int16_t)magnitude(difference(point->y,in->ball_y))<(int)radius &&
           (int16_t)magnitude(difference(point->z,in->ball_z))<(int)radius) {
            out.route=NBA_TIP_CONTACT_ACCEPT;return out;
        }
    }
    if(in->owner>=0)return out;
    int16_t dz=difference(in->ball_z,in->actor_z);
    if(dz<0)return out;
    if(dz>=(int16_t)in->head_height) {
        out.request_reach=in->upper_lock==0;
        if(in->receiver!=(int16_t)in->actor_id || difference(dz,8)>=(int16_t)in->head_height)return out;
    }
    int16_t dx=difference(in->actor_x,in->ball_x),dy=difference(in->actor_y,in->ball_y);
    if(dx>=(int)body_radius || dx< -(int)body_radius || dy>=(int)body_radius || dy< -(int)body_radius)return out;
    out.route=in->shot_latch || (in->receiver>=0 && in->receiver!=(int16_t)in->actor_id)?
        NBA_TIP_CONTACT_DEFLECT:NBA_TIP_CONTACT_ACCEPT;
    return out;
}

void nba_tip_receiver_select(NbaTipReceiver *s) {
    /* `$86:B04C-$B0C3`: preserve the raw slot11 event descriptor. Its
     * downstream presentation scheduler is not guessed to be menu audio. */
    s->event=(NbaTipEvent){0x32,1,1,0x258,0xF7BA,0x32,0};
    NbaGameplayRng rng={s->rng};
    s->receiver=(uint16_t)(s->team_group+3+(nba_gameplay_rng_next(&rng)&1));
    s->rng=rng.state;s->passer=s->actor_id;s->pass_family=0xFFFF;
    s->pass_band=12;s->receiver_mode=10;
}
void nba_tip_receiver_finish(NbaTipReceiver *s) {
    /* `$86:B0C8-$B0E1`: runs AFTER the launch child. */
    s->event_bits|=s->receiver>=5?2:4;
}
