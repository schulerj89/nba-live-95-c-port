/* Independent API-order checks derived from native STX/sink boundaries.
 * Controlled C checks only: neither ROM execution nor elapsed-time parity. */
#include "nba_setup_scheduler.h"
#include <stdio.h>
#include <string.h>

typedef struct { NbaSetupPublicationQueue *queue; unsigned calls, failures; } Context;
static void check(Context *c, bool ok) { if (!ok) ++c->failures; }
static void sink(void *opaque, const NbaSetupPublication *p) {
    Context *c = opaque;
    /* Native X advances before DMA, but the memory head stays504 until exit. */
    check(c, c->queue->head == 504);
    if (c->calls == 0) {
        check(c, p->mode == 1 && p->size == 1 && p->source == 0x123456);
    } else {
        check(c, p->mode == 8 && p->vmain == 0 && p->size == 2);
    }
    ++c->calls;
}
static void record(uint8_t *r, unsigned mode, unsigned size) {
    r[0]=(uint8_t)mode; r[1]=0x56; r[2]=0x34; r[3]=0x12;
    r[4]=(uint8_t)size; r[5]=0; r[6]=0x89; r[7]=7;
}
int main(void) {
    NbaSetupPublicationQueue queue = {0};
    Context context = {&queue,0,0};
    queue.head=504; queue.tail=8; queue.budget=231;
    record(queue.records+504,1,1); record(queue.records,0xfd,2);
    check(&context,nba_setup_queue_publish(&queue,sink,&context)==NBA_SETUP_QUEUE_COMPLETE);
    check(&context,context.calls==2 && queue.head==8 && queue.budget==0);
    queue.head=504; queue.budget=230; context.calls=0;
    check(&context,nba_setup_queue_publish(&queue,sink,&context)==NBA_SETUP_QUEUE_BUDGET_EXHAUSTED);
    check(&context,context.calls==1 && queue.head==0 && queue.budget==121);
    /* A rejected API cursor cannot publish the pending palette first. */
    queue.head=505; queue.palette_size=2; context.calls=0;
    check(&context,nba_setup_queue_publish(&queue,sink,&context)==NBA_SETUP_QUEUE_INVALID);
    check(&context,context.calls==0 && queue.head==505 && queue.palette_size==2 && queue.budget==121);
    NbaSetupEpoch epoch={0xffff,0,false}; NbaSetupEpochWait wait={0};
    check(&context,!nba_setup_epoch_wait_ready(&wait,&epoch));
    nba_setup_epoch_wait_begin(&wait,&epoch,true);
    nba_setup_epoch_nmi_increment(&epoch);
    check(&context,epoch.epoch==0 && nba_setup_epoch_wait_ready(&wait,&epoch));
    epoch.interrupt_active=true;
    check(&context,!nba_setup_epoch_wait_ready(&wait,&epoch));
    printf("Independent API callback/cursor and wait checks: %s\n",context.failures ? "FAIL" : "PASS");
    return context.failures ? 1 : 0;
}
