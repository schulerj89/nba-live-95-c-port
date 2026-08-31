#include "nba_court_presentation.h"
#include <stdio.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

static uint16_t word(const unsigned char *p) {
    return (uint16_t)(p[0]|((uint16_t)p[1]<<8));
}

int main(void) {
    unsigned char input[8];
#ifdef _WIN32
    if(_setmode(_fileno(stdin),_O_BINARY)==-1 ||
       _setmode(_fileno(stdout),_O_BINARY)==-1)return 4;
#endif
    for(;;) {
        size_t count=fread(input,1,sizeof(input),stdin);
        if(count==0)return ferror(stdin) || fflush(stdout)!=0 ? 5 : 0;
        if(count!=sizeof(input))return 2;
        if(word(input+6)>1u)return 3;
        if(fputc(nba_court_actor_visible((int16_t)word(input),
              (int16_t)word(input+2),(int16_t)word(input+4),
              word(input+6)!=0u)?1:0,stdout)==EOF)return 6;
    }
}
