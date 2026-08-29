#include "nba_tipoff.h"
#include <stdio.h>
#include <string.h>

int main(int argc,char **argv) {
    NbaAssetPack pack={0};NbaSession session;NbaTipoff game;NbaInput input={0};
    int16_t sx=0,sy=0;
    /* Native `$87:A3BB-$A3DC` witnesses, including negative remainders. */
    nba_court_project_actor(8,3,0,-128,-124,&sx,&sy);
    if(sx!=139 || sy!=122)return 13;
    nba_court_project_actor(-16,-83,0,-128,-124,&sx,&sy);
    if(sx!=29 || sy!=107)return 14;
    nba_court_project_actor(71,-33,0,-174,-144,&sx,&sy);
    if(sx!=212 || sy!=118)return 15;
    nba_court_project_actor(-62,12,7,-174,-144,&sx,&sy);
    if(sx!=124 || sy!=155)return 16;
    if(!nba_court_actor_visible(-20,-20,0,false) ||
       !nba_court_actor_visible(275,287,0,false) ||
       !nba_court_actor_visible(100,300,20,false) ||
       nba_court_actor_visible(100,300,12,false) ||
       nba_court_actor_visible(-21,0,0,false) ||
       nba_court_actor_visible(276,0,0,false))return 17;
    if(!nba_court_actor_visible(11,11,0,true) ||
       !nba_court_actor_visible(244,217,0,true) ||
       nba_court_actor_visible(10,100,0,true) ||
       nba_court_actor_visible(245,100,0,true))return 18;
    if(argc<2 || !nba_assets_load(&pack,argv[1]))return 2;
    nba_session_init(&session);
    if(!nba_tipoff_init(&game,&pack,&session))return 3;
    for(unsigned frame=1;frame<=16000;++frame) {
        NbaCourtPresentation previous=game.court_presentation;
        NbaCourtStream stream=game.court_stream;
        nba_tipoff_update(&game,&input);
        if(game.camera.presentation_ticks_0564==0) {
            nba_court_presentation_update(&previous,game.camera_x,game.camera_y,
                game.period_raw_0926,game.team_context[0].anchor_x_raw_0a,game.team_context[1].anchor_x_raw_0a);
            if(memcmp(&previous,&game.court_presentation,sizeof(previous)))return 4;
            if(!nba_court_stream_update(&stream,&pack,game.camera_x,game.camera_y,
                game.camera.previous_x,game.camera.previous_y,NULL,NULL))return 5;
            stream.scroll_x=stream.next_scroll_x;stream.scroll_y=stream.next_scroll_y;
            if(memcmp(&stream,&game.court_stream,sizeof(stream)))return 6;
        } else if(memcmp(&stream,&game.court_stream,sizeof(stream)))return 7;
        if(game.court_stream.source!=(uint16_t)(0x8006+game.camera.coarse_x*104+game.camera.coarse_y*2))return 8;
    }
    /* Render the actual port with actors/ball outside the viewport. This is
     * an isolated renderer fixture, not a natural-gameplay claim. */
    for(unsigned a=0;a<10;++a){
        game.actors[a].x_fp=10000*256;game.actors[a].y_fp=10000*256;
        /* The fixture mutates world state outside an update pass. Native OAM
         * keeps its old submissions, so explicitly hide the latched sprites
         * when validating the panorama alone. */
        game.player_screen_visible[a]=false;
    }
    game.ball.x_fp=10000*256;game.ball.y_fp=10000*256;
    NbaRenderer renderer;nba_renderer_init(&renderer);
    const int xs[]={-582,-128,74,75,194,328},ys[]={-242,-124,-53};
    for(unsigned team=0;team<29;++team) {
        session.right_team=(uint8_t)team;
        const uint32_t *panorama=nba_assets_gameplay_court_panorama(&pack,(uint8_t)team);
        if(!panorama)return 9;
        for(unsigned xi=0;xi<6;++xi)for(unsigned yi=0;yi<3;++yi) {
            game.camera_x=(int16_t)xs[xi];game.camera_y=(int16_t)ys[yi];
            nba_tipoff_render(&game,&renderer);
            /* Independent unclamped expected origin, including first pixel
             * past the OLD panorama limit at camera X75. */
            int px=xs[xi]+582,py=ys[yi]+243;
            for(unsigned row=0;row<224;++row)
                if(memcmp(renderer.pixels+row*256,panorama+(py+row)*1184+px,256*sizeof(uint32_t)))return 10;
        }
    }
    /* 8E28 loads current period 0926, NOT quarter-length setting 1701.
     * Give them opposite half selections to catch a wrong caller binding.
     * These are isolated integration scenarios, not natural period flow. */
    for(unsigned period=0;period<4;++period) {
        session.config.main_values[3]=(uint16_t)(period<2?3:0);
        if(!nba_tipoff_init(&game,&pack,&session))return 11;
        game.period_raw_0926=(uint16_t)period;
        for(unsigned frame=0;frame<300;++frame)nba_tipoff_update(&game,&input);
        unsigned side=(period<2 ? game.camera_x>=0 : game.camera_x<0)?1u:0u;
        if(game.court_presentation.basket_x_3fef!=
           (uint16_t)game.team_context[side].anchor_x_raw_0a)return 12;
    }
    puts("COURT runtime PASS:16000 frames of caller binding;522 whole-viewport renders across29 teams;4 independent period/length scenarios");
    nba_assets_free(&pack);return 0;
}
