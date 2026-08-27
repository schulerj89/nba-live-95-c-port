/* Replays `$87:8F13-$8F61` through the production facing helper. */
#include <stdio.h>
#include "nba_tipoff.h"

int main(void) {
    unsigned desired, lock, shown, timer;
    while (scanf_s("%x %x %x %x", &desired, &lock, &shown, &timer) == 4) {
        uint8_t shown_raw = (uint8_t)shown, timer_raw = (uint8_t)timer;
        nba_tipoff_ease_display_direction((uint8_t)desired, (uint16_t)lock,
                                          &shown_raw, &timer_raw);
        printf("%02x %02x\n", shown_raw, timer_raw);
    }
    return 0;
}
