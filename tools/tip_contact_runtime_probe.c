#include "nba_tipoff.h"
#include <stdio.h>

int main(int argc,char **argv) {
    NbaAssetPack pack={0};NbaSession session;NbaTipoff game;NbaInput input={0};
    if(argc!=2||!nba_assets_load(&pack,argv[1]))return 2;
    nba_session_init(&session);
    if(!nba_tipoff_init(&game,&pack,&session))return 3;
    for(unsigned frame=1;frame<=219;++frame)nba_tipoff_update(&game,&input);
    printf("TIP CONTACT runtime: actor=%d frame=%u reach=%u\n",game.tip_contact_actor,game.tip_contact_frame,game.tip_reach_mask);
    if(game.tip_contact_actor<0 || game.tip_contact_frame==0)return 4;
    /* Same caller phase; only geometry changes. A far ball must not produce
     * contact, while placing it in the body window must, at frame42. */
    if(!nba_tipoff_init(&game,&pack,&session))return 5;
    game.frame=42;game.simulation_tick=42;game.ball.state=NBA_BALL_TOSS;
    game.ball.x_fp=300*256;game.ball.y_fp=180*256;game.ball.z_fp=40*256;
    if(nba_tipoff_try_tip_contact(&game))return 6;
    game.ball.x_fp=game.actors[5].x_fp;game.ball.y_fp=game.actors[5].y_fp;
    if(!nba_tipoff_try_tip_contact(&game)||game.tip_contact_frame!=42)return 7;
    if(nba_tipoff_try_tip_contact(&game))return 8;
    if(!nba_tipoff_init(&game,&pack,&session))return 9;
    game.frame=42;game.simulation_tick=43;game.ball.state=NBA_BALL_TOSS;game.ball.z_fp=40*256;
    if(nba_tipoff_try_tip_contact(&game))return 10;
    puts("TIP CONTACT binding PASS: natural contact, changed geometry, no frame gate, single event, cadence");
    nba_assets_free(&pack);return 0;
}
