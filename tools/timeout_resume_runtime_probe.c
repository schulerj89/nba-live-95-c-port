#include "nba_tipoff.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    int frame;
    uint32_t simulation_tick;
    uint16_t clock, shot_clock, rng, session_ticks;
    NbaTipoffActor actors[NBA_GAMEPLAY_ACTOR_COUNT];
    NbaTipoffBall ball;
    NbaGameplayCamera camera;
} Frozen;

static void seed(NbaTipoff *g, NbaSession *s, uint8_t side, uint16_t count) {
    memset(g, 0, sizeof(*g));
    nba_session_init(s);
    /* Test cases are native context0(home)/1(visitor). Frontend ownership
     * remains UI left0/right1; native pause captures establish the inverse. */
    s->player_one_side = side ? 0u : 1u;
    s->match.timeouts_remaining[side] = count;
    s->game_clock_ticks = 91u;
    g->session = s;
    g->is_initialized = true;
    g->phase = NBA_TIPOFF_LIVE;
    g->live_state_raw = 2u;
    g->match_clock_raw_0928 = 4321u;
    g->rim_raw_092c = 987u;
    g->rng.state = 0x5A39u;
    g->frame = 55;
    g->simulation_tick = 77u;
    g->ball.owner_actor = 3;
    g->ball.x_fp = 12345;
    g->ball.velocity_x = -31;
    g->camera.x = 17;
    g->camera.subject_pointer_0940 = 0x34EBu;
    for (unsigned i = 0; i < NBA_GAMEPLAY_ACTOR_COUNT; ++i) {
        g->actors[i].x_fp = (int32_t)(1000 + i * 19u);
        g->actors[i].velocity_x = (int16_t)(i - 5);
        g->actors[i].control_mode = (uint8_t)(i + 1u);
    }
    static const uint16_t stamina[8] =
        {0u,1u,0x0FFFu,0x1000u,0x6FFEu,0x6FFFu,0x7FFFu,0xFFFFu};
    for (unsigned i = 0; i < 24u; ++i) g->fatigue.stamina[i] = stamina[i & 7u];
}

static Frozen frozen(const NbaTipoff *g) {
    Frozen f;
    f.frame = g->frame; f.simulation_tick = g->simulation_tick;
    f.clock = g->match_clock_raw_0928; f.shot_clock = g->rim_raw_092c;
    f.rng = g->rng.state; f.session_ticks = g->session->game_clock_ticks;
    memcpy(f.actors, g->actors, sizeof(f.actors)); f.ball = g->ball;
    f.camera = g->camera;
    return f;
}

static bool remains_frozen(const NbaTipoff *g, const Frozen *f) {
    return g->frame == f->frame && g->simulation_tick == f->simulation_tick &&
        g->match_clock_raw_0928 == f->clock && g->rim_raw_092c == f->shot_clock &&
        g->rng.state == f->rng && g->session->game_clock_ticks == f->session_ticks &&
        memcmp(g->actors, f->actors, sizeof(f->actors)) == 0 &&
        memcmp(&g->ball, &f->ball, sizeof(f->ball)) == 0 &&
        memcmp(&g->camera, &f->camera, sizeof(f->camera)) == 0;
}

static bool side_case(uint8_t side) {
    NbaTipoff g; NbaSession s; seed(&g, &s, side, side ? 3u : 2u);
    NbaInput in = {0}; in.pressed = NBA_BTN_START;
    nba_tipoff_update(&g, &in);
    if (!nba_tipoff_pause_active(&g) || g.live_state_raw != 0x80u ||
        s.match.pause.selected_side != side ||
        s.match.pause.selection != NBA_MATCH_PAUSE_SELECT_TIMEOUT) return false;
    Frozen f = frozen(&g);
    in.pressed = NBA_BTN_A; nba_tipoff_update(&g, &in);
    if (!remains_frozen(&g, &f) ||
        s.match.timeouts_remaining[side] != (uint16_t)((side ? 3u : 2u)-1u) ||
        s.match.timeouts_remaining[side ^ 1u] != NBA_MATCH_INITIAL_TIMEOUTS ||
        g.context_raw_4933 != side * 5u || g.context_raw_4935 != side * 5u ||
        s.match.pause.state != NBA_MATCH_PAUSE_TIMEOUT_TRANSITION) return false;
    static const uint16_t before[8] =
        {0u,1u,0x0FFFu,0x1000u,0x6FFEu,0x6FFFu,0x7FFFu,0xFFFFu};
    for (unsigned i=0;i<24u;++i) {
        uint16_t sum=(uint16_t)(before[i&7u]+0x1000u);
        uint16_t expected=sum>=0x7FFFu?0x7FFFu:sum;
        if(g.fatigue.stamina[i]!=expected)return false;
    }
    for (unsigned i=0;i<60u;++i) nba_tipoff_update(&g, NULL);
    if (!remains_frozen(&g,&f) ||
        s.match.pause.state != NBA_MATCH_PAUSE_MENU_AFTER_TIMEOUT ||
        s.match.pause.selection != NBA_MATCH_PAUSE_SELECT_RESUME) return false;
    in.pressed=NBA_BTN_A; nba_tipoff_update(&g,&in);
    for (unsigned i=0;i<60u;++i) nba_tipoff_update(&g,NULL);
    return remains_frozen(&g,&f) && !nba_tipoff_pause_active(&g) &&
           g.live_state_raw==2u;
}

static bool zero_and_gate_cases(void) {
    NbaTipoff g; NbaSession s; seed(&g,&s,0u,0u);
    NbaInput in={0};in.pressed=NBA_BTN_START;nba_tipoff_update(&g,&in);
    if(s.match.pause.selection!=NBA_MATCH_PAUSE_SELECT_RESUME)return false;
    Frozen f=frozen(&g);
    in.pressed=NBA_BTN_UP;nba_tipoff_update(&g,&in);
    if(s.match.pause.selection!=NBA_MATCH_PAUSE_SELECT_RESUME ||
       s.match.timeouts_remaining[0]!=0u || !remains_frozen(&g,&f))return false;
    in.pressed=NBA_BTN_A;nba_tipoff_update(&g,&in);
    if(s.match.pause.state!=NBA_MATCH_PAUSE_RESUME_TRANSITION)return false;
    for(unsigned i=0;i<60u;++i)nba_tipoff_update(&g,NULL);
    if(nba_tipoff_pause_active(&g)||g.live_state_raw!=2u ||
       s.match.timeouts_remaining[0]!=0u)return false;

    seed(&g,&s,0u,2u);g.phase=NBA_TIPOFF_FORMATION;
    in.pressed=NBA_BTN_START;nba_tipoff_update(&g,&in);
    if(nba_tipoff_pause_active(&g))return false;
    seed(&g,&s,0u,2u);s.match.flow_state=NBA_MATCH_FLOW_HORN_BALL_LIVE;
    nba_tipoff_update(&g,&in);
    return !nba_tipoff_pause_active(&g);
}

int main(void) {
    if(!side_case(0u)||!side_case(1u)||!zero_and_gate_cases()) {
        fputs("timeout/resume runtime mismatch\n",stderr);return 1;
    }
    puts("timeout/resume runtime: both sides +$1000, zero rejection, freeze/restore PASS");
    return 0;
}
