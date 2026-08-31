#include "nba_player_lab.h"
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

/* Fixed little-endian words, no struct ABI/padding in the test protocol. */
int main(int argc,char **argv) {
    if (argc!=3) return 2;
    NbaAssetPack assets={0};
    int saved=_dup(_fileno(stdout));
    if (saved<0 || _dup2(_fileno(stderr),_fileno(stdout))!=0) return 3;
    bool loaded=nba_assets_load(&assets,argv[1]);
    fflush(stdout);
    if (_dup2(saved,_fileno(stdout))!=0) return 4;
    _close(saved);
    if (!loaded) return 5;
    _setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
    bool compat=strcmp(argv[2],"compat")==0;
    if (!compat && strcmp(argv[2],"pose")!=0) return 6;
    uint16_t w[11];size_t words=compat?8u:11u;
    for (;;) {
        size_t n=fread(w,2,words,stdin);if (n==0 && feof(stdin)) break;
        if (n!=words) return 7;
        if (compat) {
            NbaPlayerSpriteComposition out={0};uint16_t r[22]={0};
            r[0]=(uint16_t)nba_player_compose_sprite_parts(&assets,
                (uint8_t)w[0],(uint8_t)w[1],(uint8_t)w[2],(uint8_t)w[3],
                w[4],w[5],(int16_t)w[6],(int16_t)w[7],&out);
            r[1]=out.count;
            for (unsigned i=0;i<4;++i) {
                r[2+i*5]=(uint16_t)out.parts[i].kind;r[3+i*5]=out.parts[i].resource;
                r[4+i*5]=(uint16_t)out.parts[i].x;r[5+i*5]=(uint16_t)out.parts[i].y;
                r[6+i*5]=(uint16_t)out.parts[i].flip;
            }
            if (fwrite(r,2,22,stdout)!=22) return 8;
        } else {
#ifdef LEGACY_ONLY
            return 9;
#else
            NbaPlayerSpritePoseInput in={w[0],w[1],w[2],w[3],w[4],w[5],
                w[6],w[7],w[8],(int16_t)w[9],(int16_t)w[10]};
            NbaPlayerSpritePoseComposition out={0};uint16_t r[36]={0};
            r[0]=(uint16_t)nba_player_compose_sprite_pose(&assets,&in,&out);
            r[1]=out.count;r[2]=out.upper_flip_aa;r[3]=out.lower_flip_ac;
            r[4]=out.head_flip_49;r[5]=out.glyph_work_0884;
            r[6]=(uint16_t)out.upper_x_b2;r[7]=(uint16_t)out.upper_y_b4;
            r[8]=(uint16_t)out.head_x_b6;r[9]=(uint16_t)out.head_y_b8;
            r[10]=(uint16_t)out.number_x_dc;r[11]=(uint16_t)out.number_y_de;
            for (unsigned i=0;i<4;++i) {
                r[12+i*6]=(uint16_t)out.parts[i].kind;r[13+i*6]=out.parts[i].resource;
                r[14+i*6]=out.parts[i].attribute;r[15+i*6]=out.parts[i].glyph_work_0884;
                r[16+i*6]=(uint16_t)out.parts[i].x;r[17+i*6]=(uint16_t)out.parts[i].y;
            }
            if (fwrite(r,2,36,stdout)!=36) return 10;
#endif
        }
    }
    nba_assets_free(&assets);return ferror(stdin)||ferror(stdout)?11:0;
}
