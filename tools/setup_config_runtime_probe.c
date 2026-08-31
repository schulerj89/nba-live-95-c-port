/* Production menu-caller adapter. Only button schedules arrive on stdin;
 * expected native configuration is never visible to this process.
 * Controlled C entry is the real Game Setup state, omitting the boot intro.
 * Stable configuration boundaries are compared, not native video timing. */
#include "nba_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_words(const uint16_t *words,unsigned count) {
    putchar('[');
    for(unsigned i=0;i<count;++i)printf("%s%u",i?",":"",words[i]);
    putchar(']');
}

static void snapshot(const NbaGame *g,unsigned action) {
    int page=-1,row=-1;
    if(g->state==NBA_STATE_GAME_SETUP) {
        page=(int)g->scene.setup.page;
        row=page==NBA_SETUP_PAGE_MAIN?(int)g->scene.setup.row:g->scene.setup.menu_row;
    }
    printf("CONFIG_STATE {\"action\":%u,\"scene\":%d,\"page\":%d,\"row\":%d,\"main\":",
           action,(int)g->state,page,row);
    print_words(g->session.config.main_values,NBA_SETUP_MAIN_VALUE_COUNT);
    printf(",\"rules\":");print_words(g->session.config.rules,NBA_SETUP_RULE_COUNT);
    printf(",\"options\":");print_words(g->session.config.options,NBA_SETUP_OPTION_COUNT);
    puts("}");
}

static void step(NbaGame *g,uint32_t held) {
    nba_game_input_update(&g->input,held);
    nba_game_tick(g,1.0f/60.0f);
}

static int button(const char *key,uint32_t *value) {
    static const char *names[]={"none","up","down","left","right","a","b","start"};
    static const uint32_t values[]={0u,NBA_BTN_UP,NBA_BTN_DOWN,NBA_BTN_LEFT,
        NBA_BTN_RIGHT,NBA_BTN_A,NBA_BTN_B,NBA_BTN_START};
    for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);++i)if(strcmp(key,names[i])==0){
        *value=values[i];return 1;
    }
    return 0;
}

int main(int argc,char **argv) {
    if(argc!=3)return 2; /* ROM, asset pack */
    NbaGame *g=(NbaGame *)calloc(1,sizeof(*g));
    if(!g)return 3;
    if(!nba_game_init(g,argv[1],argv[2])){free(g);return 4;}
    nba_audio_set_host_playback_enabled(&g->audio,false);
    if(!nba_game_enter_state(g,NBA_STATE_GAME_SETUP)){nba_game_shutdown(g);free(g);return 5;}
    for(unsigned i=0;i<400u;++i)step(g,0u);
    snapshot(g,0u);
    unsigned action=0u;char line[128],key[16],extra;int result=0;
    while(fgets(line,sizeof(line),stdin)) {
        unsigned hold,wait;uint32_t held;
        if(sscanf(line,"%15s %u %u %c",key,&hold,&wait,&extra)!=3 ||
           hold>=wait || wait>1000u || !button(key,&held)) {result=6;break;}
        for(unsigned i=0;i<wait;++i)step(g,i<hold?held:0u);
        snapshot(g,++action);
    }
    if(ferror(stdin)||!action)result=7;
    nba_game_shutdown(g);free(g);return result;
}
