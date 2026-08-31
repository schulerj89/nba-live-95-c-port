/* Controlled source-contract guards, not injected native execution.
 * The natural pointer path and original PCs are documented in the module. */
#include "nba_human_dispatch.h"
#include <stdio.h>

#define CHECK(v) do { if (!(v)) { fprintf(stderr,"human quirk line%d\n",__LINE__); return 1; } } while (0)

static NbaHumanMotion initial(void) {
    NbaHumanMotion s = {0};
    s.actor = 5; s.receiver = 0xffff; s.owner = 5;
    s.context_group_0c = 5; s.inbound_group_0952 = 0;
    s.z = 8; s.direction = 8; s.boost = 5; s.controller_word_72 = 7;
    s.dispatch_dt = 2; s.profile_42 = 0x58;
    s.velocity_x = 123; s.velocity_y = -456;
    return s;
}

int main(void) {
    /* AB0C subtracts modulo16 bits; AB0E uses the result's sign, not borrow.
     * Literal expected words include cases where that differs from saturation. */
    static const struct { unsigned before, dt, after; } cases[] = {
        {7,2,5}, {0,2,0}, {7,8,0}, {0x8002,2,0},
        {1,0x8002,0x7fff}, {0xffff,0x8000,0x7fff}, {0xffff,1,0}
    };
    for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        NbaHumanMotion s = initial();
        s.controller_word_72 = (uint16_t)cases[i].before;
        s.dispatch_dt = (uint16_t)cases[i].dt;
        CHECK(nba_human_motion_step(&s) == NBA_HUMAN_MOTION_ACCELERATE);
        CHECK(s.controller_word_72 == cases[i].after && s.boost == 5);
        CHECK(s.velocity_x == 123 && s.velocity_y == -456);
    }
    for (unsigned live = 0x80; live <= 0x81; ++live) {
        NbaHumanMotion s = initial(); s.live_state = (uint16_t)live;
        CHECK(nba_human_motion_step(&s) == NBA_HUMAN_MOTION_ACCELERATE);
        CHECK(s.boost == 3 && s.controller_word_72 == 7);
        CHECK(s.velocity_x == 123 && s.velocity_y == -456);
    }
    NbaHumanMotion s = initial(); s.live_state = 0x8080;
    CHECK(nba_human_motion_step(&s) == NBA_HUMAN_MOTION_ACCELERATE);
    CHECK(s.boost == 5 && s.controller_word_72 == 5);
    s = initial(); s.live_state = 0x807f;
    CHECK(nba_human_motion_step(&s) == NBA_HUMAN_MOTION_ACCELERATE);
    CHECK(s.boost == 3 && s.controller_word_72 == 7);
    s = initial(); s.free_throw = 1;
    CHECK(nba_human_motion_step(&s) == NBA_HUMAN_MOTION_FREE_THROW);
    CHECK(s.boost == 5 && s.controller_word_72 == 7);
    /* Grounded A82C reloads X=actor before its decrement. */
    s = initial(); s.z = 0;
    CHECK(nba_human_motion_step(&s) == NBA_HUMAN_MOTION_ACCELERATE);
    CHECK(s.boost == 3 && s.controller_word_72 == 7);
    /* Blocked A82C never consults the direction vector. */
    s = initial(); s.direction = 0xffff;
    CHECK(nba_human_motion_step(&s) == NBA_HUMAN_MOTION_ACCELERATE);
    CHECK(s.boost == 5 && s.controller_word_72 == 5);
    puts("[HUMAN MOTION QUIRK] PASS:14 controlled carried-X/word/route cases; no native state injection.");
    return 0;
}
