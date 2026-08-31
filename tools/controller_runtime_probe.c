/* Controlled C integration, not an aligned natural C/ROM game journey. */
#include "nba_tipoff.h"
#include "nba_player_setup.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if(!(x)) {fprintf(stderr,"controller integration failed line %d: %s\n",__LINE__,#x);return 1;} } while(0)

static bool capture(const NbaPlayerSetup *ui,const char *directory,unsigned choice) {
    if(!directory) return true;
    char path[4096];snprintf(path,sizeof(path),"%s/player%u.bmp",directory,choice);
    NbaRenderer renderer;nba_renderer_init(&renderer);
    nba_player_setup_render(ui,&renderer);
    return nba_renderer_save_bmp(&renderer,path);
}

int main(int argc,char **argv) {
    if(argc<2 || argc>3) return 2;
    NbaAssetPack assets={0}; CHECK(nba_assets_load(&assets,argv[1]));
    NbaSession session; nba_session_init(&session);
    session.left_team=17;session.right_team=9;
    CHECK(session.controller_selection[0]==2);
    for(unsigned pad=1;pad<5;++pad) CHECK(session.controller_selection[pad]==1);
    NbaPlayerSetup ui={0};CHECK(nba_player_setup_init(&ui,&assets,&session,NULL));
    for(unsigned i=0;i<NBA_PLAYER_SETUP_TRANSITION_FRAMES;++i) nba_player_setup_update(&ui,NULL);
    CHECK(capture(&ui,argc==3?argv[2]:NULL,2));
    NbaInput input={0};input.held=input.pressed=NBA_BTN_LEFT;
    CHECK(nba_player_setup_update(&ui,&input)==NBA_PLAYER_SETUP_SOUND_MOVE);
    CHECK(session.controller_selection[0]==1 && ui.controller_selection==1);
    CHECK(capture(&ui,argc==3?argv[2]:NULL,1));
    input.held=input.pressed=0;input.released=NBA_BTN_LEFT;nba_player_setup_update(&ui,&input);
    input.released=0;input.held=input.pressed=NBA_BTN_LEFT;
    CHECK(nba_player_setup_update(&ui,&input)==NBA_PLAYER_SETUP_SOUND_MOVE);
    CHECK(session.controller_selection[0]==0 && session.player_one_side==0);
    CHECK(capture(&ui,argc==3?argv[2]:NULL,0));
    input.held=input.pressed=NBA_BTN_LEFT|NBA_BTN_RIGHT;
    CHECK(nba_player_setup_update(&ui,&input)==NBA_PLAYER_SETUP_SOUND_NONE);
    CHECK(session.controller_selection[0]==0);
    input.held=input.pressed=NBA_BTN_RIGHT;
    nba_player_setup_update(&ui,&input);CHECK(session.controller_selection[0]==1);
    input.held=input.pressed=NBA_BTN_L;
    nba_player_setup_update(&ui,&input);CHECK(session.controller_flags[0]==0);
    input.held=input.pressed=NBA_BTN_RIGHT;
    nba_player_setup_update(&ui,&input);CHECK(session.controller_selection[0]==2);
    input.held=input.pressed=NBA_BTN_L;
    nba_player_setup_update(&ui,&input);CHECK(session.controller_flags[0]==0x8000);
    nba_player_setup_update(&ui,&input);CHECK(session.controller_flags[0]==0);
    nba_player_setup_shutdown(&ui);
    NbaTipoff t;
    CHECK(nba_tipoff_init(&t,&assets,&session));
    CHECK(t.cpu_vs_cpu && t.controllers.count[0]==0 && t.controllers.count[1]==0);
    for(unsigned choice=0;choice<3;++choice) {
        uint16_t select[5]={(uint16_t)choice,1,1,1,1},flags[5]={0};
        CHECK(nba_tipoff_initialize_controllers(&t,select,flags,0));
        CHECK(t.controllers.record[0].group==(choice==0?5:choice==2?0:-1));
        if(choice==1) {
            for(unsigned actor=0;actor<10;++actor) CHECK(t.actors[actor].controller_assignment_raw==-1);
            continue;
        }
        unsigned base=choice==0?5:0, other=choice==0?0:5;
        CHECK(t.actors[base].controller_assignment_raw==0);
        CHECK(nba_tipoff_transfer_controller(&t,base+2));
        CHECK(t.actors[base+2].controller_assignment_raw==0 && t.actors[base].controller_assignment_raw==-1);
        CHECK(nba_tipoff_transfer_controller(&t,other+2));
        CHECK(t.actors[other+2].controller_assignment_raw==-1 && t.actors[base+2].controller_assignment_raw==0);
        t.live_state_raw=0;t.pass_receiver_raw=-1;t.pass_aux_raw=-1;
        nba_tipoff_replay_ball_acquisition(&t,(uint8_t)(base+1));
        CHECK(t.actors[base+1].controller_assignment_raw==0 && t.actors[base+2].controller_assignment_raw==-1);
        CHECK(t.team_context[base/5].controller_actor_raw_41==0);
        nba_tipoff_publish_controller_input(&t,base+1,0x0130);
        CHECK(t.controllers.record[0].held==0x0130 && t.controllers.record[0].pressed==0x0130);
        CHECK(t.controllers.record[0].direction==1 && t.actors[base+1].movement_boost_timer==5);
        nba_tipoff_publish_controller_input(&t,base+1,0x0130);
        CHECK(t.controllers.record[0].pressed==0 && t.controllers.record[0].changed==0);
        nba_tipoff_publish_controller_input(&t,base+1,0);
        CHECK(t.controllers.record[0].pressed==0 && t.controllers.record[0].changed==0x0130 &&
              t.controllers.record[0].direction==8 && t.actors[base+1].movement_boost_timer==0);
        CHECK(!t.controller_contract_fault);
    }
    for(unsigned bits=0;bits<4096;++bits)
        CHECK(nba_controller_host_buttons(nba_controller_native_buttons(bits))==bits);
    printf("[CONTROLLER C REGRESSION] PASS: UI left/neutral/right, canonical records, same-team acquisition, held/release conversion. Human play remains disabled.\n");
    nba_assets_free(&assets);
    return 0;
}
