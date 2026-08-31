#ifndef NBA_BOOTSTRAP_INTERNAL_H
#define NBA_BOOTSTRAP_INTERNAL_H
#include "nba_bootstrap.h"
bool nba_bootstrap_rom_valid(const uint8_t *rom,size_t size);
bool nba_bootstrap_cpu_power_on(NbaBootstrapCpu *s);
bool nba_bootstrap_cpu_peek(const NbaBootstrapCpu *s,NbaCodecBusCycle *out);
bool nba_bootstrap_cpu_accept(NbaBootstrapCpu *s,uint8_t value);
NbaSetupSpcResidentWork nba_bootstrap_ipl_peek(const NbaSetupSpcInit *s);
bool nba_bootstrap_ipl_accept(NbaSetupSpcInit *s,uint8_t value);
uint8_t nba_bootstrap_ipl_byte(uint16_t address);
void nba_bootstrap_timer_step(NbaSetupSpcControlTimer *timer,uint8_t rate);
#endif
