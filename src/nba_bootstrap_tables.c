#include "nba_bootstrap_tables.h"
/* Compile the immutable first-fill clock/bus owner with the expanded source
 * continuation. These are link-time bindings, not runtime CPU interpretation,
 * source rewriting, state injection or another simultaneously running owner. */
#define nba_bootstrap_fill_cpu_power_on nba_bootstrap_tables_cpu_power_on
#define nba_bootstrap_fill_cpu_peek nba_bootstrap_tables_cpu_peek
#define nba_bootstrap_fill_cpu_accept nba_bootstrap_tables_cpu_accept
#define nba_bootstrap_fill_power_on nba_bootstrap_tables_power_on
#define nba_bootstrap_fill_step nba_bootstrap_tables_step
#include "nba_bootstrap_fill.c"
