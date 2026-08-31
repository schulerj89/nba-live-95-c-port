#define _CRT_SECURE_NO_WARNINGS
#include "nba_setup_screen.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    char *end = NULL;
    unsigned long frames = strtoul(argv[2], &end, 10);
    if (!end || *end || frames > 300) return 2;
    FILE *file = fopen(argv[1], "rb");
    if (!file || fseek(file, 0, SEEK_END)) return 2;
    long size = ftell(file);
    if (size < 0 || size > 4000000 || fseek(file, 0, SEEK_SET)) return 2;
    uint8_t *data = malloc(size ? (size_t)size : 1u);
    if (!data || fread(data, 1, (size_t)size, file) != (size_t)size) return 2;
    fclose(file);
    bool valid = nba_setup_screen_validate_publications(data, (size_t)size, (uint32_t)frames);
    free(data);
    return valid ? 0 : 1;
}
