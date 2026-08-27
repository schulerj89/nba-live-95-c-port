#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "nba_owner_flow.h"
#include "nba_player_lab.h"
#include "nba_gameplay_ai.h"
typedef char CheckFlowSize[sizeof(NbaOwnerFlow)==27*sizeof(uint16_t)?1:-1];
typedef char CheckChannelSize[sizeof(NbaPlayerAnimationChannels)==18*sizeof(uint16_t)?1:-1];
static int readwords(uint16_t *v,unsigned n) {
    unsigned x;for(unsigned i=0;i<n;++i){if(scanf_s("%x",&x)!=1)return 0;v[i]=(uint16_t)x;}return 1;
}
typedef struct {unsigned count,next;struct {unsigned id;NbaOwnerFlow in,out;} child[4];} Replay;
static bool callback(void *ctx,NbaOwnerFlow *s,NbaOwnerCall call,unsigned pair) {
    Replay *r=ctx;
    if(r->next>=r->count || r->child[r->next].id!=(unsigned)call ||
       memcmp(s,&r->child[r->next].in,sizeof(*s)) ||
       (call==NBA_OWNER_CALL_POSE && pair!=(unsigned)s->pair_74/2u)) {
        fprintf(stderr,"caller boundary mismatch child=%u call=%u\n",r->next,call);
        if(r->next<r->count)for(unsigned i=0;i<27;++i) {
            uint16_t a[27],b[27];memcpy(a,s,sizeof(a));memcpy(b,&r->child[r->next].in,sizeof(b));
            if(a[i]!=b[i])fprintf(stderr," word%u %04x/%04x\n",i,a[i],b[i]);
        }
        return false;
    }
    *s=r->child[r->next++].out;return true;
}
static bool escape_callback(void *ctx,NbaOwnerFlow *s,NbaOwnerCall call,unsigned pair) {
    (void)pair;
    unsigned *count=ctx;++*count;
    if(call!=NBA_OWNER_CALL_CPU)return false;
    s->mode_5e=17;return false; /* modeled native nonlocal shot return */
}
static bool self_test(void) {
    NbaOwnerFlow s={0};unsigned calls=0;
    s.pair_74=0xffff;s.controller_16=0xffff;s.delta_c8=32;s.rating_3f=17;
    s.timer_60=0;s.live_0936=2;
    if(nba_owner_flow_run(&s,escape_callback,&calls)!=NBA_OWNER_FLOW_ESCAPED ||
       calls!=1 || s.mode_5e!=17 || s.timer_60!=49)return false;
    s.pair_74=3;calls=0;
    return nba_owner_flow_run(&s,escape_callback,&calls)==NBA_OWNER_FLOW_INVALID && !calls;
}
int main(int argc,char **argv) {
    if(!self_test())return 6;
    NbaAssetPack assets;if(argc!=2 || !nba_assets_load(&assets,argv[1]))return 2;
    unsigned kind;
    while(scanf_s("%x",&kind)==1) {
        uint16_t v[27]={0};unsigned n=kind==2?26:27;
        if(!readwords(v,n))return 3;
        bool ok=true;
        if(kind==0) {
            NbaOwnerFlow s;memcpy(&s,v,sizeof(s));Replay r={0};
            if(scanf_s("%x",&r.count)!=1 || r.count>4)return 4;
            for(unsigned i=0;i<r.count;++i) {
                uint16_t words[27];if(scanf_s("%x",&r.child[i].id)!=1)return 5;
                if(!readwords(words,27))return 5;memcpy(&r.child[i].in,words,sizeof(words));
                if(!readwords(words,27))return 5;memcpy(&r.child[i].out,words,sizeof(words));
            }
            NbaOwnerFlowResult result=nba_owner_flow_run(&s,callback,&r);
            ok=result!=NBA_OWNER_FLOW_INVALID && r.next==r.count;
            memcpy(v,&s,sizeof(s));
        } else if(kind==3) {
            v[2]=nba_gameplay_contact_facing((int16_t)v[0],(int16_t)v[1]);
        } else {
            NbaPlayerAnimationChannels c;memcpy(&c,v,sizeof(c));
            if(kind==1)ok=nba_player_owner_unlatched_pose(&assets,&c,(int16_t)v[18],(int16_t)v[19],v[20],&v[21],v[22]!=0,v[23]!=0);
            else {
                uint16_t delta=(uint16_t)((v[20]<<8)|(v[20]>>8));
                ok=nba_player_animation_step_channels(&assets,&c,v[18],v[19],delta,v[21]!=0,v[22],&v[23],&v[24],&v[25]);
            }
            memcpy(v,&c,sizeof(c));
        }
        if(!ok){puts("unsupported");continue;}
        for(unsigned i=0;i<n;++i)printf(i?" %04x":"%04x",v[i]);puts("");
    }
    nba_assets_free(&assets);return 0;
}
