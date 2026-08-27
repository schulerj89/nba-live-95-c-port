#define _CRT_SECURE_NO_WARNINGS
#include "nba_tipoff.h"
#include <stdio.h>
int main(int argc,char **argv) {
    if(argc==2) {
        NbaAssetPack pack={0};NbaSession session;NbaTipoff game;
        if(!nba_assets_load(&pack,argv[1]))return 2;
        nba_session_init(&session);
        for(unsigned side=0;side<2;++side)for(unsigned bit=0;bit<2;++bit) {
            if(!nba_tipoff_init(&game,&pack,&session))return 3;
            game.tip_contact_actor=(int8_t)(side*5);game.rng.state=bit?0x8000:1;
            game.tip_event_bits_raw_13e9=0x40;
            if(!nba_tipoff_select_tip_receiver(&game))return 4;
            unsigned receiver=side*5+3+bit;
            if(game.pass_receiver_raw!=(int)receiver || game.receiver_actor!=receiver ||
               game.pass_actor_raw!=10 || game.actors[receiver].control_mode!=10 ||
               game.tip_event_bits_raw_13e9!=(0x40|(side?2:4)) ||
               game.tip_event.duration_1477!=600)return 5;
            uint16_t rng=game.rng.state;
            if(nba_tipoff_select_tip_receiver(&game)||game.rng.state!=rng)return 6;
        }
        nba_assets_free(&pack);puts("TIP RECEIVER binding PASS: both sides, both RNG choices, single consumption");return 0;
    }
    NbaTipReceiver s={0};
    while(scanf("%hu %hu %hu %hu",&s.rng,&s.actor_id,&s.team_group,&s.event_bits)==4) {
        nba_tip_receiver_select(&s);nba_tip_receiver_finish(&s);
        printf("%u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",s.rng,s.passer,s.receiver,
            s.pass_family,s.pass_band,s.receiver_mode,s.event_bits,s.event.timer_140f,
            s.event.active_148f,s.event.enabled_14a7,s.event.duration_1477,s.event.address_14bf,
            s.event.kind_1430,s.event.bank_1448);
    }
    return 0;
}
