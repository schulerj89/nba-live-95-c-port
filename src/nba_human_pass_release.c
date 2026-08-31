#include "nba_human_pass_release.h"

static bool negative(uint16_t value) { return (value & 0x8000u)!=0; }
static bool cmp_negative(uint16_t a,uint16_t b) { return negative((uint16_t)(a-b)); }

bool nba_human_pass_release_dispatch(NbaHumanPassReleaseState *s) {
    if(!s || s->pose.actor.mode_5e!=15)return false;
    s->pointer_8e=0x9c53;s->bank_90=0x0087;return true;
}

void nba_human_pass_release_turn(NbaHumanPassReleaseState *s) {
    s->scratch_ae=s->pose.actor.facing_4e;
    s->scratch_aa=(uint16_t)(s->scratch_ae-s->direction_66);
    /* A7B5 tests the unmasked subtraction. A nonzero multiple of8 still
     * turns atA7C1 after AND7 produces0. Preserve this original quirk;
     * ordinary captures do not establish reachability of that edge case. */
    if(s->scratch_aa) {
        s->scratch_aa &=7;
        s->scratch_ae=(uint16_t)((s->scratch_ae+(s->scratch_aa<4?0xffffu:1u))&7u);
        s->pose.actor.facing_4e=s->scratch_ae;
    }
}

static void clear_action(NbaHumanPassReleaseState *s) {
    /* Shared9861 entry does not assign mode or behavior+64. */
    s->timer_60=0;s->pose.actor.flags_7e=0;s->pose.actor.flags_28=0;
}
void nba_human_pass_release_normalize(NbaHumanPassReleaseState *s) {
    s->pose.actor.mode_5e=s->group_6e==s->offense_093a?1:2;
    s->behavior_64=0x2f;clear_action(s);
}

NbaHumanPassReleaseBoundary nba_human_pass_release_step(const NbaAssetPack *assets,
        const NbaHumanPassReleaseTables *tables,NbaHumanPassReleaseState *state) {
    if(!state || !tables || state->actor_index_c2>=10)return NBA_PASS_RELEASE_INVALID;
    NbaHumanPassReleaseState s=*state;
    NbaHumanPassReleaseBoundary result;
    if(s.owner_093e!=s.actor_index_c2) {
        s.timer_60=(uint16_t)(s.timer_60-s.delta_c6);
        if(negative(s.timer_60)) {
            nba_human_pass_release_normalize(&s);result=NBA_PASS_RELEASE_NORMALIZED;
        } else result=NBA_PASS_RELEASE_TIMER;
    } else {
        uint16_t family=s.family_c0;
        /* A6D0 BMI is the sign of wrapped CMP, not signed-operand ordering. */
        if(!cmp_negative(family,2) && family<5) {
            if(family==2) { *state=s;return NBA_PASS_RELEASE_AIRBORNE_A629; }
            if(family!=4) {
                s.family_c0=2;*state=s;return NBA_PASS_RELEASE_POSE_A6F8;
            }
        } else {
            if(!cmp_negative(family,5))nba_human_pass_release_turn(&s);
            if(negative(s.controller_16) && s.pose.live_0936!=0x82) {
                *state=s;return NBA_PASS_RELEASE_STEER_AD6B;
            }
            s.pointer_8e=0x3eeb;
            uint16_t upper=s.pose.actor.upper_30;
            if(upper>=0x2a && upper<0x32 &&
                    tables->thresholds_a7a0[upper-0x2a]>=s.pose.actor.phase_3a) {
                /* A790 uses existing pose resources. It does not resolve a
                 * new animation pose; B649 ownsXY, A79C separately ownsZ. */
                if(!nba_human_pass_pose_attach(assets,&s.pose))return NBA_PASS_RELEASE_INVALID;
                s.pose.ball_z=(uint16_t)(s.pose.actor.z+s.pose.scratch_04);
                *state=s;return NBA_PASS_RELEASE_ATTACHED;
            }
        }
        if(negative(s.receiver_0946)) {
            clear_action(&s);s.pose.actor.mode_5e=11;
            s.source_0942=s.source_0944=s.receiver_0946=0xffff;
            result=NBA_PASS_RELEASE_CANCELLED;
        } else {
            /* A755 indexes the real879C7B pointer table. This bounded API
             * rejects out-of-table positive receivers, rather than assuming
             * a host slot or pretending native out-of-range reads are valid. */
            if(s.receiver_0946>=10)return NBA_PASS_RELEASE_INVALID;
            s.flag_09c4=0;s.scratch_aa=s.receiver_0946;
            s.pointer_8e=tables->actor_pointers_9c7b[s.receiver_0946];
            result=NBA_PASS_RELEASE_LAUNCH_99C4;
        }
    }
    *state=s;return result;
}

void nba_human_pass_release_after_launch(NbaHumanPassReleaseState *s) {
    /* A764 CMP4 / A767 BPL retains the wrapped subtraction sign. */
    if(cmp_negative(s->family_c0,4))s->timer_60=10;
}
