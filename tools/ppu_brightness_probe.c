/* Input-only adapter to the production converter. Native RGB stays external. */
#include "nba_snes_ppu.h"
#include <stdio.h>
#include <stdint.h>

int main(void) {
    char line[80], extra;
    unsigned color, brightness, count=0;
    while (fgets(line,sizeof(line),stdin)) {
        if (sscanf(line,"%u %u %c",&color,&brightness,&extra)!=2 ||
            color>0x7fffu || brightness>15u) return 2;
        uint8_t cgram[512]={0};
        cgram[0]=(uint8_t)color;cgram[1]=(uint8_t)(color>>8);
        printf("%u\n",nba_snes_cgram_color(cgram,0,(int)brightness,0,0,0)&0xffffffu);
        ++count;
    }
    return count&&!ferror(stdin)?0:2;
}
