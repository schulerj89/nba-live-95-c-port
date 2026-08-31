#include "nba_intro_text.h"
#include "nba_rom_font.h"
#include "nba_snes_ppu.h"
#include <string.h>

enum { INTRO_TEXT_SIZE = 4568, FONT_OFFSET = 40, REPEAT_OFFSET = 4136,
       PALETTE_OFFSET = 4308, LICENSE_OFFSET = 4316, LEGAL_OFFSET = 4337 };

static uint32_t value(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}

bool nba_intro_text_payload_valid(const uint8_t *data, size_t size) {
    static const uint32_t header[8] = {1,4096,172,8,21,231,0xA98000,0xAFF2DC};
    if (!data || size != INTRO_TEXT_SIZE || memcmp(data,"NBITEXT1",8)) return false;
    for (int i=0;i<8;i++) if(value(data+8+i*4)!=header[i]) return false;
    if (memcmp(data+FONT_OFFSET,"\x10\0\x0e\0\x01\x02",6) ||
        data[LEGAL_OFFSET-1] || data[INTRO_TEXT_SIZE-1]) return false;
    for (size_t i=LICENSE_OFFSET;i<INTRO_TEXT_SIZE-1;i++) {
        if(i==LEGAL_OFFSET-1)continue;
        if ((data[i]<32u && data[i]!=13u) || data[i]>=128u) return false;
    }
    return true;
}

bool nba_intro_text_render(const NbaAssetPack *assets, NbaRenderer *renderer,
                           bool legal, int brightness) {
    if (!assets || !renderer) return false;
    nba_renderer_clear(renderer,0xFF000000u);
    const NbaAssetItem *item=nba_assets_get(assets,NBA_ASSET_INTRO_TEXT);
    if(!item || !nba_intro_text_payload_valid(item->data,item->size)) return false;
    const uint8_t *data=item->data;
    NbaRomFont font={data+FONT_OFFSET,4096,data+REPEAT_OFFSET,172,1,legal?1:0,0};
    uint8_t canvas[NBA_SNES_WIDTH*NBA_SNES_HEIGHT]={0};
    const uint8_t *text=data+(legal?LEGAL_OFFSET:LICENSE_OFFSET);
    size_t remaining=legal?230u:20u;
    int y=legal?40:104;
    /* $80:FE38-FE58 and FEAD-FED3 pass the original strings to $81:9FDF /
     * $81:A163. The latter restores the x origin after every CR and advances
     * y by descriptor height + 2 + $18DC (1 for the legal notice). */
    while(remaining) {
        size_t length=0;
        while(length<remaining && text[length]!=13u)++length;
        uint16_t width;
        if(!nba_rom_font_measure(&font,text,length,&width) ||
           !nba_rom_font_draw(&font,canvas,NBA_SNES_WIDTH,NBA_SNES_WIDTH,
                NBA_SNES_HEIGHT,128-(int)(width/2u),y,text,length)) return false;
        if(length==remaining)break;
        text+=length+1u;remaining-=length+1u;
        y+=data[FONT_OFFSET+2]+2+(legal?1:0);
    }
    uint8_t cgram[512]={0};
    memcpy(cgram,data+PALETTE_OFFSET,8);
    uint32_t colors[4];
    for(int i=0;i<4;i++)colors[i]=nba_snes_cgram_color(cgram,i,brightness,0,0,0);
    for(size_t i=0;i<sizeof(canvas);i++)renderer->pixels[i]=colors[canvas[i]&3u];
    return true;
}
