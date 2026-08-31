/* Controlled canvas construction probe. Only configuration inputs are read;
 * native expected canvas bytes remain outside the C process. */
#define _CRT_SECURE_NO_WARNINGS
#include "nba_setup_screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    NbaAssetPack assets = {0}; NbaGameConfig config = {0};
    NbaSetupScreen *setup = calloc(1, sizeof(*setup));
    uint8_t *canvas = malloc(0x10000u);
    int result = 0;
    if (!setup || !canvas || !nba_assets_load(&assets, argv[1])) { result = 3; goto done; }
    uint16_t values[NBA_SETUP_RULE_COUNT];
    for (unsigned i = 0; i < NBA_SETUP_RULE_COUNT; ++i) {
        unsigned value;
        if (scanf("%u", &value) != 1 || value > (i < 2 ? 45u : 1u)) { result = 4; goto done; }
        values[i] = (uint16_t)value;
    }
    char extra;
    if (scanf(" %c", &extra) != EOF) { result = 4; goto done; }
    nba_setup_screen_init(setup, &assets, &config);
    if (!setup->rules_vram) { result = 5; goto done; }
    memcpy(canvas, setup->rules_vram, 0x10000u);
    if (!nba_setup_screen_apply_rules_value_cells(setup, canvas, values)) { result = 6; goto done; }
    FILE *out = fopen(argv[2], "wb");
    if (!out) { result = 7; goto done; }
    size_t count = fwrite(canvas, 1, 0x10000u, out);
    int closed = fclose(out);
    if (count != 0x10000u || closed != 0) result = 8;
done:
    nba_assets_free(&assets); free(canvas); free(setup);
    return result;
}
