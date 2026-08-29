#include "nba_tipoff.h"
#include <stdio.h>
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    NbaAssetPack pack={0};NbaSession session;NbaTipoff game;
    if(!nba_assets_load(&pack,argv[1]))return 3;
    nba_session_init(&session);
    if(!nba_tipoff_init(&game,&pack,&session))return 4;
    if(SNES_ADDR_TIPOFF_BALL_INIT!=0x86e056 ||
       game.ball.x_fp || game.ball.y_fp || game.ball.z_fp!=80*256 ||
       game.ball.velocity_x || game.ball.velocity_y || game.ball.velocity_z!=600 ||
       game.catch_actor_record_raw_0910!=0x3eeb || game.context_raw_4933!=0xffff ||
       game.context_raw_4935!=0xffff || game.fouls.latched_event_raw_08f0!=0xffff ||
       game.ball_initialization.cursor!=0x34e9 || game.ball_initialization.ball_descriptor!=0x34e7 ||
       game.ball_initialization.record_id!=10 || game.ball_initialization.group!=0xffff)return 5;
    nba_assets_free(&pack);
    puts("BALL INIT RUNTIME PASS: native prefix is bound to the real game initializer");
    return 0;
}
