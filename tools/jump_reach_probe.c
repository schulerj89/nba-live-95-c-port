#define _CRT_SECURE_NO_WARNINGS
#include "nba_tipoff_flow.h"
#include "nba_player_lab.h"
#include <stdio.h>
#include <string.h>
typedef char WordSchema[(sizeof(NbaJumpReachInput)==28*sizeof(uint16_t))?1:-1];
typedef char ChannelSchema[(sizeof(NbaPlayerAnimationChannels)==18*sizeof(uint16_t))?1:-1];
static int guards(NbaAssetPack *pack) {
    NbaJumpReachInput in={0};NbaJumpReachResult out,before;
    memset(&out,0xa5,sizeof(out));before=out;
    NbaAssetPack empty={0};
    if(nba_jump_reach_decide(&empty,&in,&out) || memcmp(&out,&before,sizeof(out)))return 10;
    in.ball_z=in.subject_z=100;in.subject_vz=0xff00;in.receiver=0xffff;
    in.rating_3c=65;in.subject_z=400;
    if(nba_jump_reach_decide(pack,&in,&out) || memcmp(&out,&before,sizeof(out)))return 11;
    if(nba_jump_reach_decide(pack,NULL,&out) || nba_jump_reach_decide(pack,&in,NULL))return 12;
    for(unsigned team=0;team<29;++team)for(unsigned slot=0;slot<12;++slot) {
        uint8_t a,b;
        if(!nba_player_gameplay_jump_ratings(pack,(uint8_t)team,(uint8_t)slot,&a,&b))return 13;
        const NbaAssetItem *item=nba_assets_get(pack,NBA_ASSET_PLAYER_ROSTERS);
        const unsigned char *record=(const unsigned char *)item->data+24+(team*12+slot)*64;
        if(a!=record[29] || b!=record[30])return 15;
    }
    uint8_t a,b;
    if(nba_player_gameplay_jump_ratings(&empty,0,0,&a,&b) ||
       nba_player_gameplay_jump_ratings(pack,29,0,&a,&b) ||
       nba_player_gameplay_jump_ratings(pack,0,12,&a,&b))return 14;
    puts("jump/reach asset and atomic rejection guards passed");return 0;
}
int main(int argc,char **argv) {
    NbaAssetPack pack={0};uint16_t words[28];
    if((argc!=2 && argc!=3) || !nba_assets_load(&pack,argv[1]))return 2;
    if(argc==3 && !strcmp(argv[2],"--guards")){int code=guards(&pack);nba_assets_free(&pack);return code;}
    if(argc==3 && strcmp(argv[2],"--channels"))return 2;
    while(scanf("%hu",words)==1) {
        for(unsigned i=1;i<28;++i)if(scanf("%hu",words+i)!=1)return 3;
        NbaJumpReachInput in;NbaJumpReachResult out;memcpy(&in,words,sizeof(in));
        if(!nba_jump_reach_decide(&pack,&in,&out))return 4;
        printf("%u %u %u %u %u",out.velocity_x,out.velocity_y,out.velocity_z,out.rng,out.request_count);
        for(unsigned i=0;i<out.request_count;++i)printf(" %u %u",out.requests[i].routine,out.requests[i].value);
        if(argc==3) {
            uint16_t raw[18],boost,alternate;
            for(unsigned i=0;i<18;++i)if(scanf("%hu",raw+i)!=1)return 5;
            if(scanf("%hu %hu",&boost,&alternate)!=2)return 5;
            NbaPlayerAnimationChannels channels;memcpy(&channels,raw,sizeof(channels));
            for(unsigned i=0;i<out.request_count;++i) {
                uint32_t pc=out.requests[i].routine;uint16_t value=out.requests[i].value;
                if(pc<0x870000)continue; /* explicitly unimplemented reach children */
                NbaPlayerAnimationCommand command=pc==0x87b3bd?NBA_ANIMATION_INSTALL_BOTH:
                    pc==0x87b47a?NBA_ANIMATION_INSTALL_UPPER:NBA_ANIMATION_INSTALL_LOWER;
                if(!nba_player_animation_command(&pack,&channels,command,&value,boost!=0,alternate!=0))return 6;
            }
            memcpy(raw,&channels,sizeof(raw));for(unsigned i=0;i<18;++i)printf(" %u",raw[i]);
        }
        printf("\n");
    }
    nba_assets_free(&pack);return 0;
}
