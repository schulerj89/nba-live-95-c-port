#include "nba_assets.h"
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    NbaAssetPack pack={0};
    if(!nba_assets_load(&pack,argv[1]))return 1;
    nba_assets_free(&pack);
    return 0;
}
