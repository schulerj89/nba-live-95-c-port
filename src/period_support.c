#include "period_support.h"

bool nba_period_assignment(const NbaAssetPack *assets,
    const NbaPeriodAssignmentInput *input, NbaPeriodAssignment *state) {
    if (!input || !state) return false;
    uint8_t teams[10], roster[10];
    for (unsigned side=0;side<2;side++) {
        unsigned used=0;
        for (unsigned slot=0;slot<5;slot++) {
            unsigned i=side*5+slot, selector=input->selector[i]&0x7fu;
            if (selector>=5 || (used&(1u<<selector)) || input->roster[i]>=12)
                return false;
            used|=1u<<selector;teams[i]=input->team[side];roster[i]=input->roster[i];
        }
    }
    NbaPlayerAppearanceSetup appearance;
    if (!nba_player_appearance_setup(assets,teams,roster,&appearance))return false;
    NbaPlayerActiveAppearanceInput in={0};
    NbaPeriodAssignment next=*state;
    for (unsigned i=0;i<10;i++) {
        unsigned paired=(i<5?5u:0u)+(input->selector[i]&0x7fu);
        in.lineup_selector[i]=input->selector[i];
        in.upper_variant[i]=(uint8_t)appearance.players[i].upper_variant;
        if (!nba_player_gameplay_shot_ratings(assets,teams[paired],roster[paired],
                &in.appearance_a[i],&in.appearance_b[i]) ||
            !nba_player_gameplay_roster_address(assets,teams[i],roster[i],
                &next.roster_pointer[i]))return false;
        next.statistic_pointer[i]=(uint16_t)(0x40ebu+0x40u*(roster[i]+(i<5?0u:12u)));
    }
    NbaPlayerActiveAppearance out;
    if (!nba_player_build_active_appearance(&in,&out))return false;
    for (unsigned i=0;i<10;i++) {
        next.actor[i].variant=out.upper_variant[i];
        next.actor[i].current=next.actor[i].base=out.assignment_base[i];
        next.actor[i].alternate=out.assignment_alternate[i];
        /* D939/D9C6 writes help through the PAIRED actor, not the current
         * lineup actor. Do not replace this source-selected destination. */
        next.actor[out.assignment_base[i]/2u].help=
            (uint16_t)((input->selector[i]&0x80u)!=0);
    }
    for (unsigned side=0;side<2;side++)for (unsigned rank=0;rank<5;rank++) {
        uint16_t offset=out.sorted_actor_offset[side][rank];
        next.order[side][rank]=(uint8_t)offset;
        next.actor[offset/2u].role=(uint16_t)(4u-rank);
        /* Both D73E calls reuse 09DA: only the second sorted list survives. */
        if(side==1){next.keys[2*rank]=out.sorted_key[side][rank];next.keys[2*rank+1]=offset;}
    }
    *state=next;return true;
}

static bool object_index(uint16_t pointer,unsigned *index) {
    if(pointer<0x34eb || pointer>0x3eeb || ((pointer-0x34eb)&0xffu))return false;
    *index=(pointer-0x34eb)/256u;return true;
}
bool nba_period_object_sort(NbaPeriodObjectSort *state) {
    if(!state || state->object[11])return false;
    NbaPeriodObjectSort next=*state;unsigned seen=0;
    for(unsigned i=0;i<11;i++) {
        unsigned index;if(!object_index(next.object[i],&index) || (seen&(1u<<index)))return false;
        seen|=1u<<index;
    }
    for(unsigned i=1;i<11;i++) {
        unsigned at=i,index;object_index(next.object[at],&index);
        while(at>0) {
            unsigned previous;object_index(next.object[at-1],&previous);
            uint16_t difference=(uint16_t)((uint16_t)next.x[previous]-(uint16_t)next.x[index]);
            if(!difference || (difference&0x8000u))break;
            uint16_t pointer=next.object[at];next.object[at]=next.object[at-1];next.object[at-1]=pointer;
            next.link[previous]=(uint16_t)(0x34d3u+at*2u);
            next.link[index]=(uint16_t)(0x34d3u+(at-1)*2u);at--;
        }
    }
    *state=next;return true;
}

bool nba_period_attachment(const NbaAssetPack *assets,NbaPeriodAttachment *state) {
    if(!state || state->actor>=10 || state->owner!=state->actor ||
        state->group!=(state->actor<5?0u:5u) || state->facing>=8)return false;
    NbaPeriodAttachment next=*state;uint16_t request=12;
    if(!nba_controller_transfer(&next.controllers,next.actor,next.group) ||
       !nba_player_animation_command(assets,&next.channels,NBA_ANIMATION_CANCEL_UPPER,
                                     &request,next.boost!=0,next.alternate_lower!=0) ||
       !nba_player_animation_command(assets,&next.channels,NBA_ANIMATION_CANCEL_LOWER,
                                     &request,next.boost!=0,next.alternate_lower!=0) ||
       !nba_player_animation_command_scratch(assets,&next.channels,NBA_ANIMATION_INSTALL_BOTH,
                                     &request,next.boost!=0,next.alternate_lower!=0,&next.scratch_47) ||
       !nba_player_resolve_pose(assets,&next.channels,next.facing,
                               next.alternate_lower!=0,next.variant,&next.pose))return false;
    int16_t dx,dy,dz;
    if(!nba_player_ball_attachment_offsets(assets,next.pose.upper_resource,
        next.pose.lower_resource,next.pose.mirror_flags,&dx,&dy,&dz))return false;
    next.previous_ball_x=(uint16_t)next.ball_x;
    /* B649/B66A replace only integer words. Ball fractions and all velocity
     * words remain owned by the preceding parent, not this attachment. */
    next.ball_x=(int16_t)((uint16_t)next.x+(uint16_t)dx);
    next.ball_y=(int16_t)((uint16_t)next.y+(uint16_t)dy);
    next.ball_z=(int16_t)((uint16_t)next.z+(uint16_t)dz);
    *state=next;return true;
}
