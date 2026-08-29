/* Strict raw-word trace from real C state, not rendered/rounded telemetry. */
#include "nba_tipoff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { FILE *file; unsigned sequence, sweeps; } Trace;
static void word(FILE *f, unsigned address, uint16_t value, unsigned *count) {
    fprintf(f,"%s\"%04x\":%u",(*count)++ ? "," : "",address,value);
}
static void capture(const NbaTipoff *t,const char *phase,void *context) {
    Trace *trace=context;FILE *f=trace->file;unsigned count=0;
    if(!strcmp(phase,"actors.end"))++trace->sweeps;
    fprintf(f,"{\"sequence\":%u,\"checkpoint\":\"%s\",\"outer_frame\":%d,\"inputs\":[0,0,0,0,0],\"state\":{",
        trace->sequence++,phase,t->frame);
#define WORD(address,name,expression) word(f,address,(uint16_t)(expression),&count);
#define ACTOR_WORD(address,name,expression)
#include "differential_fields.def"
#undef WORD
#undef ACTOR_WORD
    for(unsigned i=0;i<10;++i) {
        const NbaTipoffActor *a=&t->actors[i];
#define WORD(address,name,expression)
#define ACTOR_WORD(address,name,expression) word(f,0x34eb+i*256+address,(uint16_t)(expression),&count);
#include "differential_fields.def"
#undef WORD
#undef ACTOR_WORD
    }
    fputs("},\"writers\":{}}\n",f);fflush(f);
}
int main(int argc,char **argv) {
    if(argc!=4)return 2;
    char *end=NULL;long sweeps=strtol(argv[3],&end,10);
    if(!end || *end || sweeps<1 || sweeps>1000)return 2;
    NbaAssetPack pack={0};NbaSession session;NbaTipoff game;NbaInput input={0};
    if(!nba_assets_load(&pack,argv[1]))return 3;
    nba_session_init(&session);
    if(!nba_tipoff_init(&game,&pack,&session))return 4;
    FILE *f=fopen(argv[2],"wb");if(!f)return 5;
    Trace trace={f,0,0};game.differential_observer=capture;game.differential_context=&trace;
    capture(&game,"baseline",&trace);
    for(unsigned frame=0;trace.sweeps<(unsigned)sweeps && frame<20000;++frame)
        nba_tipoff_update(&game,&input);
    int result=trace.sweeps==(unsigned)sweeps && !ferror(f)?0:6;
    if(fclose(f))result=7;
    nba_assets_free(&pack);return result;
}
