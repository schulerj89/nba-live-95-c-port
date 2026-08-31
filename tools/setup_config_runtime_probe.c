/* Production menu-caller adapter. Only button schedules arrive on stdin;
 * expected native configuration is never visible to this process.
 * Controlled C entry is the real Game Setup state, omitting the boot intro.
 * Stable configuration boundaries are compared, not native video timing. */
#define _CRT_SECURE_NO_WARNINGS
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
    printf(",\"custom\":");print_words(g->session.config.custom_rules,NBA_SETUP_RULE_COUNT);
    printf(",\"working\":");
    if (page == NBA_SETUP_PAGE_MAIN) print_words(g->scene.setup.working_main,4u);
    else if (page == NBA_SETUP_PAGE_RULES) print_words(g->scene.setup.working_rules,13u);
    else if (page == NBA_SETUP_PAGE_OPTIONS) print_words(g->scene.setup.working_options,7u);
    else print_words(NULL,0u);
    puts("}");
}

typedef struct { unsigned action,offset; } ObservationContext;
static void adjustment(void *context,const NbaSetupAdjustmentSnapshot *s) {
    ObservationContext *at=(ObservationContext *)context;
    printf("CONFIG_ADJUST {\"action\":%u,\"offset\":%u,\"pc\":%u,\"command\":%u,"
           "\"row\":%u,\"value\":%u,\"maximum\":%u,\"controller\":%u,"
           "\"previous_input\":%u,\"pending_input\":%u,\"repeat_input\":%u,"
           "\"repeat_delay\":%u,\"repeat_speed\":%u,\"repeat_flag\":%u,\"working\":",
           at->action,at->offset,s->native_pc,s->command,s->row,s->value,s->maximum,
           s->controller,s->previous_input,s->pending_input,s->repeat_input,
           s->repeat_delay,s->repeat_speed,s->repeat_flag);
    print_words(s->working,s->working_count);
    printf(",\"main\":");print_words(s->config.main_values,4u);
    printf(",\"rules\":");print_words(s->config.rules,13u);
    printf(",\"options\":");print_words(s->config.options,7u);
    puts("}");
}

static void step(NbaGame *g,uint32_t held) {
    nba_game_input_update(&g->input,held);
    nba_game_tick(g,1.0f/60.0f);
}

static int button(const char *key,uint32_t *value) {
    static const char *names[]={"none","up","down","left","right","a","b","start","x","y","l","r","select"};
    static const uint32_t values[]={0u,NBA_BTN_UP,NBA_BTN_DOWN,NBA_BTN_LEFT,
        NBA_BTN_RIGHT,NBA_BTN_A,NBA_BTN_B,NBA_BTN_START,NBA_BTN_X,NBA_BTN_Y,NBA_BTN_L,NBA_BTN_R,NBA_BTN_SELECT};
    for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);++i)if(strcmp(key,names[i])==0){
        *value=values[i];return 1;
    }
    return 0;
}

static int buttons(char *keys,uint32_t *value) {
    *value=0u;
    char *start=keys;
    while(start) {
        char *separator=strchr(start,'+');
        if(separator)*separator='\0';
        uint32_t one;
        if(!button(start,&one) || (*value & one) || (!one && (separator || start!=keys)))return 0;
        *value|=one;
        start=separator?separator+1:NULL;
    }
    return 1;
}

int main(int argc,char **argv) {
    if(argc!=3 && !(argc==4 && strcmp(argv[3],"adjustments")==0))return 2;
    NbaGame *g=(NbaGame *)calloc(1,sizeof(*g));
    if(!g)return 3;
    if(!nba_game_init(g,argv[1],argv[2])){free(g);return 4;}
    nba_audio_set_host_playback_enabled(&g->audio,false);
    if(!nba_game_enter_state(g,NBA_STATE_GAME_SETUP)){nba_game_shutdown(g);free(g);return 5;}
    for(unsigned i=0;i<400u;++i)step(g,0u);
    ObservationContext observation={0};
    if(argc==4) {
        g->scene.setup.adjustment_observer=adjustment;
        g->scene.setup.adjustment_observer_context=&observation;
    }
    snapshot(g,0u);
    unsigned action=0u;char line[128],key[64],extra;int result=0;
    while(fgets(line,sizeof(line),stdin)) {
        unsigned hold,wait;uint32_t held;
        if(sscanf(line,"%63s %u %u %c",key,&hold,&wait,&extra)!=3 ||
           hold>wait || wait==0u || wait>1000u || !buttons(key,&held)) {result=6;break;}
        observation.action=action+1u;
        for(unsigned i=0;i<wait;++i) {
            observation.offset=i;
            step(g,i<hold?held:0u);
        }
        snapshot(g,++action);
    }
    if(ferror(stdin)||!action)result=7;
    nba_game_shutdown(g);free(g);return result;
}
