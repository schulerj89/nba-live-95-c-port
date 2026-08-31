#include "nba_controller.h"
#include "nba_rom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char wram[0x20000];
static uint16_t word(unsigned address) {
    if (address + 1 >= sizeof(wram)) { fprintf(stderr,"invalid WRAM address\n"); exit(2); }
    return (uint16_t)(wram[address] | ((uint16_t)wram[address+1] << 8));
}
static void array(const char *name, const uint16_t *v, unsigned count, int comma) {
    printf("%s\"%s\":[",comma ? "," : "",name);
    for (unsigned i=0;i<count;++i) printf("%s%u",i ? "," : "",v[i]);
    printf("]");
}
int main(int argc,char **argv) {
    if (argc != 4) { fprintf(stderr,"mode wram rom\n"); return 2; }
    FILE *f=fopen(argv[2],"rb");
    if (!f || fread(wram,1,sizeof(wram),f)!=sizeof(wram) || fgetc(f)!=EOF) return 2;
    fclose(f);
    NbaControllerState state={0};
    uint16_t selection[5], flags[5];
    for(unsigned pad=0;pad<5;++pad) {
        uint16_t *r=(uint16_t *)&state.record[pad];
        for(unsigned j=0;j<32;++j) r[j]=word(0x47eb+pad*0x40+j*2);
        state.previous_selection[pad]=word(0x1677+2*pad);
        selection[pad]=word(0x166d+2*pad); flags[pad]=word(0x1681+2*pad);
    }
    for(unsigned i=0;i<10;++i) state.actor_assignment[i]=(int16_t)word(0x34eb+i*0x100+0x16);
    for(unsigned i=0;i<2;++i) {
        state.count[i]=word(0x4726+i*0x80); state.cursor[i]=word(0x4728+i*0x80);
    }
    NbaControllerInputContext context={0};
    bool valid=true, input=false, acquire=false;
    uint16_t previous_controller=word(0xa00);
    if(!strcmp(argv[1],"initialize")) valid=nba_controller_initialize(&state,selection,flags,word(0x7f8));
    else if(!strcmp(argv[1],"allocate")) valid=nba_controller_allocate(&state,selection,flags,word(0x7f8));
    else if(!strcmp(argv[1],"transfer")) {
        unsigned target=word(0xc2);
        valid=nba_controller_transfer(&state,target,word(word(0x96)+0x6e));
    } else if(!strcmp(argv[1],"acquire")) {
        acquire=true;
        unsigned target=word(word(0x9a));
        valid=nba_controller_acquire(&state,target,word(word(0x9a)+0x6e),
                                      word(0x946),word(0x944),&previous_controller);
    } else if(!strcmp(argv[1],"input")) {
        input=true;
        unsigned actor_ptr=word(0x96), record_ptr=word(0x9a);
        if(record_ptr<0x47eb || (record_ptr-0x47eb)%0x40 || (record_ptr-0x47eb)/0x40>=5) return 2;
        context.actor=word(actor_ptr); context.actor_z=word(actor_ptr+0x0c);
        NbaRom rom={0}; if(!nba_rom_load_file(&rom,argv[3])) return 2;
        unsigned player=nba_rom_read16(&rom,0x87,(uint16_t)(0x9c8f+2*context.actor));
        nba_rom_free(&rom);
        context.stamina=word(player+0x18); context.boost=word(actor_ptr+0x72);
        context.free_throw_0978=word(0x978); context.live_state_0936=word(0x936);
        context.attachment_09f6=word(0x9f6); context.traveling_17d9=word(0x17d9);
        context.owner_093e=word(0x93e); context.contact_09bc=word(0x9bc);
        context.event_0964=word(0x964); context.whistle_09b6=word(0x9b6);
        context.event_actor_492d=word(0x492d);
        nba_controller_publish_input(&state.record[(record_ptr-0x47eb)/0x40],word(0xaa),&context);
    } else return 2;
    if(!valid) return 3;
    printf("{");
    uint16_t records[160];
    memcpy(records,state.record,sizeof(records));
    array("records",records,160,0);
    array("assignments",(uint16_t *)state.actor_assignment,10,1);
    array("previous",state.previous_selection,5,1);
    array("counts",state.count,2,1); array("cursors",state.cursor,2,1);
    if(input) printf(",\"input_effects\":[%u,%u,%u]",context.boost,context.event_0964,context.event_actor_492d);
    if(acquire) printf(",\"previous_controller\":[%u]",previous_controller);
    printf("}\n");
    return 0;
}
