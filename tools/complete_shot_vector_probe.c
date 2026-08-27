#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "nba_shot_launch.h"

typedef struct { NbaShotLaunchInput in; NbaShotLaunchState s; } Row;
typedef struct { const char *name; size_t offset, size; } Field;
#define F(name) {#name,offsetof(Row,name),sizeof(((Row *)0)->name)}
static const Field fields[]={
    F(in.actor_x),F(in.actor_y),F(in.controller),F(in.basket_x),F(in.origin_x),F(in.origin_y),
    F(in.basket_fraction),F(in.team_group),F(in.distance_8c),F(in.defense_8a),F(in.movement_4c),
    F(in.modifier_b2),F(in.stamina_18),F(in.difficulty),F(in.shot_control_17c3),
    F(in.shot_assistance_17bf),F(in.hot_team_09c0),F(in.free_throw_0978),
    F(in.aim_0982),F(in.power_0980),F(in.clock_0928),F(in.period_0926),F(in.assist_clock_47),
    F(in.roster_low),F(in.roster_bank),F(in.rating_two),F(in.rating_three),F(in.rating_free),F(in.range_49),
    F(in.special_entry),F(in.boosted),F(in.alternate_lower),
    F(s.actor.animation.upper_queue_cursor),F(s.actor.animation.lower_queue_cursor),
    F(s.actor.animation.upper_state),F(s.actor.animation.lower_state),F(s.actor.animation.base_state),
    F(s.actor.animation.upper_phase),F(s.actor.animation.lower_phase),
    F(s.actor.animation.upper_accumulator),F(s.actor.animation.lower_accumulator),
    F(s.actor.animation.upper_lock),F(s.actor.animation.lower_lock),
    F(s.actor.animation.upper_queue[0]),F(s.actor.animation.upper_queue[1]),F(s.actor.animation.upper_queue[2]),
    F(s.actor.animation.lower_queue[0]),F(s.actor.animation.lower_queue[1]),F(s.actor.animation.lower_queue[2]),
    F(s.actor.velocity_x),F(s.actor.velocity_y),F(s.actor.velocity_z),F(s.actor.speed),F(s.actor.mode),
    F(s.actor.flags),F(s.actor.timer),F(s.actor.status),F(s.actor.behavior_timer),
    F(s.actor.activity),F(s.actor.bounce_count),F(s.actor.bounce_timer),
    F(s.facing),F(s.contact_inhibit),F(s.x_fraction),F(s.x),F(s.y_fraction),F(s.y),F(s.z_fraction),F(s.z),
    F(s.velocity_x),F(s.velocity_y),F(s.velocity_z),F(s.owner),F(s.last_owner),F(s.display_shooter),
    F(s.attempt_latch),F(s.dead_0966),F(s.height_0968),F(s.dead_096c),F(s.bounce_0920),F(s.inner_veto),
    F(s.live_state),F(s.timeout_0930),F(s.value),F(s.display_value),F(s.initial_value),
    F(s.roster_low),F(s.roster_bank),F(s.ball_record),F(s.assist_43),F(s.assist_45),
    F(s.player_stats[0]),F(s.player_stats[1]),F(s.player_stats[2]),F(s.player_stats[3]),F(s.player_stats[4]),
    F(s.controller_stats[0]),F(s.controller_stats[1]),F(s.controller_stats[2]),
    F(s.controller_stats[3]),F(s.controller_stats[4]),F(s.rng.state)
};
#undef F
int main(int argc,char **argv) {
    NbaAssetPack assets;
    if(argc!=2 || !nba_assets_load(&assets,argv[1])) return 2;
    unsigned count;
    while(scanf_s("%u",&count)==1) {
        Row row={0};
        for(unsigned i=0;i<count;++i) {
            char name[96];unsigned value;
            if(scanf_s("%95s %x",name,(unsigned)sizeof(name),&value)!=2) return 3;
            size_t j;
            for(j=0;j<sizeof(fields)/sizeof(fields[0]);++j) if(!strcmp(name,fields[j].name)) break;
            if(j==sizeof(fields)/sizeof(fields[0])) return 3;
            uint16_t word=(uint16_t)value; uint8_t byte=(uint8_t)value;
            memcpy((char *)&row+fields[j].offset,fields[j].size==1 ? (void *)&byte : (void *)&word,fields[j].size);
        }
        if(!nba_shot_launch(&assets,&row.in,&row.s)) { fprintf(stderr,"launch failed\n");return 4; }
        for(size_t j=0;j<sizeof(fields)/sizeof(fields[0]);++j) {
            unsigned value;
            if(fields[j].size==1) {uint8_t b;memcpy(&b,(char *)&row+fields[j].offset,1);value=b;}
            else {uint16_t w;memcpy(&w,(char *)&row+fields[j].offset,2);value=w;}
            printf("%s %04x ",fields[j].name,value);
        }
        printf("\n");
    }
    nba_assets_free(&assets);
    return 0;
}
