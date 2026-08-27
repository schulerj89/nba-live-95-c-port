#define _CRT_SECURE_NO_WARNINGS
#include "nba_tipoff_flow.h"
#include <stdio.h>
int main(void) {
    uint16_t v[30];
    while(scanf("%hu",v)==1) {
        for(unsigned i=1;i<30;++i)if(scanf("%hu",v+i)!=1)return 2;
        NbaTipContactInput in={0};
        in.actor_id=v[0];in.actor_inhibit=v[1];in.actor_group=v[2];in.upper_state=v[3];in.upper_lock=v[4];
        in.head_height=v[5];in.free_throw=v[6];in.free_throw_actor=v[7];in.live_state=v[8];in.inbound_group=v[9];
        in.owner=(int16_t)v[10];in.receiver=(int16_t)v[11];in.side_group=(int16_t)v[12];in.hoop_x=(int16_t)v[13];in.shot_latch=v[14];
        in.actor_x=(int16_t)v[15];in.actor_y=(int16_t)v[16];in.actor_z=(int16_t)v[17];
        in.ball_x=(int16_t)v[18];in.ball_y=(int16_t)v[19];in.ball_z=(int16_t)v[20];in.ball_vz=(int16_t)v[21];in.point_count=v[22];
        for(unsigned i=0;i<2;++i){in.points[i].x=(int16_t)v[23+i*3];in.points[i].y=(int16_t)v[24+i*3];in.points[i].z=(int16_t)v[25+i*3];}
        NbaTipContactResult result=nba_tip_contact_geometry(&in);
        printf("%u %u %u\n",result.route,result.request_reach,result.reset_inbound_timer);
    }
    return 0;
}
