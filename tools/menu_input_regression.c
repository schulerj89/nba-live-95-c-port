/* Deterministic C unit regression, not an independent ROM equivalence gate.
 * Native caller/dispatch parity lives in verify_setup_config_adjustments.py. */
#include "nba_menu_input.h"
#include <stdio.h>
#include <string.h>

#define CHECK(test) do { if(!(test)) { fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#test);return 1; } } while(0)

int main(void) {
    NbaMenuInput s={0};
    uint16_t held[5]={0x0800,0x0400,0x0200,0x0100,0x0080};
    uint8_t types[5]={1,1,1,1,1};
    CHECK(nba_menu_input_native_buttons(NBA_BTN_LEFT|NBA_BTN_RIGHT)==0x0300u);
    CHECK(nba_menu_input_native_buttons(NBA_BTN_B|NBA_BTN_Y|NBA_BTN_A|NBA_BTN_X|NBA_BTN_L|NBA_BTN_R)==0xc0f0u);
    CHECK(nba_menu_input_native_buttons(NBA_BTN_DEBUG_F10|NBA_BTN_DEBUG_F8)==0u);
    nba_menu_input_produce(&s,held,types);
    for(unsigned i=0;i<5u;++i) {
        CHECK(s.controller[i].delay==32u&&s.controller[i].speed==12u);
        CHECK(nba_menu_input_consume(&s)==held[i]);
        CHECK(s.selected_offset==i*2u);
    }
    CHECK(nba_menu_input_consume(&s)==0u&&s.selected_offset==8u);
    s.controller[2].pending=0x0300u;
    s.controller[2].fast=1u;s.controller[2].auxiliary=9u;
    memset(held,0,sizeof(held));nba_menu_input_produce(&s,held,types);
    CHECK(s.controller[2].previous==0u&&s.controller[2].fast==0u&&s.controller[2].auxiliary==0u);
    CHECK(s.controller[2].pending==0x0300u&&s.controller[2].delay==32u&&s.controller[2].speed==12u);
    CHECK(nba_menu_input_consume(&s)==0x0300u&&s.selected_offset==4u);
    s.controller[1].previous=7u;s.controller[1].pending=8u;types[1]=2u;
    held[1]=0xffffu;nba_menu_input_produce(&s,held,types);
    CHECK(s.controller[1].previous==7u&&s.controller[1].pending==8u);
    memset(&s,0,sizeof(s));memset(held,0,sizeof(held));memset(types,0,sizeof(types));types[0]=1u;
    held[0]=0x0100u;s.accelerate=1u;
    /* Literal cadence also appears independently in held-v2 native events. */
    const unsigned expected[]={0,32,43,53,62,70,77,83,88,93,98};unsigned seen=0;
    for(unsigned frame=0;frame<=98u;++frame) {
        nba_menu_input_produce(&s,held,types);
        uint16_t command=nba_menu_input_consume(&s);
        if(command) { CHECK(command==0x0100u&&seen<11u&&expected[seen]==frame);seen++; }
    }
    CHECK(seen==11u&&s.controller[0].fast==1u);
    held[0]=0x0300u;nba_menu_input_produce(&s,held,types);
    CHECK(nba_menu_input_consume(&s)==0x0300u&&s.controller[0].fast==1u);
    CHECK(s.controller[0].delay==32u&&s.controller[0].speed==12u);
    s.controller[0].delay=0u;s.controller[0].auxiliary=77u;
    nba_menu_input_produce(&s,held,types);
    CHECK(s.controller[0].previous==0u&&s.controller[0].pending==0u&&s.controller[0].fast==0u);
    CHECK(s.controller[0].delay==32u&&s.controller[0].speed==12u&&s.controller[0].auxiliary==0u);
    puts("PASS C menu-input regression; independent native parity is a separate gate");return 0;
}
