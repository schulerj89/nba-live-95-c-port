/* Independent controlled C edges derived from original instruction paths.
 * These are not natural native trajectories or injected native captures. */
#include "nba_controller.h"
#include "nba_rom.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"audit line %u: %s\n",__LINE__,#x); return 1; } } while(0)

int main(int argc,char **argv) {
    if(argc!=2) return 2;
    NbaRom rom={0};CHECK(nba_rom_load_file(&rom,argv[1]));
    NbaControllerState s;memset(&s,0xa5,sizeof(s));
    uint16_t selection[5]={2,0,2,1,0},flags[5]={0,0x8000,0,0,0};
    CHECK(sizeof(NbaControllerRecord)==64);
    CHECK(nba_controller_initialize(&s,selection,flags,0));
    CHECK(s.record[0].actor==0 && s.record[1].actor==5 && s.record[2].actor==1 && s.record[4].actor==6);
    CHECK(s.count[0]==2 && s.count[1]==2 && s.record[3].actor==0xa5a5 && s.record[3].group==-1);
    CHECK(s.cursor[0]==0xa5a5 && s.cursor[1]==0xa5a5);
    for(unsigned p=0;p<5;++p) {
        CHECK(s.record[p].processed==0xa5a5 && s.record[p].held==0xa5a5);
        for(unsigned w=0;w<23;++w) CHECK(s.record[p].reserved[w]==0xa5a5);
    }
    /* E294 unchanged+override intentionally leaves the old groups/actors,
     * despite cleared counts and assignment words. */
    CHECK(nba_controller_allocate(&s,selection,flags,1));
    CHECK(s.count[0]==0 && s.count[1]==0 && s.record[0].group==0 && s.record[1].group==5);
    for(unsigned actor=0;actor<10;++actor) CHECK(s.actor_assignment[actor]==-1);
    CHECK(s.record[1].alternate_direction==1);
    /* E28B masks current, E367 saves unmasked. A saved high bit therefore
     * makes the next reallocation changed, even for the same input word. */
    selection[0]=0x8002;for(unsigned p=1;p<5;++p) selection[p]=1;
    CHECK(nba_controller_initialize(&s,selection,flags,0));
    s.record[0].actor=3;s.actor_assignment[0]=-1;s.actor_assignment[3]=0;
    CHECK(nba_controller_allocate(&s,selection,flags,0));
    CHECK(s.record[0].actor==0 && s.previous_selection[0]==0x8002 && s.actor_assignment[3]==-1);

    /* BCAB clamps out-of-range cursor, whereas D2DC does not. */
    s.cursor[0]=7;NbaControllerState before=s;uint16_t previous=0;
    CHECK(!nba_controller_acquire(&s,2,0,0xffff,0xffff,&previous));
    CHECK(previous==0xffff && memcmp(&before,&s,sizeof(s))==0);
    CHECK(nba_controller_transfer(&s,2,0));
    CHECK(s.record[0].actor==2 && s.cursor[0]==1 && s.actor_assignment[0]==-1);
    /* D29F designated receiver ignores zero opposite-team count, retains
     * record group, and leaves BOTH cursors untouched. */
    s.cursor[0]=4;s.cursor[1]=3;
    CHECK(nba_controller_acquire(&s,7,5,7,0,&previous));
    CHECK(previous==0xffff && s.record[0].actor==7 && s.record[0].group==0);
    CHECK(s.cursor[0]==4 && s.cursor[1]==3 && s.count[1]==0 && s.actor_assignment[7]==0 && s.actor_assignment[2]==-1);
    before=s;
    CHECK(nba_controller_acquire(&s,8,5,8,0x10,&previous));
    CHECK(memcmp(&before,&s,sizeof(s))==0);
    CHECK(nba_controller_acquire(&s,8,5,8,0xffff,&previous));
    CHECK(memcmp(&before,&s,sizeof(s))==0);
    for(unsigned p=0;p<5;++p)s.record[p].processed=(uint16_t)(p+1);
    before=s;nba_controller_begin_sweep(&s);
    for(unsigned p=0;p<5;++p) {CHECK(s.record[p].processed==0);before.record[p].processed=0;}
    CHECK(memcmp(&before,&s,sizeof(s))==0);

    NbaControllerRecord r={0};NbaControllerInputContext c={0};
    for(unsigned alternate=0;alternate<2;++alternate) {
        r.alternate_direction=(uint16_t)alternate;
        for(unsigned dir=0;dir<16;++dir) {
            nba_controller_publish_input(&r,(uint16_t)(dir<<8),&c);
            CHECK(r.direction==nba_rom_read16(&rom,0x85,(uint16_t)((alternate?0xefed:0xf00d)+2*dir)));
        }
    }
    r=(NbaControllerRecord){0};c=(NbaControllerInputContext){0};
    c.stamina=0x7ff;nba_controller_publish_input(&r,0x0130,&c);CHECK(c.boost==0);
    c.stamina=0x800;nba_controller_publish_input(&r,0x0130,&c);CHECK(c.boost==5 && r.pressed==0);
    c.free_throw_0978=1;c.stamina=0;nba_controller_publish_input(&r,0,&c);
    CHECK(c.boost==5 && r.changed==0x0130 && r.pressed==0 && r.direction==8);
    /* EFA6 BPL tests wrapped CMP sign. These two values distinguish it
     * from an unsigned >=80 replacement, without asserting reachability. */
    c=(NbaControllerInputContext){0};c.actor=7;c.owner_093e=7;c.traveling_17d9=1;c.attachment_09f6=3;
    c.live_state_0936=0x807f;nba_controller_publish_input(&r,0x0100,&c);CHECK(c.event_0964==0);
    c.live_state_0936=0x8080;nba_controller_publish_input(&r,0x0100,&c);CHECK(c.event_0964==5 && c.event_actor_492d==7);
    c.event_0964=0;c.actor_z=1;nba_controller_publish_input(&r,0x0100,&c);CHECK(c.event_0964==0);
    c.actor_z=0;c.contact_09bc=1;nba_controller_publish_input(&r,0x0100,&c);CHECK(c.event_0964==0);
    c.contact_09bc=0;c.whistle_09b6=1;nba_controller_publish_input(&r,0x0100,&c);CHECK(c.event_0964==0);
    c.whistle_09b6=0;c.traveling_17d9=0;nba_controller_publish_input(&r,0x0100,&c);CHECK(c.event_0964==0);
    nba_rom_free(&rom);
    printf("{\"passed\":true,\"checks\":%u,\"scope\":\"controlled C branch/quirk guards and original ROM direction tables\"}\n",checks);
    return 0;
}
