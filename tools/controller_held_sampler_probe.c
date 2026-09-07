#include "nba_assets.h"
#include "nba_controller.h"
#include "nba_session.h"
#include "nba_tipoff.h"

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

/* Host-only test helper; no direct native address. Check invalid portable
 * inputs supporting `$87:9B30-$9B37` without allowing an output mutation. */
static bool sampler_boundary_cases(void) {
    static const uint16_t inputs[NBA_CONTROLLER_COUNT] = {
        0x8123u,0x4567u,0x89ABu,0xCDEFu,0x1357u
    };
    uint16_t held=0xA55Au;
    bool valid=true;
    for (unsigned pad=0;pad<NBA_CONTROLLER_COUNT;++pad) {
        held=0u;
        valid=valid && nba_controller_sample_held(inputs,pad,&held) &&
              held==inputs[pad];
    }
    held=0xA55Au;
    valid=valid && !nba_controller_sample_held(inputs,NBA_CONTROLLER_COUNT,&held) &&
          held==0xA55Au;
    valid=valid && !nba_controller_sample_held(NULL,0u,&held) && held==0xA55Au;
    valid=valid && !nba_controller_sample_held(inputs,0u,NULL);
    return valid;
}

/* Host-only production-caller test; no direct native entry address. Exercise
 * the production behavior pass around `$87:9B30-$9B37`, including distinct
 * pad samples, publication, boost, and the shared-pad process gate. */
static bool production_caller_cases(const char *asset_path) {
    static const uint16_t held[NBA_CONTROLLER_COUNT] = {
        0x8123u,0x4567u,0x89ABu,0xCDEFu,0x1357u
    };
    static const uint16_t direction[NBA_CONTROLLER_COUNT] = {1u,1u,2u,8u,8u};
    NbaAssetPack assets={0};
    NbaSession session;
    NbaTipoff tipoff;
    if (!nba_assets_load(&assets,asset_path)) return false;
    nba_session_init(&session);
    bool ok=nba_tipoff_init(&tipoff,&assets,&session);
    if (!ok) {nba_assets_free(&assets);return false;}

    ok=ok && tipoff.cpu_vs_cpu;
    for (unsigned actor=0;actor<NBA_GAMEPLAY_ACTOR_COUNT;++actor)
        ok=ok && tipoff.actors[actor].controller_assignment_raw==-1 &&
           tipoff.controllers.actor_assignment[actor]==-1;
    for (unsigned pad=0;pad<NBA_CONTROLLER_COUNT;++pad)
        ok=ok && tipoff.controllers.record[pad].group==-1 &&
           tipoff.controllers.previous_selection[pad]==1u;
    ok=ok && tipoff.controllers.count[0]==0u && tipoff.controllers.count[1]==0u;

    /* The existing frontend/headless boundary supplies only pad zero. The
     * five-word storage must convert that host mask and clear pads one-four. */
    for (unsigned pad=0;pad<NBA_CONTROLLER_COUNT;++pad)
        tipoff.pad_held_raw[pad]=0xFFFFu;
    NbaInput frontend_input={0};
    frontend_input.held=NBA_BTN_B|NBA_BTN_RIGHT|NBA_BTN_L;
    nba_tipoff_update(&tipoff,&frontend_input);
    ok=ok && tipoff.pad_held_raw[0]==0x8120u;
    for (unsigned pad=1;pad<NBA_CONTROLLER_COUNT;++pad)
        ok=ok && tipoff.pad_held_raw[pad]==0u;
    for (unsigned actor=0;actor<NBA_GAMEPLAY_ACTOR_COUNT;++actor)
        ok=ok && tipoff.actors[actor].controller_assignment_raw==-1 &&
           tipoff.controllers.actor_assignment[actor]==-1;
    for (unsigned pad=0;pad<NBA_CONTROLLER_COUNT;++pad)
        ok=ok && tipoff.controllers.record[pad].group==-1 &&
           tipoff.controllers.previous_selection[pad]==1u;

    tipoff.live_state_raw=0x0082u;
    tipoff.fouls.free_throw_state_raw_0978=0u;
    for (unsigned actor=0;actor<NBA_GAMEPLAY_ACTOR_COUNT;++actor) {
        tipoff.actors[actor].controller_assignment_raw=-1;
        tipoff.actors[actor].control_mode=7u;
        tipoff.actors[actor].reaction_threshold=0x0100u;
        tipoff.actors[actor].velocity_x=0;
        tipoff.actors[actor].velocity_y=0;
    }
    for (unsigned pad=0;pad<NBA_CONTROLLER_COUNT;++pad) {
        tipoff.pad_held_raw[pad]=held[pad];
        tipoff.controllers.record[pad].previous=0u;
        tipoff.controllers.record[pad].alternate_direction=pad==1u ? 1u : 0u;
        tipoff.actors[pad].controller_assignment_raw=(int16_t)pad;
        tipoff.controllers.actor_assignment[pad]=(int16_t)pad;
        unsigned roster=tipoff.fatigue.active_roster[pad];
        if (roster>=24u) ok=false;
        else tipoff.fatigue.stamina[roster]=0x0800u;
    }
    /* A second actor shares pad zero. If the process gate fails, its second
     * publication clears pad zero's changed/pressed words and its boost. */
    tipoff.actors[5].controller_assignment_raw=0;
    tipoff.controllers.actor_assignment[5]=0;
    tipoff.actors[5].movement_boost_timer=0x7777u;
    nba_tipoff_replay_actor_behavior_sweep(&tipoff);

    for (unsigned pad=0;pad<NBA_CONTROLLER_COUNT;++pad) {
        const NbaControllerRecord *record=&tipoff.controllers.record[pad];
        ok=ok && record->processed==1u && record->held==held[pad] &&
           record->previous==held[pad] && record->changed==held[pad] &&
           record->pressed==held[pad] && record->direction==direction[pad] &&
           tipoff.actors[pad].movement_boost_timer==5u;
    }
    ok=ok && tipoff.actors[5].movement_boost_timer==0x7777u &&
       !tipoff.controller_contract_fault;
    nba_assets_free(&assets);
    return ok;
}

/* Host-only executable fixture; no direct native address. Replay retained
 * `$87:9B30-$9B37` inputs through the production sampler and run its caller. */
int main(int argc,char **argv) {
    if (argc!=2 && !(argc==3 && strcmp(argv[2],"--caller-test")==0)) {
        fprintf(stderr,"usage: %s <asset-pack> [--caller-test]\n",argv[0]);
        return 2;
    }
    if (!sampler_boundary_cases()) return 3;
    if (argc==3) {
        bool ok=production_caller_cases(argv[1]);
        if (ok) puts("[CONTROLLER HELD SAMPLER CALLER] PASS");
        return ok ? 0 : 4;
    }
    _setmode(_fileno(stdin),_O_BINARY);
    _setmode(_fileno(stdout),_O_BINARY);
    uint16_t input[1u+NBA_CONTROLLER_COUNT];
    while (fread(input,sizeof(input),1u,stdin)==1u) {
        uint16_t sampled=0xA55Au;
        if (!nba_controller_sample_held(input+1u,input[0],&sampled) ||
            fwrite(&sampled,sizeof(sampled),1u,stdout)!=1u) return 5;
    }
    return feof(stdin) && !ferror(stdin) ? 0 : 5;
}
