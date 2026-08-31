#include "nba_human_pass.h"
#include "nba_gameplay_ai.h"
#include <stdbool.h>

/* Original ROM SHA256
 * 2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870.
 * Native boundaries and explicit initializer limit: human-pass-checkpoint.md. */
static bool negative_difference(uint16_t a,uint16_t b){
    return ((uint16_t)(a-b)&0x8000u)!=0;
}
uint16_t nba_human_pass_distance(int16_t dx,int16_t dy){
    uint16_t x=(uint16_t)dx,y=(uint16_t)dy;
    if(dx<0)x=(uint16_t)(0u-x);
    if(dy<0)y=(uint16_t)(0u-y);
    /* $F1D7/$F1E2/$F207 branch on wrapped CMP sign. The close-slope path
     * weights the minor axis by3/8, with native truncation and addition wrap;
     * replacing this with high+minor/4 changes neutral pass selection. */
    if(negative_difference(y,x)){
        uint16_t term=negative_difference(x,(uint16_t)(y<<1))?
            (uint16_t)((uint16_t)((y>>1)+y)>>2):(uint16_t)(y>>2);
        return (uint16_t)(x+term);
    }
    uint16_t term=negative_difference(y,(uint16_t)(x<<1))?
        (uint16_t)((uint16_t)((x>>1)+x)>>2):(uint16_t)(x>>2);
    return (uint16_t)(y+term);
}

NbaHumanPassSelection nba_human_pass_select(const NbaHumanPassInput *s){
    NbaHumanPassSelection out={NBA_HUMAN_PASS_INVALID,0,0xffffu,0xffffu,0x640u};
    if(!s)return out;
    /* $DF8A-$DF92 writes this even if no receiver can be selected. Bit$10
     * is the original acquisition veto, not an invented controller switch. */
    out.controller_tag_0944=(uint16_t)(s->controller_id_090e|0x10u);
    if(s->actor>=10u||s->direction>8u||(s->group_first!=0u&&s->group_first!=5u))return out;
    const NbaHumanPassActor *source=&s->actors[s->actor];
    for(uint16_t local=0;local<5u;++local){
        uint16_t slot=(uint16_t)(s->group_first+local);
        const NbaHumanPassActor *candidate=&s->actors[slot];
        if(slot==s->actor)continue;
        uint16_t distance;
        int16_t dx=(int16_t)(uint16_t)((uint16_t)candidate->x-(uint16_t)source->x);
        int16_t dy=(int16_t)(uint16_t)((uint16_t)candidate->y-(uint16_t)source->y);
        if(s->direction==8u){
            if(candidate->mode==8u)continue;
            bool better_in_suffix=false;
            /* $E0B5-$E0E2 restarts the preference scan at the CURRENT
             * candidate, not the first teammate. A better earlier teammate
             * therefore stops restricting later candidates. Keep that native
             * suffix behavior; do not turn this into a global preference.
             * Confirmed naturally in left capture entry17: accepted +8C161
             * then +8C351 around source281 (native events22/27). */
            for(uint16_t j=local;j<5u;++j){
                const NbaHumanPassActor *later=&s->actors[s->group_first+j];
                if(later->mode!=8u&&negative_difference(later->anchor_distance_8c,source->anchor_distance_8c)){
                    better_in_suffix=true;break;
                }
            }
            if(better_in_suffix&&negative_difference(source->anchor_distance_8c,candidate->anchor_distance_8c))continue;
            distance=nba_human_pass_distance(dx,dy);
            if(!negative_difference(distance,out.score))continue;
        }else{
            /* $DFBB-$DFBE is CMP7/BPL, not an unsigned mode>=7 test. */
            if(!negative_difference(candidate->mode,7u))continue;
            uint16_t direction=nba_gameplay_target_direction(dx,dy,&distance);
            if(direction!=8u){
                uint16_t delta=(uint16_t)((direction-s->direction)&7u);
                if(delta>=4u)delta=(uint16_t)(8u-delta);
                distance=(uint16_t)(distance+(delta<<8));
            }
            /* $E010-$E014 accepts equality: later directional ties win. */
            if(distance!=out.score&&!negative_difference(distance,out.score))continue;
        }
        out.score=distance;out.receiver_slot=slot;
        out.receiver_identity=(uint16_t)(local+s->context_group);
    }
    /* $E085-$E088 tests the score itself. Even a directional candidate
     * accepted at exactly$0640 does not enter the pass initializer. */
    out.route=out.score==0x640u?NBA_HUMAN_PASS_NO_RECEIVER:NBA_HUMAN_PASS_CONTINUE_INITIALIZER;
    return out;
}
