#include <stdio.h>
#include "nba_player_lab.h"
int main(int argc,char **argv){NbaAssetPack p;if(argc!=2||!nba_assets_load(&p,argv[1]))return 2;unsigned r,a,x,y;
 while(scanf("%x %x %x %x",&r,&a,&x,&y)==4){NbaRomSpriteOamComposition o;if(!nba_rom_sprite_resource_compose(&p,(uint16_t)r,(uint16_t)a,(int16_t)(uint16_t)x,(int16_t)(uint16_t)y,&o)){puts("-");continue;}
  printf("%x",o.count);for(unsigned i=0;i<o.count;++i)printf(" %04x%02x%02x%02x%02x",(uint16_t)o.entries[i].x,o.entries[i].y,o.entries[i].tile,o.entries[i].attribute,o.entries[i].large);putchar('\n');}nba_assets_free(&p);return 0;}
