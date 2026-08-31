/* Current C runtime journey: no score/clock/actor seeds. Only normal input
 * and the same existing direct Tipoff scene entry used by the CLI. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_tipoff.h"
#include "nba_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool frame(NbaTipoff *t,NbaRenderer *r,const char *out,const char *name) {
    char p[1024];
    if(snprintf(p,sizeof(p),"%s/%s.bmp",out,name)<0)return false;
    nba_tipoff_render(t,r);return nba_renderer_save_bmp(r,p);
}
int main(int argc,char **argv) {
    if(argc!=3)return 2;
    NbaAssetPack assets={0};NbaSession session;NbaInput input={0};
    NbaTipoff *t=(NbaTipoff *)calloc(1,sizeof(*t));
    NbaRenderer *r=(NbaRenderer *)calloc(1,sizeof(*r));
    if(!t || !r || !nba_assets_load(&assets,argv[1]))return 3;
    nba_session_init(&session);nba_renderer_init(r);
    if(!nba_tipoff_init(t,&assets,&session))return 4;
    unsigned pause_holds=0,formation_holds=0,dead_runs=0,dead_holds=0,live_steps=0;
    unsigned final_views=0;uint16_t last_clock=0xFFFFu;
    for(unsigned tick=1;tick<=55000u;++tick) {
        uint32_t held=tick==4500u?NBA_BTN_START:tick==4622u?NBA_BTN_DOWN:
                      tick==4624u?NBA_BTN_A:0u;
        uint16_t before=t->match_clock_raw_0928,live=t->live_state_raw;
        NbaTipoffActor actors[10];memcpy(actors,t->actors,sizeof(actors));
        nba_game_input_update(&input,held);nba_tipoff_update(t,&input);
        if(tick>4500u && tick<=4620u) {
            if(t->match_clock_raw_0928!=before || memcmp(actors,t->actors,sizeof(actors)))return 5;
            ++pause_holds;
        }
        if(live==0x81u && t->match_clock_raw_0928==before)++formation_holds;
        if(live==0x82u) {
            if(t->match_clock_raw_0928==before)++dead_holds;else ++dead_runs;
        }
        if(live<0x80u && t->match_clock_raw_0928!=before)++live_steps;
        if(tick==2450u && !frame(t,r,argv[2],"first-panel"))return 6;
        if(tick==2520u && !frame(t,r,argv[2],"first-panel-later-clock"))return 6;
        if(tick==2660u && !frame(t,r,argv[2],"cleared-panel"))return 6;
        if(tick==3100u && !frame(t,r,argv[2],"second-basket-pending"))return 6;
        if(tick==4501u && !frame(t,r,argv[2],"pause-fresh-score"))return 6;
        if(tick==4620u && !frame(t,r,argv[2],"pause-held-120"))return 6;
        if(t->match_clock_raw_0928<3590u && t->fouls.whistle_timer_raw_08de<0 &&
           t->hud.clock_mirror_raw_08f6!=last_clock && final_views<2u) {
            if(final_views==0u || last_clock-t->hud.clock_mirror_raw_08f6>=60u) {
                if(!frame(t,r,argv[2],final_views?"final-minute-later":"final-minute"))return 6;
                last_clock=t->hud.clock_mirror_raw_08f6;++final_views;
            }
        }
        printf("HUD_RUNTIME %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u ",
            tick,t->simulation_tick,t->live_state_raw,t->match_clock_raw_0928,
            t->hud_clock_snapshot_raw_092a,t->rim_raw_092c,t->shot_clock_mirror_raw_09c6,
            session.score[0],session.score[1],(uint16_t)t->fouls.whistle_timer_raw_08de,
            t->fouls.whistle_state_raw_08e6,t->fouls.whistle_state_mirror_raw_08e8,
            t->hud.phase_raw_08e4,t->hud.clock_mirror_raw_08f6,t->hud.clock_frame_raw_08f4,
            t->hud.pending_routine,session.match.pause.state);
        for(unsigned i=0;i<8u;++i)printf("%02x",t->hud.clock_text_raw_4a60[i]);
        putchar('\n');
        if(final_views==2u && dead_holds>0u)break;
    }
    printf("HUD_RUNTIME_COUNTS %u %u %u %u %u %u\n",pause_holds,formation_holds,
        dead_runs,dead_holds,live_steps,final_views);
    return pause_holds==120u && formation_holds && dead_runs && dead_holds &&
           live_steps && final_views==2u?0:7;
}
