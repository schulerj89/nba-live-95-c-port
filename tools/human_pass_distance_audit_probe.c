#include "nba_human_pass.h"
#include <stdio.h>
int main(void) {
    unsigned x, y;
    while (scanf_s("%x %x", &x, &y) == 2)
        printf("%u\n", nba_human_pass_distance((int16_t)(uint16_t)x,
                                             (int16_t)(uint16_t)y));
    return 0;
}
