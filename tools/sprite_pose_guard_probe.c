#include "nba_player_lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static bool refused(const NbaAssetPack *pack,const NbaPlayerSpritePoseInput *in) {
    NbaPlayerSpritePoseComposition out,old;
    memset(&out,0xa5,sizeof(out));old=out;
    if (nba_player_compose_sprite_pose(pack,in,&out) || memcmp(&out,&old,sizeof(out))) return false;
    ++checks;return true;
}
static void w32(uint8_t *p,uint32_t v) {
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
int main(int argc,char **argv) {
    if (argc!=2) return 2;
    NbaAssetPack pack={0};if (!nba_assets_load(&pack,argv[1])) return 3;
    NbaPlayerSpritePoseInput valid={332,1168,0,0x591,0x8004,0,6,0x3000,0xffff,32767,-32768};
    NbaPlayerSpritePoseComposition out;
    if (!nba_player_compose_sprite_pose(&pack,&valid,&out)) return 4;
    if (!refused(NULL,&valid) || !refused(&pack,NULL) || nba_player_compose_sprite_pose(&pack,&valid,NULL)) return 5;
    ++checks;
    for (unsigned field=0;field<2;++field) {
        const uint16_t values[]={0x830,0x831,0x7fff,0x8000,0xffff};
        for (unsigned j=0;j<5;++j) {
            NbaPlayerSpritePoseInput in=valid;
            if (field) in.lower_d4=values[j];else in.upper_d6=values[j];
            if (!refused(&pack,&in)) return 6;
        }
    }
    NbaAssetItem *item=NULL;
    for (unsigned i=0;i<pack.item_count;++i) if (pack.items[i].id==NBA_ASSET_PLAYER_ANIMATIONS) item=&pack.items[i];
    if (!item) return 7;
    NbaAssetItem saved=*item;uint8_t *copy=malloc(item->size);if (!copy) return 8;
    memcpy(copy,item->data,item->size);item->data=copy;
    for (unsigned field=24;field<=44;field+=20) {
        const uint32_t values[]={0xffffffff,0xfffffffe,saved.size,saved.size-1,saved.size-0x830};
        for (unsigned j=0;j<5;++j) {
            memcpy(copy,saved.data,saved.size);w32(copy+field,values[j]);
            if (!refused(&pack,&valid)) return 9;
        }
    }
    memcpy(copy,saved.data,saved.size);copy[0]^=1;if (!refused(&pack,&valid)) return 10;
    memcpy(copy,saved.data,saved.size);w32(copy+8,5);if (!refused(&pack,&valid)) return 11;
    memcpy(copy,saved.data,saved.size);w32(copy+12,0);if (!refused(&pack,&valid)) return 12;
    item->size=79;if (!refused(&pack,&valid)) return 13;
    *item=saved;item->data=NULL;if (!refused(&pack,&valid)) return 14;
    *item=saved;free(copy);nba_assets_free(&pack);
    printf("SPRITE_POSE_ATOMIC_GUARDS %u PASS\n",checks);return 0;
}
