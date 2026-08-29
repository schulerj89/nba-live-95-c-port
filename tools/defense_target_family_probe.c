#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include "nba_gameplay_ai.h"
#include "nba_gameplay_ball.h"

#define SIZE 0x4B00u
static uint16_t word(const uint8_t *raw, unsigned at) {
    return (uint16_t)(raw[at] | (uint16_t)raw[at + 1u] << 8);
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    FILE *rom_file = fopen(argv[1], "rb");
    if (!rom_file) return 5;
    static uint8_t rom[0x400000];
    size_t rom_size = fread(rom, 1u, sizeof(rom), rom_file);
    fclose(rom_file);
    uint8_t raw[SIZE];
    _setmode(_fileno(stdin), _O_BINARY);
    while (fread(raw, 1u, SIZE, stdin) == SIZE) {
        uint16_t entry = word(raw, 0u);
        uint16_t subject = word(raw, 0x96u);
        uint16_t paired = word(raw, 0x9Au);
        uint16_t context = word(raw, 0x9Eu);
        uint16_t paired_context = word(raw, paired + 0x70u);
        uint16_t player_table = 0x3449u + word(raw, paired) * 4u;
        uint16_t player_record = player_table + 1u < SIZE ?
            word(raw, player_table) : 0u;
        uint8_t player_bank = player_table + 2u < SIZE ? raw[player_table + 2u] : 0u;
        size_t rating_offset = ((size_t)(player_bank & 0x7Fu) * 0x8000u) +
                               (size_t)(player_record - 0x8000u) + 0x37u;
        if (subject + 0x92u >= SIZE || paired + 0x92u >= SIZE ||
            context + 0x32u >= SIZE) return 3;
        uint8_t mode = (uint8_t)word(raw, subject + 0x5Eu);
        NbaGameplayDefenseTargetInput input = {
            .actor_x = (int16_t)word(raw, subject + 4u),
            .actor_y = (int16_t)word(raw, subject + 8u),
            .actor_pair_direction_raw_86 = (uint8_t)word(raw, subject + 0x86u),
            .actor_pair_distance_raw_8a = word(raw, subject + 0x8Au),
            .paired_x = (int16_t)word(raw, paired + 4u),
            .paired_y = (int16_t)word(raw, paired + 8u),
            .paired_velocity_x = (int16_t)word(raw, paired + 0x0Eu),
            .paired_velocity_y = (int16_t)word(raw, paired + 0x10u),
            .paired_anchor_direction_raw_88 = (uint8_t)word(raw, paired + 0x88u),
            .paired_anchor_distance_raw_8c = word(raw, paired + 0x8Cu),
            .paired_position_raw_92 = (uint8_t)word(raw, paired + 0x92u),
            .context_anchor_x = (int16_t)word(raw, context + 0x0Au),
            .context_mode_raw_30 = word(raw, context + 0x30u),
            .context_flags_raw_32 = word(raw, context + 0x32u),
            .paired_on_three_point_arc = nba_gameplay_shot_value(
                false, (int16_t)word(raw, paired + 4u),
                (int16_t)word(raw, paired + 8u),
                paired_context + 0x0Au < SIZE &&
                (int16_t)word(raw, paired_context + 0x0Au) >= 0) == 3u,
            .paired_three_point_rating = player_record >= 0x8000u &&
                rating_offset < rom_size ? rom[rating_offset] : 0u,
        };
        NbaGameplayDefenseTargetOutput output = {0};
        if (!nba_gameplay_defense_mode_target(mode, &input, &output)) return 4;
        printf("%04x %04x %04x %04x %04x %04x %04x %04x %04x\n", entry,
               (uint16_t)output.target_x, (uint16_t)output.target_y,
               output.target_written, output.stop_velocity,
               input.paired_on_three_point_arc,
               input.paired_three_point_rating,
               (uint16_t)input.context_anchor_x,
               paired_context + 0x0Au < SIZE ? word(raw, paired_context + 0x0Au) : 0u);
    }
    return ferror(stdin) ? 1 : 0;
}
