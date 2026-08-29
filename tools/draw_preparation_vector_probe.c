#include <stdio.h>
#include "nba_gameplay_ai.h"

int main(void) {
    unsigned v[17];
    while (scanf("%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
            &v[0],&v[1],&v[2],&v[3],&v[4],&v[5],&v[6],&v[7],&v[8],
            &v[9],&v[10],&v[11],&v[12],&v[13],&v[14],&v[15],&v[16]) == 17) {
        NbaGameplayDrawPreparationInput input = {
            .direction = {(uint8_t)v[0],(uint8_t)v[1],(uint16_t)v[2],
                (uint16_t)v[3],(uint16_t)v[4],v[5]!=0,
                (int16_t)(uint16_t)v[6],(int16_t)(uint16_t)v[7]},
            .status=(uint16_t)v[2],.upper_resource=(uint16_t)v[8],
            .lower_resource=(uint16_t)v[9],
            .world_x=(int16_t)(uint16_t)v[10],
            .world_y=(int16_t)(uint16_t)v[11],
            .world_z=(int16_t)(uint16_t)v[12],
            .screen_x=(int16_t)(uint16_t)v[13],
            .screen_y=(int16_t)(uint16_t)v[14],
            .head_base=(uint16_t)v[15],.palette_offset=(uint16_t)v[16]
        };
        NbaGameplayDrawPreparation output;
        nba_gameplay_prepare_player_draw(&input,&output);
        printf("%x %x %x %x %x %x %x %x\n",output.direction,output.status,
            output.upper_resource,output.lower_resource,output.head_resource,
            output.attribute,(uint16_t)output.x,(uint16_t)output.y);
    }
    return 0;
}
