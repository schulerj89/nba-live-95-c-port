#ifndef NBA_BOOTSTRAP_FILL_H
#define NBA_BOOTSTRAP_FILL_H
#include "nba_bootstrap_internal.h"

/* Next bounded reset revision. core is the single carried CPU/SPC clock owner;
 * VRAM/DMA add hardware state. Frozen bootstrap checkpoint remains unchanged.
 * Only normal first channel1 mode09 fixed-source VRAM fill is implemented. */
typedef enum { NBA_BOOT_FILL_NONE, NBA_BOOT_FILL_SYNC, NBA_BOOT_FILL_GLOBAL,
    NBA_BOOT_FILL_CHANNEL, NBA_BOOT_FILL_READ, NBA_BOOT_FILL_WRITE,
    NBA_BOOT_FILL_FINISH } NbaBootstrapFillPhase;
typedef struct {
    NbaBootstrap core;
    uint8_t vram[65536];
    uint16_t vram_address, vram_read_buffer;
    uint8_t vram_control;
    struct {
        NbaBootstrapFillPhase phase;
        uint32_t transferred, clock_counter;
        uint8_t source_index, value, cpu_speed;
        bool pending, start_delay;
    } dma;
} NbaBootstrapFill;

/* Observer extension kinds preserve the frozen CPU/SPC event schema. During
 * DMA, core.cpu_cycles counts completed CPU cycles; native current cycle is
 * one greater while that concrete CPU memory access is suspended. */
#define NBA_BOOT_FILL_DMA_EVENT ((NbaBootstrapEventKind)7)
#define NBA_BOOT_FILL_DMA_END_EVENT ((NbaBootstrapEventKind)8)
bool nba_bootstrap_fill_power_on(NbaBootstrapFill *state,const uint8_t *rom,
    size_t rom_size,NbaBootstrapProfile profile);
bool nba_bootstrap_fill_step(NbaBootstrapFill *state,NbaBootstrapObserver observer,void *context);

/* Internal concrete CPU source continuation; not native-entry initialization. */
bool nba_bootstrap_fill_cpu_power_on(NbaBootstrapCpu *state);
bool nba_bootstrap_fill_cpu_peek(const NbaBootstrapCpu *state,NbaCodecBusCycle *cycle);
bool nba_bootstrap_fill_cpu_accept(NbaBootstrapCpu *state,uint8_t value);
#endif
