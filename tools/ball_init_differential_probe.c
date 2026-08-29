/* Replay one complete native WRAM entry through the translated initializer.
 * Only the explicit C/WRAM adapter is test-specific; the helper is production. */
#include "nba_tipoff.h"
#include <stdio.h>
#include <string.h>
static unsigned char memory[0x20000];
static uint16_t read_word(unsigned p){return (uint16_t)(memory[p]|memory[p+1]<<8);}
static void write_word(unsigned p,uint16_t v){memory[p]=(unsigned char)v;memory[p+1]=(unsigned char)(v>>8);}
int main(int argc,char **argv) {
    if(argc!=3)return 2;
    FILE *in=fopen(argv[1],"rb");if(!in)return 3;
    size_t n=fread(memory,1,sizeof(memory),in);int tail=fgetc(in);fclose(in);
    if(n!=sizeof(memory)||tail!=EOF)return 4;
    uint16_t cursor=read_word(0x9a);
    /* The caller supplies the ordinary object-list cursor; aliasing native
     * work fields is outside this prefix's valid startup input contract. */
    if(cursor!=0x34e7u)return 5;
    NbaTipBallInitialization s;
#define WORD(address,field) s.field=read_word(address);
#include "ball_init_fields.def"
#undef WORD
    nba_tip_ball_initialize(&s);
#define WORD(address,field) write_word(address,s.field);
#include "ball_init_fields.def"
#undef WORD
    FILE *out=fopen(argv[2],"wb");if(!out)return 6;
    n=fwrite(memory,1,sizeof(memory),out);int error=fclose(out);
    return n==sizeof(memory)&&!error?0:7;
}
