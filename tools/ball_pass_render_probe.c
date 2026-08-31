/* Actual public actor loop, telemetry and renderer. No alternative physics. */
#define _CRT_SECURE_NO_WARNINGS
#include "../src/nba_tipoff.c"
#include <stdio.h>
static NbaTipoff game,before;
static NbaSession session,session_before;
static NbaRenderer renderer;
static NbaGameplayTelemetry telemetry;
static uint64_t hash_bytes(const void *p,size_t n) {
    const unsigned char *b=p;uint64_t h=UINT64_C(14695981039346656037);
    while(n--){h^=*b++;h*=UINT64_C(1099511628211);}return h;
}
int main(int argc,char **argv) {
    if(argc!=2)return 2;NbaAssetPack assets={0};NbaInput input={0};
    if(!nba_assets_load(&assets,argv[1]))return 3;
    nba_session_init(&session);if(!nba_tipoff_init(&game,&assets,&session))return 4;
    nba_renderer_init(&renderer);
    unsigned checked=0,bad=0,passmask=0,rendered=0;
    for(unsigned frame=1;frame<=63800u;++frame) {
        nba_tipoff_update(&game,&input);
        before=game;session_before=session;
        nba_tipoff_capture_telemetry(&game,&input,&telemetry);
        for(unsigned i=0;i<10u;++i) {
            NbaTipoffActor *a=&game.actors[i];NbaGameplayActorTelemetry *t=&telemetry.actors[i];
            if(a->animation_resources_valid) {
                ++checked;
                if(t->draw_upper_resource_raw!=a->upper_animation_resource_raw_2a ||
                   t->draw_lower_resource_raw!=a->lower_animation_resource_raw_2c) {
                    if(bad<8u)printf("BODY_FAIL %u %u %u %u %u %u\n",frame,i,
                        a->upper_animation_resource_raw_2a,a->lower_animation_resource_raw_2c,
                        t->draw_upper_resource_raw,t->draw_lower_resource_raw);
                    ++bad;
                }
                if(a->control_mode==15u)passmask|=1u<<(a->direction&7u);
            }
            /* Only body-resource-derived diagnostic fields are excluded.
             * Every other telemetry byte, including ball/actor coordinates,
             * fractions, velocity, RNG, clock, modes and head direction stays. */
            t->draw_upper_resource_raw=t->draw_lower_resource_raw=0;
            memset(t->appearance_resource_raw,0,sizeof(t->appearance_resource_raw));
            memset(t->appearance_opaque_pixels,0,sizeof(t->appearance_opaque_pixels));
            t->appearance_flags_raw=0;
        }
        printf("STATE %u %016llx\n",frame,(unsigned long long)hash_bytes(&telemetry,sizeof(telemetry)));
        if(frame>=275u && frame<=390u) {
            nba_tipoff_render(&game,&renderer);++rendered;
        }
        if(memcmp(&game,&before,sizeof(game)) || memcmp(&session,&session_before,sizeof(session)))return 6;
    }
    printf("RESULT 63800 %u %u %u %u\n",checked,bad,passmask,rendered);
    nba_assets_free(&assets);return bad?10:0;
}
