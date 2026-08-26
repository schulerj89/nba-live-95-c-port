/* Replays `$85:96B5-$9961` through the production actor commit helper. */
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>

#include "nba_gameplay_ai.h"

#define WRAM_SIZE 0x4B00u

static uint16_t word(const uint8_t *raw, unsigned address) {
    return (uint16_t)(raw[address] | ((uint16_t)raw[address + 1u] << 8));
}

int main(void) {
    uint8_t raw[WRAM_SIZE];
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, sizeof(raw), stdin) == sizeof(raw)) {
        unsigned base = word(raw, 0x0096u);
        if (base + 0xA3u >= WRAM_SIZE) return 2;
        NbaGameplayActorCommit actor = {
            .x_fraction = word(raw, base + 0x02u),
            .x = (int16_t)word(raw, base + 0x04u),
            .y_fraction = word(raw, base + 0x06u),
            .y = (int16_t)word(raw, base + 0x08u),
            .z_fraction = word(raw, base + 0x0Au),
            .z = (int16_t)word(raw, base + 0x0Cu),
            .velocity_x = (int16_t)word(raw, base + 0x0Eu),
            .velocity_y = (int16_t)word(raw, base + 0x10u),
            .velocity_z = (int16_t)word(raw, base + 0x12u),
            .speed_raw_4a = word(raw, base + 0x4Au),
            .movement_distance_raw_4c = word(raw, base + 0x4Cu),
            .facing_raw_4e = (uint8_t)word(raw, base + 0x4Eu),
            .behavior_flags_raw_7e = word(raw, base + 0x7Eu),
            .velocity_direction_raw_a2 = (uint8_t)word(raw, base + 0xA2u),
            .previous_x_fraction_raw_94 = word(raw, base + 0x94u),
            .previous_x_raw_96 = (int16_t)word(raw, base + 0x96u),
            .previous_y_fraction_raw_98 = word(raw, base + 0x98u),
            .previous_y_raw_9a = (int16_t)word(raw, base + 0x9Au),
            .planar_scratch_raw_a0 = word(raw, base + 0xA0u)
        };
        nba_gameplay_actor_commit(&actor, 2u, true);
        printf("%04x %04x %04x %04x %04x %04x %04x %04x "
               "%04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
               actor.x_fraction, (uint16_t)actor.x,
               actor.y_fraction, (uint16_t)actor.y,
               actor.z_fraction, (uint16_t)actor.z,
               (uint16_t)actor.velocity_x, (uint16_t)actor.velocity_y,
               (uint16_t)actor.velocity_z, actor.speed_raw_4a,
               actor.movement_distance_raw_4c, actor.facing_raw_4e,
               actor.velocity_direction_raw_a2,
               actor.previous_x_fraction_raw_94,
               (uint16_t)actor.previous_x_raw_96,
               actor.previous_y_fraction_raw_98,
               (uint16_t)actor.previous_y_raw_9a,
               actor.planar_scratch_raw_a0);
    }
    return ferror(stdin) ? 1 : 0;
}
