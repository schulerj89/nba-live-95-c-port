#include "nba_audio_events.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    NbaAudioEventOperation operation;
    uint16_t event,crowd,rng;
} Observation;
typedef struct {
    NbaAudioEventState *state;
    uint16_t *rng;
    uint16_t returns[14];
    unsigned return_count,return_index,count;
    int error;
    Observation observations[20];
} Probe;

static uint16_t observe(void *context,const NbaAudioEventOperation *operation) {
    Probe *p=(Probe *)context;
    if(p->count>=20u){p->error=1;return 0;}
    Observation *out=&p->observations[p->count++];
    out->operation=*operation;out->event=p->state->event_bits_13e7;
    out->crowd=p->state->crowd_bits_13e9;out->rng=*p->rng;
    if(operation->kind==NBA_AUDIO_EVENT_COMMAND){
        if(p->return_index>=p->return_count){p->error=1;return 0;}
        return p->returns[p->return_index++];
    }
    return 0;
}

int main(int argc,char **argv) {
    int discard=argc==2 && strcmp(argv[1],"--discard-operations")==0;
    if(argc!=1 && !discard)return 1;
    char line[1024];
    while(fgets(line,sizeof(line),stdin)) {
        unsigned long values[21];unsigned count=0;char *next=line;
        if(strchr(line,'\n')==NULL && !feof(stdin))return 2;
        while(*next){
            while(isspace((unsigned char)*next))++next;
            if(!*next)break;
            if(count>=21u || !isdigit((unsigned char)*next))return 3;
            char *end=NULL;errno=0;unsigned long value=strtoul(next,&end,10);
            if(errno || end==next || value>65535ul || (*end && !isspace((unsigned char)*end)))return 4;
            values[count++]=value;next=end;
        }
        if(count<7u || values[6]>14u || count!=7u+values[6])return 5;
        NbaAudioEventState state={(uint16_t)values[1],(uint16_t)values[2],
                                 (uint16_t)values[3],(uint16_t)values[4]};
        uint16_t rng=(uint16_t)values[5];Probe probe={0};
        probe.state=&state;probe.rng=&rng;probe.return_count=(unsigned)values[6];
        for(unsigned i=0;i<probe.return_count;++i)probe.returns[i]=(uint16_t)values[7+i];
        nba_audio_events_dispatch(&state,&rng,discard?NULL:observe,&probe);
        if(probe.error || (!discard && probe.return_index!=probe.return_count))return 6;
        printf("{\"id\":%lu,\"output\":[%u,%u,%u],\"returns_consumed\":%u,\"operations\":[",
               values[0],state.event_bits_13e7,state.crowd_bits_13e9,rng,probe.return_index);
        for(unsigned i=0;i<probe.count;++i){
            const Observation *o=&probe.observations[i];const NbaAudioEventOperation *op=&o->operation;
            printf("%s[%u,%lu,%lu,%u,%u,%u,%u,%u,%u]",i?",":"",(unsigned)op->kind,
                (unsigned long)op->caller_pc,(unsigned long)op->target_pc,
                op->command,op->index,op->value,o->event,o->crowd,o->rng);
        }
        puts("]}");
    }
    return ferror(stdin)?7:0;
}
