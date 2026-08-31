#include "nba_tipoff.h"
#include <stdio.h>
#include <string.h>
typedef struct { unsigned begins,ends;int pending; } Seen;
static void observe(const NbaTipoff *t,const char *phase,void *context) {
    Seen *s=context;
    if(t->simulation_tick&1u){s->pending=-100;return;}
    if(!strcmp(phase,"actors.begin")) {if(s->pending)s->pending=-100;else s->pending=1;++s->begins;}
    else {if(s->pending!=1)s->pending=-100;else s->pending=0;++s->ends;}
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    NbaAssetPack pack={0};NbaSession a,b;NbaTipoff x,y;NbaInput input={0};Seen seen={0};
    if(!nba_assets_load(&pack,argv[1]))return 3;
    nba_session_init(&a);b=a;
    if(!nba_tipoff_init(&x,&pack,&a)||!nba_tipoff_init(&y,&pack,&b))return 4;
    x.differential_observer=observe;x.differential_context=&seen;
    for(unsigned i=0;i<2000;++i) {
        unsigned previous_ends=seen.ends;
        nba_tipoff_update(&x,&input);nba_tipoff_update(&y,&input);
        unsigned executed=seen.ends-previous_ends;
        NbaGameplayTelemetry telemetry;nba_tipoff_capture_telemetry(&x,&input,&telemetry);
        if(executed>1||x.actor_pass_executed!=(executed!=0)||
           telemetry.scheduler_due_raw!=executed||telemetry.actor_pass_dt_raw!=executed*2||
           telemetry.actor_pass_mask_raw!=executed*0x3ff)return 7;
        for(unsigned actor=0;actor<10;++actor)
            if(telemetry.actor_pass_order_raw[actor]!=(executed?actor:0xffu))return 8;
        NbaTipoff xc=x,yc=y;xc.differential_observer=NULL;xc.differential_context=NULL;
        xc.session=yc.session=NULL;
        if(memcmp(&xc,&yc,sizeof(xc))||memcmp(&a,&b,sizeof(a))||seen.pending)return 5;
    }
    if(!seen.begins||seen.begins!=seen.ends||seen.ends!=x.actor_update_tick)return 6;
    /* The diagnostic must clear even when a pause returns before gameplay. */
    if(!nba_tipoff_pause_can_enter(&x))return 9;
    unsigned held_ends=seen.ends;uint32_t held_tick=x.actor_update_tick;
    input.pressed=NBA_BTN_START;nba_tipoff_update(&x,&input);input.pressed=0;
    for(unsigned i=0;i<3;++i) {
        nba_tipoff_update(&x,&input);
        NbaGameplayTelemetry telemetry;nba_tipoff_capture_telemetry(&x,&input,&telemetry);
        if(!nba_tipoff_pause_active(&x)||seen.ends!=held_ends||x.actor_update_tick!=held_tick||
           x.actor_pass_executed||telemetry.scheduler_due_raw||telemetry.actor_pass_mask_raw)return 10;
    }
    printf("DIFFERENTIAL OBSERVER PASS: 2000 unchanged updates, %u actual sweeps\n",seen.ends);
    nba_assets_free(&pack);return 0;
}
