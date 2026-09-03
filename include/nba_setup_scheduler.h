#ifndef NBA_SETUP_SCHEDULER_H
#define NBA_SETUP_SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Bounded scheduler primitives translated from the canonical ROM. These do
 * not predict CPU/DMA wall time and are not yet the Setup constructor driver.
 */
typedef struct {
    uint16_t epoch;            /* $0564 */
    uint16_t epoch_block;      /* $059C */
    bool interrupt_active;    /* main execution resumes after RTI */
} NbaSetupEpoch;

typedef struct {
    uint16_t loaded_epoch;     /* A loaded by $80:86B4 */
    uint16_t compare_mask;     /* incoming M is preserved by $86B0 */
    bool waiting;
} NbaSetupEpochWait;

void nba_setup_epoch_wait_begin(NbaSetupEpochWait *wait,
                                const NbaSetupEpoch *scheduler,
                                bool accumulator8);
/* $80:84A3-$84AB. Publication and queue work must precede this call; native
 * callbacks/audio and interrupt return must follow it. */
void nba_setup_epoch_nmi_increment(NbaSetupEpoch *scheduler);
bool nba_setup_epoch_wait_ready(const NbaSetupEpochWait *wait,
                                const NbaSetupEpoch *scheduler);

typedef enum {
    NBA_SETUP_QUEUE_COMPLETE,
    NBA_SETUP_QUEUE_BUDGET_EXHAUSTED,
    NBA_SETUP_QUEUE_UNSUPPORTED,
    NBA_SETUP_QUEUE_INVALID
} NbaSetupQueueResult;

typedef struct {
    uint8_t mode;
    uint8_t bbus;
    /* $FF leaves VMAIN alone. $00/$81 set it before the job and restore
     * $80 afterward, as the two native special branches do. */
    uint8_t vmain;
    uint32_t source;
    uint16_t size;
    uint16_t destination; /* VRAM word address, or CGRAM color index */
} NbaSetupPublication;

typedef void (*NbaSetupPublicationSink)(void *context,
                                       const NbaSetupPublication *publication);

typedef struct {
    uint8_t records[0x200]; /* native $0100-$02FF, not host pointers */
    uint16_t head;         /* $35: published at service exit, not each job */
    uint16_t tail;         /* $37 */
    uint16_t budget;       /* $39, already initialized by NMI */
    uint16_t palette_size;
    uint16_t palette_destination;
    uint32_t palette_source;
} NbaSetupPublicationQueue;

/* $80:821A-$83CE translated DMA modes 1, $FC, $FD and $D6. The other
 * non-DMA/transfer-mode branches fail explicitly. A stopped job retains its
 * budget and cursor. This models publication intent, not bus/scanout time. */
NbaSetupQueueResult nba_setup_queue_publish(NbaSetupPublicationQueue *queue,
                                           NbaSetupPublicationSink sink,
                                           void *context);

#endif
