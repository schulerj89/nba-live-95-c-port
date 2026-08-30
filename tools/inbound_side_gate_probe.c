#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include "nba_gameplay_ai.h"
int main(void) {
    unsigned anchor, owner, receiver;
    while (scanf("%x %x %x", &anchor, &owner, &receiver) == 3)
        printf("%u\n", nba_gameplay_inbound_side_allows(
            (int16_t)(uint16_t)anchor, (int16_t)(uint16_t)owner,
            (int16_t)(uint16_t)receiver) ? 1u : 0u);
    return 0;
}
