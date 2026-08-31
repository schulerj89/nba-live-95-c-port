#include "nba_tipoff.h"
#include <stdio.h>
#include <string.h>

static NbaCameraInput camera_input(const NbaTipoff *t,unsigned subject) {
    NbaCameraInput in={0};
    in.subject=subject<10?nba_gameplay_camera_subject(t->actors[subject].x_fp,t->actors[subject].y_fp):
        nba_gameplay_camera_subject(t->ball.x_fp,t->ball.y_fp);
    in.ball_height=(int16_t)((uint32_t)t->ball.z_fp>>8);
    in.side_group=t->camera_side_group_raw==255?-1:t->camera_side_group_raw;
    in.basket_left=t->team_context[0].anchor_x_raw_0a;in.basket_right=t->team_context[1].anchor_x_raw_0a;
    in.live_state=t->live_state_raw;in.alternate_08bc=t->camera_alternate_raw_08bc;in.alternate_mode_08cc=t->camera_alternate_mode_raw_08cc;
    return in;
}
int main(int argc,char **argv) {
    NbaAssetPack pack={0};NbaSession session;NbaTipoff t;NbaInput input={0};
    if(argc!=2||!nba_assets_load(&pack,argv[1]))return 2;
    nba_session_init(&session);
    if(!nba_tipoff_init(&t,&pack,&session))return 3;
    if(t.camera.x!=-128||t.camera.y!=-124||t.camera.initialized_4a54!=0xffff||t.live_state_raw!=0x81)return 4;
    for(unsigned frame=1;frame<=400;++frame) {
        nba_tipoff_update(&t,&input);
        if(!t.tip_possession_frame && t.live_state_raw!=0x81)return 5;
        if(t.tip_possession_frame && frame<=t.tip_possession_frame+2 && t.live_state_raw!=0)return 6;
    }
    if(!t.tip_possession_frame)return 14;
    /* Isolated caller inputs, deliberately BEFORE the old frame-200 gate.
     * Assert actor-vs-ball Z, reversed basket signs and alternate flags are
     * passed to the independently ROM-replayed core without substitution. */
    for(unsigned variant=0;variant<6;++variant) {
        if(!nba_tipoff_init(&t,&pack,&session))return 7;
        t.frame=42;t.simulation_tick=42;t.possession_actor=8;t.camera_side_group_raw=0;
        t.actors[8].x_fp=80*256;t.actors[8].y_fp=40*256;t.actors[8].z_fp=180*256;
        t.team_context[0].anchor_x_raw_0a=(variant&1)?336:-336;
        t.live_state_raw=variant<2?1:0x81;
        t.camera_alternate_raw_08bc=variant>=2?1:0;t.camera_alternate_mode_raw_08cc=variant<4?1:2;
        NbaGameplayCamera expected=t.camera;
        nba_tipoff_update(&t,&input);
        NbaCameraInput in=camera_input(&t,8);nba_gameplay_camera_step(&expected,&in);
        if(t.camera.subject_actor!=8||t.camera.subject_pointer_0940!=0x3ceb||
            t.camera.x!=expected.x||t.camera.y!=expected.y||memcmp(&t.camera.proxy,&in.subject,sizeof(in.subject)))return 8;
    }
    /* Resolve before wait; copy after wait. Changing the owner while waiting
     * changes 0940 in the wrapper but must not change the selected XY. */
    if(!nba_tipoff_init(&t,&pack,&session))return 9;
    t.frame=42;t.simulation_tick=42;t.camera.presentation_ticks_0564=0;t.possession_actor=7;
    t.actors[7].x_fp=100*256;t.actors[7].y_fp=20*256;
    NbaGameplayCamera held=t.camera;
    nba_tipoff_update(&t,&input);
    if(!t.camera.caller_waiting||t.camera.x!=held.x||t.camera.y!=held.y||t.camera.subject_pointer_0940!=0x3beb)return 10;
    t.possession_actor=8;t.actors[7].x_fp=101*256;
    nba_tipoff_update(&t,&input);
    if(t.camera.caller_waiting||t.camera.subject_actor!=7||t.camera.proxy.x_integer!=101||t.camera.subject_pointer_0940!=0x3ceb)return 11;
    uint16_t ticks=1;if(nba_gameplay_camera_ready(&ticks)||ticks!=1)return 12;
    ticks=7;if(!nba_gameplay_camera_ready(&ticks)||ticks!=0)return 13;
    /* A period-ending host update earns a credit, then the lifecycle returns
     * before cpu_update_camera. A credit alone is not a 95AC wait entry.
     * Check both an absent wait and an already pending one through the full
     * presentation/restart. These are controlled host-binding contracts,
     * not a claim about the original game's full-frame scheduling. */
    for(unsigned pending=0;pending<2;++pending) {
        nba_session_init(&session);
        if(!nba_tipoff_init(&t,&pack,&session))return 15;
        for(unsigned frame=0;frame<400;++frame)nba_tipoff_update(&t,&input);
        t.match_clock_raw_0928=0;t.possession_actor=7;
        t.camera.presentation_ticks_0564=(uint16_t)pending;
        t.camera.caller_waiting=pending!=0;
        t.camera.subject_pointer_0940=(uint16_t)(pending?0x3beb:0x38eb);
        NbaGameplayCamera before=t.camera;
        nba_tipoff_update(&t,&input);
        if(t.period_raw_0926!=1||session.match.flow_state!=NBA_MATCH_FLOW_PERIOD_PRESENTATION_PENDING)return 16;
        if(t.actor_pass_executed)return 22;
        if(t.camera.presentation_ticks_0564!=pending+1||t.camera.caller_waiting!=(pending!=0))return 17;
        NbaGameplayCamera earned=before;earned.presentation_ticks_0564=(uint16_t)(pending+1);
        if(memcmp(&t.camera,&earned,sizeof(earned)))return 18;
        unsigned presentation=0;
        while(session.match.flow_state>=NBA_MATCH_FLOW_PERIOD_PRESENTATION_PENDING) {
            nba_tipoff_update(&t,&input);
            if(t.actor_pass_executed)return 23;
            if(memcmp(&t.camera,&earned,sizeof(earned))||++presentation>2000)return 19;
        }
        if(!presentation||session.match.flow_state!=NBA_MATCH_FLOW_LIVE||t.possession_actor!=-1)return 20;
        nba_tipoff_update(&t,&input);
        unsigned subject=pending?7u:t.possession_actor<0?255u:(unsigned)t.possession_actor;
        NbaCameraInput resumed=camera_input(&t,subject);
        if(t.camera.caller_waiting||t.camera.presentation_ticks_0564!=0||
           t.camera.subject_actor!=subject||memcmp(&t.camera.proxy,&resumed.subject,sizeof(resumed.subject))||
           t.camera.subject_pointer_0940!=nba_gameplay_camera_resolve(t.possession_actor))return 21;
    }
    puts("CAMERA binding PASS: native tip/owned states;6 actor-height/orientation/flag cases;pre-200 dispatch;latched subject;pause/resume credits;both period-return wait phases");
    nba_assets_free(&pack);return 0;
}
