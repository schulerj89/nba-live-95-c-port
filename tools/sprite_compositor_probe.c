#include "nba_player_lab.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t actor, team, roster, side, direction;
    uint16_t upper, lower;
    int16_t lower_x, lower_y;
    uint16_t resource[4];
    int16_t x[4], y[4];
    uint8_t kind[4];
} Witness;

/* First complete native `$80:AD92->$80:B348` opening submission at setup
 * frame 3043. Screenshots are not inputs: these are literal resource/origin
 * words from player_resource_calls.txt paired with the actor records captured
 * at the same `$87:A47A` pass. */
static const Witness witnesses[] = {
    {8,3,3,1,3,0x00F0,0x044C,0x50,0xA4,
     {0x04C4,0x0592,0x00F0,0x044C},{0x50,0x4D,0x50,0x50},{0x78,0x7D,0x85,0xA4},{0,1,2,3}},
    {2,18,1,0,4,0x00F1,0x044D,0xB8,0x96,
     {0x04CA,0x0593,0x00F1,0x044D},{0xB5,0xB8,0xBB,0xB8},{0x6A,0x6F,0x78,0x96},{0,1,2,3}},
    {6,3,0,1,4,0x00F1,0x068A,0xE3,0x8C,
     {0x053D,0x0593,0x00F1,0x068A},{0xE0,0xE3,0xE6,0xE3},{0x5A,0x5F,0x68,0x8C},{0,1,2,3}},
    {5,3,2,1,2,0x00F1,0x068A,0x75,0x7D,
     {0x04F7,0x0591,0x00F1,0x068A},{0x78,0x75,0x72,0x75},{0x4B,0x50,0x59,0x7D},{0,1,2,3}},
    {0,18,2,0,6,0x00F3,0x068C,0x8B,0x7A,
     {0x04A4,0x00F3,0x0591,0x068C},{0x8B,0x8C,0x8C,0x8B},{0x49,0x55,0x4F,0x7A},{0,2,1,3}},
    {1,18,0,0,0,0x00F3,0x068C,0x1D,0x6B,
     {0x0517,0x00F3,0x0593,0x068C},{0x1D,0x1C,0x1C,0x1D},{0x3A,0x46,0x40,0x6B},{0,2,1,3}},
    {7,3,1,1,0,0x00F3,0x044F,0x48,0x62,
     {0x04BD,0x00F3,0x0593,0x044F},{0x49,0x48,0x48,0x48},{0x37,0x43,0x3D,0x62},{0,2,1,3}},
    {3,18,3,0,7,0x00F4,0x0450,0xB0,0x54,
     {0x04CD,0x00F4,0x0592,0x0450},{0xB0,0xB0,0xAD,0xB0},{0x29,0x35,0x2E,0x54},{0,2,1,3}}
};

int main(int argc, char **argv) {
    NbaAssetPack pack = {0};
    if (argc != 2 || !nba_assets_load(&pack, argv[1])) return 2;
    for (unsigned w = 0; w < sizeof(witnesses)/sizeof(witnesses[0]); ++w) {
        const Witness *expected = &witnesses[w];
        NbaPlayerSpriteComposition actual = {0};
        if (!nba_player_compose_sprite_parts(&pack, expected->team,
                expected->roster, expected->side, expected->direction,
                expected->upper, expected->lower, expected->lower_x,
                expected->lower_y, &actual) || actual.count != 4u) return 10+(int)w;
        for (unsigned p = 0; p < 4u; ++p) {
            if (actual.parts[p].resource != expected->resource[p] ||
                actual.parts[p].x != expected->x[p] ||
                actual.parts[p].y != expected->y[p] ||
                actual.parts[p].kind != (NbaPlayerSpritePartKind)expected->kind[p]) {
                fprintf(stderr,"actor%u part%u got k%u %04X @%d,%d expected k%u %04X @%d,%d\n",
                    expected->actor,p,actual.parts[p].kind,actual.parts[p].resource,
                    actual.parts[p].x,actual.parts[p].y,expected->kind[p],
                    expected->resource[p],expected->x[p],expected->y[p]);
                return 30+(int)w;
            }
        }
    }
    unsigned exhaustive = 0;
    for (unsigned team = 0; team < 29u; ++team)
        for (unsigned roster = 0; roster < 12u; ++roster)
            for (unsigned direction = 0; direction < 8u; ++direction)
                for (unsigned side = 0; side < 2u; ++side) {
                    uint16_t upper = 0, lower = 0;
                    NbaPlayerSpriteComposition c = {0};
                    if (!nba_player_animation_resources(&pack,(uint8_t)team,
                            (uint8_t)roster,(uint8_t)direction,0,0,
                            &upper,&lower) ||
                        !nba_player_compose_sprite_parts(&pack,(uint8_t)team,
                            (uint8_t)roster,(uint8_t)side,(uint8_t)direction,
                            upper,lower,100,150,&c)) {
                        fprintf(stderr,"exhaustive compose failed t%u r%u d%u s%u u%04X l%04X\n",
                            team,roster,direction,side,upper,lower);
                        return 50;
                    }
                    NbaPlayerSpriteDiagnostics d;
                    if (!nba_player_sprite_diagnose_resources(&pack,
                            (uint8_t)team,(uint8_t)roster,(uint8_t)side,
                            (uint8_t)direction,upper,lower,&d)) return 54;
                    unsigned expected_count = d.number_composed ? 4u : 3u;
                    if (c.count != expected_count ||
                        c.parts[0].kind != NBA_PLAYER_SPRITE_HEAD ||
                        c.parts[c.count-1].kind != NBA_PLAYER_SPRITE_LOWER ||
                        c.parts[c.count-1].x != 100 || c.parts[c.count-1].y != 150) {
                        fprintf(stderr,"shape mismatch t%u r%u d%u s%u count%u/%u\n",
                            team,roster,direction,side,c.count,expected_count);
                        return 51;
                    }
                    if (d.number_composed) {
                        unsigned number = direction >= 2u && direction <= 4u ? 1u : 2u;
                        if (c.parts[number].kind != NBA_PLAYER_SPRITE_NUMBER) {
                            fprintf(stderr,"number order mismatch t%u r%u d%u s%u\n",
                                team,roster,direction,side);
                            return 52;
                        }
                    }
                    ++exhaustive;
                }
    NbaPlayerSpriteComposition poison = {0};
    poison.count = 3u; poison.parts[0].resource = 0xBEEFu;
    NbaPlayerSpriteComposition before = poison;
    if (nba_player_compose_sprite_parts(&pack,29u,0u,0u,0u,
            0x00F0u,0x044Cu,0,0,&poison) ||
        memcmp(&poison,&before,sizeof(poison))) {
        fputs("atomic rejection failed\n",stderr); return 53;
    }
    nba_assets_free(&pack);
    printf("SPRITE COMPOSITOR PASS: 8 native players, 32 ordered calls; %u all-team/direction/side cases\n",exhaustive);
    return 0;
}
