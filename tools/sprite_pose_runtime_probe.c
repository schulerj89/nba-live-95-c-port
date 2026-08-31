#include "nba_player_lab.h"
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

static bool put_word(uint16_t value) {
    uint8_t bytes[2]={(uint8_t)value,(uint8_t)(value>>8)};
    return fwrite(bytes,1,sizeof(bytes),stdout)==sizeof(bytes);
}

int main(int argc,char **argv) {
    if(_setmode(_fileno(stdout),_O_BINARY)==-1)return 1;
    NbaAssetPack pack={0};
    if(argc!=2 || !nba_assets_load(&pack,argv[1]))return 2;
    if(!nba_assets_player_draw_inputs_valid(&pack))return 3;
    for(uint16_t upper=0;upper<0x830u;++upper)
        for(uint16_t direction=0;direction<8u;++direction) {
            uint16_t order=0xa55au,number=0x5aa5u;
            if(!nba_player_sprite_pose_table_inputs(&pack,upper,direction,
                    &order,&number) || !put_word(order) || !put_word(number))
                return 4;
        }
    for(uint8_t team=0;team<29u;++team)
        for(uint8_t roster=0;roster<12u;++roster)
            for(uint8_t side=0;side<2u;++side) {
                uint8_t teams[10],rosters[10];
                for(unsigned i=0;i<10u;++i){teams[i]=team;rosters[i]=roster;}
                NbaPlayerAppearanceSetup setup;
                uint16_t head=0,palette=0;
                if(!nba_player_appearance_setup(&pack,teams,rosters,&setup) ||
                   !nba_player_sprite_pose_identity(&pack,team,roster,side,
                                                    &head,&palette))return 5;
                const NbaPlayerAppearance *expected=&setup.players[side?5u:0u];
                if(head!=expected->head_resource ||
                   palette!=expected->palette_offset)return 6;
                if(!put_word(head)||!put_word(palette))return 7;
            }
    uint16_t a=0x1234u,b=0x5678u;
    if(nba_player_sprite_pose_table_inputs(&pack,0x830u,0u,&a,&b) ||
       a!=0x1234u || b!=0x5678u ||
       nba_player_sprite_pose_table_inputs(&pack,0u,8u,&a,&b) ||
       a!=0x1234u || b!=0x5678u ||
       nba_player_sprite_pose_identity(&pack,29u,0u,0u,&a,&b) ||
       a!=0x1234u || b!=0x5678u)return 8;
    nba_assets_free(&pack);return 0;
}
