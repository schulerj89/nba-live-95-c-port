#ifndef NBA_SETUP_CODEC_WORK_H
#define NBA_SETUP_CODEC_WORK_H

#include <stdbool.h>
#include <stdint.h>

/* Experimental source continuation, not wired to production. Every accepted
 * cycle represents one native CPU bus/idle cycle; instruction_end is the only
 * point at which a pending interrupt may suspend this continuation. The caller
 * owns clock, refresh, DMA and memory and resolves each read/write at that bus
 * access. No captured timing or output bytes are accepted by this interface.
 */
typedef enum {
    NBA_CODEC_READ,
    NBA_CODEC_WRITE,
    NBA_CODEC_IDLE
} NbaCodecBusKind;

typedef struct {
    uint32_t source_pc;
    uint32_t address;
    uint8_t value;
    uint8_t master_clocks; /* intrinsic FastROM cost; excludes refresh/DMA */
    NbaCodecBusKind kind;
    bool instruction_end;
} NbaCodecBusCycle;

typedef enum {
    NBA_CODEC_WORK_RUNNING,
    NBA_CODEC_WORK_DONE,
    NBA_CODEC_WORK_UNSUPPORTED,
    NBA_CODEC_WORK_LIMIT
} NbaCodecWorkStatus;

typedef struct {
    uint16_t value;         /* live A on C62B entry */
    uint16_t symbol;        /* live X */
    uint16_t stream_cursor; /* live Y */
    uint16_t stack_pointer;
    uint8_t data_bank;
    uint8_t status;         /* native mode, D=0, M=0, X=0 required */
} NbaCodecWorkEntry;

typedef struct {
    NbaCodecBusCycle cycle;
    int8_t read_shift; /* -1 for instruction fetch, 0/8 for operand data */
    bool rmw_value;
} NbaCodecPendingCycle;

typedef struct {
    NbaCodecWorkEntry registers;
    NbaCodecPendingCycle pending[10];
    uint8_t pending_count;
    uint8_t pending_index;
    uint16_t read_value;
    uint16_t operand;
    uint16_t saved_value;
    uint16_t output_size;
    uint16_t return_address[256];
    uint8_t return_kind[256];
    uint16_t return_depth;
    uint32_t resume;
    uint64_t instructions;
    uint64_t instruction_limit;
    int8_t rmw_delta;
    bool branch;
    NbaCodecWorkStatus status;
} NbaSetupCodecWork;

/* Scope: C62B entry to C682 entry, FB46 only. The live bus must contain DP
 * source/destination operands ($0C..$13) and the immediate queue state. Only
 * the empty $86DA helper path is implemented. DB must address WRAM/IO mirrors.
 * Source/destination/stack state comes from the caller, never a native snapshot.
 */
bool nba_setup_codec_work_begin(NbaSetupCodecWork *work,
                                const NbaCodecWorkEntry *entry,
                                uint64_t instruction_limit);
bool nba_setup_codec_work_peek(const NbaSetupCodecWork *work,
                               NbaCodecBusCycle *cycle);
/* The caller has completed the exposed cycle, including a write if any.
 * read_value matters only for READ. This commits an instruction's local effects
 * at its completion; the next instruction's reads remain unresolved.
 */
bool nba_setup_codec_work_accept(NbaSetupCodecWork *work, uint8_t read_value);

#endif
