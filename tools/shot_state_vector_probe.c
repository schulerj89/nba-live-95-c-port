#include <stdio.h>
#include <string.h>
#include "nba_shot_state.h"
typedef struct {
    uint16_t op,amount,input[5];
    NbaShotMomentum momentum;
    NbaShotFatigue fatigue;
    NbaShotClock clock;
} Row;
typedef char ShotStateRowMustBe292Bytes[(sizeof(Row)==292)?1:-1];
int main(int argc,char **argv) {
    NbaAssetPack pack;
    if(argc!=2 || !nba_assets_load(&pack,argv[1]))return 2;
    uint16_t words[146];
    while(scanf_s("%hx",&words[0])==1) {
        for(unsigned i=1;i<146;++i)if(scanf_s("%hx",&words[i])!=1)return 3;
        Row r;memcpy(&r,words,sizeof(r));
        bool ok=true;
        switch(r.op) {
        case 0:ok=nba_shot_momentum_make(&r.momentum,r.input[0],r.input[1],r.input[2],r.input[3],r.input[4]);break;
        case 1:ok=nba_shot_fatigue_step(&pack,&r.fatigue);break;
        case 2:ok=nba_shot_stamina_recover(&pack,&r.fatigue);break;
        case 3:nba_shot_stamina_grant(&r.fatigue,r.amount);break;
        case 4:nba_shot_stamina_fixed_grant(&r.fatigue);break;
        case 5:nba_shot_stamina_init(&r.fatigue);break;
        case 6:nba_shot_momentum_reset(&r.momentum);break;
        case 7:nba_shot_clock_step(&r.clock);break;
        case 8:nba_shot_fatigue_timer_init(&r.fatigue);break;
        default:return 3;
        }
        if(!ok){fprintf(stderr,"invalid shot-state input op=%u\n",r.op);return 4;}
        memcpy(words,&r,sizeof(r));
        for(unsigned i=0;i<146;++i)printf("%04x%c",words[i],i==145?'\n':' ');
    }
    nba_assets_free(&pack);return 0;
}
