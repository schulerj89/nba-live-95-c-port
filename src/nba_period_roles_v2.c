#include "nba_period_roles_v2.h"
#include <string.h>
static bool n(uint16_t a){return (a&0x8000u)!=0;}
static uint16_t minus(uint16_t a,uint16_t b){return (uint16_t)(a-b);}
static uint16_t ptr(unsigned i){return (uint16_t)(0x34ebu+256u*i);}
static int actor(uint16_t p){return p>=0x34ebu&&p<=0x3debu&&((p-0x34ebu)&255u)==0u?(int)((p-0x34ebu)/256u):-1;}
/* B971/B993, BE25/BE42: wrapped abs and CMP/N, NOT widened abs/max. */
static void distance(NbaPeriodRoleState *s,uint16_t x,uint16_t y){
 uint16_t t;if(n(x))x=minus(0,x);if(n(y))y=minus(0,y);
 if(n(minus(x,y))){t=x;x=y;y=t;}y>>=2;s->dx_00aa=(uint16_t)(x+y);s->dy_00ae=y;
}
static void reaction(NbaPeriodRoleStateV2 *s,unsigned i){
 NbaPeriodRoleState *p=&s->prefix;NbaPeriodRoleExtraActor *a=&s->actors[i];uint16_t old;
 a->clear_7e=0; /* B95E: also happens on the inbound-actor early return. */
 if(s->live_0936==0x82&&a->id_00==s->inbound_0954)return; /* B969 preserves60/RNG */
 distance(p,minus((uint16_t)p->actors[i].x,(uint16_t)p->ball_x),minus((uint16_t)p->actors[i].y,(uint16_t)p->ball_y));
 p->dy_00ae=p->dx_00aa;
 /* 80CEE7..CEFC, shared07F6, zero recovery is part of the original RNG. */
 old=s->rng_07f6;s->rng_07f6=old?(uint16_t)((uint16_t)(old<<1)^((old&0x8000u)?0x1d87u:0u)):0x9146u;
 p->dx_00aa=(uint16_t)((s->rng_07f6&0x78u)+p->dy_00ae);
 if(!n(minus(p->dx_00aa,0x96u)))p->dx_00aa=0x96; /* B9C0 CMP/BMI, wrapped */
 a->reaction_60=p->dx_00aa;
}
static void rebuild(NbaPeriodRoleStateV2 *s,unsigned entry){
 NbaPeriodRoleState *p=&s->prefix;unsigned pass,j,team;
 /* BD19 copies78 first; BD55/BDB2 copy76 in opponent,entry order.
  * Do not collapse these passes: B95C consumes shared RNG on each call. */
 for(pass=0;pass<3;pass++){
  team=pass==1?(entry^1u):entry;p->pair_009a=ptr(team*5);p->counter_00c2=4;
  for(j=0;j<5;j++){
   unsigned i=team*5+j;NbaPeriodRoleExtraActor *a=&s->actors[i];
   if(pass==0){
    p->actors[i].assignment_74=a->alternate_78;
    if(n(minus(a->mode_5e,7))&&a->id_00!=p->owner_093e){a->behavior_64=0x2f;p->actor_0096=p->pair_009a;reaction(s,i);}
   }else{
    if(n(minus(a->mode_5e,7)))a->mode_5e=s->live_0936==0x82&&a->team_6e==s->side_0952?1:2;
    a->saved_84=a->mode_5e;p->actors[i].assignment_74=a->base_76;
    if(n(minus(a->mode_5e,7))){a->behavior_64=0;p->actor_0096=p->pair_009a;reaction(s,i);}
   }
   p->pair_009a=(uint16_t)(p->pair_009a+256u);p->counter_00c2=minus(p->counter_00c2,1);
  }
 }
 p->rebuild_09d6=0; /* BE03, ONLY after all three real child passes. */
}
static NbaPeriodRoleBoundaryV2 stop(NbaPeriodRoleKindV2 k,uint32_t pc,uint16_t pointer){
 NbaPeriodRoleBoundaryV2 b={NBA_PERIOD_ROLES_V2_INVALID,0,0,0};b.kind=k;b.source_pc=pc;b.record_pointer=pointer;return b;
}
static NbaPeriodRoleBoundaryV2 planner(NbaPeriodRoleStateV2 *s,unsigned entry){
 NbaPeriodRoleState *p=&s->prefix;unsigned team=entry^1u,j;int index;uint16_t assignment;
 p->object_008e=ptr(team*5);p->focal_y_00ba=s->contexts[entry].anchor_0a;
 p->counter_00c2=4;p->best_00be=0x7fff;p->focal_x_00b6=0xffff;
 for(j=0;j<5;j++){
  unsigned i=team*5+j;distance(p,minus((uint16_t)p->actors[i].x,p->focal_y_00ba),(uint16_t)p->actors[i].y);
  s->actors[i].anchor_distance_8c=p->dx_00aa;
  if(n(minus(p->dx_00aa,p->best_00be))){p->nearest_0092=p->object_008e;p->best_00be=p->dx_00aa;}
  /* BE73/BE76 for81/82 skips live-play mode and assignment cleanup. */
  p->object_008e=(uint16_t)(p->object_008e+256u);p->counter_00c2=minus(p->counter_00c2,1);
 }
 s->nearest_anchor_09e2=p->nearest_0092;p->object_008e=p->ball_pointer_0910;
 if(!n(minus(s->ball_anchor_distance_8c,0xf0))){
  p->pair_009a=s->nearest_offense_09de;index=actor(p->pair_009a);
  if(index<0)return stop(NBA_PERIOD_ROLES_V2_RECORD_READ,0x85bf51,p->pair_009a);
  if(n(p->actors[index].assignment_74)){
   p->direction_00b2=s->actors[index].alternate_78;
   return stop(NBA_PERIOD_ROLES_V2_ASSIGNMENT_CHILD,0x85bf5b,p->pair_009a);
  }
 }
 if(!n(p->owner_093e)){
  p->pair_009a=p->ball_pointer_0910;assignment=s->ball_assignment_74;
  if(n(assignment)){
   p->direction_00b2=s->ball_alternate_78;
   return stop(NBA_PERIOD_ROLES_V2_ASSIGNMENT_CHILD,0x85bf98,p->pair_009a);
  }
  if((assignment&1u)||assignment>18u)return stop(NBA_PERIOD_ROLES_V2_RECORD_READ,0x85bfab,assignment);
  index=assignment/2;p->actor_0096=ptr((unsigned)index);
  /* BF89 dereferences BALL+74, even if selected actor belongs to entry
   * context. No owner-XY replacement or defense-context side check. */
  if(n(minus(s->actors[index].mode_5e,7))){
   s->actors[index].mode_5e=4;
   if(s->actors[index].anchor_distance_8c>=s->ball_anchor_distance_8c)s->actors[index].boost_72=0x14; /* BFC7 BCS */
  }
  /* BFDF/C04F skip live-play helper allocation for81/82. */
 }else if(n(s->receiver_0946)){
  p->actor_0096=p->nearest_09da;index=actor(p->actor_0096);
  if(index<0)return stop(NBA_PERIOD_ROLES_V2_RECORD_READ,0x85c05c,p->actor_0096);
  if(!n(minus(s->actors[index].mode_5e,6))&&s->actors[index].mode_5e!=6){
   p->nearest_0092=ptr(team*5);p->counter_00c2=4;p->best_00be=0x7fff;
   for(j=0;j<5;j++){
    unsigned i=team*5+j;
    if(n(minus(s->actors[i].mode_5e,7))&&!n(minus(p->best_00be,p->actors[i].focal_distance_8e))){
     p->actor_0096=p->nearest_0092;p->best_00be=p->actors[i].focal_distance_8e;
    } /* C086: last equal candidate wins, unlike BC initial scan. */
    p->nearest_0092=(uint16_t)(p->nearest_0092+256u);p->counter_00c2=minus(p->counter_00c2,1);
   }
   if(p->best_00be==0x7fff)return stop(NBA_PERIOD_ROLES_V2_FIRST_RETURN,0x85c051,0);
   index=actor(p->actor_0096);
  }
  if(n(minus(s->actors[index].mode_5e,7)))s->actors[index].mode_5e=4;
 }
 p->entry_context_00a6=(uint16_t)(p->context_009e+0x49u);p->nearest_09da=5;
 for(j=0;j<5;j++){
  assignment=s->contexts[team].order_49[j];p->focal_x_00b6=assignment;p->entry_context_00a6++;
  index=assignment/2;p->pair_009a=ptr((unsigned)index);
  if(n(p->actors[index].assignment_74)){
   p->direction_00b2=s->actors[index].alternate_78;
   return stop(NBA_PERIOD_ROLES_V2_ASSIGNMENT_CHILD,0x85c0dd,p->pair_009a);
  }
  p->nearest_09da--; /* C0F0: ends atZERO, not a nearest actor pointer. */
 }
 return stop(NBA_PERIOD_ROLES_V2_FIRST_RETURN,0x85c0f5,0);
}
static uint16_t assigned(const NbaPeriodRoleStateV2 *s,unsigned i,unsigned set){return set==0?s->prefix.actors[i].assignment_74:set==1?s->actors[i].base_76:s->actors[i].alternate_78;}
bool nba_period_roles_v2_begin(NbaPeriodRolesV2 *w,const NbaPeriodRoleStateV2 *s){
 unsigned i,set,team,j,seen;if(!w||!s)return false;
 if(s->live_0936!=0x81&&s->live_0936!=0x82)return false;
 if(s->prefix.ball_pointer_0910!=0x3eeb)return false;
 for(i=0;i<10;i++){
  if(s->actors[i].id_00!=i||s->actors[i].team_6e!=(i<5?0u:5u))return false;
  for(set=0;set<3;set++){
   uint16_t a=assigned(s,i,set);if((a&1u)||a>18u||(a/2u<5u)==(i<5u)||assigned(s,a/2u,set)!=i*2u)return false;
  }
 }
 for(team=0;team<2;team++){
  seen=0;for(j=0;j<5;j++){
   uint16_t a=s->contexts[team].order_49[j];if((a&1u)||a>18u||(a/2u<5u)==(team==0u)||(seen&(1u<<(a/2u))))return false;seen|=1u<<(a/2u);
  }
 }
 memset(w,0,sizeof(*w));if(!nba_period_roles_begin(&w->prefix,&s->prefix))return false;w->valid=true;return true;
}
NbaPeriodRoleBoundaryV2 nba_period_roles_v2_advance(NbaPeriodRolesV2 *w,NbaPeriodRoleStateV2 *s){
 NbaPeriodRoleBoundaryV2 b={NBA_PERIOD_ROLES_V2_INVALID,0,0,0};NbaPeriodRoleBoundary old;unsigned call;
 if(!w||!s||!w->valid)return b;if(w->waiting)return w->boundary;
 call=w->prefix.calls;old=nba_period_roles_advance(&w->prefix,&s->prefix);
 if(old.kind==NBA_PERIOD_ROLES_REBUILD||old.kind==NBA_PERIOD_ROLES_PLANNER){
  if(old.kind==NBA_PERIOD_ROLES_REBUILD)rebuild(s,call);
  b=planner(s,call);
  if(b.kind==NBA_PERIOD_ROLES_V2_FIRST_RETURN){w->prefix.calls++;w->prefix.boundary.kind=call==0?NBA_PERIOD_ROLES_FIRST_RETURN:NBA_PERIOD_ROLES_COMPLETE;}
 }else b.kind=old.kind==NBA_PERIOD_ROLES_FIRST_RETURN?NBA_PERIOD_ROLES_V2_FIRST_RETURN:NBA_PERIOD_ROLES_V2_COMPLETE;
 if(b.kind==NBA_PERIOD_ROLES_V2_FIRST_RETURN||b.kind==NBA_PERIOD_ROLES_V2_COMPLETE){
  b.kind=w->prefix.calls==1?NBA_PERIOD_ROLES_V2_FIRST_RETURN:NBA_PERIOD_ROLES_V2_COMPLETE;b.source_pc=w->prefix.calls==1?0x86e1ee:0x86e1f7;
 }
 b.completed_calls=w->prefix.calls;w->boundary=b;w->waiting=true;return b;
}
bool nba_period_roles_v2_resume(NbaPeriodRolesV2 *w){
 if(!w||!w->valid||!w->waiting||w->boundary.kind!=NBA_PERIOD_ROLES_V2_FIRST_RETURN)return false;
 if(!nba_period_roles_resume(&w->prefix))return false;w->waiting=false;return true;
}
