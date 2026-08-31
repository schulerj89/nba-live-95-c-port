/* Controlled direct selector and real static caller; not a scene journey. */
#include "../src/nba_tipoff.c"
#include <stdio.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main(void) {
    uint8_t bytes[32];
    static NbaTipoff game;
#ifdef _WIN32
    if (_setmode(_fileno(stdin), _O_BINARY) == -1 ||
        _setmode(_fileno(stdout), _O_BINARY) == -1) return 4;
#endif
    for (;;) {
        size_t count=fread(bytes,1,sizeof(bytes),stdin);
        if (!count) return ferror(stdin)?5:0;
        if (count!=sizeof(bytes)) return 2;
        uint16_t w[16];
        for(unsigned i=0;i<16;++i)w[i]=(uint16_t)(bytes[2*i]|((uint16_t)bytes[2*i+1]<<8));
        if(w[0]>1u || w[1]>7u || w[2]>255u)return 3;
        uint8_t result;
        if(w[0]==0u) {
            if(w[6]>1u)return 3;
            NbaGameplayDrawDirection input={
                .current_direction=(uint8_t)w[1],.control_mode=(uint8_t)w[2],
                .actor_status=w[3],.upper_state=w[4],.anchor_direction=w[5],
                .candidate_valid=w[6]!=0,.candidate_dx=(int16_t)w[7],
                .candidate_dy=(int16_t)w[8]};
            result=nba_gameplay_draw_direction(&input);
        } else {
            if(w[6]>=10u || (w[8]>=10u && w[8]!=0xffffu) ||
               (w[9]>=10u && w[9]!=0xffffu) || w[4]>255u || w[5]>255u)return 3;
            memset(&game,0,sizeof(game));
            unsigned index=w[6];
            NbaTipoffActor *actor=&game.actors[index];
            actor->direction=(uint8_t)w[1];actor->control_mode=(uint8_t)w[2];
            actor->actor_status_raw_28=w[3];actor->animation_state=(uint8_t)w[4];
            actor->anchor_direction_raw=(uint8_t)w[5];
            actor->x_fp=(int32_t)(int16_t)w[10]*256;
            actor->y_fp=(int32_t)(int16_t)w[11]*256;
            game.camera.subject_pointer_0940=w[7];
            game.possession_actor=(int8_t)w[8];game.pass_receiver_raw=(int16_t)w[9];
            game.ball.x_fp=(int32_t)(int16_t)w[12]*256;
            game.ball.y_fp=(int32_t)(int16_t)w[13]*256;
            if(w[9]<10u && w[9]!=index) {
                game.actors[w[9]].x_fp=(int32_t)(int16_t)w[14]*256;
                game.actors[w[9]].y_fp=(int32_t)(int16_t)w[15]*256;
            }
            static NbaTipoff before;
            memcpy(&before,&game,sizeof(game));
            result=actor_draw_direction(&game,index);
            if(memcmp(&before,&game,sizeof(game)))return 6;
        }
        if(fwrite(&result,1,1,stdout)!=1)return 5;
    }
}
