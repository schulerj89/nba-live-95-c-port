#include "nba_period_restart_v2.h"
#include <string.h>

typedef struct { int16_t x,y; uint16_t direction,unused; } Formation;
/* Actual ROM $80:D06A, D092, D0BA. The fourth word is fetched by neither
 * coordinate path; keep it visible as provenance, not a behavior input. */
static const Formation formation[3][5]={
 {{8,3,6,0},{-16,-83,0,4},{-24,80,4,2},{104,-56,7,8},{96,59,5,6}},
 {{110,9,6,0},{94,131,6,4},{-335,53,2,2},{-300,68,6,8},{-306,-73,0,6}},
 {{63,3,6,0},{51,146,6,4},{-387,56,2,2},{-324,72,6,8},{-336,-70,0,6}}
};

static uint16_t actor_pointer(unsigned actor) { return (uint16_t)(0x34ebu+actor*0x100u); }
static bool regulation(const NbaPeriodRestart *w) { return w->input.period>0u&&w->input.period<4u; }
static void mirror(Formation *f) {
    f->x=(int16_t)-f->x;f->y=(int16_t)-f->y;
    f->direction=(uint16_t)((f->direction-4u)&7u);
}
static void pair_before(NbaPeriodRestart *w,NbaPeriodRestartState *s) {
    Formation a,b;unsigned i=w->pair,j;
    if(regulation(w)) {
        bool swap,flip;unsigned period=w->input.period;
        if(w->input.tip_winner==5u){swap=period!=3u;flip=period!=2u;}
        else {swap=period==3u;flip=period==2u;}
        a=formation[swap?2:1][i];b=formation[swap?1:2][i];
        if(flip){mirror(&a);mirror(&b);} /* $DDFD-$DEA4 */
    }else {
        a=formation[0][i];
        /* $DDD8-$DDFA uses context0's sign only for opening/OT.
         * DDE7-DDED negates Y too; keeping Y was a port error, not a quirk. */
        if(w->input.anchor_x[0]>=0){a.x=(int16_t)-a.x;a.y=(int16_t)-a.y;a.direction=(uint16_t)((a.direction-4u)&7u);}
        b=a;mirror(&b); /* $DEDE-$DF24: opening also negates paired Y. */
    }
    for(j=0;j<2;j++) {
        unsigned actor=i+j*5u;NbaPeriodRestartActor *p=&s->actors[actor];Formation f=j?b:a;
        /* $E053 loops to DDA7, NOT DDA4: A is the incremented pair index.
         * Original +A6 therefore receives 0..4 for BOTH teams, not zero. */
        p->field_a6=(uint16_t)i; /* $DDA9/$DDAC */
        p->x=p->target_x=f.x;p->y=p->target_y=f.y;
        p->direction=p->requested_direction=p->movement_direction=f.direction;
        p->id=(uint16_t)actor;p->team_group=(uint16_t)(j*5u);
        p->team_context=(uint16_t)(j?0x476b:0x46eb); /* $DF27-$DF48 */
        /* $DF4B-$DFB1. Actor fractions +02/+06/+0A are NOT reset here. */
        p->z=0;p->velocity_x=p->velocity_y=p->velocity_z=0;
        p->speed=p->contact_inhibit=p->action_timer=p->behavior_timer=0;
        p->boost_timer=p->recovery_inhibit=p->behavior_flags=0;
        p->upper_phase=p->lower_phase=0;
        p->upper_accumulator=p->lower_accumulator=0;
        p->upper_lock=p->lower_lock=0;p->formation_timer=300;
    }
}
static void pair_after(NbaPeriodRestart *w,NbaPeriodRestartState *s) {
    unsigned i=w->pair,j;
    for(j=0;j<2;j++) {
        unsigned actor=i+j*5u;NbaPeriodRestartActor *p=&s->actors[actor];
        /* $DFED-$E020 occurs AFTER the two appearance calls. */
        p->focal_distance=(uint16_t)(i?120:0);p->mode=(uint16_t)(i?2:4);
        p->list_link=s->list_cursor;s->object_list[2u*i+j]=actor_pointer(actor);
        s->list_cursor=(uint16_t)(s->list_cursor+2u); /* $E023-$E03D */
    }
}
static void ball_initialize(NbaPeriodRestartState *s) {
    NbaPeriodRestartBall *b=&s->ball;
    b->list_link=s->list_cursor;s->object_list[10]=0x3eeb;
    s->list_cursor=(uint16_t)(s->list_cursor+2u);s->object_list[11]=0;
    s->list_flag_08fe=0;s->ball_pointer_0910=0x3eeb;
    b->x_fraction=b->y_fraction=b->z_fraction=0;b->x=b->y=0;b->z=80;
    b->velocity_x=b->velocity_y=0;b->velocity_z=600;
    b->team_group=0xffff;b->id=10;
    s->context_4933=s->context_4935=s->event_08f0=0xffff; /* E056..E0A9 */
}
static void cancellation(NbaPeriodRestartState *s) {
    s->live_0936=0;s->receiver_0946=s->pass_actor_0942=s->pass_word_0944=0xffff;
    s->pass_word_094a=s->activity_0948=s->transfer_09b8=0; /* A60D..A625 */
}
static void ownership_reset(NbaPeriodRestartState *s) {
    s->owner_093e=s->camera_093a=s->last_side_093c=s->last_actor_097e=0xffff;
    s->owner_pointer_0940=0; /* E0C7 -> $87:A9D0, signed owner negative */
    s->ball_pointer_0910=0x3eeb;
    /* Original period $87:9797 jumps to8C86, bypassing the DA18 bulk clear.
     * Preserve stale $09BA and $09B0/$09B2; C37D does not clear them either. */
}
static void target(NbaPeriodRestart *w,NbaPeriodRestartState *s) {
    unsigned side=s->side_0952/5u,actor=side*5u+2u;
    bool positive=w->input.anchor_x[side]>=0;
    s->actor_0954=(uint16_t)actor; /* $C37D-$C388 */
    s->target_x_0958=(int16_t)(positive?-394:394);
    s->target_y_095a=(int16_t)(positive?64:-64);
    s->direction_095c=(uint16_t)(positive?2:6); /* $C548-$C575 */
    /* These two layout0 points have opposite X sign from the selected anchor
     * and Y in [-72,72): C5C1 takes C5D6, never the random C5ED path.
     * C579's corner clamp leaves both points unchanged. */
    s->play_0996=s->play_request_0994=1;
    s->actors[actor].target_x=s->target_x_0958;
    s->actors[actor].target_y=s->target_y_095a; /* $C5AD-$C5BD */
}
static void attach(NbaPeriodRestartState *s) {
    NbaPeriodRestartActor *p=&s->actors[s->actor_0954];NbaPeriodRestartBall *b=&s->ball;
    s->live_0936=0x82;s->timer_092e=300;
    b->x=p->x;b->y=p->y;b->x_fraction=b->y_fraction=b->z_fraction=0;
    b->z=24;b->velocity_x=b->velocity_y=b->velocity_z=0;
    s->attachment_0968=s->attachment_09f6=24;
    s->owner_093e=s->actor_0954;p->mode=11;s->camera_093a=s->side_0952;
    s->owner_pointer_0940=actor_pointer(s->actor_0954); /* E17F -> A9D0 */
}
static NbaPeriodRestartBoundary boundary(NbaPeriodRestart *w,NbaPeriodRestartBoundaryKind kind,uint32_t pc,uint32_t child,unsigned actor) {
    w->boundary.kind=kind;w->boundary.source_pc=pc;w->boundary.child_pc=child;
    w->boundary.actor=(uint16_t)actor;w->waiting=true;return w->boundary;
}
bool nba_period_restart_begin(NbaPeriodRestart *w,const NbaPeriodRestartInput *input) {
    if(!w||!input||input->period>4u||(input->tip_winner!=0u&&input->tip_winner!=5u))return false;
    memset(w,0,sizeof(*w));w->input=*input;w->valid=true;return true;
}
NbaPeriodRestartBoundary nba_period_restart_advance(NbaPeriodRestart *w,NbaPeriodRestartState *s) {
    NbaPeriodRestartBoundary invalid={NBA_PERIOD_INVALID,0,0,UINT16_MAX};
    if(!w||!s||!w->valid)return invalid;if(w->waiting)return w->boundary;
    for(;;)switch(w->phase) {
    case 0:
        if(!w->pair)s->list_cursor=0x34d3; /* DD89 parent precondition */
        pair_before(w,s);w->phase=1;
        return boundary(w,NBA_PERIOD_APPEARANCE,0x86dfcb,0x87aab2,w->pair);
    case 1:w->phase=2;return boundary(w,NBA_PERIOD_APPEARANCE,0x86dfd8,0x87aab2,w->pair+5u);
    case 2:
        pair_after(w,s);w->pair++;
        if(w->pair<5u){w->phase=0;break;}
        w->phase=3;return boundary(w,NBA_PERIOD_BALL_BEFORE,0x86e056,0,UINT16_MAX);
    case 3:ball_initialize(s);w->phase=4;
        return boundary(w,NBA_PERIOD_APPEARANCE_GEOMETRY,0x86e0ac,0x86d85e,UINT16_MAX);
    case 4:w->phase=5;return boundary(w,NBA_PERIOD_OBJECT_SORT,0x86e0b0,0x86d5db,UINT16_MAX);
    case 5:w->phase=6;return boundary(w,NBA_PERIOD_CANCEL_BEFORE,0x86e0b4,0x86a60d,UINT16_MAX);
    case 6:cancellation(s);w->phase=7;return boundary(w,NBA_PERIOD_CANCEL_AFTER,0x86e0b8,0,UINT16_MAX);
    case 7:
        ownership_reset(s);
        if(!regulation(w)){s->live_0936=0x81;w->phase=10;return boundary(w,NBA_PERIOD_OPENING,0x86e1ac,0,UINT16_MAX);}
        s->side_0952=(uint16_t)(w->input.tip_winner^(w->input.period==3u?0u:5u));s->layout_0956=0;w->phase=8;
        return boundary(w,NBA_PERIOD_TARGET_BEFORE,0x86e102,0x85c37d,s->side_0952+2u);
    case 8:target(w,s);w->phase=9;return boundary(w,NBA_PERIOD_TARGET_AFTER,0x86e106,0,UINT16_MAX);
    case 9:attach(s);w->phase=10;return boundary(w,NBA_PERIOD_CONTROLLER,0x86e183,0x86bc9b,s->actor_0954);
    default:return boundary(w,NBA_PERIOD_DONE,0,0,UINT16_MAX);
    }
}
bool nba_period_restart_resume(NbaPeriodRestart *w) {
    if(!w||!w->valid||!w->waiting||w->phase>=10u)return false;
    w->waiting=false;return true;
}
