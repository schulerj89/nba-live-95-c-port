#define _CRT_SECURE_NO_WARNINGS
#include "nba_tipoff_flow.h"
#include <stdio.h>
int main(int argc,char **argv) {
    NbaAssetPack pack={0};uint16_t v[14];if(argc!=2||!nba_assets_load(&pack,argv[1]))return 2;
    while(scanf("%hu",v)==1) {
        for(unsigned i=1;i<14;++i)if(scanf("%hu",v+i)!=1)return 3;
        NbaGraphicsScratchState s={v[0],v[1]};
        for(unsigned i=0;i<3;++i)s.slots[i]=(NbaGraphicsScratchSlot){v[2+i*4],v[3+i*4],v[4+i*4]};
        if(!nba_graphics_scratch_step(&pack,&s,2))return 4;
        v[0]=s.rng;v[1]=s.scratch_0046;
        for(unsigned i=0;i<3;++i){v[2+i*4]=s.slots[i].record;v[3+i*4]=s.slots[i].current;v[4+i*4]=s.slots[i].timer;}
        for(unsigned i=0;i<14;++i)printf("%s%u",i?" ":"",v[i]);puts("");
    }
    nba_assets_free(&pack);return 0;
}
