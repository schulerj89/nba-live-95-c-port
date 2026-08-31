#include "nba_setup_scheduler.h"

static uint16_t read16(const uint8_t *bytes) {
    return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

void nba_setup_epoch_wait_begin(NbaSetupEpochWait *wait,
                                const NbaSetupEpoch *scheduler,
                                bool accumulator8) {
    /* Original quirk: $86B0 never changes M. The header $EF1A caller
     * compares16 bits, but $8959 calls with M=1 and compares only the low
     * byte. Natural v2 waits at labels867/1497 load $8D from epoch$018D.
     * Do not normalize these callers to a16-bit wait. */
    wait->compare_mask = accumulator8 ? 0xFFu : 0xFFFFu;
    wait->loaded_epoch = scheduler->epoch & wait->compare_mask;
    wait->waiting = true;
}

void nba_setup_epoch_nmi_increment(NbaSetupEpoch *scheduler) {
    if (scheduler->epoch_block == 0)
        scheduler->epoch = (uint16_t)(scheduler->epoch + 1u);
}

bool nba_setup_epoch_wait_ready(const NbaSetupEpochWait *wait,
                                const NbaSetupEpoch *scheduler) {
    return wait->waiting && !scheduler->interrupt_active &&
           wait->loaded_epoch != (scheduler->epoch & wait->compare_mask);
}

NbaSetupQueueResult nba_setup_queue_publish(NbaSetupPublicationQueue *queue,
                                           NbaSetupPublicationSink sink,
                                           void *context) {
    NbaSetupPublication publication;
    uint16_t cursor;
    if (!queue || !sink || queue->head >= 0x200u || queue->tail >= 0x200u ||
        (queue->head & 7u) || (queue->tail & 7u))
        return NBA_SETUP_QUEUE_INVALID;
    cursor = queue->head;

    /* $821A tests only the LOW BYTE in M=1, even though $8233 later
     * transfers/clears the full word. Preserve that original contract. */
    if ((queue->palette_size & 0xFFu) != 0) {
        uint16_t original_budget = queue->budget;
        publication.mode = 0;
        publication.bbus = 0x22;
        publication.vmain = 0xFF; /* palette DMA does not change VMAIN */
        publication.source = queue->palette_source & 0xFFFFFFu;
        publication.size = queue->palette_size;
        publication.destination = queue->palette_destination & 0xFFu;
        sink(context, &publication);
        /* SEC; SBC size; SBC #$50 -- the first borrow feeds the second. */
        queue->budget = (uint16_t)(original_budget - queue->palette_size -
                                  0x50u - (original_budget < queue->palette_size));
        queue->palette_size = 0;
    }

    while (cursor != queue->tail) {
        const uint8_t *record = queue->records + cursor;
        uint16_t overhead;
        uint16_t size = read16(record + 4);
        publication.vmain = 0xFF;
        switch (record[0]) {
        case 1: /* $825C-$82A2 */
            overhead = 0x6C;
            publication.mode = 1;
            publication.bbus = 0x18;
            break;
        case 0xFD: /* $82E8-$832F, fixed source to low VRAM port */
            overhead = 0x78;
            publication.mode = 8;
            publication.bbus = 0x18;
            publication.vmain = 0;
            break;
        case 0xFC: /* $8332-$8375, M=1 CMP #$D6 then high port */
            overhead = 0x77;
            publication.mode = 8;
            publication.bbus = 0x19;
            break;
        case 0xD6: /* $8378-$83CB, increment-32 VRAM mode */
            overhead = 0x8A;
            publication.mode = 1;
            publication.bbus = 0x18;
            publication.vmain = 0x81;
            break;
        default:
            return NBA_SETUP_QUEUE_UNSUPPORTED;
        }
        /* Both native BCC exits leave $39 unchanged for this job. */
        if (queue->budget < overhead || queue->budget - overhead < size) {
            queue->head = cursor; /* $82E3: STX $35 */
            return NBA_SETUP_QUEUE_BUDGET_EXHAUSTED;
        }
        queue->budget = (uint16_t)(queue->budget - overhead - size);
        publication.source = record[1] | ((uint32_t)record[2] << 8) |
                             ((uint32_t)record[3] << 16);
        publication.size = size;
        publication.destination = read16(record + 6);
        cursor = (uint16_t)((cursor + 8u) & 0x1FFu);
        sink(context, &publication);
    }
    queue->head = cursor; /* $83CE: STX $35 */
    return NBA_SETUP_QUEUE_COMPLETE;
}
