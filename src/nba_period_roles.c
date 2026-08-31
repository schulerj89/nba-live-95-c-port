#include "nba_period_roles.h"
#include <string.h>

static bool negative(uint16_t a){return (a&0x8000u)!=0u;}
static uint16_t sub(uint16_t a,uint16_t b){return (uint16_t)(a-b);}
static uint16_t neg(uint16_t a){return (uint16_t)(0u-a);}
static uint16_t actor_pointer(unsigned actor){return (uint16_t)(0x34ebu+actor*0x100u);}

/* $85:F347..F3BA, with actualF09Abyte table. CMP/BPL below tests the N bit
 * of wrapped subtraction, not C signed comparisons of the two operands.
 * This preserves0x8000negation, ASLoverflow and original diagonal boundaries.
 * F37F's equality path even gives(0,1) direction1/distance0; do not smooth it. */
static void pair_geometry(NbaPeriodRoleState *s){
    static const uint8_t direction_map[16]={0,1,2,1,4,3,2,3,0,7,6,7,4,5,6,5};
    uint16_t a=s->dx_00aa,b=s->dy_00ae,key=0,t,difference;
    if((a|b)==0u){s->dx_00aa=0;s->direction_00b2=8;return;}
    if(negative(a)){a=neg(a);key|=8u;}
    if(negative(b)){b=neg(b);key|=4u;}
    difference=sub(sub(b,1),a); /* F37A..F381: equal OR negative enters swap */
    if(difference==0u||negative(difference)){t=a;a=b;b=t;key|=2u;}
    a=(uint16_t)(a<<1); /* F394 ASL dp, truncates before subsequentLSR */
    if(negative(sub(sub(b,1),a)))key|=1u;
    s->direction_00b2=direction_map[key];
    s->dx_00aa=(uint16_t)((a>>3)+b);s->dy_00ae=b;
}

static void focal_geometry(NbaPeriodRoleState *s,NbaPeriodRoleActor *a){
    uint16_t x=sub((uint16_t)a->x,s->focal_x_00b6);
    uint16_t y=sub((uint16_t)a->y,s->focal_y_00ba),t;
    if(negative(x))x=neg(x);if(negative(y))y=neg(y);
    /* BCA4..BCB2 uses CMP/BPL; 8000 is not normalized to a positive int. */
    if(negative(sub(x,y))){t=x;x=y;y=t;}
    y>>=2;s->dx_00aa=(uint16_t)(x+y);s->dy_00ae=y;
    a->focal_distance_8e=s->dx_00aa;
}

static NbaPeriodRoleKind initial_scan(NbaPeriodRoles *w,NbaPeriodRoleState *s){
    unsigned entry=w->calls,defense=entry^1u,i;
    s->context_009e=(uint16_t)(entry?0x476b:0x46eb); /* E1E5/E1EE */
    s->entry_context_00a6=s->context_009e; /* BC07..BC0E */
    s->context_009e=s->contexts[entry].opponent_02;
    if(!negative(s->owner_093e)){
        /* BC15 dereferences0910, NOT the owner actor's integerXY. */
        s->object_008e=s->ball_pointer_0910;
        s->focal_x_00b6=(uint16_t)s->ball_x;s->focal_y_00ba=(uint16_t)s->ball_y;
    }else{
        s->focal_x_00b6=(uint16_t)s->predicted_x_0918;
        s->focal_y_00ba=(uint16_t)s->predicted_y_091a;
    }
    s->actor_0096=s->contexts[defense].first_actor_04;
    s->counter_00c2=4;s->best_00be=0x7fff;
    for(i=0;i<5;i++){
        unsigned index=defense*5u+i;NbaPeriodRoleActor *a=&s->actors[index];
        if(!negative(a->assignment_74)){
            unsigned paired=a->assignment_74/2u;NbaPeriodRoleActor *b=&s->actors[paired];
            s->pair_009a=actor_pointer(paired); /* BC49:879C7Btable, even0..18 */
            s->dx_00aa=sub((uint16_t)b->x,(uint16_t)a->x);
            s->dy_00ae=sub((uint16_t)b->y,(uint16_t)a->y);
            pair_geometry(s); /* BC64->F34F, no external result adapter */
            a->pair_distance_8a=s->dx_00aa;b->pair_distance_8a=s->dx_00aa;
            if(s->direction_00b2!=8u){
                a->direction_86=s->direction_00b2;
                b->direction_86=(uint16_t)(s->direction_00b2^4u);
            } /* BC79direction8 deliberately preservesbotholdwords */
        }
        focal_geometry(s,a);
        if(negative(sub(s->dx_00aa,s->best_00be))){
            s->nearest_0092=s->actor_0096;s->best_00be=s->dx_00aa;
        } /* otherwise carried92 survives, including a no-winner scan */
        s->actor_0096=(uint16_t)(s->actor_0096+0x100u);
        s->counter_00c2=(uint16_t)(s->counter_00c2-1u);
    }
    s->nearest_09da=s->nearest_0092; /* BCDF..BCE1 before every cadence gate */
    if(s->rebuild_09d6!=0u){s->cadence_09d2=30;return NBA_PERIOD_ROLES_REBUILD;}
    s->cadence_09d2=sub(s->cadence_09d2,s->delta_00c6); /* BCEDactualcarriedC6 */
    if(negative(s->cadence_09d2)){
        s->cadence_09d2=(uint16_t)(s->cadence_09d2+30u);
        if(!negative(s->camera_093a))return NBA_PERIOD_ROLES_PLANNER;
    }
    return NBA_PERIOD_ROLES_FIRST_RETURN; /* BD06; no9D6clear on this path */
}

bool nba_period_roles_begin(NbaPeriodRoles *w,const NbaPeriodRoleState *s){
    unsigned i;
    if(!w||!s)return false;
    if(s->contexts[0].opponent_02!=0x476b||s->contexts[1].opponent_02!=0x46eb||
       s->contexts[0].first_actor_04!=0x34eb||s->contexts[1].first_actor_04!=0x39eb)return false;
    if(!negative(s->owner_093e)&&s->ball_pointer_0910!=0x3eeb)return false;
    for(i=0;i<10;i++)if(!negative(s->actors[i].assignment_74)&&
       ((s->actors[i].assignment_74&1u)!=0u||s->actors[i].assignment_74>18u))return false;
    memset(w,0,sizeof(*w));w->valid=true;return true;
}
NbaPeriodRoleBoundary nba_period_roles_advance(NbaPeriodRoles *w,NbaPeriodRoleState *s){
    NbaPeriodRoleBoundary b={NBA_PERIOD_ROLES_INVALID,0,0};
    if(!w||!s||!w->valid)return b;if(w->waiting)return w->boundary;
    b.kind=initial_scan(w,s);
    if(b.kind==NBA_PERIOD_ROLES_REBUILD)b.source_pc=0x85bd0d;
    else if(b.kind==NBA_PERIOD_ROLES_PLANNER)b.source_pc=0x85be06;
    else{
        w->calls++;b.source_pc=w->calls==1u?0x86e1ee:0x86e1f7;
        if(w->calls==2u)b.kind=NBA_PERIOD_ROLES_COMPLETE;
    }
    b.completed_calls=w->calls;w->boundary=b;w->waiting=true;return b;
}
bool nba_period_roles_resume(NbaPeriodRoles *w){
    if(!w||!w->valid||!w->waiting||w->boundary.kind!=NBA_PERIOD_ROLES_FIRST_RETURN)return false;
    w->waiting=false;return true;
}
