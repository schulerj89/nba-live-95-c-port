#include "nba_owner_flow.h"
#include "nba_gameplay_ai.h"

/* $86:F34F-$F439. Ordinary owner caller, not the F43A inbound continuation.
 * Keep 16-bit wrap and signed N tests; callbacks preserve native call order. */
NbaOwnerFlowResult nba_owner_flow_run(NbaOwnerFlow *s,
                                     NbaOwnerFlowCall call,void *context) {
    if(!s || !call)return NBA_OWNER_FLOW_INVALID;
    if((int16_t)s->owner_093e>=0)s->veto_09f8=0;
    if(s->shooting_09bc && s->deferred_0a02)s->deferred_0a02=2;
    s->owner_team_09f4=s->team_6e;
    if((int16_t)(uint16_t)(s->play_0996-6u)<0 &&
       s->owner_093e!=s->inbound_actor_0954 && !s->transfer_09b8)s->request_0994=1;
    /* F38A-F3B6: both components stop, including a stationary owner's
     * preserved facing when F02D returns direction8. */
    if(s->dead_0968 && s->attached_09f6>=2) {
        uint8_t direction=nba_gameplay_target_direction((int16_t)s->vx_0e,(int16_t)s->vy_10,0);
        if(direction!=8)s->facing_4e=direction;
        s->vx_0e=s->vy_10=0;
    }
    /* F3B7: base pairing +74 indexes the ten actor-pointer table. */
    if((int16_t)s->pair_74>=0) {
        if(s->pair_74>=20 || (s->pair_74&1))return NBA_OWNER_FLOW_INVALID;
        if(!call(context,s,NBA_OWNER_CALL_POSE,s->pair_74/2))return NBA_OWNER_FLOW_INVALID;
    }
    if(s->actor_id!=s->owner_093e) {
        s->mode_5e=1;s->behavior_64=0x2f;s->timer_60=0;s->flags_7e=0;
        return NBA_OWNER_FLOW_LOST;
    }
    if(s->live_0936==0x82)return NBA_OWNER_FLOW_INBOUND;
    s->offense_093a=s->team_6e;
    uint16_t remainder=(uint16_t)(s->timer_60-s->delta_c8);
    if((int16_t)remainder>0) {
        s->timer_60=remainder;return NBA_OWNER_FLOW_HOLD;
    }
    s->timer_60=(uint16_t)(remainder+0x40u+(s->rating_3f&0xffu));
    if((int16_t)s->controller_16<0) {
        if(!call(context,s,NBA_OWNER_CALL_CPU,0))return NBA_OWNER_FLOW_ESCAPED;
        if(!s->recovery_7a && !call(context,s,NBA_OWNER_CALL_FORMATION,0))return NBA_OWNER_FLOW_INVALID;
        if(!call(context,s,NBA_OWNER_CALL_RECEIVER,0))return NBA_OWNER_FLOW_INVALID;
    }
    return NBA_OWNER_FLOW_RETURN;
}
