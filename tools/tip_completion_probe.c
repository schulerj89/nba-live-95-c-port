#define _CRT_SECURE_NO_WARNINGS
#include "nba_tipoff_flow.h"
#include <stdio.h>
#include <string.h>
typedef char ProbeWordSchema[(sizeof(NbaTipCompletion)==9*sizeof(uint16_t))?1:-1];
int main(void) {
    uint16_t words[9];
    while(scanf("%hu",words)==1) {
        for(unsigned i=1;i<9;++i)if(scanf("%hu",words+i)!=1)return 2;
        NbaTipCompletion s;memcpy(&s,words,sizeof(s));bool tip=nba_tip_complete_acquisition(&s);
        memcpy(words,&s,sizeof(s));printf("%u",tip);
        for(unsigned i=0;i<9;++i)printf(" %u",words[i]);putchar('\n');
    }return 0;
}
