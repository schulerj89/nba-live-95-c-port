#define _CRT_SECURE_NO_WARNINGS
#include "nba_human_pass_pose.h"
#include <stdio.h>
#include <string.h>

static unsigned char data[0x10000];
typedef char PoseStateMustContain32Words[sizeof(NbaHumanPassPoseState)==32*sizeof(uint16_t)?1:-1];
static void put16(unsigned a, unsigned v) { data[a]=(unsigned char)v;data[a+1]=(unsigned char)(v>>8); }
static void put32(unsigned a, unsigned v) { put16(a,v);put16(a+2,v>>16); }
static void bank(unsigned a,unsigned v) { put16(80+a-0x8000,v); }
int main(void) {
    NbaAssetPack pack={0};pack.is_loaded=true;pack.item_count=1;
    pack.items[0].id=NBA_ASSET_PLAYER_ANIMATIONS;pack.items[0].size=sizeof(data);pack.items[0].data=data;
    unsigned mode, values[32], bytes[5];
    while(scanf("%u",&mode)==1) {
        for(unsigned i=0;i<32;++i)if(scanf("%u",&values[i])!=1||values[i]>65535)return 2;
        for(unsigned i=0;i<5;++i)if(scanf("%u",&bytes[i])!=1||bytes[i]>255)return 2;
        memset(data,0,sizeof(data));memcpy(data,"NBPANIM1",8);put32(8,6);put32(12,57);put32(20,80);
        const unsigned offsets[8]={0x8100,0x8930,0x9160,0x9990,0xa1c0,0xa9f0,0xb220,0xba50};
        put32(24,offsets[0]);put32(56,offsets[2]);put32(60,offsets[3]);put32(64,offsets[4]);
        put32(68,offsets[5]);put32(72,offsets[6]);put32(76,offsets[7]);
        for(unsigned i=0;i<0x830;++i) {
            data[offsets[0]+i]=(unsigned char)bytes[0];data[offsets[1]+i]=(unsigned char)bytes[1];
            for(unsigned point=0;point<2;++point)for(unsigned k=0;k<3;++k)
                data[offsets[2+point*3+k]+i]=(unsigned char)(bytes[2+k]+point);
        }
        bank(0xc218,0x9100);bank(0xc28a,0x9200);bank(0xc2fc,0x9000);
        for(unsigned i=0;i<9;++i){bank(0x9008+2*i,0x9300);bank(0x9108+2*i,0x9400);bank(0x9208+2*i,0x9500);}
        for(unsigned i=0;i<16;++i){bank(0x9300+2*i,114+i);bank(0x9400+2*i,202+i);bank(0x9500+2*i,302+i);}
        uint16_t words[32];for(unsigned i=0;i<32;++i)words[i]=(uint16_t)values[i];
        NbaHumanPassPoseState state;memcpy(&state,words,sizeof(state));bool ok=false;
        switch(mode){case 0:ok=nba_human_pass_pose_offset(&pack,&state);break;
        case 1:ok=nba_human_pass_pose_attach(&pack,&state);break;
        case 2:ok=nba_human_pass_pose_resolve(&pack,&state);break;
        case 3:ok=nba_human_pass_pose_prefix(&pack,&state);break;
        case 4:ok=nba_human_pass_pose_commit(&state);break;
        case 5:ok=nba_human_pass_pose_prepare(&pack,&state);break;default:return 2;}
        memcpy(words,&state,sizeof(state));printf("%u",ok?1u:0u);
        for(unsigned i=0;i<32;++i)printf(" %u",words[i]);puts("");
    }
    return 0;
}
