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
        nba_tipoff_update(&x,&input);nba_tipoff_update(&y,&input);
        NbaTipoff xc=x,yc=y;xc.differential_observer=NULL;xc.differential_context=NULL;
        xc.session=yc.session=NULL;
        if(memcmp(&xc,&yc,sizeof(xc))||memcmp(&a,&b,sizeof(a))||seen.pending)return 5;
    }
    if(!seen.begins||seen.begins!=seen.ends||seen.ends!=x.actor_update_tick)return 6;
    printf("DIFFERENTIAL OBSERVER PASS: 2000 unchanged updates, %u actual sweeps\n",seen.ends);
    nba_assets_free(&pack);return 0;
}
