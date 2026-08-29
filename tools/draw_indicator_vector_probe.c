#include <stdio.h>
#include "nba_court_presentation.h"
int main(void){unsigned sx,sy,c;while(scanf("%x %x %x",&sx,&sy,&c)==3){NbaCourtPlayerIndicator o;
 nba_court_player_indicator((int16_t)(uint16_t)sx,(int16_t)(uint16_t)sy,(uint8_t)c,&o);
 printf("%x %x %x %x %x\n",o.resource,o.attribute,(uint16_t)o.x,(uint16_t)o.y,(uint16_t)o.actor_screen_x);}return 0;}
