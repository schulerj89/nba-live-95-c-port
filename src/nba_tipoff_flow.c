#include "nba_tipoff_flow.h"
#include "nba_gameplay_ai.h"
#include "nba_shot_action.h"

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

static int32_t arithmetic_shift(int32_t value,unsigned bits) {
    int32_t divisor=(int32_t)(1u<<bits);
    return value>=0?value/divisor:-((-value+divisor-1)/divisor);
}
static int16_t launch_axis(int16_t target,int16_t origin,int16_t duration) {
    return (int16_t)(((int32_t)difference(target,origin)*256)/duration);
}
bool nba_tip_launch(const NbaAssetPack *assets,NbaTipLaunch *s) {
    /* `$86:99C4-$9C44` plus `$9C45-$9C6E`. Native positions are integer
     * words here; fractional words survive all endpoint/nudge operations. */
    int16_t duration,vertical,unused;
    uint8_t family=s->pass_family<0?0:s->pass_family>0?1:2;
    if(s->band%6 || !nba_assets_gameplay_pass_launch(assets,family,(uint8_t)(s->band/6),
            &duration,&vertical,&unused) || duration<=0)return false;
    int16_t x=(int16_t)(s->receiver_x+arithmetic_shift((int32_t)s->receiver_vx*duration,8));
    int16_t y=(int16_t)(s->receiver_y+arithmetic_shift((int32_t)s->receiver_vy*duration,8));
    if(x>362 || x< -362) {
        x=x>362?362:-362;s->receiver_vx=launch_axis(x,s->receiver_x,duration);
    }
    if(y>192 || y< -192) {
        y=y>192?192:-192;s->receiver_vy=launch_axis(y,s->receiver_y,duration);
    }
    s->launch_source_lo=s->source_lo;s->launch_source_hi=s->source_hi;
    s->owner=0xffff;s->latch=1;s->inhibit=20;s->ball_record=0x3eeb;
    s->ball_vx=launch_axis(x,s->ball_x,duration);s->ball_vy=launch_axis(y,s->ball_y,duration);
    s->ball_vz=(int16_t)(vertical-((uint16_t)s->passer_z>=16?128:0));
    unsigned shift=s->upper_state==0x2b || s->upper_state==0x2c?6:7;
    s->ball_x=(int16_t)(s->ball_x+arithmetic_shift(s->ball_vx,shift));
    s->ball_y=(int16_t)(s->ball_y+arithmetic_shift(s->ball_vy,shift));
    s->ball_z=(int16_t)(s->ball_z+arithmetic_shift(s->ball_vz,shift));
    if(s->receiver_mode!=14)s->receiver_timer=(uint16_t)(duration+15);
    if(s->passer_mode!=15) {
        NbaShotAction action={0};nba_shot_action_restore(&action,s->passer_group,s->active_group);
        s->passer_mode=action.mode;s->passer_timer=action.timer;s->behavior_timer=action.behavior_timer;
        s->flags=action.flags;s->status=action.status;
    }
    if((int16_t)(uint16_t)(s->live_state-0x81)<0)s->live_state=0;
    return true;
}

bool nba_tip_complete_acquisition(NbaTipCompletion *s) {
    /* `$86:D365-$D3B0`: first tip, initial inbound pickup, and completion
     * are distinct paths. A whistle may retain state, not pass ownership. */
    bool initial=s->transfer==0 && (int16_t)s->receiver<0;
    if(initial && s->live_state==0x81)return true;
    if(!(initial && s->live_state==0x82)) {
        if(s->live_state==0x82 && (int16_t)(uint16_t)(s->play-6)<0)s->request=1;
        if(s->whistle==0)s->live_state=0;
    }
    s->passer=s->receiver=s->aux=0xffff;s->transfer=0;s->ball_vz=0;
    return false;
}
