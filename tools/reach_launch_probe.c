#define _CRT_SECURE_NO_WARNINGS
#include "nba_tipoff_flow.h"
#include <stdio.h>
#include <string.h>
int main(void) {
    uint16_t v[14];
    while(scanf("%hu",v)==1) {
        for(unsigned i=1;i<14;++i)if(scanf("%hu",v+i)!=1)return 2;
        NbaReachLaunch s;memcpy(&s,v,sizeof(s));nba_reach_launch(&s);memcpy(v,&s,sizeof(s));
        for(unsigned i=0;i<14;++i)printf("%s%u",i?" ":"",v[i]);puts("");
    }
    return 0;
}
