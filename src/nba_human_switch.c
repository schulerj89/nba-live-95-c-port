#include "nba_human_switch.h"
#include "nba_gameplay_ai.h"

/* Original ROM SHA256
 * 2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870.
 * Source and natural-call evidence: docs/human-switch-checkpoint.md. */
static bool negative_difference(uint16_t a, uint16_t b) {
    return ((uint16_t)(a-b)&0x8000u)!=0;
}

static uint16_t neutral_distance(int16_t dx, int16_t dy) {
    uint16_t x=(uint16_t)dx,y=(uint16_t)dy;
    if(dx<0)x=(uint16_t)(0u-x);
    if(dy<0)y=(uint16_t)(0u-y);
    /* $E26D-$E27B uses the sign of a wrapped CMP, not unsigned max().
     * Preserve that native edge behavior even for magnitude $8000. */
    if(negative_difference(x,y)){uint16_t tmp=x;x=y;y=tmp;}
    return (uint16_t)(x+(y>>2));
}

NbaHumanSwitchRoute nba_human_switch_control(NbaHumanSwitchState *s) {
    if(!s)return NBA_HUMAN_SWITCH_INVALID;
    /* $E143-$E14B: this unsigned count gate precedes all other reads. */
    if(s->context_controller_count>=5u)return NBA_HUMAN_SWITCH_ALL_CONTROLLED;
    if(s->actor>=10u || s->controller_090c>=5u || s->direction>8u ||
       (s->group_first!=0u&&s->group_first!=5u) ||
       s->group_end!=s->group_first+5u)
        return NBA_HUMAN_SWITCH_INVALID;
    uint16_t chosen=s->fallback_actor, score=0x640u;
    uint16_t local_index=0;
    /* Native C2 is saved on entry but not initialized before the scan. If
     * A6 remains stale, its transfer index likewise comes from incoming C2. */
    uint16_t chosen_local=s->incoming_index_c2;
    const NbaHumanSwitchActor *current=&s->actors[s->actor];
    int16_t anchor_x=s->direction==8u?s->neutral_anchor_x:current->x;
    int16_t anchor_y=s->direction==8u?s->neutral_anchor_y:current->y;
    for(uint16_t i=s->group_first;i<s->group_end;++i,++local_index){
        if(i!=s->actor&&s->controllers.actor_assignment[i]>=0)continue;
        if(s->direction!=8u&&i==s->actor)continue;
        int16_t dx=(int16_t)(uint16_t)((uint16_t)s->actors[i].x-(uint16_t)anchor_x);
        int16_t dy=(int16_t)(uint16_t)((uint16_t)s->actors[i].y-(uint16_t)anchor_y);
        uint16_t distance;
        if(s->direction==8u){
            distance=neutral_distance(dx,dy);
            /* $E288-$E28A strictly improves: equal scores retain earlier. */
            if(!negative_difference(distance,score))continue;
        }else{
            uint16_t direction=nba_gameplay_target_direction(dx,dy,&distance);
            if(direction!=8u){
                uint16_t delta=(uint16_t)((direction-s->direction)&7u);
                if(delta>=4u)delta=(uint16_t)(8u-delta);
                distance=(uint16_t)(distance+(delta<<6));
            }
            /* $E1D3-$E1D7 explicitly accepts equality: the later candidate
             * wins directional ties, unlike the neutral scan. */
            if(distance!=score&&!negative_difference(distance,score))continue;
        }
        score=distance;chosen=i;chosen_local=local_index;
    }
    /* $E1F6 does not test whether a candidate was found. Preserve incoming
     * A6 on the no-improvement path; reject unmapped pointers at the host
     * boundary instead of inventing a safer in-game selection. */
    if(chosen==s->actor)return NBA_HUMAN_SWITCH_RETAIN;
    if(chosen>=10u)return NBA_HUMAN_SWITCH_INVALID;
    s->controllers.actor_assignment[chosen]=s->controllers.actor_assignment[s->actor];
    s->controllers.actor_assignment[s->actor]=-1;
    /* $E201 reads source actor+6E, not context+0C. $E214 targets published
     * controller090C, not a round-robin pad chosen by $86:BC9B. */
    s->controllers.record[s->controller_090c].actor=(uint16_t)(chosen_local+current->group);
    return NBA_HUMAN_SWITCH_TRANSFER;
}
