#ifndef NBA_BOOTSTRAP_NMI_H
#define NBA_BOOTSTRAP_NMI_H
#include "nba_bootstrap_fill.h"

/* Still one console owner. Hardware below is newly represented state, not
 * another CPU/SPC clock, RAM, queue, RNG or controller-data authority. */
typedef struct {
    bool enabled, flag, need;
    uint8_t delay, open_bus;
    bool irq_flag;
    bool auto_enabled, auto_active, auto_disabled, auto_strobe;
    uint64_t auto_start, auto_next;
    bool controller_pending, requested_strobe;
    bool active, after_dma;
    uint8_t phase;
    uint16_t vector;
    uint32_t return_pc, trigger_pc;
    uint64_t entries;
} NbaBootstrapNmiState;

typedef struct {
    NbaBootstrapFill machine; /* First member: all existing carried hardware. */
    NbaBootstrapNmiState nmi;
} NbaBootstrapNmi;

#define NBA_BOOT_NMI_BUS_EVENT ((NbaBootstrapEventKind)9)
#define NBA_BOOT_NMI_ENTRY_EVENT ((NbaBootstrapEventKind)10)

/* ROM-only cold initialization. No native prestate or controller/SPC reply
 * inputs. Stops before unowned controller strobe/serial input, IRQ modes,
 * first OAM write at80:8184, or the existing DSP/source boundary. IRQ entry
 * is owned only at the literal80:AACD/AAD0 sound poll. No RTI or later sound
 * response is supplied. A refusal is terminal, including a hardware refusal
 * within a cycle; it is not an adapter resume point. */
bool nba_bootstrap_nmi_power_on(NbaBootstrapNmi *state,const uint8_t *rom,
    size_t rom_size,NbaBootstrapProfile profile);
bool nba_bootstrap_nmi_step(NbaBootstrapNmi *state,NbaBootstrapObserver observer,void *context);
bool nba_bootstrap_nmi_cpu_power_on(NbaBootstrapCpu *state);
bool nba_bootstrap_nmi_cpu_peek(const NbaBootstrapCpu *state,NbaCodecBusCycle *cycle);
bool nba_bootstrap_nmi_cpu_enter_vector(NbaBootstrapCpu *state,uint16_t vector);
bool nba_bootstrap_nmi_cpu_accept(NbaBootstrapCpu *state,uint8_t value);
#endif
