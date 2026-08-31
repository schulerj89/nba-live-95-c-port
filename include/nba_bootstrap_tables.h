#ifndef NBA_BOOTSTRAP_TABLES_H
#define NBA_BOOTSTRAP_TABLES_H
#include "nba_bootstrap_fill.h"
/* Same persistent hardware state, expanded concrete CPU source only. No
 * memcpy handoff at80C0 and no second RAM, queue, RNG or clock authority. */
typedef NbaBootstrapFill NbaBootstrapTables;
bool nba_bootstrap_tables_power_on(NbaBootstrapTables *state,const uint8_t *rom,
    size_t rom_size,NbaBootstrapProfile profile);
bool nba_bootstrap_tables_step(NbaBootstrapTables *state,NbaBootstrapObserver observer,void *context);
bool nba_bootstrap_tables_cpu_power_on(NbaBootstrapCpu *state);
bool nba_bootstrap_tables_cpu_peek(const NbaBootstrapCpu *state,NbaCodecBusCycle *cycle);
bool nba_bootstrap_tables_cpu_accept(NbaBootstrapCpu *state,uint8_t value);
#endif
