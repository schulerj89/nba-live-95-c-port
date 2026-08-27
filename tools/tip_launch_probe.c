#define _CRT_SECURE_NO_WARNINGS
#include "nba_tipoff_flow.h"
#include <stdio.h>
#include <string.h>
typedef char ProbeWordSchema[(sizeof(NbaTipLaunch)==35*sizeof(uint16_t))?1:-1];
int main(int argc,char **argv) {
    NbaAssetPack pack={0};uint16_t words[35];
    if(argc!=2 || !nba_assets_load(&pack,argv[1]))return 2;
    while(scanf("%hu",words)==1) {
        for(unsigned i=1;i<35;++i)if(scanf("%hu",words+i)!=1)return 3;
        NbaTipLaunch s;memcpy(&s,words,sizeof(s));if(!nba_tip_launch(&pack,&s))return 4;
        memcpy(words,&s,sizeof(s));for(unsigned i=0;i<35;++i)printf("%u%c",words[i],i==34?'\n':' ');
    }
    nba_assets_free(&pack);return 0;
}
